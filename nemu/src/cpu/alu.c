#include "cpu/cpu.h"
//Zero Flag ── Set if result is zero; cleared otherwise.
void set_ZF(uint32_t result, size_t data_size){
    result = result & (0xffffffff >> (32 - data_size));
    cpu.eflags.ZF = (result == 0);
}
//Sign Flag ── Set equal to high-order bit of result (0 is positive, 1 if negative).
void set_SF(uint32_t result, size_t data_size){
    result = sign_ext(result & (0xffffffff >> (32 - data_size)), data_size);
    cpu.eflags.SF = sign(result);
}
//Parity Flag ── Set if low-order eight bits of result contain an even number of 1 bits; cleared otherwise.
void set_PF(uint32_t result){
    int sum =0;
    for(int i=0;i<8;i++){
        if(result %2 ==0) sum++;
        result >>=1;
    }
    if(sum %2 ==0) cpu.eflags.PF = 1;
    else cpu.eflags.PF = 0;
}


//Carry Flag ── Set on high-order bit carry or borrow; cleared otherwise
void set_CF_add(uint32_t result, uint32_t src, size_t data_size){
    result = sign_ext(result & (0xffffffff >> (32 - data_size)), data_size);
    src = sign_ext(src & (0xffffffff >> (32 - data_size)), data_size);
    cpu.eflags.CF = result < src;
}
void set_CF_adc(uint32_t result, uint32_t src, size_t data_size){
    result = sign_ext(result & (0xffffffff >> (32 - data_size)), data_size);
    src = sign_ext(src & (0xffffffff >> (32 - data_size)), data_size);
    if(result < src || (cpu.eflags.CF==1 && result == src)) cpu.eflags.CF = 1;
    else cpu.eflags.CF = 0;
}
void set_CF_sub(uint32_t dest, uint32_t src, size_t data_size){
    dest = sign_ext(dest & (0xffffffff >> (32 - data_size)), data_size);
    src = sign_ext(src & (0xffffffff >> (32 - data_size)), data_size);
    cpu.eflags.CF = dest < src;
}
void set_CF_sbb(uint32_t dest, uint32_t src, size_t data_size){
    dest = sign_ext(dest & (0xffffffff >> (32 - data_size)), data_size);
    src = sign_ext(src & (0xffffffff >> (32 - data_size)), data_size);
    if(dest < src || (cpu.eflags.CF==1 && dest == src)) cpu.eflags.CF = 1;
    else cpu.eflags.CF = 0;
}
//Overflow Flag ── Set if result is too large a positive number or too small a negative number (excluding sign-bit) to fit in destination operand; cleared otherwise.
void set_OF_add(uint32_t result, uint32_t src, uint32_t dest, size_t data_size) {
	switch(data_size) {
		case 8: 
			result = sign_ext(result & 0xFF, 8); 
			src = sign_ext(src & 0xFF, 8); 
			dest = sign_ext(dest & 0xFF, 8); 
			break;
		case 16: 
			result = sign_ext(result & 0xFFFF, 16); 
			src = sign_ext(src & 0xFFFF, 16); 
			dest = sign_ext(dest & 0xFFFF, 16); 
			break;
		default: break;
	}
	if(sign(src) == sign(dest)) {
		if(sign(src) != sign(result))
			cpu.eflags.OF = 1;
		else
			cpu.eflags.OF = 0;
	} else {
		cpu.eflags.OF = 0;
	}
}
void set_OF_sub(uint32_t result, uint32_t src, uint32_t dest, size_t data_size) {
    //signed
	switch(data_size) {
		case 8: 
			result = sign_ext(result & 0xFF, 8); 
			src = sign_ext(src & 0xFF, 8); 
			dest = sign_ext(dest & 0xFF, 8); 
			break;
		case 16: 
			result = sign_ext(result & 0xFFFF, 16); 
			src = sign_ext(src & 0xFFFF, 16); 
			dest = sign_ext(dest & 0xFFFF, 16); 
			break;
		default: break;
	}
	if(sign(src) != sign(dest)) {
		if(sign(src) == sign(result))
			cpu.eflags.OF = 1;
		else
			cpu.eflags.OF = 0;
	} 
	else {
		cpu.eflags.OF = 0;
	}
	//unsigned
}
/*--------------------------------------------*/

uint32_t alu_add(uint32_t src, uint32_t dest, size_t data_size)
{   //res = dest + src
#ifdef NEMU_REF_ALU
	return __ref_alu_add(src, dest, data_size);
#else
    uint32_t res = dest + src; 
    set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
    set_CF_add(res, src, data_size); 
    set_OF_add(res, src, dest, data_size);
    return res & (0xFFFFFFFF >> (32 - data_size)); 
#endif
}

uint32_t alu_adc(uint32_t src, uint32_t dest, size_t data_size)
{   //res = dest + src + CF
#ifdef NEMU_REF_ALU
	return __ref_alu_adc(src, dest, data_size);
#else
	uint32_t res = dest + src + cpu.eflags.CF;  
    set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
    set_CF_adc(res, src, data_size); 
    set_OF_add(res, src, dest, data_size);
    return res & (0xFFFFFFFF >> (32 - data_size)); 
#endif
}

uint32_t alu_sub(uint32_t src, uint32_t dest, size_t data_size)
{   //res = dest - src
#ifdef NEMU_REF_ALU
	return __ref_alu_sub(src, dest, data_size);
#else
    uint32_t res = 0;
    res = dest - src;  
	set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
    set_CF_sub(dest, src, data_size);
    set_OF_sub(res, src, dest, data_size);
	return res & (0xFFFFFFFF >> (32 - data_size)); 
#endif
}

uint32_t alu_sbb(uint32_t src, uint32_t dest, size_t data_size)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_sbb(src, dest, data_size);
#else
	uint32_t res = 0;
    res = dest - src - cpu.eflags.CF;  
	set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
    set_CF_sbb(dest, src, data_size);
    set_OF_sub(res, src, dest, data_size);
	return res & (0xFFFFFFFF >> (32 - data_size)); 
#endif
}

uint64_t alu_mul(uint32_t src, uint32_t dest, size_t data_size)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_mul(src, dest, data_size);
#else
    uint64_t res = 0;
    uint64_t ss = src;
    uint64_t dd = dest;
    res = ss * dd;
    if((res >> data_size) == 0) {
        cpu.eflags.OF = 0;
        cpu.eflags.CF = 0;
    }
    else{
        cpu.eflags.OF = 1;
        cpu.eflags.CF = 1;
    }
	return res; 
#endif
}

int64_t alu_imul(int32_t src, int32_t dest, size_t data_size)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_imul(src, dest, data_size);
#else
	uint64_t res = 0;
    uint64_t ss = sign_ext_64(src, data_size);
    uint64_t dd = sign_ext_64(dest, data_size);
    res = ss * dd;
    /*if(((res& (0xFFFFFFFF<<(data_size-1)))==0) ||
    ((res& (0xFFFFFFFF<<(data_size-1))) == (0xFFFFFFFF<<(data_size-1)))){
        cpu.eflags.OF = 0;
        cpu.eflags.CF = 0;
    }
    else{
        cpu.eflags.OF = 1;
        cpu.eflags.CF = 1;
    }*/
	return res; 
#endif
}

uint32_t alu_div(uint64_t src, uint64_t dest, size_t data_size)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_div(src, dest, data_size);
#else
    if(src == 0) return 0;
    uint32_t res = dest / src;
    return res & (0xFFFFFFFF >> (32 - data_size));
#endif
}

// need to implement alu_imod before testing
int32_t alu_idiv(int64_t src, int64_t dest, size_t data_size)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_idiv(src, dest, data_size);
#else
	if(src == 0) return 0;
    uint32_t res = dest / src;
    return sign_ext(res & (0xFFFFFFFF >> (32 - data_size)), data_size);
#endif
}

uint32_t alu_mod(uint64_t src, uint64_t dest)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_mod(src, dest);
#else
	return dest % src;
#endif
}

int32_t alu_imod(int64_t src, int64_t dest)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_imod(src, dest);
#else
	return dest % src;
#endif
}

uint32_t alu_and(uint32_t src, uint32_t dest, size_t data_size)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_and(src, dest, data_size);
#else
    uint32_t res = 0;
    res = src & dest;
    cpu.eflags.CF = 0;
    cpu.eflags.OF = 0;
    set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
	return res & (0xFFFFFFFF >> (32 - data_size)); 
#endif
}

uint32_t alu_xor(uint32_t src, uint32_t dest, size_t data_size)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_xor(src, dest, data_size);
#else
    uint32_t res = 0;
    res = src ^ dest;
    cpu.eflags.CF = 0;
    cpu.eflags.OF = 0;
    set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
	return res & (0xFFFFFFFF >> (32 - data_size)); 
#endif
}

uint32_t alu_or(uint32_t src, uint32_t dest, size_t data_size)
{
#ifdef NEMU_REF_ALU
	return __ref_alu_or(src, dest, data_size);
#else
	uint32_t res = 0;
    res = src | dest;
    cpu.eflags.CF = 0;
    cpu.eflags.OF = 0;
    set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
	return res & (0xFFFFFFFF >> (32 - data_size)); 
#endif
}
uint32_t alu_shl(uint32_t src, uint32_t dest, size_t data_size)
{   
#ifdef NEMU_REF_ALU
	return __ref_alu_shl(src, dest, data_size);
#else
    uint32_t res = 0;
    res = dest & (0xFFFFFFFF >> (32 - data_size)); 
    res = res << src;
    cpu.eflags.CF = (dest >> (data_size-src)) %2;
    set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
    return ( res & (0xFFFFFFFF >> (32 - data_size))); 
#endif
}

uint32_t alu_shr(uint32_t src, uint32_t dest, size_t data_size)
{ // 逻辑右移 高位补0
#ifdef NEMU_REF_ALU
	return __ref_alu_shr(src, dest, data_size);
#else
    uint32_t res = 0;
    res = dest & (0xFFFFFFFF >> (32 - data_size)); 
	while(src !=0){
	    cpu.eflags.CF = res %2;
	    res = (res>>1) & 0x7fffffff;
	    src--;
	}
	set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
	return ( res & (0xFFFFFFFF >> (32 - data_size))); 
#endif
}

uint32_t alu_sar(uint32_t src, uint32_t dest, size_t data_size)
{ // 算术右移 高位补符 
#ifdef NEMU_REF_ALU
	return __ref_alu_sar(src, dest, data_size);
#else
	uint32_t res = 0;
    res = dest & (0xFFFFFFFF >> (32 - data_size)); 
    uint32_t sign_bit = ( res>>(data_size-1) )%2;
    
	while(src !=0){
	    cpu.eflags.CF = res %2;
	    res = ((res>>1) & 0x7fffffff) | (sign_bit<<(data_size-1)) ;
	    src--;
	}
	set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
	return ( res & (0xFFFFFFFF >> (32 - data_size))); 
#endif
}

uint32_t alu_sal(uint32_t src, uint32_t dest, size_t data_size)
{   
#ifdef NEMU_REF_ALU
	return __ref_alu_sal(src, dest, data_size);
#else
	uint32_t res = 0;
    res = dest & (0xFFFFFFFF >> (32 - data_size)); 
    res = res << (src-1);
    cpu.eflags.CF = (res >> (data_size-1)) %2;
    res = res << 1;
    set_ZF(res, data_size);
    set_SF(res, data_size);
    set_PF(res);
    return ( res & (0xFFFFFFFF >> (32 - data_size))); 
#endif
}
