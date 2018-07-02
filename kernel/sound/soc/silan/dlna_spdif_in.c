/* spdif.c
* ALSA SoC Audio Layer -Silan S/PDIF Controller driver
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include <silan_padmux.h>
#include <silan_irq.h>

#include "suv_pcm.h"
#include "dlna_spdif_in.h"

#define RX_FLS       4
#define DEVICE_NAME "silan-spdif-in"

struct spdifin_status_info{
    int lock;
    int rate;
    int rawdata;
    int set_padmux;
};
struct spdifin_status_info   spdifin_status;
struct silan_spdif_info {
	u32 phy_base;
	spinlock_t lock;
	struct device *dev;
	void __iomem *regs;
	struct clk *sclk;
	struct sl_pcm_dma_params dma_params;
};
struct dentry *my_debugfs_root;
static struct silan_spdif_info spdif_info;

static void silan_spdif_reset(void)
{
    u32 regval;
    regval = readl(0xba000018);
    regval &= ~(1<<2);
    regval &= ~(1<<3);
    writel(regval, 0xba000018);
    regval |= (1<<2);
    regval |= (1<<3);
    writel(regval, 0xba000018);
}

static void silan_spdif_reinit(void)
{
	struct silan_spdif_info *spdif = &spdif_info;
	void __iomem *regs = spdif->regs;
	u32 sys_clk, ratio_32k, ratio_48k, ratio_96k, ratio_192k, ratio_44k, ratio_88k, ratio_176k;

	/* set ratio registers */
	sys_clk = clk_get_rate(spdif->sclk)/1000; //40MHz
	ratio_32k = sys_clk/(32*2);
	ratio_48k = sys_clk/(48*2);
	ratio_96k = sys_clk/(96*2);
	ratio_192k = sys_clk/(192*2);
	ratio_44k = sys_clk*10/(441*2);
	ratio_88k = sys_clk*10/(882*2);
	ratio_176k = sys_clk*10/(1764*2);

	writel(0x202, regs + SPDIF_IN_LENGTH_DLT);
	writel(((ratio_32k+(ratio_32k>>5))<<16) + (ratio_32k-(ratio_32k>>5)), regs + SPDIF_IN_FS32K);
	writel(((ratio_48k+(ratio_48k>>5))<<16) + (ratio_48k-(ratio_48k>>5)), regs + SPDIF_IN_FS48K);
	writel(((ratio_96k+(ratio_96k>>5))<<16) + (ratio_96k-(ratio_96k>>5)), regs + SPDIF_IN_FS96K);
	writel(((ratio_192k+(ratio_192k>>5))<<16) + (ratio_192k-(ratio_192k>>5)), regs + SPDIF_IN_FS192K);
	writel(((ratio_44k+(ratio_44k>>5))<<16) + (ratio_44k-(ratio_44k>>5)), regs + SPDIF_IN_FS44K);
	writel(((ratio_88k+(ratio_88k>>5))<<16) + (ratio_88k-(ratio_88k>>5)), regs + SPDIF_IN_FS88K);
	writel(((ratio_176k+(ratio_176k>>5))<<16) + (ratio_176k-(ratio_176k>>5)), regs + SPDIF_IN_FS176K);

	/* set fifo level */
	writel(RX_FLS, regs + SPDIF_IN_FLS);

}

static ssize_t j_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    if(copy_to_user(user_buf,&spdifin_status,sizeof(spdifin_status)))
        return -EFAULT;
    return sizeof(spdifin_status);
}
static ssize_t j_write(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    if(copy_from_user(&spdifin_status,user_buf,sizeof(spdifin_status)))
        return -EFAULT;
    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN0, PAD_OFF);
    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN1, PAD_OFF);
    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN2, PAD_OFF);
    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN3, PAD_OFF);

    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_CH2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_CH2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN2_CH2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN3_CH2, PAD_OFF);

    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_SEL, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_SEL, PAD_OFF);
    /*
     * IN      SEL
     * 0---\
     *     |----0-----|
     * 4---/          |
     *                |
     * 1---\          |
     *     |----1-----|
     * 5---/          |
     *                |---------------
     * 2---\          |
     *     |----2-----|
     * 6---/          |
     *                |
     * 3---\          |
     *     |----3-----|
     * 7---/          |
     *
     */
    switch(spdifin_status.set_padmux & 0x0f) {
        case 0x0:
            silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN0, PAD_ON);
            break;
        case 0x1:
            silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN1, PAD_ON);
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_SEL, PAD_ON);
            break;
        case 0x2:
            silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN2, PAD_ON);
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_SEL, PAD_ON);
            break;
        case 0x3:
            silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN3, PAD_ON);
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_SEL, PAD_ON);
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_SEL, PAD_ON);
            break;
        case 0x04:
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_CH2, PAD_ON);
            break;
        case 0x05:
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_CH2, PAD_ON);
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_SEL, PAD_ON);
            break;
        case 0x06:
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN2_CH2, PAD_ON);
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_SEL, PAD_ON);
            break;
        case 0x07:
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN3_CH2, PAD_ON);
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_SEL, PAD_ON);
            silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_SEL, PAD_ON);
            break;
    }
    return sizeof(spdifin_status);
}
static const struct file_operations spdifin_fops = {
    .owner = THIS_MODULE,
    .read = j_read,
    .write = j_write,
};

static void spdif_snd_rxctrl(struct silan_spdif_info *spdif, int on)
{
	u32 config = 0;
	u32  data = 0;
	void __iomem *regs = spdif->regs;

	if (on)
	{
		/* enable interrupt */
		writel((SPDIF_IN_CS_STROBE_MASK | SPDIF_IN_PARITY_ERR_MASK | SPDIF_IN_OVER_FLOW_MASK | SPDIF_IN_LOCK_MASK | SPDIF_IN_UNLOCK_MASK | SPDIF_IN_PCM_RAW), regs + SPDIF_IN_INTMASK);

		writel(0x90, regs + SPDIF_IN_CONF1);

		/* enable spdif in */
		config = readl(regs + SPDIF_IN_CONF0);
		config |= (SPDIF_IN_EN | SPDIF_IN_DMAEN);
		writel(config, regs + SPDIF_IN_CONF0);
	}
	else
	{
		/* disable interrupt */
		writel(SPDIF_IN_IRQ_DISABLE, regs + SPDIF_IN_INTMASK);

		/* disable spdif in */
		config = readl(regs + SPDIF_IN_CONF0);
		config &= ~(SPDIF_IN_EN | SPDIF_IN_DMAEN);
		config |= SPDIF_IN_RSWRESET;
		writel(config, regs + SPDIF_IN_CONF0);
		data = readl(regs + SPDIF_IN_DR0);
		data &= 0xff000000;
		writel(data,regs + SPDIF_IN_DR0);
	}
}

static int spdif_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct silan_spdif_info *spdif = &spdif_info;
    if((spdifin_status.set_padmux>>16)&0xff){
        silan_spdif_reset();
        silan_spdif_reinit();
        spdifin_status.set_padmux &= 0xff;
    }
	spdif->dma_params.dma_addr = (dma_addr_t)(spdif->phy_base + SPDIF_IN_DR0);
	snd_soc_dai_set_dma_data(dai, substream, &spdif->dma_params);
	 my_debugfs_root=debugfs_create_dir("spdifin",NULL);
	spdif_snd_rxctrl(&spdif_info, 1);
	debugfs_create_file("info",0444,my_debugfs_root,NULL,&spdifin_fops);

	return 0;
}

static int spdif_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	return 0;
}

static int spdif_set_sysclk(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int spdif_trigger(struct snd_pcm_substream *substream, int cmd, 
	struct snd_soc_dai *dai)
{
	struct silan_spdif_info *spdif = &spdif_info;
	unsigned long flags;

	switch(cmd)
	{
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			spin_lock_irqsave(&spdif->lock, flags);
			spdif_snd_rxctrl(spdif, 1);
			spin_unlock_irqrestore(&spdif->lock, flags);
			break;

		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			spin_lock_irqsave(&spdif->lock, flags);
			spdif_snd_rxctrl(spdif, 0);
			spin_unlock_irqrestore(&spdif->lock, flags);
			break;

		default:
			return -EINVAL;
	}
	return 0;
}

static int spdif_hw_params(struct snd_pcm_substream *substream, 
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai) 
{
	return 0;
}

static void spdif_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct silan_spdif_info *spdif = &spdif_info;
	/* spdif clock disable */
//	clk_disable(spdif->sclk);
	spdif_snd_rxctrl(&spdif_info, 0);
//	debugfs_remove_recursive(my_debugfs_root);
}

#ifdef CONFIG_PM
static int spdif_suspend(struct snd_soc_dai *dai)
{
	spdif_snd_rxctrl(&spdif_info, 0);
	return 0;
}

static int spdif_resume(struct snd_soc_dai *dai)
{
	spdif_snd_rxctrl(&spdif_info, 1);
	return 0;
}
#else
#define spdif_suspend NULL
#define spdif_resume  NULL
#endif

static irqreturn_t silan_spdif_in_int_handler(int irq, void *dev_id)
{
	struct silan_spdif_info *spdif = &spdif_info;
	void __iomem *regs = spdif->regs;
	u32 irq_status, rate;
    u32 regval;
    u32 sys_clk;
    int range[6];
	irq_status = readl(regs + SPDIF_IN_MASKINT);
	if (!irq_status)
		return IRQ_NONE;

	rate = (readl(regs + SPDIF_IN_FSSTATUS) & SPDIF_IN_RATE_MASK); //thre first read maybe wrong rate
	if(spdifin_status.rate != rate)
	{
		spdifin_status.rate = rate;
	}
	if (irq_status & SPDIF_IN_LOCK)
	{
		writel(SPDIF_IN_LOCK, regs + SPDIF_IN_INTCLR);

		/* get rate */
		rate = (readl(regs + SPDIF_IN_FSSTATUS) & SPDIF_IN_RATE_MASK); //thre first read maybe wrong rate
		printk("silan spdif in int: get rate %d.\n", rate);
		rate = (readl(regs + SPDIF_IN_FSSTATUS) & SPDIF_IN_RATE_MASK);
		rate = (readl(regs + SPDIF_IN_FSSTATUS) & SPDIF_IN_RATE_MASK);
		rate = (readl(regs + SPDIF_IN_FSSTATUS) & SPDIF_IN_RATE_MASK);
		printk("silan spdif in int: get rate %d.\n", rate);
		printk("silan spdif in int: LOCK.\n");
		spdifin_status.rate = rate;
		spdifin_status.lock = 1;

		regval = readl(regs + SPDIF_IN_STATUS);  //SPDIF_IN_STATUS bits 18 Non pcm flag, bits 19 Dts flag
		spdifin_status.rawdata = (regval >> 2)&0x30000; //
		regval = readl(regs + SPDIF_IN_PCPD); //PCPD 0x1: ac3,   0xa,0xb,0xc:dts
		spdifin_status.rawdata |= regval & 0xf;
	}

	if (irq_status & SPDIF_IN_PARITY_ERR)
	{
		writel(SPDIF_IN_PARITY_ERR, regs + SPDIF_IN_INTCLR);
		spdifin_status.lock = 0;
		//printk("silan spdif in int: parity error.\n");
	}

	if (irq_status & SPDIF_IN_CS_STROBE)
	{
		writel(SPDIF_IN_CS_STROBE, regs + SPDIF_IN_INTCLR);
		regval = readl(regs + SPDIF_IN_STATUS);
		spdifin_status.rawdata = (regval >> 2)&0x30000;
		regval = readl(regs + SPDIF_IN_PCPD);
		spdifin_status.rawdata |= regval & 0xf;
	}

	if (irq_status & SPDIF_IN_OVER_FLOW)
	{
		writel(SPDIF_IN_OVER_FLOW, regs + SPDIF_IN_INTCLR);
		//printk("*");
	}

	if (irq_status & SPDIF_IN_UNLOCK)
	{

		writel(SPDIF_IN_UNLOCK, regs + SPDIF_IN_INTCLR);
		//printk("silan spdif in int: UNLOCK.\n");
		spdifin_status.lock = 0;

    //regval=readl(regs +  SPDIF_IN_SUBFINTRV);
     //printk(" ## SUBFINTRV:%x...###\n", regval);
        regval=readl(regs + SPDIF_IN_LENGTH);
        //printk("##### %08x #####\n", regval);
        if(regval != 0x3ff) {
            regval = (regval>>8) & 0xff;
            //    if (((regval>>8) & 0xff) < 0x27) {
            //   printk("##### %08x #####\n", readl(regs + SPDIF_IN_LENGTH));
            //}
            sys_clk = clk_get_rate(spdif->sclk)/1000; //150MHz
            range[0]=sys_clk/(128*44)*3;
            range[1]=sys_clk/(128*48)*3;
            range[2]=sys_clk/(128*88)*3;
            range[3]=sys_clk/(128*96)*3;
            range[4]=sys_clk/(128*176)*3;
            range[5]=sys_clk/(128*192)*3;
            //printk("========range[3]:%d,%d,%d",range[0],range[1],range[2]);
            //printk("======regval:%d]n",regval);
            if(regval > (range[0] - 8)){
                writel(0xc0c, regs + SPDIF_IN_LENGTH_DLT);
            }
            else if(regval > (range[1] - 8)){
                writel(0x808, regs + SPDIF_IN_LENGTH_DLT);
            }
            else if(regval > (range[3] - 4)){
                writel(0x606, regs + SPDIF_IN_LENGTH_DLT);
            }
            else {
                writel(0x303, regs + SPDIF_IN_LENGTH_DLT);
                //printk("===========0x303\n");
            }
        }
	}

	if (irq_status & SPDIF_IN_UB_DIF)
	{
		writel(SPDIF_IN_UB_DIF, regs + SPDIF_IN_INTCLR);
		printk("silan spdif in int: user data\n");
		printk("UB: %x %x %x \n", readl(regs + SPDIF_IN_UB_REG0), readl(regs + SPDIF_IN_UB_REG1), readl(regs + SPDIF_IN_UB_REG2));
		printk("pcm or raw : %x\n", readl(regs + SPDIF_IN_STATUS));
	}

	if (irq_status & SPDIF_IN_PCM_RAW)
	{
		writel(SPDIF_IN_PCM_RAW, regs + SPDIF_IN_INTCLR);
		regval = readl(regs + SPDIF_IN_STATUS);
		spdifin_status.rawdata = (regval >> 2)&0x30000;
		regval = readl(regs + SPDIF_IN_PCPD);
		spdifin_status.rawdata |= regval & 0xf;
	}

	return IRQ_HANDLED;
}

static int silan_spdif_probe(struct snd_soc_dai *dai)
{
	int ret;
	struct silan_spdif_info *spdif = &spdif_info;
	void __iomem *regs = spdif->regs;
	u32 sys_clk, ratio_32k, ratio_48k, ratio_96k, ratio_192k, ratio_44k, ratio_88k, ratio_176k;
	/* set ratio registers */
	sys_clk = clk_get_rate(spdif->sclk)/1000; //40MHz

	ratio_32k = sys_clk/(32*2);
	ratio_48k = sys_clk/(48*2);
	ratio_96k = sys_clk/(96*2);
	ratio_192k = sys_clk/(192*2);
	ratio_44k = sys_clk*10/(441*2);
	ratio_88k = sys_clk*10/(882*2);
	ratio_176k = sys_clk*10/(1764*2);

	writel(0x202, regs + SPDIF_IN_LENGTH_DLT);
	writel(((ratio_32k+(ratio_32k>>5))<<16) + (ratio_32k-(ratio_32k>>5)), regs + SPDIF_IN_FS32K);
	writel(((ratio_48k+(ratio_48k>>5))<<16) + (ratio_48k-(ratio_48k>>5)), regs + SPDIF_IN_FS48K);
	writel(((ratio_96k+(ratio_96k>>5))<<16) + (ratio_96k-(ratio_96k>>5)), regs + SPDIF_IN_FS96K);
	writel(((ratio_192k+(ratio_192k>>5))<<16) + (ratio_192k-(ratio_192k>>5)), regs + SPDIF_IN_FS192K);
	writel(((ratio_44k+(ratio_44k>>5))<<16) + (ratio_44k-(ratio_44k>>5)), regs + SPDIF_IN_FS44K);
	writel(((ratio_88k+(ratio_88k>>5))<<16) + (ratio_88k-(ratio_88k>>5)), regs + SPDIF_IN_FS88K);
	writel(((ratio_176k+(ratio_176k>>5))<<16) + (ratio_176k-(ratio_176k>>5)), regs + SPDIF_IN_FS176K);

	/* set fifo level */
	writel(RX_FLS, regs + SPDIF_IN_FLS);

	/* config interrupt */
	ret = request_irq(PIC_IRQ_SPDIF_IN, silan_spdif_in_int_handler, 0, DEVICE_NAME, NULL);
	if (ret)
	{
		printk("failed to request spdif in irq.\n");
		return -EINTR;
	}

	return 0;
}

static struct snd_soc_dai_ops silan_spdif_dai_ops = {
	.startup    = spdif_startup,
	.set_fmt	= spdif_set_fmt,
	.set_sysclk = spdif_set_sysclk,
	.trigger	= spdif_trigger,
	.hw_params	= spdif_hw_params,
	.shutdown	= spdif_shutdown,
};

struct snd_soc_dai_driver silan_spdif_in_dai = {
	.probe = silan_spdif_probe,
	.capture = {
		.stream_name = "SPDIF IN",
		.channels_min = 1,
		.channels_max = 2,
		.rates = (SNDRV_PCM_RATE_32000 |
			  SNDRV_PCM_RATE_44100 |
			  SNDRV_PCM_RATE_48000 |
			  SNDRV_PCM_RATE_96000 |
              SNDRV_PCM_RATE_192000),
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_3LE,
	},
	.ops = &silan_spdif_dai_ops,
	.suspend = spdif_suspend,
	.resume  = spdif_resume,
};

static __devinit int silan_spdif_in_dev_probe(struct platform_device *pdev)
{
	struct resource *mem_res, *dma_res;
	struct silan_spdif_info *spdif;
	int ret;

	dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if(!dma_res)
	{
		dev_err(&pdev->dev, "Unable to get dma resource.\n");
		return -ENXIO;
	}
	
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!mem_res)
	{
		dev_err(&pdev->dev, "Unable to get mem resource.\n");
		return -ENXIO;
	}
	
	spdif = &spdif_info;
	spdif->dev = &pdev->dev;

	spin_lock_init(&spdif->lock);
	
	spdif->sclk = clk_get(&pdev->dev, "spdif_in");
	if(IS_ERR(spdif->sclk))
	{
		dev_err(&pdev->dev, "failed to get source clock\n");
		ret = -ENOENT;
		goto err1;
	}
	clk_enable(spdif->sclk);

    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN0, PAD_OFF);
    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN1, PAD_OFF);
    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN2, PAD_OFF);
    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN3, PAD_OFF);

    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_CH2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_CH2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN2_CH2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN3_CH2, PAD_OFF);
    /*OFF, OFF is 00  mean default channel 0*/
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN0_SEL, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_SPDIF_IN1_SEL, PAD_OFF);
    silan_padmux_ctrl(SILAN_PADMUX_SPDIF_IN0, PAD_ON);


	if(!request_mem_region(mem_res->start, resource_size(mem_res), pdev->name))
	{
		dev_err(&pdev->dev, "Unable to get register mem region.\n");
		ret = -EBUSY;
		goto err2;	
	}	
	
	spdif->regs = ioremap(mem_res->start, resource_size(mem_res));
	if(spdif->regs == NULL)
	{
		dev_err(&pdev->dev, "Unable to remap regs.\n");
		ret = -ENXIO;
		goto err3;
	}

	spdif->phy_base = mem_res->start;
	spdif->dma_params.dma = dma_res->start;
	spdif->dma_params.burstsize = 4;
	spdif->dma_params.type = SL_DMATYPE_SPDIF_IN;

	dev_set_drvdata(&pdev->dev, spdif);

	ret = snd_soc_register_dai(&pdev->dev, &silan_spdif_in_dai);
	if(ret != 0)
	{
		dev_err(&pdev->dev, "fail to register dai.\n");
		goto err4;
	}
	
	return 0;
err4:
	iounmap(spdif->regs);
err3:
	release_mem_region(mem_res->start, resource_size(mem_res));
err2:
	clk_disable(spdif->sclk);
	clk_put(spdif->sclk);
err1:
	return ret;
}

static __devexit int silan_spdif_in_dev_remove(struct platform_device *pdev)
{
	struct silan_spdif_info *spdif = &spdif_info;
	struct resource *mem_res;
	
	snd_soc_unregister_dai(&pdev->dev);
	
	iounmap(spdif->regs);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(mem_res)
		release_mem_region(mem_res->start, resource_size(mem_res));

	return 0;
}

static struct platform_driver silan_spdif_in_driver = {
	.probe	= silan_spdif_in_dev_probe,
	.remove	= silan_spdif_in_dev_remove,
	.driver	= {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init spdif_in_init(void)
{
	return platform_driver_register(&silan_spdif_in_driver);
}
module_init(spdif_in_init);

static void __exit spdif_in_exit(void)
{
	platform_driver_unregister(&silan_spdif_in_driver);
}
module_exit(spdif_in_exit);

MODULE_DESCRIPTION("Silan S/PDIF IN Controller Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:silan-spdif-in");
