/*
 * spd_codec.c -- ALSA SoC Codec driver
 * Created by Luxuegang <luxuegang@silan.com>
 * 
 * A virtual Codec driver for a sound card.
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

static int spd_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int spd_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	return 0;
}

#define SPD_RATES SNDRV_PCM_RATE_8000_192000

#define SPD_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | \
		SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops codec_spd_ops = {
	.set_sysclk = spd_set_dai_sysclk,
	.set_fmt = spd_set_dai_fmt,
};

struct snd_soc_dai_driver codec_spd_dai = {
	.name = "silan-spdcodec-dai",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SPD_RATES,
		.formats = SPD_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SPD_RATES,
		.formats = SPD_FORMATS,
	},
	.ops = &codec_spd_ops,
};

static struct snd_soc_codec_driver silan_soc_spdcodec;

static int silan_spdcodec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &silan_soc_spdcodec, &codec_spd_dai, 1);
}

static int silan_spdcodec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver silan_spdcodec_driver = {
	.probe	= silan_spdcodec_probe,
	.remove	= silan_spdcodec_remove,
	.driver	= {
		.name	= "silan-spdcodec",
		.owner	= THIS_MODULE,
	},
};

static int __init spd_codec_init(void)
{
	return platform_driver_register(&silan_spdcodec_driver);
}
module_init(spd_codec_init);

static void __exit spd_codec_exit(void)
{
	platform_driver_register(&silan_spdcodec_driver);
}
module_exit(spd_codec_exit);

MODULE_DESCRIPTION("SILAN SUV ALSA soc codec driver for spd");
MODULE_AUTHOR("Lu Xuegang <luxuegang@silan.com>");
MODULE_LICENSE("GPL");
