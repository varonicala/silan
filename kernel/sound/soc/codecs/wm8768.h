/*
 * wm8768.h  --  WM8768 ASoC codec driver
 *
 * Copyright 2008 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _WM8768_H
#define _WM8768_H

#define WM8768_DAC1LVOL		0x00
#define WM8768_DAC1RVOL		0x01
#define WM8768_R2			0x02
#define WM8768_R3			0x03
#define WM8768_DAC2LVOL		0x04
#define WM8768_DAC2RVOL		0x05
#define WM8768_DAC3LVOL		0x06
#define WM8768_DAC3RVOL		0x07
#define WM8768_MASTDA		0x08
#define WM8768_R9			0x09
#define WM8768_R10			0x0A
#define WM8768_R11			0x0B
#define WM8768_ADC_MPD		0x0C
#define WM8768_DAC4LVOL		0x0D
#define WM8768_DAC4RVOL		0x0E
#define WM8768_R15			0x0F
#define WM8768_RESET		0x1F

typedef enum
{
	MCLK_SEL_START = 0,
	MCLK_SEL_256FS = MCLK_SEL_START,
	MCLK_SEL_384FS,
	MCLK_SEL_12M,
	MCLK_SEL_24M,
	MCLK_SEL_48K256FS,
	MCLK_SEL_96K256FS,
	MCLK_SEL_44K256FS,
	MCLK_SEL_88K256FS,
	MCLK_SEL_48K384FS,
	MCLK_SEL_96K384FS,
	MCLK_SEL_44K384FS,
	MCLK_SEL_88K384FS,
	MCLK_SEL_END,
}MCLKFS;

struct wm8768_setup_data {
	int            spi;
	int            i2c_bus;
	unsigned short i2c_address;
};

#endif
