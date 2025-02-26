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

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

#if   defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif

#define MTRACE_LOG_FILE "mtrace.log"
static FILE *mtrace_file=NULL;

void init_mtrace(){
  mtrace_file=fopen(MTRACE_LOG_FILE,"w");
  if(mtrace_file==NULL){
    printf("Fail to open mtrace log file\n");
    return;
  }
  // 初始化 mtrace_thing
  // mtrace_thing.start=0x00000000;
  // mtrace_thing.end=0xFFFFFFFF;
  // mtrace_thing.filter_en=false;
  // mtrace_thing.filter_data=0xFFFFFFFF;
}

void close_mtrace(){
  if(mtrace_file != NULL){
      fclose(mtrace_file);
      mtrace_file = NULL;
  }
}

void mtrace_log(char type,paddr_t addr,word_t data,int len){
  if(mtrace_file==NULL) return;
  // if(addr<mtrace_thing.start || addr>mtrace_thing.end) return;
  // if(mtrace_thing.filter_en && data!=mtrace_thing.filter_data) return;
  switch (type){
    case 'R':
        fprintf(mtrace_file,"R 0x%08x 0x%08x %d\n",addr,data,len);
        break;
    case 'W':
        fprintf(mtrace_file,"W 0x%08x 0x%08x %d\n",addr,data,len);
        break;
    default:
        printf("No such type,only R or W enable\n");
        break;
  }
}

void mtrace_filter_output(paddr_t start_addr, paddr_t end_addr, bool filter_en, uint32_t filter_data){
    FILE *fp = fopen(MTRACE_LOG_FILE, "r");
    if (fp == NULL) {
        printf("Failed to open mtrace log file\n");
        return;
    }
    char type;
    paddr_t addr;
    uint32_t data;
    int len;
    // 逐行读取日志文件
    while(fscanf(fp, " %c 0x%x 0x%x %d",&type,&addr,&data,&len)==4) {
        if (addr < start_addr || addr > end_addr) {
            continue;
        }
        if (filter_en && data != filter_data) {
            continue; 
        }
        printf(" %c 0x%08x 0x%08x %d\n", type, addr, data, len);
    }
    fclose(fp);
}
uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

static void out_of_bound(paddr_t addr) {
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
}

void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif
  IFDEF(CONFIG_MEM_RANDOM, memset(pmem, rand(), CONFIG_MSIZE));
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}

word_t paddr_read(paddr_t addr, int len) {
#ifdef CONFIG_MEMORY_TRACE
  mtrace_log('R',addr,pmem_read(addr,len),len);
#endif
  if (likely(in_pmem(addr)))return pmem_read(addr, len);
  IFDEF(CONFIG_DEVICE, return mmio_read(addr, len));
  out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
#ifdef CONFIG_MEMORY_TRACE
  mtrace_log('W',addr,data,len);
#endif
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
  IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return);
  out_of_bound(addr);
  // if (likely(in_pmem(addr))) {
  //   pmem_write(addr, len, data);
  // } 
  // else IFDEF(CONFIG_DEVICE, {
  //   mmio_write(addr, len, data);
  // })
  // else {
  //   out_of_bound(addr);
  //   return;
  // }
  // // 记录写操作到 mtrace
  // mtrace_log('W', addr, data, len);
}
