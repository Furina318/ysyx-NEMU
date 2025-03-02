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
#include <cpu/cpu.h>
#include <difftest-def.h>
#include <memory/paddr.h>
#include <string.h>
#include <stdlib.h>

// typedef struct {
//   word_t gpr[32]; // 通用寄存器 x0-x31
//   vaddr_t pc;
//   // uint32_t csr[4096]; // 控制和状态寄存器
// } riscv32_reg_state;
// extern riscv32_reg_state ref_regs;

// 在DUT host memory的`buf`和REF guest memory的`addr`之间拷贝`n`字节,
// `direction`指定拷贝的方向, `DIFFTEST_TO_DUT`表示往DUT拷贝, `DIFFTEST_TO_REF`表示往REF拷贝
__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  if(direction == DIFFTEST_TO_REF){
    printf("addr->%lx  imm:%lx\n", addr,(word_t)buf);
    memcpy(guest_to_host(addr), buf, n);
  }else if(direction == DIFFTEST_TO_DUT){
    // memcpy(buf, guest_to_host(addr), n);
    paddr_read(addr,n);
  }else{
    printf("Invaild direction in difftest_memcpy\n");
    assert(0);  
  }
}
// `direction`为`DIFFTEST_TO_DUT`时, 获取REF的寄存器状态到`dut`;
// `direction`为`DIFFTEST_TO_REF`时, 设置REF的寄存器状态为`dut`;
__EXPORT void difftest_regcpy(void *dut, bool direction) {
  if(direction == DIFFTEST_TO_REF){
    for(uint16_t i=0;i<32;i++){
      cpu.gpr[i]=dut->gpr[i];
    }
    cpu.pc=dut->pc;
  }else if(direction == DIFFTEST_TO_DUT){
    for(uint16_t i=0;i<32;i++){
      dut->gpr[i]=cpu.gpr[i];
    }
    dut->pc=cpu.pc;
  }else{
    printf("Invaild direction in difftest_regcpy\n");
    assert(0);
  }
}
// 让REF执行`n`条指令
__EXPORT void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

__EXPORT void difftest_raise_intr(word_t NO) {
  assert(0);
}

__EXPORT void difftest_init(int port) {
  void init_mem();
  init_mem();
  /* Perform ISA dependent initialization. */
  init_isa();
  cpu.gpr[0]=0;
  cpu.pc=RESET_VECTOR;
}
