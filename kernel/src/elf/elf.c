#include "common.h"
#include "memory.h"
#include "string.h"

#include <elf.h>

#ifdef HAS_DEVICE_IDE
#define ELF_OFFSET_IN_DISK 0
#endif

#define STACK_SIZE (1 << 20)

void ide_read(uint8_t *, uint32_t, uint32_t);
void create_video_mapping();
uint32_t get_ucr3();

uint32_t loader()
{
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph, *eph;

#ifdef HAS_DEVICE_IDE
	uint8_t buf[4096];
	ide_read(buf, ELF_OFFSET_IN_DISK, 4096);
	elf = (void *)buf;
	Log("ELF loading from hard disk.");
#else
	elf = (void *)0x0;
	Log("ELF loading from ram disk.");
#endif

	/* Load each program segment */
	ph = (void *)elf + elf->e_phoff; //e_phoff程序头表在ELF文件中的偏移量
	eph = ph + elf->e_phnum; //e_phnum程序头表中包含表项的个数
	for (; ph < eph; ph++)
	{
		if (ph->p_type == PT_LOAD)
		{
		    uint32_t va = ph->p_vaddr;
#ifdef IA32_PAGE
            
            uint32_t bg_addr = mm_malloc(va, ph->p_memsz);
            uint32_t pa = (bg_addr & 0xfffff000) + (va & 0xfff);
            va = (uint32_t)(pa_to_va(pa));

#endif
            /* TODO: copy the segment from the ELF file to its proper memory area */
            /*将ELF文件中起始于p_offset，大小为p_filesz字节的数据拷贝到内存中起始于p_vaddr的位置*/
            memcpy((void*)va, (void*)((uint32_t)elf+ph->p_offset) ,ph->p_filesz);
            /* TODO: zeror the memory area [vaddr + file_sz, vaddr + mem_sz) */
            /*并将内存中剩余的p_memsz - p_filesz字节的内容清零*/
            memset((void*)(va + ph->p_filesz), 0, ph->p_memsz - ph->p_filesz);

#ifdef IA32_PAGE
			/* Record the program break for future use */
			extern uint32_t brk;
			uint32_t new_brk = ph->p_vaddr + ph->p_memsz - 1;
			if (brk < new_brk)
			{
				brk = new_brk;
			}
#endif
		}
	}

	volatile uint32_t entry = elf->e_entry;

#ifdef IA32_PAGE
	mm_malloc(KOFFSET - STACK_SIZE, STACK_SIZE);
#ifdef HAS_DEVICE_VGA
	create_video_mapping();
#endif
	write_cr3(get_ucr3());
#endif
	return entry;
}
