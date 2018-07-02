#include <linux/module.h>
#include <linux/types.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <silan_resources.h>
#include "dsp_ctl.h"
#include "dsp_mem.h"

#define DSP_CTL_REG					0
#define DSP_STUS_REG				4
#define DSP_AREA0_REG				8
#define DSP_AREA0_LEN_REG			12
#define DSP_AREA1_REG				16
#define DSP_AREA1_LEN_REG			20

#define DSP_MMAP_AREA1_OFFSET		0x8000000;
#define DEV_FIRMWARE_NAME			"dsp_firmware.bin"

DSP_CTL_INFO dsp_info;

int dsp_set_stall(SILAN_DSPNAME dspname,int enable)
{ 
	u32 base,regval;
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	if(dspname == SILAN_DSP0)
		base = dsp_info.dsp0_base;
	else
		base = dsp_info.dsp1_base;

	regval = __raw_readl((void __iomem *)(base+DSP_CTL_REG));
	if(enable)
		regval |= DSP_CONFIG_STALL;
	else
		regval &= ~DSP_CONFIG_STALL;
	__raw_writel(regval,(void __iomem *)(base+DSP_CTL_REG));
	__raw_writel(regval,(void __iomem *)(base+DSP_CTL_REG));
    return 0;
}

int dsp_set_mips2dsp_int(SILAN_DSPNAME dspname,int enable)
{ 
	u32 base,regval;
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	if(dspname == SILAN_DSP0)
		base = dsp_info.dsp0_base;
	else
		base = dsp_info.dsp1_base;

	regval = __raw_readl((void __iomem *)(base+DSP_CTL_REG));
	if(enable)
		regval |= DSP_CONFIG_SW_INT;
	else
		regval &= ~DSP_CONFIG_SW_INT;

	__raw_writel(regval,(void __iomem *)(base+DSP_CTL_REG));
	__raw_writel(regval,(void __iomem *)(base+DSP_CTL_REG));

	return 0;
}

int dsp_set_area0_attr(SILAN_DSPNAME dspname,u32 offset,u32 size)
{
	u32 base;
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	if(dspname == SILAN_DSP0)
		base = dsp_info.dsp0_base;
	else
		base = dsp_info.dsp1_base;

	__raw_writel(offset,(void __iomem *)(base+DSP_AREA0_REG));
	__raw_writel(offset,(void __iomem *)(base+DSP_AREA0_REG));
	__raw_writel(size,(void __iomem *)(base+DSP_AREA0_LEN_REG));
	__raw_writel(size,(void __iomem *)(base+DSP_AREA0_LEN_REG));
    return 0;
}

int dsp_set_area1_attr(SILAN_DSPNAME dspname,u32 offset,u32 size)
{
	u32 base;
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	if(dspname == SILAN_DSP0)
		base = dsp_info.dsp0_base;
	else
		base = dsp_info.dsp1_base;

	__raw_writel(offset,(void __iomem *)(base+DSP_AREA1_REG));
	__raw_writel(offset,(void __iomem *)(base+DSP_AREA1_REG));
	__raw_writel(size,(void __iomem *)(base+DSP_AREA1_LEN_REG));
	__raw_writel(size,(void __iomem *)(base+DSP_AREA1_LEN_REG));
    return 0;
}

int dsp_mmap_addr(SILAN_DSPNAME dspname,dma_addr_t mips_addr,dma_addr_t *dsp_addr)
{

	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	if(dsp_addr == NULL)
		return -1;

	if(dspname == SILAN_DSP0)
	{
		if((mips_addr < dsp_info.area1_map_dma) || (mips_addr>= dsp_info.area1_len+dsp_info.area1_map_dma))
			return -1;
		*dsp_addr = mips_addr-dsp_info.area1_map_dma+DSP_MMAP_AREA1_OFFSET;
	}
	return 0;
}

int dsp_get_input_addr(SILAN_DSPNAME dspname,u8 **cpu_addr,dma_addr_t *dsp_addr, u32 taskid)
{
	*cpu_addr =	dsp_info.mmap_addr[taskid]; 
	dsp_mmap_addr(dspname,dsp_info.area1_map_dma+MIPS_DSP_SYSCHANGE_SIZE+MIPS_DSP_MMAP_BLOCK*taskid,dsp_addr);
	
	return 0;
}


int dsp_get_output_addr(SILAN_DSPNAME dspname,u8** cpu_addr,dma_addr_t *dsp_addr, u32 taskid)
{
	*cpu_addr = dsp_info.mmap_addr[taskid]+MMAP_BLOCK_INPUT;
	dsp_mmap_addr(dspname,dsp_info.area1_map_dma+MIPS_DSP_SYSCHANGE_SIZE+MIPS_DSP_MMAP_BLOCK*taskid+MMAP_BLOCK_INPUT,dsp_addr);

	return 0;
}


int dsp_get_system_addr(SILAN_DSPNAME dspname,u8** cpu_addr,dma_addr_t *dsp_addr)
{
	*cpu_addr = dsp_info.area1_map_cpu;
	dsp_mmap_addr(dspname,dsp_info.area1_map_dma,dsp_addr);
	return 0;
}

int dsp_load_firmware(void)
{
	const struct firmware *silan_dsp_fw;
	struct platform_device *pdev;
	pdev = platform_device_register_simple("dspfirmware",0,NULL,0);
	if(IS_ERR(pdev))
	{
		printk("dspcore:failed to register firmware \n");
		return PTR_ERR(pdev);
	}

	printk("request firmware size  000 , %d\n", silan_dsp_fw->size);
	if(request_firmware(&silan_dsp_fw,DEV_FIRMWARE_NAME,&pdev->dev) < 0)
	{
		printk("request firmware timeout..\n");
		platform_device_unregister(pdev);
		return -ENODEV;
	}

	platform_device_unregister(pdev);

	printk("request firmware size, %p  %d\n", dsp_info.area0_map_cpu, silan_dsp_fw->size);
	memcpy((unsigned char*)dsp_info.area0_map_cpu,silan_dsp_fw->data,silan_dsp_fw->size);

	//printk("request firmware size   ddd , %p  %d\n", dsp_info.area0_map_cpu, silan_dsp_fw->size);
	release_firmware(silan_dsp_fw);

	//printk("request firmware size   oooddd , %p  %d\n", dsp_info.area0_map_cpu, silan_dsp_fw->size);
	return 0;
}

int dsp_ctl_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct clk *clk;
	
	res = platform_get_resource(pdev,IORESOURCE_MEM,0);
	if(res == NULL)
	{
		printk("can't find DSP0 resource\n");
		return - ENOENT;
	}	
    
    printk("[DSP] CFG BASE: 0x%x\n", res->start);
    
	dsp_info.dsp0_base = (u32)ioremap(res->start,(res->end - res->start));
	if(dsp_info.dsp0_base ==(u32) NULL)
	{
		printk("cannot map DSP0 io\n");
		return -ENXIO;
	}

	res = platform_get_resource(pdev,IORESOURCE_MEM,1);	
	if(res == NULL)
	{
		printk("can't find DSP1 resource\n");
		return - ENOENT;
	}	
    
    printk("[DSP] CXC BASE: 0x%x\n", res->start);
    
	dsp_info.dsp1_base = (u32)ioremap(res->start,(res->end - res->start));
	if(dsp_info.dsp1_base == (u32)NULL)
	{
		printk("cannot map DSP1 io\n");
		return - ENOENT;
	}

	dsp_info.dev = &pdev->dev;

	clk = clk_get(NULL, "dsp0");
	if (IS_ERR(clk)) 
	{
		printk( KERN_ERR "Failed to set vpp clk\n");
		return PTR_ERR(clk);
	}
	clk_enable(clk);

	dsp_set_stall(SILAN_DSP0,1);
	return dsp_mem_init(&dsp_info);
}
