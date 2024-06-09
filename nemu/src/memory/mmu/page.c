#include "cpu/cpu.h"
#include "memory/memory.h"

// translate from linear address to physical address

typedef union{
    struct{
        uint32_t offset: 12;
        uint32_t page: 10;
        uint32_t dir: 10;
    };
    uint32_t val;
}laddr_u;


paddr_t page_translate(laddr_t laddr)
{
#ifndef TLB_ENABLED
    uint32_t paddr = laddr;

    laddr_u la;
    la.val = laddr;
    
	uint32_t pgdir = cpu.cr3.dir << 12;
	uint32_t pgtbl = paddr_read(pgdir + la.dir*4, 4);
	assert(pgtbl & 1);
	
	paddr = paddr_read((pgtbl & 0xfffff000) + la.page*4, 4);
	assert(paddr & 1);
	
	paddr = (paddr & 0xfffff000) + la.offset;
	
	return paddr;
	
#else
	return tlb_read(laddr) | (laddr & PAGE_MASK);
#endif
}
