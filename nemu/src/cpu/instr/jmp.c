#include "cpu/instr.h"

make_instr_func(jmp_near)
{
    OPERAND rel;
    rel.type = OPR_IMM;
    rel.sreg = SREG_CS;
    rel.data_size = data_size;
    rel.addr = eip + 1;

    operand_read(&rel);

    int offset = sign_ext(rel.val, data_size);
    // thank Ting Xu from CS'17 for finding this bug
    print_asm_1("jmp", "", 1 + data_size / 8, &rel);

    cpu.eip += offset;

    return 1 + data_size / 8;
}

make_instr_func(jmp_short)
{
    OPERAND rel;
    rel.type = OPR_IMM;
    rel.sreg = SREG_CS;
    rel.data_size = 8;
    rel.addr = eip + 1;

    operand_read(&rel);

    int8_t offset = rel.val;
    print_asm_1("jmp", "", 1 + rel.data_size / 8, &rel);

    cpu.eip += offset;

    return 1 + rel.data_size / 8;
}
make_instr_func(jmp_near_indirect)
{
    int len = 1;
    opr_src.data_size = data_size;
    len += modrm_rm(eip+1, &opr_src);
    operand_read(&opr_src);
    cpu.eip = opr_src.val;
    return 0;
}

make_instr_func(jmp_far_imm){
    opr_dest.type = opr_src.type = OPR_IMM;
    opr_dest.sreg = opr_src.sreg = SREG_CS;
    opr_dest.data_size = data_size;
    opr_dest.addr = eip + 1;
    opr_src.data_size = 16;
    opr_src.addr = eip + (1 + data_size/8);
    operand_read(&opr_src);
    operand_read(&opr_dest);
    cpu.cs.val = (uint16_t)opr_src.val;
    load_sreg(SREG_CS);
    cpu.eip = opr_dest.val;
    return 0;
}






