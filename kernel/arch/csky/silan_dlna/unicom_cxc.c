 /*
 * Copyright (C) 2012 Silan Corporation
 * Written by: panjianguang <panjianguang@silan.com.cn>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include "silan_unicom.h"
static DEFINE_SPINLOCK(read_fifo_lock); 
static DEFINE_MUTEX(create_fifo_read_lock);

u32 unicom_base;
struct silan_unicom_cxc_priv 
{
	u8* unicom2mipsAddr;
	u32 size;
};

static inline u32 unicom_cxc_read_reg(unsigned int reg)
{
	return __raw_readl((void __iomem *)(unicom_base + reg));
}

static inline void unicom_cxc_write_reg(unsigned int val, unsigned int reg)
{
	__raw_writel(val, (void __iomem *)(unicom_base + reg));
}

/* Mailbox FIFO handle functions */
static int silan_unicom_cxc_fifo_read(struct silan_unicom_cxc *unicom, unsigned int *buffer,int module_id, int i, int num)
{
	
	spin_lock(&read_fifo_lock);
	if(!unicom_cxc_read_reg(UNICOM_MUTEX(module_id)))
	{	
		while(i < num)
		{
			
			*(buffer++) = unicom_cxc_read_reg(UNICOM_DATA(i));
			i++;
		}
		
		unicom_cxc_write_reg(0,UNICOM_MUTEX(module_id));
		spin_unlock(&read_fifo_lock);
		return 0;
	}
	spin_unlock(&read_fifo_lock);
	return -1;
}

static int silan_unicom_cxc_fifo_write(struct silan_unicom_cxc *unicom, unsigned int *buffer, int module_id, int i, int num)
{
	
	if(!unicom_cxc_read_reg(UNICOM_MUTEX(module_id)))
	{
		while(i < num)
		{
			unicom_cxc_write_reg(*(buffer++),UNICOM_DATA(i));
			i++;
		}
	}
	unicom_cxc_write_reg(0,UNICOM_MUTEX(module_id));
	return 0;
}

int silan_unicom_cxc_get_time(void)
{
	printk("get_time flag\n");
	return 0;
}
	
int silan_unicom_cxc_set_time(void)
{
	printk("set_time flag\n");
//	unicom_cxc_write_reg(0, UNICOM_DATA(15));
	return 0;
}

int cxc_set_mcu2unicom_rtc_clr(void)
{
	unicom_cxc_write_reg(0x01,CXC_HOST_INT_CLR);	
	return 0;
}

int cxc_set_mcu2unicom_ir_clr(void)
{
	unicom_cxc_write_reg(0x02,CXC_HOST_INT_CLR);	
	return 0;
}

int cxc_set_mcu2unicom_keypad_clr(void)
{
	unicom_cxc_write_reg(0x08,CXC_HOST_INT_CLR);	
	return 0;
}

int cxc_set_mcu2unicom_spiflash_clr(void)
{
	unicom_cxc_write_reg(0x04,CXC_HOST_INT_CLR);	
	return 0;
}

int cxc_set_mips2unicom_rtc_int(void)
{
	unicom_cxc_write_reg(0,CXC_UNICOM_INT_MASK);	
	unicom_cxc_write_reg(0x01,CXC_UNICOM_INT_SET);	
	return 0;
}
int cxc_set_mips2unicom_spiflash_int(void)
{
	unicom_cxc_write_reg(0,CXC_UNICOM_INT_MASK);	
	unicom_cxc_write_reg(0x04,CXC_UNICOM_INT_SET);	
	return 0;
}

int silan_unicom_cxc_reg_init(void)
{
	unicom_cxc_write_reg(1,CXC_UNICOM_INT_MASK);	
	unicom_cxc_write_reg(0xffffffff,CXC_HOST_INT_CLR);
	unicom_cxc_write_reg(0xffffffff,CXC_UNICOM_INT_CLR);

	return 0;
}

static struct silan_unicom_cxc_ops silan_unicom_cxc_ops = 
{
	.fifo_read	= silan_unicom_cxc_fifo_read,
	.fifo_write	= silan_unicom_cxc_fifo_write,
};

/* UNICOM private */
static struct silan_unicom_cxc_priv silan_unicom_cxc_priv = 
{

};

static struct silan_unicom_cxc unicom_info = {
	.name	= "silan_unicom_cxc",
	.ops	= &silan_unicom_cxc_ops,
	.priv	= &silan_unicom_cxc_priv,
};

struct silan_unicom_cxc* silan_unicom_cxc_get(const char *name)
{
	if(strcmp(unicom_info.name,name) != 0)
		return NULL;
	else
	{
		printk("unicom.name squ\n");
	}
	return &unicom_info;
	
}

int silan_unicom_cxc_put(const char *name)
{
	if(strcmp(unicom_info.name,name) != 0)
		return -ENODEV;

	return 0;
}

int silan_unicom_cxc_probe(struct platform_device *pdev)
{
	struct resource *res;
	res = platform_get_resource(pdev,IORESOURCE_MEM,0);
	if(res == NULL)
	{
		printk("can't find UNICOM_CXC resource\n");
		return - ENOENT;
	}
	else
	{
		printk(" find UNICOM_CXC resource ok\n");
	}

	unicom_base =(u32)ioremap(res->start,(res->end - res->start)+1);

	if(unicom_base == (u32)NULL)
	{
		printk("cannot map UNICOM_ CXC io\n");
		return -ENXIO;
	}
	else
	{
		printk("get UNICOM_CXC map ok \n");
	}
	
	unicom_info.rtc_irq = platform_get_irq(pdev,0);							//get rtc_irq
	if(unicom_info.rtc_irq ==  -ENXIO)
	{
		printk("cannot find RTC_IRQ\n");
		return -ENXIO;
	}
	else
	{
		printk("find RTC_IRQ ok \n");
	}

	unicom_info.ir_irq = platform_get_irq(pdev,0);							//get ir_irq
	if(unicom_info.ir_irq ==  -ENXIO)
	{
		printk("cannot find IR_IRQ\n");
		return -ENXIO;
	}
	else
	{
		printk("find IR_IRQ ok \n");
	}
	
	unicom_info.spiflash_irq = platform_get_irq(pdev,0);							//get ir_irq
	if(unicom_info.spiflash_irq ==  -ENXIO)
	{
		printk("cannot find SPIFLASH_IRQ\n");
		return -ENXIO;
	}
	else
	{
		printk("find SPIFLASH_IRQ ok \n");
	}
	
	unicom_info.keypad_irq = platform_get_irq(pdev,0);							//get ir_irq
	if(unicom_info.keypad_irq ==  -ENXIO)
	{
		printk("cannot find KEYPAD_IRQ\n");
		return -ENXIO;
	}
	else
	{
		printk("find KEYPAD_IRQ ok \n");
	}
	return 0;
}

