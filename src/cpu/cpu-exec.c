/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <memory/paddr.h>
#include <common.h>
#include "/home/furina/ysyx-workbench/nemu/src/monitor/sdb/watchpoint.h"
#include "/home/furina/ysyx-workbench/nemu/src/monitor/sdb/sdb.h"

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10
#define IRINGBUF_SIZE 16
#define MAX_FTRACE_SIZE 1000

typedef struct {
  vaddr_t pc;                      //指令pc
  uint32_t inst;                   //指令编码
  char logbuf[128];                //反汇编
}instInfo;

typedef struct{
  instInfo entries[IRINGBUF_SIZE]; //环形缓冲区
  int w_ptr;                       //写指针
  int r_ptr;                       //读指针
  bool full;                       //溢出检测
}Iringbuf;

static Iringbuf iringbuf;

void iringbuf_init(){
  iringbuf.w_ptr=0;
  iringbuf.r_ptr=0;
  iringbuf.full=false;
  memset(iringbuf.entries,' ',sizeof(iringbuf.entries));
}

void iringbuf_push(vaddr_t pc,uint32_t inst,char *logbuf){
  iringbuf.entries[iringbuf.w_ptr].pc=pc;
  iringbuf.entries[iringbuf.w_ptr].inst=inst;
  strncpy(iringbuf.entries[iringbuf.w_ptr].logbuf,logbuf,sizeof(iringbuf.entries[iringbuf.w_ptr].logbuf)-1);
  iringbuf.entries[iringbuf.w_ptr].logbuf[sizeof(iringbuf.entries[iringbuf.w_ptr].logbuf)-1]='\0';
  iringbuf.w_ptr=(iringbuf.w_ptr+1)%IRINGBUF_SIZE;//更新写指针
  if(iringbuf.full) iringbuf.r_ptr=(iringbuf.r_ptr+1)%IRINGBUF_SIZE;//如果满，则更新读指针
  iringbuf.full=(iringbuf.r_ptr==iringbuf.w_ptr);
}

void iringbuf_dummy(vaddr_t error_pc){
  printf("Recent inst\n");
  int count=iringbuf.full ? IRINGBUF_SIZE : iringbuf.w_ptr;
  for(int i=0;i<count;i++){
    int index=(iringbuf.r_ptr+i)%IRINGBUF_SIZE;
    if(iringbuf.entries[index].pc==error_pc) printf(" --> ");
    else printf("     ");
    printf("%-20s %02x %02x %02x %02x\n",
              // iringbuf.entries[index].pc,
              iringbuf.entries[index].logbuf,
              (iringbuf.entries[index].inst >> 24) & 0xff,
              (iringbuf.entries[index].inst >> 16) & 0xff,
              (iringbuf.entries[index].inst >> 8) & 0xff,
              iringbuf.entries[index].inst & 0xff);
  }
}

typedef struct {
  uint32_t addr;  // 函数地址
  uint32_t size;  // 函数大小
  char name[64];  // 函数名
} func_symbol_t;
extern int func_count;
extern func_symbol_t func_table[];

typedef struct{
  vaddr_t pc;//函数调用地址
  char *name;//函数名
  vaddr_t back;//返回地址
}ftrace_info;

static ftrace_info ftrace[MAX_FTRACE_SIZE];
static int ftrace_size=0;//当前调用栈深度

typedef struct {
  char *name;
  uint64_t call_count;
} func_call_stats_t;

static func_call_stats_t func_call_stats[MAX_FTRACE_SIZE];
static int func_call_stats_size = 0;

void ftrace_call(vaddr_t pc,char *name,vaddr_t back,vaddr_t dnpc){
  if(ftrace_size>=MAX_FTRACE_SIZE){
    printf("Call stack overflow!\n");
    return;
  }

  // 查找或创建函数调用统计记录
  int index = -1;
  for (int i = 0; i < func_call_stats_size; i++) {
    if (strcmp(func_call_stats[i].name, name) == 0) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    index = func_call_stats_size++;
    func_call_stats[index].name = name;
    func_call_stats[index].call_count = 0;
  }
  // 增加调用次数
  func_call_stats[index].call_count++;
  
  // 输出调用信息
  printf("0x%x: ",pc);
  for(int i=0;i<ftrace_size;i++){
    printf("  "); // 缩进
  }
  printf("call [%s @ 0x%08x]\n",name,dnpc);
  // printf("call [0x%x]\n",back);
  // 压栈
  ftrace[ftrace_size].pc=pc;
  ftrace[ftrace_size].name=name;
  ftrace[ftrace_size].back=back;
  ftrace_size++;
}

void ftrace_ret(vaddr_t pc,char *name){
  if(ftrace_size <= 0){
    printf("Call stack underflow!\n");
    return;
  }
  // 检查返回地址是否匹配栈顶记录
  // if(pc!=ftrace[ftrace_size-1].back){
  //   printf("Mismatched return address! Expected 0x%08x, got 0x%08x\n",
  //          ftrace[ftrace_size-1].back,pc);
  // }
  ftrace_size--;
  // 输出返回信息
  printf("0x%x: ",pc);
  for (int i = 0; i < ftrace_size; i++) {
    printf("  "); // 缩进
  }
  printf("ret  [%s]\n", name);
}

char *get_func_name(vaddr_t addr){
  for(int i=0;i<func_count;i++){
    if(addr>=func_table[i].addr && addr<func_table[i].addr+func_table[i].size){// 检查给出的地址是否落在区间[Value, Value + Size)内
      return func_table[i].name;
    }
  }
  return "???"; // 未知函数
}

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
#ifdef CONFIG_WATCHPOINTS
  if(CONFIG_WATCHPOINTS){
    WP *wp=get_wp_head();
    while(wp!=NULL){
      word_t val=expr(wp->expr);
      if(val!=wp->old_val){
        printf("Watchpoint NO.%d: Expression '%s' changed from 0x%08x to 0x%08x.\n", wp->NO, wp->expr, wp->old_val, val);
        nemu_state.state=NEMU_STOP;
        wp->old_val=val;
        //sdb_mainloop();
        break;
      }
      // wp->old_val=val;
      wp=wp->next;
    }
  }
#endif
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
#ifdef CONFIG_FUNC_TRACE
  uint32_t opcode = s->isa.inst & 0x7f;
  vaddr_t target=s->dnpc;
  if(opcode==0x6f){ //JAL指令（函数调用）11011 11//JALR指令11001 11
    vaddr_t ret_addr=pc+4;
    char *name=get_func_name(target);
    ftrace_call(pc,name,ret_addr,s->dnpc);
  }else if(opcode==0x67){//JALR指令11001 11
    if(s->isa.inst==0x00008067){//ret指令
      if(ftrace_size>0){//判断是否为函数返回
        ftrace_ret(pc,ftrace[ftrace_size-1].name);
      }
    }
    // if(ftrace_size>0 && target==ftrace[ftrace_size-1].back){
    //   char *name=get_func_name(target);
    //   ftrace_call(target,name,pc+4);
    // }
  }
#endif
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst;
#ifdef CONFIG_ISA_x86
  for (i = 0; i < ilen; i ++) {
#else
  for (i = ilen - 1; i >= 0; i --) {
#endif
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst, ilen);

  iringbuf_push(cpu.pc,s->isa.inst,s->logbuf);
#endif
}

static void execute(uint64_t n) {
  Decode s;
  IFDEF(CONFIG_MEMORY_TRACE,init_mtrace());
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
  if(nemu_state.state==NEMU_END || nemu_state.state==NEMU_ABORT){
    IFDEF(CONFIG_MEMORY_TRACE,close_mtrace());
  }
  if(nemu_state.state==NEMU_ABORT){//程序出错时
    vaddr_t error_pc=cpu.pc;
    iringbuf_dummy(error_pc);
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");

  Log("Function call statistics:");
  for (int i = 0; i < func_call_stats_size; i++) {
    Log("  %-20s: %" PRIu64 " calls", func_call_stats[i].name, func_call_stats[i].call_count);
  }
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT: case NEMU_QUIT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;iringbuf_init();
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}
