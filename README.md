# ysyx-NEMU
A NEMU for ysyx-PAs learning(update to PA2)

【**仅供交流学习使用，未经许可禁止传播！**】

NEMU(NJU Emulator) is a simple but complete full-system emulator designed for teaching purpose. Originally it supports x86, mips32, riscv64, and riscv32. **This repo only guarantees the support for riscv32.（updating)**

[**目前nemu在microbench中ref规模的跑分265Marks**]

![nemu_test](file:///home/furina/Pictures/Screenshots/Screenshot%20from%202025-03-16%2019-25-53.png)

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
* I/O and devices are not supply now, but it will happen in the near future


About the debugger, here are some instructions(EXPR and N are you need to enter):

* "c", Continue the execution of the program(defined in nemu/src/monitor/sdb/sdb.c file)
* "help", Display information about all supported commands
* "q", Exit NEMU
* "si N", Let the program excute N instuctions and then suspend the excution(if the N is not given or N is negative, the default value is 1)
* "info r/w", Print register status with "r", or print the moniter status with "w"
* "x EXPR", Scan N pieces of memory base on 'EXPR'
* "p EXPR", Find the value of the expression 'EXPR'
* "w EXPR", Set watchpoint on 'EXPR',the programme will stop when it change
* "d N", Delete a watchpoint NO.n you set
  
* Trace:
* (open the Testing-and-Debugging directory in the NEMU by typing 'make menuconfig' and open the corresponding option)
  * itrace: It can record every instruction executed by the client,and display the error instruction and disassembly result(by'-->') when the program is abnormal terminal(defined nemu/src/cpu/cpu-exe.c file)
  * mtrace: Trace memory reads and wirtes,you can enter 'mtrace SART END FILTER_EN FILTER' to see more details
  * ftrace: Trace the address,name and call of the function
* Trace module has optimized.Such as ftrace statistics function calls memory alignment,itrace filters out the execution of branch junmp instructions,and so on.
