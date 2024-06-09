#include "cpu/instr.h"
/*
Put the implementations of `lgdt' instructions here.
*/

make_instr_func(lgdt){
    int len = 1;
    opr_dest.data_size = data_size;
    opr_src.data_size = 16;
    len += modrm_rm(eip+1, &opr_src);
    modrm_rm(eip+1, &opr_dest);
    opr_dest.addr += 2;
    operand_read(&opr_dest);
    operand_read(&opr_src);
    cpu.gdtr.base = opr_dest.val;
    cpu.gdtr.limit = opr_src.val;
    return len;
}
