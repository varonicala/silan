/* linux/drivers/spi/spi_s3c24xx_gpio.c
 *
 * Copyright (c) 2006 Ben Dooks
 * Copyright (c) 2006 Simtec Electronics
 *
 * S3C24XX GPIO based SPI driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/spi_gpio.h>

#include <silan_padmux.h>

struct silan_spigpio {
    struct spi_bitbang        bitbang;
    
    struct platform_device    *dev;
};

static unsigned pin_clk, pin_mosi, pin_miso;
static u16 pin_cs1, pin_cs2; 
#if 0
#define pin_clk  (4*32+18)
#define pin_cs1	 (4*32+19)
#define pin_cs2  (4*32+19)
#define pin_mosi (4*32+16)
#define pin_miso (4*32+17)
#endif

static inline void setsck(struct spi_device *dev, int on)
{
    gpio_set_value(pin_clk, on ? 1 : 0);
}

static inline void setmosi(struct spi_device *dev, int on)
{
    gpio_set_value(pin_mosi, on ? 1 : 0);
}

static inline u32 getmiso(struct spi_device *dev)
{
    return gpio_get_value(pin_miso) ? 1 : 0;
}

#define spidelay(x) ndelay(x)

#include "spi_bitbang_txrx.h"

static u32 silan_spigpio_txrx_mode0(struct spi_device *spi,
                                    unsigned nsecs, u32 word, u8 bits)
{
    if(bits == 16)
    {
        u32 temp = ( (word >> 8 )) & 0xff;
        word = ((word << 8) & 0xff00) | temp;
    }
    return bitbang_txrx_be_cpha0(spi, nsecs, 0, 0, word, bits);
}

static u32 silan_spigpio_txrx_mode1(struct spi_device *spi,
                                    unsigned nsecs, u32 word, u8 bits)
{
    return bitbang_txrx_be_cpha1(spi, nsecs, 0, 0, word, bits);
}

static u32 silan_spigpio_txrx_mode2(struct spi_device *spi,
                                    unsigned nsecs, u32 word, u8 bits)
{
    return bitbang_txrx_be_cpha0(spi, nsecs, 1, 0, word, bits);
}

static u32 silan_spigpio_txrx_mode3(struct spi_device *spi,
                                    unsigned nsecs, u32 word, u8 bits)
{
    return bitbang_txrx_be_cpha1(spi, nsecs, 1, 0, word, bits);
}


static void silan_spigpio_chipselect(struct spi_device *dev, int value)
{
    if(dev->chip_select == 2)
        gpio_set_value(pin_cs1, !value);
    else if(dev->chip_select == 3)
        gpio_set_value(pin_cs2, !value);
}

static int silan_spigpio_probe(struct platform_device *dev)
{
    struct spi_master    *master;
    struct silan_spigpio  *sp;
	struct spi_gpio_platform_data *pdata;
    int ret;

	master = spi_alloc_master(&dev->dev, sizeof(struct silan_spigpio));
    if (master == NULL) {
        dev_err(&dev->dev, "failed to allocate spi master\n");
        ret = -ENOMEM;
        goto err;
    }

	pdata = dev->dev.platform_data;
	if (!pdata)
		return -ENXIO;
    
	sp = spi_master_get_devdata(master);

    platform_set_drvdata(dev, sp);

    /* setup spi bitbang adaptor */
    sp->bitbang.master = spi_master_get(master);
    sp->bitbang.master->bus_num = 1;//info->bus_num;
    sp->bitbang.master->num_chipselect = 5;//info->num_chipselect;
    sp->bitbang.chipselect = silan_spigpio_chipselect;

    sp->bitbang.txrx_word[SPI_MODE_0] = silan_spigpio_txrx_mode0;
    sp->bitbang.txrx_word[SPI_MODE_1] = silan_spigpio_txrx_mode1;
    sp->bitbang.txrx_word[SPI_MODE_2] = silan_spigpio_txrx_mode2;
    sp->bitbang.txrx_word[SPI_MODE_3] = silan_spigpio_txrx_mode3;

    /* set state of spi pins, always assume that the clock is
     * available, but do check the MOSI and MISO. */
    
	pin_clk  = pdata->sck;
	pin_cs1  = pdata->num_chipselect;
	pin_mosi = pdata->mosi;
	pin_miso = pdata->miso;

#if defined(CONFIG_MIPS_SILAN_SUVIII) || defined(CONFIG_SILAN_SUVIII)
	/* gpio pad enable */
	silan_gpiobitpadmux_ctrl(pin_clk, PAD_ON);
	silan_gpiobitpadmux_ctrl(pin_cs1, PAD_ON);
	silan_gpiobitpadmux_ctrl(pin_mosi, PAD_ON);
	silan_gpiobitpadmux_ctrl(pin_miso, PAD_ON);
#endif

	printk("##Spi-gpio: using pins %u (SCK) %u (CS) %u (MOSI) %u (MISO)\n",
			pin_clk, pin_cs1, pin_mosi, pin_miso
			);

	gpio_request(pin_clk, "silan-gpio");
    gpio_direction_output(pin_clk, 0);
    gpio_set_value(pin_clk, 0);
    
    gpio_request(pin_cs1, "silan-gpio");
    gpio_direction_output(pin_cs1, 1);
    gpio_set_value(pin_cs1, 1);

	/*
    gpio_request(pin_cs2, "silan-gpio");
    gpio_direction_output(pin_cs2, 1);
    gpio_set_value(pin_cs2, 1);
*/
    gpio_request(pin_mosi, "silan-gpio");
    gpio_direction_output(pin_mosi, 1);
    gpio_set_value(pin_mosi, 0);

    gpio_request(pin_miso, "silan-gpio");
    gpio_direction_input(pin_miso);

    ret = spi_bitbang_start(&sp->bitbang);
    if (ret)
        goto err_no_bitbang;

    return 0;

 err_no_bitbang:
    spi_master_put(sp->bitbang.master);
 err:
    return ret;
}

static int silan_spigpio_remove(struct platform_device *dev)
{
    struct silan_spigpio *sp = platform_get_drvdata(dev);

    spi_bitbang_stop(&sp->bitbang);
    spi_master_put(sp->bitbang.master);

    return 0;
}

/* all gpio should be held over suspend/resume, so we should
 * not need to deal with this
*/

#define silan_spigpio_suspend NULL
#define silan_spigpio_resume NULL

/* work with hotplug and coldplug */
MODULE_ALIAS("platform:spi_silan_gpio");

static struct platform_driver silan_spigpio_drv = {
    .probe      = silan_spigpio_probe,
    .remove     = silan_spigpio_remove,
    .suspend    = silan_spigpio_suspend,
    .resume     = silan_spigpio_resume,
    .driver     = {
        .name   = "spi_silan_gpio",
        .owner  = THIS_MODULE,
    },
};

static int __init silan_spigpio_init(void)
{
    return platform_driver_register(&silan_spigpio_drv);
}

static void __exit silan_spigpio_exit(void)
{
    platform_driver_unregister(&silan_spigpio_drv);
}

module_init(silan_spigpio_init);
module_exit(silan_spigpio_exit);

MODULE_DESCRIPTION("S3C24XX SPI Driver");
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_LICENSE("GPL");
