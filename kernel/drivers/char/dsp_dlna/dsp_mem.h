#ifndef _DSP_MEM_H
#define _DSP_MEM_H
#include <asm/page.h>
#include "dsp_ctl.h"

#ifdef CONFIG_MIPS_SILAN_DLNA
#define DEV_FIRMWARE_SIZE			0x300000//0x300000//0x14b000
#else
#define DEV_FIRMWARE_SIZE			0x800000
#endif
#define MIPS_DSP_SYSCHANGE_SIZE		(PAGE_SIZE)	
#define MIPS_DSP_IN_SIZE			0x100000
#define MIPS_DSP_OUT_SIZE			0x100000

#define MIPS_DSP_MMAP_BLOCK			0x80000 // input: 0x2000 output: 0x1e000
#define MMAP_BLOCK_INPUT			0x8000
#define MMAP_BLOCK_OUTPUT			0x78000

#define MIPS_DSP_EXCHANGE_SIZE		(MIPS_DSP_SYSCHANGE_SIZE+MIPS_DSP_IN_SIZE+MIPS_DSP_OUT_SIZE)

int dsp_mem_init(DSP_CTL_INFO *dsp_info);

#endif

