#include "nemu.h"
#include "cpu/fpu.h"

FPU fpu;
// special values
FLOAT p_zero, n_zero, p_inf, n_inf, p_nan, n_nan;


// the last three bits of the significand are reserved for the GRS bits
inline uint32_t internal_normalize(uint32_t sign, int32_t exp, uint64_t sig_grs) 
/* normalization 规格化 
sign符号（负为1 正为0）
exp 阶码（含偏置常数） 32位带符号数
sig_grs 尾数（包含隐藏1、保护位） 64位无符号数
    小数点后有26位 
    （加减中小数点前最多有两位 用64位装是为了照顾乘除法）*/
{
    
	bool overflow = false; // true if the result is INFINITY or 0 during normalize
	uint32_t sticky = 0;
    //--------------- normalize toward right 右规 ---------------//
	if ((sig_grs >> (23 + 3)) > 1 || exp < 0)
	{
		
		while ((((sig_grs >> (23 + 3)) > 1) && exp < 0xff) // condition 1
			   ||										   // or
			   (sig_grs > 0x04 && exp < 0)				   // condition 2
			   )
		{
            /* TODO: shift right, pay attention to sticky bit*/
            //尾数右移 阶码自增 粘位一直粘着
        	sticky = sticky | (sig_grs & 0x1);
        	sig_grs = sig_grs >> 1;
            sig_grs |= sticky;
            exp = exp + 1;
		}

		if (exp >= 0xff) //阶码上溢 
		{
			/* TODO: assign the number to infinity */
			exp = 0xff;
			sig_grs = 0;
			overflow = true;
		}
		if (exp == 0) //非规 阶码偏置常数-126 需要再右移一次
		{
			// we have a denormal here, the exponent is 0, but means 2^-126,
			// as a result, the significand should shift right once more
			/* TODO: shift right, pay attention to sticky bit*/
			sticky = sticky | (sig_grs & 0x1);
        	sig_grs = sig_grs >> 1;
            sig_grs |= sticky;
		}
		if (exp < 0) //阶码下溢
		{
			/* TODO: assign the number to zero */
			exp = 0;
			sig_grs = 0;
			overflow = true;
		}
	}
	//--------------- normalize toward left 左规 ---------------//
	else if (((sig_grs >> (23 + 3)) == 0) && exp > 0)  
	{
		while (((sig_grs >> (23 + 3)) == 0) && exp > 0)
		{
			/* TODO: shift left */
        	sig_grs = sig_grs << 1;
        	exp = exp - 1;
		}
		if (exp == 0) // denormal 非规 尾数额外右移一次
		{
			/* TODO: shift right, pay attention to sticky bit*/
			sticky = sticky | (sig_grs & 0x1);
        	sig_grs = sig_grs >> 1;
            sig_grs |= sticky;
		}
	}
	// two denormals result in a normal
	else if (exp == 0 && sig_grs >> (23 + 3) == 1)
	{
		exp++;
	}
	
	//去掉三个保护位GRS：0不管123舍掉4不确定567进位
    //4处理方法：就近舍入到偶数：保证舍入后fraction最低位为0
	if (!overflow)
	{
		/* TODO: round up and remove the GRS bits */
		//舍入若产生尾数加1，有可能出现破坏规格化的情况
        //此时需要进行额外的一次右规并判断阶码上溢的情况
        uint32_t grs = sig_grs & 7;
        sig_grs >>= 3;
		if( (grs > 4) || ((grs == 4) && (sig_grs & 1)) ){
		    sig_grs++;
		}
        if((sig_grs >> 23) > 1) {
            sig_grs = sig_grs >> 1;
            exp = exp + 1;
            if(exp >= 0xff){
                exp = 0xff;
                sig_grs = 0;
                overflow = true;
            }
        }
        else if( ((sig_grs>>23) == 1) && exp == 0){
            exp++;
        }
	}
	FLOAT f;
	f.sign = sign;
	f.exponent = (uint32_t)(exp & 0xff);
	f.fraction = sig_grs; // here only the lowest 23 bits are kept
	return f.val;
}


CORNER_CASE_RULE corner_add[] = {
	{P_ZERO_F, P_ZERO_F, P_ZERO_F},
	{N_ZERO_F, P_ZERO_F, P_ZERO_F},
	{P_ZERO_F, N_ZERO_F, P_ZERO_F},
	{N_ZERO_F, N_ZERO_F, N_ZERO_F},
	{P_INF_F, N_INF_F, N_NAN_F},
	{N_INF_F, P_INF_F, N_NAN_F},
	{P_INF_F, P_NAN_F, P_NAN_F},
	{P_INF_F, N_NAN_F, N_NAN_F},
	{N_INF_F, P_NAN_F, P_NAN_F},
	{N_INF_F, N_NAN_F, N_NAN_F},
	{N_NAN_F, P_NAN_F, P_NAN_F},
};

// a + b
uint32_t internal_float_add(uint32_t b, uint32_t a)
{

	// corner cases
	int i = 0;
	for (; i < sizeof(corner_add) / sizeof(CORNER_CASE_RULE); i++){
		if (a == corner_add[i].a && b == corner_add[i].b)
			return corner_add[i].res;	}
	if (a == P_ZERO_F || a == N_ZERO_F)	return b;
	if (b == P_ZERO_F || b == N_ZERO_F)	return a;
	FLOAT f, fa, fb;	fa.val = a;	fb.val = b;
	// infity, NaN
	if (fa.exponent == 0xff)	return a;
	if (fb.exponent == 0xff)	return b;
	if (fa.exponent > fb.exponent){
		fa.val = b;
		fb.val = a;
	}

	uint32_t sig_a, sig_b, sig_res;
	sig_a = fa.fraction;
	if (fa.exponent != 0)	sig_a |= 0x800000; // the hidden 1
	sig_b = fb.fraction;
	if (fb.exponent != 0)	sig_b |= 0x800000; // the hidden 1
    //现在的sig是补上隐藏1的尾数部分

	// alignment shift for fa 对阶 fa为小阶的 小阶向大阶看齐 尾数右移shift位
	//exp是带偏置常数的 (规格化数-127 or 非规格化数-126)
	uint32_t shift = 0;
	shift = (fb.exponent == 0 ? fb.exponent + 1 : fb.exponent) - (fa.exponent == 0 ? fa.exponent + 1 : fa.exponent);

	assert(shift >= 0);
	
    //对阶右移的时候不能扔掉右端位
    //引入三个保护位： guard保护位, round舍入位, sticky粘位
	sig_a = (sig_a << 3); 
	sig_b = (sig_b << 3);
    //隐藏1+尾数（23位）+保护位（3位）
	uint32_t sticky = 0;
	while (shift > 0)
	{
		sticky = sticky | (sig_a & 0x1);
		sig_a = sig_a >> 1;
		sig_a |= sticky;
		shift--;
	}
    //完成小阶向大阶看齐，阶码为大的那个阶码

    // fraction add 尾数加法 + 正负处理
	if (fa.sign)	sig_a *= -1;
	if (fb.sign)    sig_b *= -1;
	sig_res = sig_a + sig_b;
	if (sign(sig_res)){
		f.sign = 1;
		sig_res *= -1;
	}
	else f.sign = 0;
	
	//现在小数点后面有26位 不能直接砍掉后三位 要规格化一下
	uint32_t exp_res = fb.exponent;
	return internal_normalize(f.sign, exp_res, sig_res);
}

CORNER_CASE_RULE corner_sub[] = {
	{P_ZERO_F, P_ZERO_F, P_ZERO_F},
	{N_ZERO_F, P_ZERO_F, N_ZERO_F},
	{P_ZERO_F, N_ZERO_F, P_ZERO_F},
	{N_ZERO_F, N_ZERO_F, P_ZERO_F},
	{P_NAN_F, P_NAN_F, P_NAN_F},
	{N_NAN_F, P_NAN_F, P_NAN_F},
	{P_NAN_F, N_NAN_F, P_NAN_F},
	{N_NAN_F, N_NAN_F, N_NAN_F},
};

// a - b
uint32_t internal_float_sub(uint32_t b, uint32_t a)
{
	// change the sign of b
	int i = 0;
	for (; i < sizeof(corner_sub) / sizeof(CORNER_CASE_RULE); i++)
	{
		if (a == corner_sub[i].a && b == corner_sub[i].b)
			return corner_sub[i].res;
	}
	if (a == P_NAN_F || a == N_NAN_F)
		return a;
	if (b == P_NAN_F || b == N_NAN_F)
		return b;
	FLOAT fb;
	fb.val = b;
	fb.sign = ~fb.sign;
	return internal_float_add(fb.val, a);
}

CORNER_CASE_RULE corner_mul[] = {
	{P_ZERO_F, P_INF_F, N_NAN_F},
	{P_ZERO_F, N_INF_F, N_NAN_F},
	{N_ZERO_F, P_INF_F, N_NAN_F},
	{N_ZERO_F, N_INF_F, N_NAN_F},
	{P_INF_F, P_ZERO_F, N_NAN_F},
	{P_INF_F, N_ZERO_F, N_NAN_F},
	{N_INF_F, P_ZERO_F, N_NAN_F},
	{N_INF_F, N_ZERO_F, N_NAN_F},
};


//乘除法 尾数相乘除 阶码相加减

// a * b
uint32_t internal_float_mul(uint32_t b, uint32_t a)
{
	int i = 0;
	for (; i < sizeof(corner_mul) / sizeof(CORNER_CASE_RULE); i++)
	{
		if (a == corner_mul[i].a && b == corner_mul[i].b)
			return corner_mul[i].res;
	}

	if (a == P_NAN_F || a == N_NAN_F || b == P_NAN_F || b == N_NAN_F)
		return a == P_NAN_F || b == P_NAN_F ? P_NAN_F : N_NAN_F;

	FLOAT fa, fb, f;
	fa.val = a;
	fb.val = b;
	f.sign = fa.sign ^ fb.sign;

	if (a == P_ZERO_F || a == N_ZERO_F)
		return fa.sign ^ fb.sign ? N_ZERO_F : P_ZERO_F;
	if (b == P_ZERO_F || b == N_ZERO_F)
		return fa.sign ^ fb.sign ? N_ZERO_F : P_ZERO_F;
	if (a == P_INF_F || a == N_INF_F)
		return fa.sign ^ fb.sign ? N_INF_F : P_INF_F;
	if (b == P_INF_F || b == N_INF_F)
		return fa.sign ^ fb.sign ? N_INF_F : P_INF_F;

	uint64_t sig_a, sig_b, sig_res;
	sig_a = fa.fraction;
	if (fa.exponent != 0)
		sig_a |= 0x800000; // the hidden 1
	sig_b = fb.fraction;
	if (fb.exponent != 0)
		sig_b |= 0x800000; // the hidden 1

	if (fa.exponent == 0)
		fa.exponent++;
	if (fb.exponent == 0)
		fb.exponent++;

	sig_res = sig_a * sig_b; // 24b * 24b
	uint32_t exp_res = 0;

	/* TODO: exp_res = ? leave space for GRS bits. */
	exp_res = fa.exponent + fb.exponent - 127 - 23 + 3;
	//index.md : 在进行乘法时，两个整数表示的尾数通过乘法获取64位的乘积后，结果的阶码要减去23，并保留GRS bits，最终的阶码为fa.exponent + fb.exponent - 127 - 23 + 3
	return internal_normalize(f.sign, exp_res, sig_res);
}


CORNER_CASE_RULE corner_div[] = {
	{P_ZERO_F, P_ZERO_F, N_NAN_F},
	{N_ZERO_F, P_ZERO_F, N_NAN_F},
	{P_ZERO_F, N_ZERO_F, N_NAN_F},
	{N_ZERO_F, N_ZERO_F, N_NAN_F},
	{P_INF_F, P_ZERO_F, P_INF_F},
	{N_INF_F, P_ZERO_F, N_INF_F},
	{P_INF_F, N_ZERO_F, N_INF_F},
	{N_INF_F, N_ZERO_F, P_INF_F},
	{P_INF_F, P_INF_F, N_NAN_F},
	{N_INF_F, P_INF_F, N_NAN_F},
	{P_INF_F, N_INF_F, N_NAN_F},
	{N_INF_F, N_INF_F, N_NAN_F},
};
// a / b
uint32_t internal_float_div(uint32_t b, uint32_t a)
{

	int i = 0;
	for (; i < sizeof(corner_div) / sizeof(CORNER_CASE_RULE); i++)
	{
		if (a == corner_div[i].a && b == corner_div[i].b)
			return corner_div[i].res;
	}

	FLOAT f, fa, fb;
	fa.val = a;
	fb.val = b;

	if (a == P_NAN_F || a == N_NAN_F || b == P_NAN_F || b == N_NAN_F)
		return a == P_NAN_F || b == P_NAN_F ? P_NAN_F : N_NAN_F;
	if (a == P_INF_F || a == N_INF_F)
	{
		return fa.sign ^ fb.sign ? N_INF_F : P_INF_F;
	}
	if (b == P_ZERO_F || b == N_ZERO_F)
	{
		return fa.sign ^ fb.sign ? N_INF_F : P_INF_F;
	}
	if (a == P_ZERO_F || a == N_ZERO_F)
	{
		fa.sign = fa.sign ^ fb.sign;
		return fa.val;
	}
	if (b == P_INF_F || b == N_INF_F)
	{
		return fa.sign ^ fb.sign ? N_ZERO_F : P_ZERO_F;
	}

	f.sign = fa.sign ^ fb.sign;

	uint64_t sig_a, sig_b, sig_res;
	sig_a = fa.fraction;
	if (fa.exponent != 0)
		sig_a |= 0x800000; // the hidden 1
	sig_b = fb.fraction;
	if (fb.exponent != 0)
		sig_b |= 0x800000; // the hidden 1

	// efforts to maintain the precision of the result
	int shift = 0;
	while (sig_a >> 63 == 0)
	{
		sig_a <<= 1;
		shift++;
	}
	while ((sig_b & 0x1) == 0)
	{
		sig_b >>= 1;
		shift++;
	}

	sig_res = sig_a / sig_b;

	if (fa.exponent == 0)
		fa.exponent++;
	if (fb.exponent == 0)
		fb.exponent++;
	uint32_t exp_res = fa.exponent - fb.exponent + 127 - (shift - 23 - 3);
	return internal_normalize(f.sign, exp_res, sig_res);
}

void fpu_load(uint32_t val)
{
	fpu.status.top = fpu.status.top == 0 ? 7 : fpu.status.top - 1;
	fpu.regStack[fpu.status.top].val = val;
}

uint32_t fpu_store()
{
	uint32_t val = fpu.regStack[fpu.status.top].val;
	fpu.status.top = (fpu.status.top + 1) % 8;
	return val;
}

uint32_t fpu_peek()
{
	uint32_t val = fpu.regStack[fpu.status.top].val;
	return val;
}

void fpu_add(uint32_t val)
{
	/*
	float *a = (float*)&fpu.regStack[fpu.status.top].val;
	float *b = (float*)&val;
	float c = *a + *b;
	uint32_t *d = (uint32_t *)&c;
	fpu.regStack[fpu.status.top].val = *d;
	*/
	fpu.regStack[fpu.status.top].val = internal_float_add(val, fpu.regStack[fpu.status.top].val);
}

void fpu_add_idx(uint32_t idx, uint32_t store_idx)
{
	/*
	float *a = (float*)&fpu.regStack[fpu.status.top].val;
	float *b = (float*)&fpu.regStack[(fpu.status.top + idx) % 8].val;
	float c = *a + *b;
	uint32_t *d = (uint32_t *)&c;
	fpu.regStack[(fpu.status.top + store_idx) % 8].val = *d;
	*/
	uint32_t a = fpu.regStack[fpu.status.top].val;
	uint32_t b = fpu.regStack[(fpu.status.top + idx) % 8].val;
	fpu.regStack[(fpu.status.top + store_idx) % 8].val = internal_float_add(b, a);
}

void fpu_sub(uint32_t val)
{
	/*
	float *a = (float*)&fpu.regStack[fpu.status.top].val;
	float *b = (float*)&val;
	float c = *a - *b;
	//printf("f %f - %f = %f\n", *a, *b, c);
	uint32_t *d = (uint32_t *)&c;
	fpu.regStack[fpu.status.top].val = *d;
	*/
	fpu.regStack[fpu.status.top].val = internal_float_sub(val, fpu.regStack[fpu.status.top].val);
}

void fpu_mul(uint32_t val)
{
	/*
	float *a = (float*)&fpu.regStack[fpu.status.top].val;
	float *b = (float*)&val;
	float c = *a * *b;
	uint32_t *d = (uint32_t *)&c;
	fpu.regStack[fpu.status.top].val = *d;
	*/
	fpu.regStack[fpu.status.top].val = internal_float_mul(val, fpu.regStack[fpu.status.top].val);
}

void fpu_mul_idx(uint32_t idx, uint32_t store_idx)
{
	/*
	float *a = (float*)&fpu.regStack[fpu.status.top].val;
	float *b = (float*)&fpu.regStack[(fpu.status.top + idx) % 8].val;
	float c = *a * *b;
	uint32_t *d = (uint32_t *)&c;
	fpu.regStack[(fpu.status.top + store_idx) % 8].val = *d;
	*/
	uint32_t a = fpu.regStack[fpu.status.top].val;
	uint32_t b = fpu.regStack[(fpu.status.top + idx) % 8].val;
	fpu.regStack[(fpu.status.top + store_idx) % 8].val = internal_float_mul(b, a);
}

void fpu_div(uint32_t val)
{
	/*
	float *a = (float*)&fpu.regStack[fpu.status.top].val;
	float *b = (float*)&val;
	float c = *a / *b;
	// printf("f %f / %f = %f\n", *a, *b, c);
	uint32_t *d = (uint32_t *)&c;
	fpu.regStack[fpu.status.top].val = *d;
	*/
	fpu.regStack[fpu.status.top].val = internal_float_div(val, fpu.regStack[fpu.status.top].val);
}

void fpu_xch(uint32_t idx)
{
	idx = (fpu.status.top + idx) % 8;
	uint32_t temp = fpu.regStack[fpu.status.top].val;
	fpu.regStack[fpu.status.top].val = fpu.regStack[idx].val;
	fpu.regStack[idx].val = temp;
}

void fpu_copy(uint32_t idx)
{
	idx = (fpu.status.top + idx) % 8;
	fpu.regStack[idx].val = fpu.regStack[fpu.status.top].val;
}

void fpu_cmp(uint32_t idx)
{
	idx = (fpu.status.top + idx) % 8;
	float *a = (float *)&fpu.regStack[fpu.status.top].val;
	float *b = (float *)&fpu.regStack[idx].val;
	if (*a > *b)
	{
		fpu.status.c0 = fpu.status.c2 = fpu.status.c3 = 0;
		//printf("f %f > %f\n", *a, *b);
		//printf("f %x > %x\n", *((uint32_t *)a), *((uint32_t *)b));
	}
	else if (*a < *b)
	{
		fpu.status.c0 = 1;
		fpu.status.c2 = fpu.status.c3 = 0;
		//printf("f %f < %f\n", *a, *b);
	}
	else
	{
		fpu.status.c0 = fpu.status.c2 = 0;
		fpu.status.c3 = 1;
		//printf("f %f == %f\n", *a, *b);
	}
}

void fpu_cmpi(uint32_t idx)
{
	idx = (fpu.status.top + idx) % 8;
	float *a = (float *)&fpu.regStack[fpu.status.top].val;
	float *b = (float *)&fpu.regStack[idx].val;
	if (*a > *b)
	{
		cpu.eflags.ZF = cpu.eflags.PF = cpu.eflags.CF = 0;
	}
	else if (*a < *b)
	{
		cpu.eflags.ZF = cpu.eflags.PF = 0;
		cpu.eflags.CF = 1;
	}
	else
	{
		cpu.eflags.CF = cpu.eflags.PF = 0;
		cpu.eflags.ZF = 1;
	}
}
