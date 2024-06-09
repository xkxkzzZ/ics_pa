#include "cpu/instr.h"
/*
Put the implementations of `ret' instructions here.
*/
make_instr_func(ret_near)
{
    cpu.esp += 4;  
    cpu.eip = vaddr_read(cpu.esp - 4, SREG_SS, 4);
    return 0;
}

make_instr_func(ret_near_imm16)
{
    cpu.esp += 4;  
    cpu.eip = vaddr_read(cpu.esp - 4, SREG_CS, 4);
    return 0;
}