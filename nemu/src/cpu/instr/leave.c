#include "cpu/instr.h"
/*
Put the implementations of `leave' instructions here.
*/
make_instr_func(leave){
    cpu.esp = cpu.ebp;
    cpu.esp += 4;
    cpu.ebp = vaddr_read(cpu.esp - 4, SREG_SS, 4);
    return 1;
}