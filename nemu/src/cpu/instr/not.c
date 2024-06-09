#include "cpu/instr.h"
/*
Put the implementations of `not' instructions here.
*/
static void instr_execute_1op(){
    operand_read(&opr_src);
    opr_src.val = alu_xor(opr_src.val, 0xffffffff, opr_src.data_size);
    operand_write(&opr_src);
}

make_instr_impl_1op(not, rm, v);
make_instr_impl_1op(not, rm, b);

