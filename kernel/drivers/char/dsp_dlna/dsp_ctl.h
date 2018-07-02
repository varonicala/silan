#ifndef _DSP_CTL_H_
#define _DSP_CTL_H_
#include "mailbox.h"

/*dsp cfg0*/
#define DSP_CONFIG_STALL			(0x1<<1)
#define DSP_CONFIG_VECTOR			(0x1<<2)
#define DSP_CONFIG_SW_INT			(0x1<<8)

/*dsp cfg1*/

/*dsp cfg2*/
#define DSP_AREA0_OFFSET(x)			((x)<<0)

/*dsp cfg3*/
#define DSP_AREA0_LEN(x)			((x)<<0)

/*dsp cfg4*/
#define DSP_AREA1_OFFSET(x)			((x)<<0)

/*dsp cfg4*/
#define DSP_AREA1_LEN(x)			((x)<<0)

int dsp_set_stall(SILAN_DSPNAME dspname,int enable);
int dsp_set_mips2dsp_int(SILAN_DSPNAME dspname,int enable);
int dsp_set_area0_attr(SILAN_DSPNAME dspname,u32 offset,u32 size);
int dsp_set_area1_attr(SILAN_DSPNAME dspname,u32 offset,u32 size);
int dsp_mmap_addr(SILAN_DSPNAME dspname,dma_addr_t mips_addr,dma_addr_t *dsp_addr);
int dsp_get_input_addr(SILAN_DSPNAME dspname,u8** cpu_addr,dma_addr_t *dsp_addr, u32 taskid);
int dsp_get_output_addr(SILAN_DSPNAME dspname,u8** cpu_addr,dma_addr_t *dsp_addr, u32 taskid);
int dsp_get_system_addr(SILAN_DSPNAME dspname,u8** cpu_addr,dma_addr_t *dsp_addr);
int dsp_load_firmware(void);
int dsp_ctl_probe(struct platform_device *pdev);


typedef struct dsp_ctl_info
{
	u32 dsp0_base;
	u32 dsp1_base;
	/*dsp code area*/
	u8* area0_map_cpu;
	dma_addr_t area0_map_dma;
	u32 area0_len;
	/*dsp data exchange area*/
	u8* area1_map_cpu;
	dma_addr_t area1_map_dma;
	u32 area1_len;

	struct device *dev;
	u8* mmap_cpu;
	u32 mmap_size;

	u8* mmap_addr[16];
	u32 mmap_block_size;
#ifdef CONFIG_MIPS_SILAN_DLNA    
    u32 dlna_ring_buff_size;
    dma_addr_t dsp_dlna_ring_buffer_dma;
    u8*dsp_dlna_ring_buffer_cpu;
#endif
}DSP_CTL_INFO;

#endif

