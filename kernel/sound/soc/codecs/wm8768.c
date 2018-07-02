/*
 * wm8768.c  --  WM8768 ALSA SoC Audio driver
 *
 * Copyright 2008 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "wm8768.h"

/*
 * We can't read the WM8768 register space so we cache them instead.
 * Note that the defaults here aren't the physical defaults, we latch
 * the volume update bits, mute the output and enable infinite zero
 * detect.
 */
static const u16 wm8768_reg[] = {
	0x01ff,	0x01ff,	0x0120,	0x0000,
	0x01ff,	0x01ff, 0x01ff, 0x01ff,
	0x01ff, 0x0000, 0x0080, 0x0000,
	0x0000, 0x01ff, 0x01ff, 0x0000,
	0x0000
};

/* codec private data */
struct wm8768_priv {
	enum snd_soc_control_type control_type;
};

static const DECLARE_TLV_DB_SCALE(wm8768_tlv, -12750, 50, 1);

static const struct snd_kcontrol_new wm8768_snd_controls[] = {
SOC_SINGLE_TLV("DAC Playback Volume", WM8768_MASTDA, 
		0, 255, 0, wm8768_tlv),
SOC_DOUBLE_R_TLV("DAC1 Playback Volume", WM8768_DAC1LVOL, WM8768_DAC1RVOL,
		0, 255, 0, wm8768_tlv),
SOC_DOUBLE_R_TLV("DAC2 Playback Volume", WM8768_DAC2LVOL, WM8768_DAC2RVOL,
		0, 255, 0, wm8768_tlv),
SOC_DOUBLE_R_TLV("DAC3 Playback Volume", WM8768_DAC3LVOL, WM8768_DAC3RVOL,
		0, 255, 0, wm8768_tlv),
SOC_DOUBLE_R_TLV("DAC4 Playback Volume", WM8768_DAC4LVOL, WM8768_DAC4RVOL,
		0, 255, 0, wm8768_tlv),

SOC_SINGLE("DAC1 Playback Switch", WM8768_R9, 3, 1, 0),
SOC_SINGLE("DAC2 Playback Switch", WM8768_R9, 4, 1, 0),
SOC_SINGLE("DAC3 Playback Switch", WM8768_R9, 5, 1, 0),
SOC_SINGLE("DAC4 Playback Switch", WM8768_R15, 2, 1, 0),

SOC_SINGLE("DACDeemp", WM8768_R2, 1, 1, 0),
SOC_SINGLE("DAC1Deemp", WM8768_R9, 6, 1, 0),
SOC_SINGLE("DAC2Deemp", WM8768_R9, 7, 1, 0),
SOC_SINGLE("DAC3Deemp", WM8768_R9, 8, 1, 0),
SOC_SINGLE("DAC4Deemp", WM8768_R15, 4, 1, 0),
};

/*
 * DAPM controls.
 */
static const struct snd_soc_dapm_widget wm8768_dapm_widgets[] = {
SND_SOC_DAPM_DAC("DAC", "Playback", WM8768_R2, 2, 0),
SND_SOC_DAPM_DAC("DAC1", "Playback", WM8768_R10, 1, 0),
SND_SOC_DAPM_DAC("DAC2", "Playback", WM8768_R10, 2, 0),
SND_SOC_DAPM_DAC("DAC3", "Playback", WM8768_R10, 3, 0),
SND_SOC_DAPM_DAC("DAC4", "Playback", WM8768_R15, 1, 0),

SND_SOC_DAPM_INPUT("DIN1"),
SND_SOC_DAPM_INPUT("DIN2"),
SND_SOC_DAPM_INPUT("DIN3"),
SND_SOC_DAPM_INPUT("DIN4"),

SND_SOC_DAPM_OUTPUT("LOUT1"),
SND_SOC_DAPM_OUTPUT("ROUT1"),
SND_SOC_DAPM_OUTPUT("LOUT2"),
SND_SOC_DAPM_OUTPUT("ROUT2"),
SND_SOC_DAPM_OUTPUT("LOUT3"),
SND_SOC_DAPM_OUTPUT("ROUT3"),
SND_SOC_DAPM_OUTPUT("LOUT4"),
SND_SOC_DAPM_OUTPUT("ROUT4"),
};

static const struct snd_soc_dapm_route wm8768_intercon[] = {
/* Inputs */
	{"DIN1", NULL, "DAC1"},
	{"DIN2", NULL, "DAC2"},
	{"DIN3", NULL, "DAC3"},
	{"DIN4", NULL, "DAC4"},

/* Outputs */
	{"LOUT1", NULL, "DAC1"},
	{"ROUT1", NULL, "DAC1"},
	{"LOUT2", NULL, "DAC2"},
	{"ROUT2", NULL, "DAC2"},
	{"LOUT3", NULL, "DAC3"},
	{"ROUT3", NULL, "DAC3"},
	{"LOUT4", NULL, "DAC4"},
	{"ROUT4", NULL, "DAC4"},
};

static int wm8768_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u16 mute_reg = snd_soc_read(codec, WM8768_R2);

	if (mute)
		snd_soc_write(codec, WM8768_R2, mute_reg | 1);
	else
		snd_soc_write(codec, WM8768_R2, mute_reg & ~1);

	return 0;
}

static int wm8768_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	u16 dac = snd_soc_read(codec, WM8768_R3);

	dac &= ~0x30;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		dac |= 0x10;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		dac |= 0x20;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		dac |= 0x30;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, WM8768_R3, dac);

	return 0;
}

static int wm8768_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 iface = snd_soc_read(codec, WM8768_R3);
	u16 ms = snd_soc_read(codec, WM8768_R10);

	/* Currently only I2S is supported by the driver, though the
	 * hardware is more flexible.
	 */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iface &= ~0x03;
		iface |= 0x02;
		break;
	default:
		printk(KERN_ERR "%s unsupported daifmt\n", __func__);
		return -EINVAL;
	}

	/* The hardware only support full slave mode */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		ms &= ~0x20;
		break;
	default:
		printk(KERN_ERR "%s unsupported slave mode\n", __func__);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iface &= ~0x0c;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |=  0x08;
		iface &= ~0x04;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface |= 0x04;
		iface &= ~0x08;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x0c;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, WM8768_R10, ms);
	snd_soc_write(codec, WM8768_R3, iface);

	return 0;
}

static int wm8768_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int wm8768_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	u16 reg1, reg2;
	int i;

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {
			/* Power everything up... */
			reg1 = snd_soc_read(codec, WM8768_R10);
			reg2 = snd_soc_read(codec, WM8768_R2);
			snd_soc_write(codec, WM8768_R10, reg1 & ~0x10);
			snd_soc_write(codec, WM8768_R2, reg2 & ~0x4);
			/* ..then sync in the register cache. */
			for (i = 0; i < ARRAY_SIZE(wm8768_reg); i++)
				snd_soc_write(codec, i, snd_soc_read(codec, i));
		}
		break;

	case SND_SOC_BIAS_OFF:
		reg1 = snd_soc_read(codec, WM8768_R2);
		reg2 = snd_soc_read(codec, WM8768_R10);
		snd_soc_write(codec, WM8768_R2, reg1 | 0x4);
		snd_soc_write(codec, WM8768_R10, reg2 | 0x10);
		break;
	}

	codec->dapm.bias_level = level;

	return 0;
}

#define WM8768_RATES (SNDRV_PCM_RATE_8000_192000)

#define WM8768_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops wm8768_dai_ops = {
	.hw_params = wm8768_hw_params,
	.digital_mute = wm8768_mute,
	.set_fmt = wm8768_set_dai_fmt,
	.set_sysclk = wm8768_set_dai_sysclk,
};

static struct snd_soc_dai_driver wm8768_dai = {
	.name = "silan-wmcodec-dai",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = WM8768_RATES,
		.formats = WM8768_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = WM8768_RATES,
		.formats = WM8768_FORMATS,
	},
	.ops = &wm8768_dai_ops,
};

static int wm8768_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	wm8768_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int wm8768_resume(struct snd_soc_codec *codec)
{
	wm8768_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

static int wm8768_probe(struct snd_soc_codec *codec)
{
#if defined(CONFIG_SPI_SILAN1) || defined(CONFIG_I2C_CODEC_SILAN)
	struct wm8768_priv *wm8768 = snd_soc_codec_get_drvdata(codec);
#endif
	int ret = 0;

#if defined(CONFIG_SPI_SILAN1) || defined(CONFIG_I2C_CODEC_SILAN)
	ret = snd_soc_codec_set_cache_io(codec, 7, 9, wm8768->control_type);
	if(ret < 0) {
		printk(KERN_ERR "wm8768: failed to configure cache I/O: %d\n", ret);
		return ret;
	}
#endif

	wm8768_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	snd_soc_add_controls(codec, wm8768_snd_controls,
					ARRAY_SIZE(wm8768_snd_controls));

	return ret;
}

/* power down chip */
static int wm8768_remove(struct snd_soc_codec *codec)
{
	wm8768_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

#if !defined(CONFIG_SPI_SILAN1) && !defined(CONFIG_I2C_CODEC_SILAN)
static unsigned int wm8768_read_reg_cache(struct snd_soc_codec *codec, unsigned int reg)
{
	return 0;
}

static int wm8768_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	return 0;
}
#endif

static struct snd_soc_codec_driver soc_codec_dev_wm8768 = {
	.probe	= wm8768_probe,
	.remove	= wm8768_remove,
	.suspend = wm8768_suspend,
	.resume = wm8768_resume,
	.set_bias_level = wm8768_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(wm8768_reg),
	.reg_word_size = sizeof(u16),
	.reg_cache_default = wm8768_reg,
	.dapm_widgets = wm8768_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(wm8768_dapm_widgets),
	.dapm_routes = wm8768_intercon,
	.num_dapm_routes = ARRAY_SIZE(wm8768_intercon),
#if !defined(CONFIG_SPI_SILAN1) && !defined(CONFIG_I2C_CODEC_SILAN)
	.read = wm8768_read_reg_cache,
	.write = wm8768_write,
#endif
};

#if defined(CONFIG_SPI_SILAN1)
static int __devinit wm8768_spi_probe(struct spi_device *spi)
{
	struct wm8768_priv *wm8768;
	int ret;

	wm8768 = kzalloc(sizeof(struct wm8768_priv), GFP_KERNEL);
	if(wm8768 == NULL)
		return -ENOMEM;

	wm8768->control_type = SND_SOC_SPI;
	spi_set_drvdata(spi, wm8768);
	
	ret = snd_soc_register_codec(&spi->dev, &soc_codec_dev_wm8768, 
			&wm8768_dai, 1);
	if(ret < 0)
		kfree(wm8768);

	return ret;
}

static int __devexit wm8768_spi_remove(struct spi_device *spi)
{
	snd_soc_unregister_codec(&spi->dev);
	kfree(spi_get_drvdata(spi));

	return 0;
}

static struct spi_driver wm8768_spi_driver = {
	.driver	= {
		.name	= "silan-wmcodec",
		.owner	= THIS_MODULE,
	},
	.probe	= wm8768_spi_probe,
	.remove = __devexit_p(wm8768_spi_remove),
};

#elif defined(CONFIG_I2C_CODEC_SILAN)
static __devinit int wm8768_i2c_probe(struct i2c_client *i2c,
					const struct i2c_device_id *id)
{
	struct wm8768_priv *wm8768;
	int ret;

	wm8768 = kzalloc(sizeof(struct wm8768_priv), GFP_KERNEL);
	if(wm8768 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, wm8768);
	wm8768->control_type = SND_SOC_I2C;

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_wm8768,
			&wm8768_dai, 1);
	if(ret < 0)
		kfree(wm8768);

	return ret;
}

static __devexit int wm8768_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static const struct i2c_device_id wm8768_i2c_id[] = {
	{ "wm8768", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, wm8768_i2c_id);

static struct i2c_driver wm8768_i2c_driver = {
	.driver = {
		.name	= "silan-wmcodec",
		.owner	= THIS_MODULE,
	},
	.probe	= wm8768_i2c_probe,
	.remove	= __devexit_p(wm8768_i2c_remove),
	.id_table = wm8768_i2c_id,
};

#else
static int __devinit wm8768_dev_probe(struct platform_device *pdev)
{
	struct wm8768_priv *wm8768;

	wm8768 = kzalloc(sizeof(struct wm8768_priv), GFP_KERNEL);
	if(wm8768 == NULL)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, wm8768);

	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_wm8768, 
			&wm8768_dai, 1);

	return 0;
}

static int __devexit wm8768_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);

	return 0;
}

static struct platform_driver wm8768_driver = {
	.driver = {
		.name = "silan-wmcodec",
		.owner = THIS_MODULE,
	},
	.probe = wm8768_dev_probe,
	.remove = wm8768_dev_remove,
};
#endif

static int __init wm8768_modinit(void)
{
	int ret = 0;
#if defined(CONFIG_SPI_SILAN1)
	ret = spi_register_driver(&wm8768_spi_driver);
	if(ret != 0)
		printk(KERN_ERR "Failed to register wm8768 SPI driver: %d\n", ret);
#elif defined(CONFIG_I2C_CODEC_SILAN)
	ret = i2c_add_driver(&wm8768_i2c_driver);
	if(ret != 0) 
		printk(KERN_ERR "Failed to register wm8768 I2C driver: %d\n", ret);
#else
	ret = platform_driver_register(&wm8768_driver);
#endif
	return ret;
}
module_init(wm8768_modinit);

static void __exit wm8768_codec_exit(void)
{
#if defined(CONFIG_SPI_SILAN1)
	spi_unregister_driver(&wm8768_spi_driver);
#elif defined(CONFIG_I2C_CODEC_SILAN)
	i2c_del_driver(&wm8768_i2c_driver);
#else
	platform_driver_unregister(&wm8768_driver);
#endif
}
module_exit(wm8768_codec_exit);


MODULE_DESCRIPTION("ASoC WM8768 driver");
MODULE_AUTHOR("Lu Xuegang <luxuegang@silan.com.cn>");
MODULE_LICENSE("GPL");
