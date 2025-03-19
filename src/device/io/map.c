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

#include <isa.h>
#include <memory/host.h>
#include <memory/vaddr.h>
#include <device/map.h>
#include <string.h>
#include <stdlib.h>

static FILE *dtrace_file=NULL;
#define DTRACE_LOG_FILE "dtrace.log"
void init_dtrace(){
  dtrace_file=fopen(DTRACE_LOG_FILE,"w");
  if(dtrace_file==NULL){
    printf("Fail to open dtrace log file\n");
    return;
  }
}
void close_dtrace(){
  if(dtrace_file!=NULL){
    fclose(dtrace_file);
    dtrace_file=NULL;
  }
}
#define dtrace_log_write(...) IFDEF(CONFIG_DEVICE_TRACE, \
  do { \
    if (dtrace_file != NULL) { \
      fprintf(dtrace_file, __VA_ARGS__); \
      fflush(dtrace_file); \
    } \
  } while (0) \
)
void print_dtrace_file(const char *device_name,const char *op) {
  FILE *file = fopen(DTRACE_LOG_FILE, "r");
  if (file == NULL) {
      printf("Failed to open file: %s\n", DTRACE_LOG_FILE);
      return;
  }
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    if(device_name != NULL && strstr(line,device_name) == NULL) continue;
    if(op != NULL){
      if(strcmp(op, "read") == 0 && strstr(line, "read") == NULL) continue;
      if(strcmp(op, "write") == 0 && strstr(line, "write") == NULL) continue;
    }
    printf("%s", line); // 逐行输出文件内容
  }
  fclose(file);
}

#define IO_SPACE_MAX (32 * 1024 * 1024)

static uint8_t *io_space = NULL;
static uint8_t *p_space = NULL;

uint8_t* new_space(int size) {
  uint8_t *p = p_space;
  // page aligned;
  size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
  p_space += size;
  assert(p_space - io_space < IO_SPACE_MAX);
  return p;
}

static void check_bound(IOMap *map, paddr_t addr) {
  if (map == NULL) {
    Assert(map != NULL, "address (" FMT_PADDR ") is out of bound at pc = " FMT_WORD, addr, cpu.pc);
  } else {
    Assert(addr <= map->high && addr >= map->low,
        "address (" FMT_PADDR ") is out of bound {%s} [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
        addr, map->name, map->low, map->high, cpu.pc);
  }
}

static void invoke_callback(io_callback_t c, paddr_t offset, int len, bool is_write) {
  if (c != NULL) { c(offset, len, is_write); }
}

void init_map() {
  io_space = malloc(IO_SPACE_MAX);
  assert(io_space);
  p_space = io_space;
}

word_t map_read(paddr_t addr, int len, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr);
  paddr_t offset = addr - map->low;
  invoke_callback(map->callback, offset, len, false); // prepare data to read
  word_t ret = host_read(map->space + offset, len);
#ifdef CONFIG_DEVICE_TRACE
  dtrace_log_write("[dtrace] read %10s at " FMT_PADDR " %d\n",map->name,addr,len);
#endif
  return ret;
}

void map_write(paddr_t addr, int len, word_t data, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr);
  paddr_t offset = addr - map->low;
  host_write(map->space + offset, len, data);
  invoke_callback(map->callback, offset, len, true);
#ifdef CONFIG_DEVICE_TRACE
  dtrace_log_write("[dtrace] wirte %10s at " FMT_PADDR " with " FMT_WORD " %d\n",map->name,addr,data,len);
#endif
}
