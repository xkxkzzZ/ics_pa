#include "cpu/instr.h"
/*
Put the implementations of `push' instructions here.
*/

static void instr_execute_1op(){
    operand_read(&opr_src);
    vaddr_write(cpu.esp - 4, SREG_SS, 4, opr_src.val);
    cpu.esp -= 4;
}
make_instr_impl_1op(push, r, v);
make_instr_impl_1op(push, rm, v);
make_instr_impl_1op(push, i, b);
make_instr_impl_1op(push, i, v);