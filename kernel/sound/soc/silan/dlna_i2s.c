#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <silan_padmux.h>

#include "suv_pcm.h"
#include "suv_dlna_i2s.h"

struct silan_i2s_hdmi_params {
	u32	phy_base;
	void __iomem *regs;
	struct clk *clk;
};

struct silan_i2s_hdmi_dev {
	struct device *dev;
	struct sl_pcm_dma_params dma_params[2];
	struct silan_i2s_hdmi_params i2s_params[2];
};

static void silan_snd_txctrl(struct silan_i2s_hdmi_dev *dev, int on)
{
	u32 ctrl = 0, cr = 0;
	u32 clk_cr,imr_x;

	cr = readl(dev->i2s_params[0].regs + TCR_X(0));
	ctrl = readl(dev->i2s_params[0].regs + IIS_CTRL);
	clk_cr =  readl(dev->i2s_params[0].regs + CLK_CTRL);
	imr_x = readl(dev->i2s_params[0].regs + IMR_X(0));
	if (on)
	{
		cr |= RTXEN;
		ctrl |= ITXEN;
		writel(0, dev->i2s_params[0].regs + IIS_CTRL);
		writel(IIS_EN, dev->i2s_params[0].regs + IIS_CTRL);
		writel(SRST_ALL, dev->i2s_params[0].regs + SRESET);
		writel(IIS_EN, dev->i2s_params[0].regs + IIS_CTRL);

		writel(ctrl, dev->i2s_params[0].regs + IIS_CTRL);
		writel(clk_cr, dev->i2s_params[0].regs + CLK_CTRL);

		writel(imr_x, dev->i2s_params[0].regs + IMR_X(0));
	}
	else
	{
		//cr &= ~RTXEN;
		//ctrl &= ~ITXEN;
		//cr |= RTFF;
		//ctrl |= TXFLUSH;
	}

	writel(ctrl, dev->i2s_params[0].regs + IIS_CTRL);
	writel(cr, dev->i2s_params[0].regs + TCR_X(0));
}

static void silan_snd_rxctrl(struct silan_i2s_hdmi_dev *dev, int on)
{
	u32 ctrl = 0, cr = 0;

	cr = readl(dev->i2s_params[1].regs + RCR_X(0));
	ctrl = readl(dev->i2s_params[1].regs + IIS_CTRL);

	if (on)
	{
		cr |= RTXEN;
		ctrl |= IRXEN;
	}
	else
	{
		cr &= ~RTXEN;
		ctrl &= ~IRXEN;
		cr |= RTFF;
		ctrl |= RXFLUSH;
	}

	writel(cr, dev->i2s_params[1].regs + RCR_X(0));
	writel(ctrl, dev->i2s_params[1].regs + IIS_CTRL);
}

/*
 * Set SUV I2S DAI format
 */
static int silan_i2s_hdmi_set_fmt(struct snd_soc_dai *dai,
		unsigned int fmt)
{
	return 0;
}

static int silan_i2s_hdmi_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	return 0;
}

/*
 * Set Clock dividers
 */
static int silan_i2s_hdmi_set_clkdiv(struct snd_soc_dai *dai,
	int div_id, int div)
{
	return 0;
}

static int silan_i2s_hdmi_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct silan_i2s_hdmi_dev *dev = snd_soc_dai_get_drvdata(dai);
	int chnl;
	u32 ctrl = 0, cr = 0, clk_cr = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	{
		ctrl = readl(dev->i2s_params[0].regs + IIS_CTRL);
		ctrl &= ~TX_UNITE_CHNLMASK;
		cr = readl(dev->i2s_params[0].regs + TCR_X(0));
	}
	else 
	{
		ctrl = readl(dev->i2s_params[1].regs + IIS_CTRL);
		ctrl &= ~RX_UNITE_CHNLMASK;
		cr = readl(dev->i2s_params[1].regs + RCR_X(0));
	}
	cr &= ~RTWLEN_MASK;

	chnl = params_channels(params)/2 + params_channels(params)%2;
	if (chnl > 4 || chnl < 1) {
		dev_err(dev->dev, "Unsupported i2s channels\n");
		return -EINVAL;
	}

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		cr |= WLEN_16BIT<<RTWLEN_SHIFT;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		cr |= WLEN_24BIT<<RTWLEN_SHIFT;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		cr |= WLEN_32BIT<<RTWLEN_SHIFT;
		break;
	default:
		dev_err(dev->dev, "Unsupported pcm format\n");
		return -EINVAL;
	}

	switch (params_rate(params)) {
	    case 8000:
        case 11025:
        case 12000:
        case 16000:
        case 22050:
        case 24000:
        case 32000:
        case 44100:
        case 48000:
        case 88200:
        case 96000:
        case 176400:
        case 192000:
            if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#if defined(CONFIG_I2S_DLNA_DAC_MASTER)
                clk_cr |=  MODE_EN | (WSS32 << WSS_SHIFT) | CLKEN;
#else
                clk_cr |=  (WSS32 << WSS_SHIFT) | CLKEN;
#endif
                writel(clk_cr, dev->i2s_params[0].regs + CLK_CTRL);
                if (clk_set_rate(dev->i2s_params[0].clk, params_rate(params)) != 0) {
                    printk("i2s: unsupport sample rate (%d)\n", params_rate(params));
                    return -EINVAL;
                }
            } else {

#if defined(CONFIG_I2S_DLNA_ADC_MASTER)
                clk_cr |=  MODE_EN | (WSS32 << WSS_SHIFT) | CLKEN;
#else
                clk_cr |=  (WSS32 << WSS_SHIFT) | CLKEN;
#endif
                writel(clk_cr, dev->i2s_params[1].regs + CLK_CTRL);
                if (clk_set_rate(dev->i2s_params[1].clk, params_rate(params)) != 0) {
                    printk("i2s: unsupport sample rate (%d)\n", params_rate(params));
                    return -EINVAL;
                }
            }
            break;
        default:
            dev_err(dev->dev, "Invalid sampling rate %d\n", params_rate(params));
            return -EINVAL;
    }

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ctrl |= TX_UNITE_EN;
		ctrl |= (((1<<chnl)-1) << TX_UNITE_CHNLSHIFT);
		writel(ctrl, dev->i2s_params[0].regs + IIS_CTRL);
		writel(cr, dev->i2s_params[0].regs + TCR_X(0));
	} else {
		ctrl |= RX_UNITE_EN;
		ctrl |= (((1<<chnl)-1) << RX_UNITE_CHNLSHIFT);
		writel(ctrl, dev->i2s_params[1].regs + IIS_CTRL);
		writel(cr, dev->i2s_params[1].regs + RCR_X(0));
	}

	return 0;
}

static void silan_i2s_hdmi_start(struct silan_i2s_hdmi_dev *dev, 
				struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		silan_snd_txctrl(dev, 1);
	else
		silan_snd_rxctrl(dev, 1);
}

static void silan_i2s_hdmi_stop(struct silan_i2s_hdmi_dev *dev, 
				struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		silan_snd_txctrl(dev, 0);
	else
		silan_snd_rxctrl(dev, 0);
}

static int silan_i2s_hdmi_trigger(struct snd_pcm_substream *substream, int cmd,
			       struct snd_soc_dai *dai)
{
	struct silan_i2s_hdmi_dev *dev = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		silan_i2s_hdmi_start(dev, substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		silan_i2s_hdmi_stop(dev, substream);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int silan_i2s_hdmi_startup(struct snd_pcm_substream *substream, 
				struct snd_soc_dai *dai)
{
	struct silan_i2s_hdmi_dev *dev = snd_soc_dai_get_drvdata(dai);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dev->dma_params[0].dma_addr = (dma_addr_t)(dev->i2s_params[0].phy_base + TFR_X(0));
		snd_soc_dai_set_dma_data(dai, substream, &dev->dma_params[0]);
	}
	else {
		dev->dma_params[1].dma_addr = (dma_addr_t)(dev->i2s_params[1].phy_base + RFR_X(0));
		snd_soc_dai_set_dma_data(dai, substream, &dev->dma_params[1]);
	}

	return 0;
}

static int silan_i2s_hdmi_probe(struct snd_soc_dai *dai)
{
	struct silan_i2s_hdmi_dev *dev = snd_soc_dai_get_drvdata(dai);
	u32 ctrl = 0;
	u32 tx_cr = 0;
	u32 rx_cr = 0;
	int i;
	for(i = 0; i < 2; i++) {
		/* disable the iis firstly */
		writel(0, dev->i2s_params[i].regs + IIS_CTRL);
		writel(IIS_EN, dev->i2s_params[i].regs + IIS_CTRL);
		writel(SRST_ALL, dev->i2s_params[i].regs + SRESET);
		writel(IIS_EN, dev->i2s_params[i].regs + IIS_CTRL);

		ctrl = IIS_EN | ITXEN | IRXEN | 
				RXFLUSH | TXFLUSH; 
		writel(ctrl, dev->i2s_params[i].regs + IIS_CTRL);
	
		// Config IIS FIFO Threshold
		rx_cr = RTFF | RTXDMAEN | RTXJUST_HIGH | RTXMODE_IIS;
		tx_cr = rx_cr;
		tx_cr |= ((IIS_FIFO_DEPTH-IIS_DMABREQ_SIZE-1) << RTXFTHD_SHIFT);
		rx_cr |= ((IIS_DMABREQ_SIZE-1) << RTXFTHD_SHIFT);
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(0));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(0));
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(1));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(1));
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(2));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(2));
		
		writel(TXFOM | TXFEM | RXFOM | RXDAM, dev->i2s_params[i].regs + IMR_X(0));
	}

	return 0;
}

static void silan_i2s_hdmi_shutdown(struct snd_pcm_substream *substream, 
				struct snd_soc_dai *dai)
{
    struct silan_i2s_hdmi_dev *dev = snd_soc_dai_get_drvdata(dai);
    u32 ctrl = 0;
    u32 tx_cr = 0;
    u32 rx_cr = 0;
    int i;
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        i = 0;
		/* disable the iis firstly */
		writel(0, dev->i2s_params[i].regs + IIS_CTRL);
		writel(IIS_EN, dev->i2s_params[i].regs + IIS_CTRL);
		writel(SRST_ALL, dev->i2s_params[i].regs + SRESET);
		writel(IIS_EN, dev->i2s_params[i].regs + IIS_CTRL);

		ctrl = IIS_EN | ITXEN | IRXEN |
				RXFLUSH | TXFLUSH;
		writel(ctrl, dev->i2s_params[i].regs + IIS_CTRL);

		// Config IIS FIFO Threshold
		rx_cr = RTFF | RTXDMAEN | RTXJUST_HIGH | RTXMODE_IIS;
		tx_cr = rx_cr;
		tx_cr |= ((IIS_FIFO_DEPTH-IIS_DMABREQ_SIZE-1) << RTXFTHD_SHIFT);
		rx_cr |= ((IIS_DMABREQ_SIZE-1) << RTXFTHD_SHIFT);
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(0));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(0));
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(1));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(1));
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(2));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(2));

		writel(TXFOM | TXFEM | RXFOM | RXDAM, dev->i2s_params[i].regs + IMR_X(0));
    }
    else {
        i =1;
		/* disable the iis firstly */
		writel(0, dev->i2s_params[i].regs + IIS_CTRL);
		writel(IIS_EN, dev->i2s_params[i].regs + IIS_CTRL);
		writel(SRST_ALL, dev->i2s_params[i].regs + SRESET);
		writel(IIS_EN, dev->i2s_params[i].regs + IIS_CTRL);

		ctrl = IIS_EN | ITXEN | IRXEN |
				RXFLUSH | TXFLUSH;
		writel(ctrl, dev->i2s_params[i].regs + IIS_CTRL);

		// Config IIS FIFO Threshold
		rx_cr = RTFF | RTXDMAEN | RTXJUST_HIGH | RTXMODE_IIS;
		tx_cr = rx_cr;
		tx_cr |= ((IIS_FIFO_DEPTH-IIS_DMABREQ_SIZE-1) << RTXFTHD_SHIFT);
		rx_cr |= ((IIS_DMABREQ_SIZE-1) << RTXFTHD_SHIFT);
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(0));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(0));
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(1));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(1));
		writel(rx_cr, dev->i2s_params[i].regs + RCR_X(2));
		writel(tx_cr, dev->i2s_params[i].regs + TCR_X(2));

		writel(TXFOM | TXFEM | RXFOM | RXDAM, dev->i2s_params[i].regs + IMR_X(0));
    }
}

#ifdef CONFIG_PM_SLEEP

static int silan_i2s_hdmi_suspend(struct snd_soc_dai *dai)
{
	struct silan_i2s_hdmi_dev *dev = snd_soc_dai_get_drvdata(dai);

	clk_disable(dev->i2s_params[0].clk);
	clk_disable(dev->i2s_params[1].clk);

	return 0;
}

static int silan_i2s_hdmi_resume(struct snd_soc_dai *dai)
{
	struct silan_i2s_hdmi_dev *dev = snd_soc_dai_get_drvdata(dai);

	clk_enable(dev->i2s_params[0].clk);
	clk_enable(dev->i2s_params[1].clk);

	return 0;
}

#else /* !CONFIG_PM_SLEEP */

#define silan_i2s_hdmi_suspend NULL
#define silan_i2s_hdmi_resume NULL
#endif

#define SILAN_I2S_RATES SNDRV_PCM_RATE_8000_192000

static struct snd_soc_dai_ops silan_i2s_hdmi_dai_ops = {
	.startup = silan_i2s_hdmi_startup,
	.shutdown = silan_i2s_hdmi_shutdown,
	.trigger = silan_i2s_hdmi_trigger,
	.hw_params = silan_i2s_hdmi_hw_params,
	.set_fmt = silan_i2s_hdmi_set_fmt,
	.set_clkdiv = silan_i2s_hdmi_set_clkdiv,
	.set_sysclk = silan_i2s_hdmi_set_sysclk,
};

static struct snd_soc_dai_driver silan_i2s_hdmi_dai = {
	.probe = silan_i2s_hdmi_probe,
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SILAN_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | 
			SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE,},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SILAN_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | 
			SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE,},
	.ops = &silan_i2s_hdmi_dai_ops,
	.suspend = silan_i2s_hdmi_suspend,
	.resume = silan_i2s_hdmi_resume,
};
EXPORT_SYMBOL_GPL(silan_i2s_hdmi_dai);

static __devinit int silan_iis_hdmi_dev_probe(struct platform_device *pdev)
{
	struct silan_i2s_hdmi_dev *dev;
	struct resource *mem_res[2], *dma_res[2];
	int ret, i;
 
	mem_res[0] = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mem_res[1] = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!mem_res[0] || !mem_res[1]) {
		dev_err(&pdev->dev, "Unable to get address\n");
		return -ENXIO;
	}

	if (!request_mem_region(mem_res[0]->start, resource_size(mem_res[0]), pdev->name)
			|| !request_mem_region(mem_res[1]->start, resource_size(mem_res[1]), pdev->name)) {
		dev_err(&pdev->dev, "Unable to request mem region\n");
		ret = -EBUSY;
		goto err0;
	}

	dev = kzalloc(sizeof(struct silan_i2s_hdmi_dev), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		goto err1;
	}

	dma_res[0] = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	dma_res[1] = platform_get_resource(pdev, IORESOURCE_DMA, 1);
	if (!dma_res[0] || !dma_res[1]) {
		dev_err(&pdev->dev, "no DMA resource\n");
		ret = -ENXIO;
		goto err2;
	}
	
	for(i = 0; i < 2; i++){
		dev->i2s_params[i].phy_base = mem_res[i]->start;
		dev->i2s_params[i].regs = ioremap(mem_res[i]->start, resource_size(mem_res[i]));
		dev->dma_params[i].dma = dma_res[i]->start;
		dev->dma_params[i].burstsize = 4;
	}

	dev->i2s_params[0].clk = clk_get(&pdev->dev, "i2s_out");
	dev->i2s_params[1].clk = clk_get(&pdev->dev, "i2s_in");
	dev->dma_params[0].type = SL_DMATYPE_IIS_OUT;
	dev->dma_params[1].type = SL_DMATYPE_IIS_IN;
	if (IS_ERR(dev->i2s_params[0].clk) || IS_ERR(dev->i2s_params[1].clk)) {
		dev_err(&pdev->dev, "fail to get clk\n");
		ret = -ENODEV;
		goto err3;
	}
	clk_enable(dev->i2s_params[0].clk);
	clk_enable(dev->i2s_params[1].clk);
	clk_set_rate(dev->i2s_params[0].clk, 44100);

#ifdef CONFIG_I2S_DLNA_DAC_INTRA
	// IIS DAC with intra CODEC, IIS must Slave Mode(MODE_EN=0)
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_MCLK, PAD_OFF);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD0, PAD_OFF);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD1, PAD_OFF);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD2, PAD_OFF);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC, PAD_OFF);
	silan_padmux2_ctrl(SILAN_PADMUX2_CODEC_DAC_SEL, PAD_ON);
#endif

#ifdef CONFIG_I2S_DLNA_DAC_SLAVE
	// IIS DAC with extra CODEC, IIS Slave Mode(MODE_EN=0)
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_MCLK, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD0, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD1, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD2, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC, PAD_ON);
	silan_padmux2_ctrl(SILAN_PADMUX2_CODEC_DAC_SEL, PAD_OFF);
	silan_padmux2_ctrl(SILAN_PADMUX2_IISDAC_MODE, PAD_OFF);
#endif

#ifdef CONFIG_I2S_DLNA_DAC_MASTER
	// IIS DAC with extra CODEC, IIS Master Mode(MODE_EN=1)
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_MCLK, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD0, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD1, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_FD2, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC, PAD_ON);
	silan_padmux2_ctrl(SILAN_PADMUX2_CODEC_DAC_SEL, PAD_OFF);
	silan_padmux2_ctrl(SILAN_PADMUX2_IISDAC_MODE, PAD_ON);
#endif

#ifdef CONFIG_I2S_DLNA_ADC_INTRA
	// IIS ADC with intra CODEC, IIS Must Slave Mode(MODE_EN=0)
	//silan_padmux_ctrl(SILAN_PADMUX_IISDAC_MCLK, PAD_OFF);
	silan_padmux_ctrl(SILAN_PADMUX_IISADC_FD0, PAD_OFF);
	silan_padmux_ctrl(SILAN_PADMUX_IISADC, PAD_OFF);
	silan_padmux2_ctrl(SILAN_PADMUX2_CODEC_ADC_SEL, PAD_ON);
#endif

#ifdef CONFIG_I2S_DLNA_ADC_SLAVE
	// IIS ADC with extra CODEC, IIS Slave Mode(MODE_EN=0)
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_MCLK, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISADC_FD0, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISADC, PAD_ON);
	silan_padmux2_ctrl(SILAN_PADMUX2_CODEC_ADC_SEL, PAD_OFF);
	silan_padmux2_ctrl(SILAN_PADMUX2_IISADC_MODE, PAD_OFF);
#endif

#ifdef CONFIG_I2S_DLNA_ADC_MASTER
	// IIS ADC with extra CODEC, IIS Master Mode(MODE_EN=0)
	silan_padmux_ctrl(SILAN_PADMUX_IISDAC_MCLK, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISADC_FD0, PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_IISADC, PAD_ON);
	silan_padmux2_ctrl(SILAN_PADMUX2_CODEC_ADC_SEL, PAD_OFF);
	silan_padmux2_ctrl(SILAN_PADMUX2_IISADC_MODE, PAD_ON);
#endif
	dev->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, dev);

	ret = snd_soc_register_dai(&pdev->dev, &silan_i2s_hdmi_dai);
	if (ret != 0) {
		dev_err(&pdev->dev, "fail to register dai\n");
		goto err4;
	}

	return 0;

err4:
	clk_disable(dev->i2s_params[0].clk);
	clk_disable(dev->i2s_params[1].clk);
	clk_put(dev->i2s_params[0].clk);
	clk_put(dev->i2s_params[1].clk);
err3:
	iounmap(dev->i2s_params[0].regs);
	iounmap(dev->i2s_params[1].regs);
err2:
	kfree(dev);
err1:
	release_mem_region(mem_res[0]->start, resource_size(mem_res[0]));
	release_mem_region(mem_res[1]->start, resource_size(mem_res[1]));
err0:
	return ret;
}

static __devexit int silan_iis_hdmi_dev_remove(struct platform_device *pdev)
{
	struct silan_i2s_hdmi_dev *dev = dev_get_drvdata(&pdev->dev);
	struct resource *mem_res[2];
	int i;

	snd_soc_unregister_dai(&pdev->dev);
	
	for(i = 0; i < 2; i++){
		clk_disable(dev->i2s_params[i].clk);
		clk_put(dev->i2s_params[i].clk);
		dev->i2s_params[i].clk = NULL;
	}
	kfree(dev);

	for(i = 0; i < 2; i++){
		iounmap(dev->i2s_params[i].regs);
		mem_res[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (mem_res[i])
			release_mem_region(mem_res[i]->start, resource_size(mem_res[i]));
	}

	return 0;
}

static struct platform_driver silan_iis_hdmi_driver = {
	.probe	= silan_iis_hdmi_dev_probe,
	.remove	= silan_iis_hdmi_dev_remove,
	.driver	= {
		.name	= "silan-i2s-hdmi",
		.owner	= THIS_MODULE,
	},
};

static int __init silan_i2s_hdmi_init(void)
{
	return platform_driver_register(&silan_iis_hdmi_driver);
}

static void __exit silan_i2s_hdmi_exit(void)
{
	platform_driver_unregister(&silan_iis_hdmi_driver);
}

module_init(silan_i2s_hdmi_init);
module_exit(silan_i2s_hdmi_exit);

/* Module information */
MODULE_AUTHOR("luxuegang@silan.com.cn");
MODULE_DESCRIPTION("Silan I2S HDMI SoC Interface");
MODULE_LICENSE("GPL");
