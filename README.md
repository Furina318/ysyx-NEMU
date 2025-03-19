# ysyx-NEMU
A NEMU for ysyx-PAs learning(update to PA3)

【**仅供交流学习使用，未经许可禁止传播！**】

NEMU(NJU Emulator) is a simple but complete full-system emulator designed for teaching purpose. Originally it supports x86, mips32, riscv64, and riscv32. **This repo only guarantees the support for riscv32.（updating)**

[**具体思路不展开，代码对应部分附有大量注释**]

To build programs run above NEMU, refer to the [AM project](https://github.com/NJU-ProjectN/abstract-machine).

**For the sake of academic integrity,this code is for reference,see the handout for details.(https://ysyx.oscc.cc)**

NEMU runs on Linux, and before you use it, some tool chain you need to download according to the handout.If you have any optimization suggestions or doubts about the NEMU code,you can contact the author directly.

The main features of NEMU include
* a small monitor with a simple debugger
  * single step
  * register/memory examination
  * expression evaluation without the support of symbols
  * watch point
  * differential testing against reference design (e.g. QEMU)
  * snapshot
* CPU core with support of most common ISAs
  * riscv32
    * floating point instructions are not supported
    * those instructions that are only in RV32IC are not supported
* memory
* interrupt and exception
* I/O and devices(输入输出设备已完成）
  *timer, keyboard, VGA, serial, IDE, i8259 PIC
  *port-mapped I/O and memory-mapped I/O
  



