#include "cpu/instr.h"
/*
Put the implementations of `pop' instructions here.
*/
static void instr_execute_1op(){
    cpu.esp += 4;
    opr_src.val = vaddr_read(cpu.esp - 4, SREG_SS, 4);
    operand_write(&opr_src);
}
make_instr_impl_1op(pop, r, v);
