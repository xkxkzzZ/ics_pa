#include "cpu/instr.h"
/*
Put the implementations of `call' instructions here.
*/

make_instr_func(call_near)
{
    
    opr_src.type = OPR_IMM;
    opr_src.sreg = SREG_CS;
    opr_src.addr = eip + 1;
    
    int len = 1 + data_size / 8;
    opr_src.data_size = data_size;
    opr_dest.data_size = data_size;
    
    operand_read(&opr_src);
    
    vaddr_write(cpu.esp - 4, SREG_SS, 4, cpu.eip + len);
    cpu.esp -= 4;
    
    cpu.eip += sign_ext(opr_src.val, data_size);

    return len;
}
make_instr_func(call_near_indirect)
{
    int len = 1;
    opr_src.data_size = opr_dest.data_size = data_size;
    len += modrm_rm(eip+1, &opr_src);
    operand_read(&opr_src);
    
    vaddr_write(cpu.esp - 4, SREG_SS, 4, cpu.eip + len);
    cpu.esp -= 4; 
    
    cpu.eip = opr_src.val;
    return 0;
}
