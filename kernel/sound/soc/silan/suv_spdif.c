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

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include <silan_padmux.h>

#include "suv_pcm.h"
#include "suv_spdif.h"

struct silan_spdif_info {
	spinlock_t lock;
	struct device *dev;
	void __iomem *regs;
	struct clk *sclk;
	struct sl_pcm_dma_params *dma_playback;
};

static struct sl_pcm_dma_params spdif_stereo_out;
static struct silan_spdif_info spdif_info;

static void spdif_snd_txctrl(struct silan_spdif_info *spdif, int on)
{
	void __iomem *regs = spdif->regs;
	u32 spdcon;
	
	spdcon = readl(regs + SPD_CTL) & ~SPD_CTL_MASK;
	if(on)
		writel(spdcon | SPD_CTL_ON, regs + SPD_CTL);
	else
		writel(spdcon & ~SPD_CTL_ON, regs + SPD_CTL);
}

static int spdif_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	return 0;
}

static int spdif_set_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	return 0;
}

static int spdif_trigger(struct snd_pcm_substream *substream, int cmd, 
	struct snd_soc_dai *dai)
{
	struct silan_spdif_info *spdif = &spdif_info;
	unsigned long flags;
	
	switch(cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		spin_lock_irqsave(&spdif->lock, flags);
		spdif_snd_txctrl(spdif, 1);
		spin_unlock_irqrestore(&spdif->lock, flags);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		spin_lock_irqsave(&spdif->lock, flags);
		spdif_snd_txctrl(spdif, 0);
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
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct silan_spdif_info *spdif = &spdif_info;
	void __iomem *regs = spdif->regs;
	struct sl_pcm_dma_params *dma_data;
	u32 con, csd1;
	unsigned long flags;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = spdif->dma_playback;
	else {
		dev_err(spdif->dev, "Capture is not supported\n");
		return -EINVAL;
	}

	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);

	spin_lock_irqsave(&spdif->lock, flags);
	con = readl(regs + SPD_CTL) & ~CON_FMT_MASK;

	switch(params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		con |= CON_PCM_16BIT;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		con |= CON_PCM_24BIT;
		break;
	default:
		dev_err(spdif->dev, "Unsupported data size.\n");
		goto err;
	}
	
	csd1 = readl(regs + SPD_CSD1) & ~CSTAS_SAMP_FREQ_MASK;
	switch(params_rate(params)) {
	case 44100:
		csd1 |= CSTAS_SAMP_FREQ_44;
		clk_set_rate(spdif->sclk, 44100);
		break;
	case 48000:
		csd1 |= CSTAS_SAMP_FREQ_48;
		clk_set_rate(spdif->sclk, 48000);
		break;
	case 32000:
		csd1 |= CSTAS_SAMP_FREQ_32;
		clk_set_rate(spdif->sclk, 32000);
		break;
	case 96000:
		clk_set_rate(spdif->sclk, 96000);
		csd1 |= CSTAS_SAMP_FREQ_96;
		break;
	default:
		dev_err(spdif->dev, "Invalid sampling rate %d\n", params_rate(params));
		goto err;	
	}
	
	writel(csd1, spdif->regs + SPD_CSD1);
	writel(con, spdif->regs + SPD_CTL);
	spin_unlock_irqrestore(&spdif->lock, flags);

	return 0;

err:
	spin_unlock_irqrestore(&spdif->lock, flags);
	return -EINVAL;
}

static void spdif_shutdown(struct snd_pcm_substream *substream, 
	struct snd_soc_dai *dai)
{
	struct silan_spdif_info *spdif = &spdif_info;
	void __iomem *regs = spdif->regs;
	u32 spdcon;
	
	spdcon = readl(regs + SPD_CTL) & ~SPD_CTL_MASK;
	writel(spdcon & ~SPD_CTL_ON, regs + SPD_CTL);
}

#ifdef CONFIG_PM
static int spdif_suspend(struct snd_soc_dai *dai)
{
	struct silan_spdif_info *spdif = &spdif_info;
	void __iomem *regs = spdif->regs;
	u32 spdcon;

	spdcon = readl(regs + SPD_CTL) & ~SPD_CTL_MASK;
	writel(spdcon & ~SPD_CTL_ON, regs + SPD_CTL);

	cpu_relax();
	return 0;
}

static int spdif_resume(struct snd_soc_dai *dai)
{
	struct silan_spdif_info *spdif = &spdif_info;
	void __iomem *regs = spdif->regs;
	u32 spdcon;

	spdcon = readl(regs + SPD_CTL) & ~SPD_CTL_MASK;
	writel(spdcon | SPD_CTL_ON, regs + SPD_CTL);

	return 0;
}
#else
#define spdif_suspend NULL
#define spdif_resume  NULL
#endif

static int silan_spdif_probe(struct snd_soc_dai *dai)
{
	struct silan_spdif_info *spdif = &spdif_info;

	writel(SPD_FF_FLUSH, spdif->regs + SPD_FF_CTL);
	writel(0, spdif->regs+SPD_USR1);
	writel(0, spdif->regs+SPD_USR2);
	writel(0, spdif->regs+SPD_USR3);
	writel(SPD_Q2W_CFG, spdif->regs + SPD_Q2W);
	
	writel(SPD_CSD1_CFG, spdif->regs + SPD_CSD1);
	writel(SPD_CSD2_CFG, spdif->regs + SPD_CSD2);
	writel(0, spdif->regs + SPD_CSD3);
	writel(0, spdif->regs + SPD_CSD4);
	writel(0, spdif->regs + SPD_CSD5);
	writel(0, spdif->regs + SPD_CSD6);

	writel(SPD_FF_CFG, spdif->regs + SPD_FF_CTL);
	writel(SPD_IRQ_CFG, spdif->regs + SPD_IRQEN);
	writel(0, spdif->regs + SPD_IRQEN);
	writel(SPD_STS_CLR, spdif->regs + SPD_STS);
	writel(SPD_STS_CFG, spdif->regs + SPD_STS);
	writel(SPD_CTL_SW_RST, spdif->regs + SPD_CTL);
	writel(SPD_CTL_CFG, spdif->regs + SPD_CTL);	

	return 0;
}

static struct snd_soc_dai_ops silan_spdif_dai_ops = {
	.set_fmt	= spdif_set_fmt,
	.set_sysclk = spdif_set_sysclk,
	.trigger	= spdif_trigger,
	.hw_params	= spdif_hw_params,
	.shutdown	= spdif_shutdown,
};

struct snd_soc_dai_driver silan_spdif_dai = {
	.probe = silan_spdif_probe,
	.playback = {
		.stream_name = "S/PDIF Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = (SNDRV_PCM_RATE_32000 |
			  SNDRV_PCM_RATE_44100 |
			  SNDRV_PCM_RATE_48000 |
			  SNDRV_PCM_RATE_96000 ),
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_3LE,
	},
	.ops = &silan_spdif_dai_ops,
	.suspend = spdif_suspend,
	.resume  = spdif_resume,
};

static __devinit int silan_spdif_dev_probe(struct platform_device *pdev)
{
	struct resource *mem_res, *dma_res;
	struct silan_spdif_info *spdif;
	int ret;
	
	dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if(!dma_res) {
		dev_err(&pdev->dev, "Unable to get dma resource.\n");
		return -ENXIO;
	}
	
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!mem_res) {
		dev_err(&pdev->dev, "Unable to get mem resource.\n");
		return -ENXIO;
	}
	
	spdif = &spdif_info;
	spdif->dev = &pdev->dev;

	spin_lock_init(&spdif->lock);
	
	spdif->sclk = clk_get(&pdev->dev, "spdif");
	if(IS_ERR(spdif->sclk)) {
		dev_err(&pdev->dev, "failed to get source clock\n");
		ret = -ENOENT;
		goto err1;
	}
	clk_enable(spdif->sclk);

	/* open the spdif pad */
	silan_padmux_ctrl(SILAN_PADMUX_SPDIF, PAD_ON);

	if(!request_mem_region(mem_res->start, resource_size(mem_res), 
		pdev->name)) {
		dev_err(&pdev->dev, "Unable to get register mem region.\n");
		ret = -EBUSY;
		goto err2;	
	}	
	
	spdif->regs = ioremap(mem_res->start, resource_size(mem_res));
	if(spdif->regs == NULL) {
		dev_err(&pdev->dev, "Unable to remap regs.\n");
		ret = -ENXIO;
		goto err3;
	}

	dev_set_drvdata(&pdev->dev, spdif);

	ret = snd_soc_register_dai(&pdev->dev, &silan_spdif_dai);
	if(ret != 0) {
		dev_err(&pdev->dev, "fail to register dai.\n");
		goto err4;
	}
	
	spdif_stereo_out.dma_addr = mem_res->start + SPD_FF_WDAT;
	spdif_stereo_out.dma = dma_res->start;
	spdif_stereo_out.burstsize = 4;
	spdif_stereo_out.type = SL_DMATYPE_SPDIF_OUT;

	spdif->dma_playback = &spdif_stereo_out;

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

static __devexit int silan_spdif_dev_remove(struct platform_device *pdev)
{
	struct silan_spdif_info *spdif = &spdif_info;
	struct resource *mem_res;
	
	snd_soc_unregister_dai(&pdev->dev);
	
	iounmap(spdif->regs);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM,0);
	if(mem_res)
		release_mem_region(mem_res->start, resource_size(mem_res));

	return 0;
}

static struct platform_driver silan_spdif_driver = {
	.probe	= silan_spdif_dev_probe,
	.remove	= silan_spdif_dev_remove,
	.driver	= {
		.name	= "silan-spdif",
		.owner	= THIS_MODULE,
	},
};

static int __init spdif_init(void)
{
	return platform_driver_register(&silan_spdif_driver);
}
module_init(spdif_init);

static void __exit spdif_exit(void)
{
	platform_driver_unregister(&silan_spdif_driver);
}
module_exit(spdif_exit);

MODULE_AUTHOR("Lu Xuegang, <luxuegang@silan.com>");
MODULE_DESCRIPTION("Silan S/PDIF Controller Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:silan-spdif");
