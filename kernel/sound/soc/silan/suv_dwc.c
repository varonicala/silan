/*
 * Author: Panjianguang
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include "../codecs/wm8768.h"

static unsigned int rates[33 * 2];

#ifdef CONFIG_SILAN_DLNA
#define SND_CARD_NUM	4
#else
#define SND_CARD_NUM	2
#endif

static struct platform_device *suv_dwc_snd_device[SND_CARD_NUM];

static int suv_dwc_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static void suv_dwc_shutdown(struct snd_pcm_substream *substream)
{
}

static int suv_dwc_hw_params(struct snd_pcm_substream *substream,struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int clk = 0;
	int ret = 0;
	int fs_mode;
	unsigned long rate = params_rate(params);
	long err, cerr;
	int i, bi;

	err = 999999;
	bi = 0;
	for (i = 0; i < 2*33; i++) {
		cerr = rates[i] - rate;
		if (cerr < 0)
			cerr = -cerr;
		if (cerr < err) {
			err = cerr;
			bi = i;
		}
	}
	if (bi / 33 == 1)
		fs_mode = MCLK_SEL_256FS;
	else
		fs_mode = MCLK_SEL_384FS;

	clk = (fs_mode == MCLK_SEL_384FS ? 384 : 256) * rate;

#if 0
	if ((err * 100 / rate) > 5) {
		printk(KERN_ERR "SUV DWC: effective frequency "
		       "too different from desired (%ld%%)\n",
		       err * 100 / rate);
		return -EINVAL;
	}
#endif

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_IB_IF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai,0, clk,SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
	/* set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, clk,SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops suv_dwc_ops = {
	.startup = suv_dwc_startup,
	.shutdown = suv_dwc_shutdown,
	.hw_params = suv_dwc_hw_params,
};

static struct snd_soc_dai_link suv_i2s_inner_dai_link = {
    .name = "IIS CODEC",
	.stream_name = "IIS CODEC",
    .codec_name = "silan-inner",
	.codec_dai_name = "silan-inner-dai",
    .cpu_dai_name = "silan-i2s-hdmi",
	.platform_name = "silan-pcm",
	.ops = &suv_dwc_ops,
};

static struct snd_soc_dai_link suv_i2s_wmcodec_dai_link = {
    .name = "IIS CODEC",
	.stream_name = "IIS CODEC",
    .codec_name = "silan-wmcodec",
    .codec_dai_name = "silan-wmcodec-dai",
    .cpu_dai_name = "silan-i2s-hdmi",
	.platform_name = "silan-pcm",
	.ops = &suv_dwc_ops,
};

static struct snd_soc_dai_link suv_spd_dai_link = {
	.name = "SPDCODEC SPDIF TX",
	.stream_name = "Playback",
	.codec_name = "silan-spdcodec",
	.codec_dai_name = "silan-spdcodec-dai",
	.cpu_dai_name = "silan-spdif",
	.platform_name = "silan-pcm",
	.ops = &suv_dwc_ops,
};

#ifdef CONFIG_SILAN_DLNA
static struct snd_soc_dai_link suv_spd_in_dai_link = {
	.name = "SPDCODEC SPDIF RX",
	.stream_name = "Capture",
	.codec_name = "silan-spdcodec",
	.codec_dai_name = "silan-spdcodec-dai",
	.cpu_dai_name = "silan-spdif-in",
	.platform_name = "silan-pcm",
	.ops = &suv_dwc_ops,
};
#endif
static struct snd_soc_dai_link suv_hdmi_dai_link = {
	.name = "SPDCODEC HDMI TX",
	.stream_name = "Playback",
	.codec_name = "silan-spdcodec",
	.codec_dai_name = "silan-spdcodec-dai",
	.cpu_dai_name = "silan-hdmi-audio",
	.platform_name = "silan-hdmi-pcm",
	.ops = &suv_dwc_ops,
};

static struct snd_soc_card snd_soc_suv_card[] = {
	{
		.name = "SILAN_I2SINNERACODEC",
		.dai_link = &suv_i2s_inner_dai_link,
		.num_links = 1,
	},
    {
		.name = "SILAN_I2SEXTRAACODEC",
		.dai_link = &suv_i2s_wmcodec_dai_link,
		.num_links = 1,
	},
	{	
		.name = "SILAN_SPDACODEC",
		.dai_link = &suv_spd_dai_link,
		.num_links = 1,
	},
#ifdef CONFIG_SILAN_DLNA
	{	
		.name = "SILAN_SPDINACODEC",
		.dai_link = &suv_spd_in_dai_link,
		.num_links = 1,
	},
#endif
};

static int __init suv_dwc_init(void)
{
	int ret;
	int i;

	printk(KERN_INFO "SILAN SUV SoC Audio driver\n");

	for(i=0; i < SND_CARD_NUM; i++){
		suv_dwc_snd_device[i] = platform_device_alloc("soc-audio", i);
		if (!suv_dwc_snd_device[i]) {
			printk(KERN_ERR "SILAN SUV SoC Audio: "
			       "Unable to register device%d\n", i);
			return -ENOMEM;
		}
	}

	for(i=0; i < SND_CARD_NUM; i++) {
		platform_set_drvdata(suv_dwc_snd_device[i], &snd_soc_suv_card[i]);
		ret = platform_device_add(suv_dwc_snd_device[i]);
		if (ret) {
			printk(KERN_ERR "SILAN SUV SoC Audio: Unable to add\n");
			platform_device_put(suv_dwc_snd_device[i]);
		}
	}

	return ret;
}

static void __exit suv_dwc_exit(void)
{
	int i;

	for(i=0; i < SND_CARD_NUM; i++) 
		platform_device_unregister(suv_dwc_snd_device[i]);
}

module_init(suv_dwc_init);
module_exit(suv_dwc_exit);

MODULE_AUTHOR("Panjianguang, panjianguang@silan.com.cn");
MODULE_DESCRIPTION("SILAN SUV ALSA SoC audio driver");
MODULE_LICENSE("GPL");
