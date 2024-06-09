#ifndef __INSTR_JMP_H__
#define __INSTR_JMP_H__

/*
Put the declarations of `jump' instructions here.
*/
make_instr_func(jmp_near);
make_instr_func(jmp_short);
make_instr_func(jmp_near_indirect);
make_instr_func(jmp_far_imm);
#endif
