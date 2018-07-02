#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <silan_memory.h>
#include "silan_unicom.h"

#define MBRIDGE_ADDR_SIZE  	16

u32  framebuf_dma;
u32  vir_mbridge_addr;
/*
static int silan_unicom_mbridge_fifo_read()
{

}


static int silan_unicom_mbridge_fifo_write()
{

}
*/

int silan_unicom_mbridge_probe(struct platform_device *pdev)
{
	int size;

	size =  MBRIDGE_ADDR_SIZE;
	framebuf_dma = prom_phy_mem_malloc(size, SILAN_DEV_UNICOM);
	if(!framebuf_dma)
	{
		printk("unicom_mbridge dma buffer malloc failed\n");
        ret = -ENOMEM;
	}
	else
	{
		printk("framebuf_dma=:%#x\n",framebuf_dma);
	}
	vir_mbridge_addr = phys_to_virt(framebuf_dma);

}

