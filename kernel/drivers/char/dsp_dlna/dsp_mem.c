#include <linux/module.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include "dsp_mem.h"
#include "silan_memory.h"

int dsp_mem_init(DSP_CTL_INFO *dsp_info)
{
#if 0//def CONFIG_MIPS_SILAN_DLNA
    dsp_info->area0_map_cpu=(unsigned char*)dma_alloc_coherent(dsp_info->dev,DEV_FIRMWARE_SIZE,(dma_addr_t*)&dsp_info->area0_map_dma,GFP_KERNEL);
	if(dsp_info->area0_map_cpu == NULL)
		return -ENOMEM;
#else
	dsp_info->area0_map_dma = 0;
    dsp_info->area0_map_cpu = NULL;

	dsp_info->area0_map_dma = (dma_addr_t)prom_phy_mem_malloc(DEV_FIRMWARE_SIZE ,0);
	if(unlikely(0 == dsp_info->area0_map_dma))
	{
		printk("dsp mem malloc failed\n");
		return -ENOMEM;
	}		
 				
	dsp_info->area0_map_cpu = (u8 *)ioremap_nocache(dsp_info->area0_map_dma,DEV_FIRMWARE_SIZE);
	if(unlikely(NULL == dsp_info->area0_map_cpu))
	{
		printk("dsp mem ioremap failed\n");
		return -ENOMEM;
	}	
#endif    
	dsp_info->area0_len = DEV_FIRMWARE_SIZE;

	dsp_set_area0_attr(SILAN_DSP0,dsp_info->area0_map_dma,dsp_info->area0_len);

    printk("[DSP] AREA 0 CFG INFO: ADDR: 0x%d,  SIZE: %dKB\n",dsp_info->area0_map_dma,dsp_info->area0_len/1024);

    return 0;
}

