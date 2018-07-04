/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006, 07 MIPS Technologies, Inc.
 *   written by Ralf Baechle (ralf@linux-mips.org)
 *     written by Ralf Baechle <ralf@linux-mips.org>
 *
 * Copyright (C) 2008 Wind River Systems, Inc.
 *   updated by Tiejun Chen <tiejun.chen@windriver.com>
 *
 * 1. Probe driver for the SILAN SUV's UART ports:
 *
 * UART becoming ttyS0.
 
 * 2. Register DWAC MAC platform device on SILAN SUV.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/stmmac.h>
#include <linux/pmem.h>
#include <linux/interrupt.h>
#include <linux/mmc/silan_mmc.h>
#include <linux/i2c-gpio.h>

#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_gpio.h>
#include <silan_nand.h>
#include <silan_def.h>
#include <silan_padmux.h>
#include <linux/amba/bus.h>
#include <linux/pwm_backlight.h>
#include <linux/timed_gpio.h>
#include <linux/leds.h>

#define SUV_I_BUS_CLK    74250000
#define SUV_II_BUS_CLK    36000000

static struct resource uart_pl011_resource[] =
{
    {
        .start  = SILAN_UART2_PHY_BASE,
        .end    = SILAN_UART2_PHY_BASE + SILAN_UART2_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = PIC_IRQ_UART2,
        .end    = PIC_IRQ_UART2,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct platform_device silan_uart_device =
{
    .name            = "silan-uart2",
    //.name            = "uart-pl011",
    .id              = 0,
    .num_resources   = ARRAY_SIZE(uart_pl011_resource),
    .resource        = uart_pl011_resource,
};

static struct resource uart_uart3_resource[] =
{
    {
        .start  = SILAN_UART3_PHY_BASE,
        .end    = SILAN_UART3_PHY_BASE + SILAN_UART3_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = PIC_IRQ_UART3,
        .end    = PIC_IRQ_UART3,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct platform_device silan_uart3_device = 
{
    .name            = "silan-uart3",
    .id                = 1,
    .num_resources    = ARRAY_SIZE(uart_uart3_resource),
    .resource        = uart_uart3_resource,
};

static struct resource uart_uart4_resource[] = 
{
    {
        .start  = SILAN_UART4_PHY_BASE,
        .end    = SILAN_UART4_PHY_BASE + SILAN_UART4_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = PIC_IRQ_UART4,
        .end    = PIC_IRQ_UART4,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct platform_device silan_uart4_device = 
{
    .name            = "silan-uart4",
    .id                = 2,
    .num_resources    = ARRAY_SIZE(uart_uart4_resource),
    .resource        = uart_uart4_resource,
};

static int silan_phy_reset(void * priv) 
{
#if 0
    OUT_GPIO(3);
    SET_GPIO(3);
    udelay(1);
    CLR_GPIO(3);
    udelay(2);
    SET_GPIO(3);
#endif
    return 0;
}

static struct plat_stmmacphy_data phy_private_data = 
{
    .bus_id = 0,
    .phy_addr = -1,
    .phy_mask = 0,
    .interface = PHY_INTERFACE_MODE_MII,
    .phy_reset = &silan_phy_reset,
};

static struct resource silan_phy_resources[] = 
{
    [0] = {
        .name   = "phyirq",
        .start    = -1,    /* since we want to work on poll mode */
        .end    = -1,
        .flags    = IORESOURCE_IRQ,
    },
};
 
static struct platform_device silan_phy_device = 
{
    .name            = "stmmacphy",
    .id                = -1,
    .num_resources    = ARRAY_SIZE(silan_phy_resources),
    .resource        = silan_phy_resources,
    .dev = {
        .platform_data = &phy_private_data,
    }
};

static struct plat_stmmacenet_data mac_private_data = 
{
    .bus_id       = 0,
    .pbl          = 8,
    .has_gmac     = 1,
    .enh_desc     = 1,
    .tx_coe       = 1,
    .clk_csr      = 0,
    .bugged_jumbo = 0,
    .pmt          = 0,
};

static struct resource silan_mac_resources[] = 
{
    [0] = {
        .start    = SILAN_GMAC_PHY_BASE,
        .end      = SILAN_GMAC_PHY_BASE + SILAN_GMAC_SIZE,
        .flags    = IORESOURCE_MEM,
    },
    [1] = {
        .name   = "macirq",
        .start  = PIC_IRQ_GMAC,
        .end    = PIC_IRQ_GMAC,
        .flags  = IORESOURCE_IRQ,
    }
};

static struct platform_device silan_mac_device = 
{
    .name    = "stmmaceth",
    .id      = -1,
    .dev     = {
        .platform_data = &mac_private_data,
    },
    .num_resources    = ARRAY_SIZE(silan_mac_resources),
    .resource    = silan_mac_resources,
};

static struct resource silan_drm_resource[] = 
{
    {
        .start  = SILAN_DRM_DMA_PHY_BASE,
        .end    = SILAN_DRM_DMA_PHY_BASE + SILAN_DRM_DMA_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = SILAN_DRM_BUF_PHY_BASE,
        .end    = SILAN_DRM_BUF_PHY_BASE + SILAN_DRM_BUF_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = SILAN_DRM_CORE_PHY_BASE,
        .end    = SILAN_DRM_CORE_PHY_BASE + SILAN_DRM_CORE_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = PIC_IRQ_DRM,
        .end    = PIC_IRQ_DRM,
        .flags  = IORESOURCE_IRQ,
    },    
};

static struct platform_device silan_drm_device = 
{
    .name            = "silan-drm",
    .id              = -1,
    .num_resources   = ARRAY_SIZE(silan_drm_resource),
    .resource        = silan_drm_resource,
};

static struct resource silan_hostusb_resource[] = 
{
    [0] = {
        .start    = SILAN_HOSTUSB_PHY_BASE,
        .end    = SILAN_HOSTUSB_PHY_BASE + SILAN_HOSTUSB_SIZE,
        .flags    = IORESOURCE_MEM,
    },
    
    [1] = {
        .name   = "hostusb_irq",
        .start    = PIC_IRQ_USB_HOST,
        .end    = PIC_IRQ_USB_HOST,
        .flags    = IORESOURCE_IRQ,
    },
};

static struct platform_device silan_hostusb_device = 
{
    .name            = "silan-hostusb",
    .id                = -1,
    .num_resources    = ARRAY_SIZE(silan_hostusb_resource),
    .resource        = silan_hostusb_resource,
};

static struct resource silan_otgusb_resource[] = 
{
    [0] = {
        .start  = SILAN_OTGUSB_PHY_BASE,
        .end    = SILAN_OTGUSB_PHY_BASE + SILAN_OTGUSB_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    
    [1] = {
        .name   = "otgusb_irq",
        .start  = PIC_IRQ_USB_OTG,
        .end    = PIC_IRQ_USB_OTG,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct platform_device silan_otgusb_device = 
{
    .name           = "silan-otgusb",
    .id             = -1,
    .num_resources  = ARRAY_SIZE(silan_otgusb_resource),
    .resource       = silan_otgusb_resource,
};
/*
static struct resource silan_i2c_resource[] = 
{
    [0] = {
        .start  = SILAN_I2C1_PHY_BASE,
        .end    = SILAN_I2C1_PHY_BASE + SILAN_I2C1_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .start  = PIC_IRQ_I2C1,
        .end    = PIC_IRQ_I2C1,
        .flags  = IORESOURCE_IRQ,
    },    
    [2] = {
        .start  = 8,
        .end    = 8,
        .flags  = IORESOURCE_DMA,
    },
    [3] = {
        .start  = 7,
        .end    = 7,
        .flags  = IORESOURCE_DMA,
    },
};

static struct platform_device silan_i2c_device = 
{
    .name           = "silan-i2c",
    .id             = -1,
    .num_resources  = ARRAY_SIZE(silan_i2c_resource),
    .resource       = silan_i2c_resource,
};
*/
static struct resource silan_spi_resource[] = {
    [0] = {
        .start = SILAN_SPI_PHY_BASE,
        .end   = SILAN_SPI_PHY_BASE+SILAN_SPI_SIZE,
        .flags = IORESOURCE_MEM,
        },
};

static struct platform_device silan_spi_device ={
    .name          = "silan-spi",
    .id            = -1,
    .num_resources = ARRAY_SIZE(silan_spi_resource),
    .resource      = silan_spi_resource,
};

static struct resource silan_spi_ctrl_resource[] = {
    [0] = {
        .start = SILAN_SPI_CTRL_PHY_BASE,
        .end   = SILAN_SPI_CTRL_PHY_BASE+SILAN_SPI_CTRL_SIZE,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device silan_spi_ctrl_device ={
    .name          = "silan-spictrl",
    .id            = -1,
    .num_resources = ARRAY_SIZE(silan_spi_ctrl_resource),
    .resource      = silan_spi_ctrl_resource,
};

static struct resource silan_rtc_resource[] = {
    [0] = {
        .start = SILAN_RTC_PHY_BASE,
        .end   = SILAN_RTC_PHY_BASE+SILAN_RTC_SIZE,
        .flags = IORESOURCE_MEM,
    },

    [1] = {
        .start  = PIC_IRQ_RTC,
        .end    = PIC_IRQ_RTC,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct platform_device silan_rtc_device ={
    .name          = "silan-rtc",
    .id            = -1,
    .num_resources = ARRAY_SIZE(silan_rtc_resource),
    .resource      = silan_rtc_resource,
};

static struct resource silan_wtg_resource[] = {
    [0] = {
        .start = SILAN_WTG_PHY_BASE,
        .end   = SILAN_WTG_PHY_BASE+SILAN_WTG_SIZE,
        .flags = IORESOURCE_MEM,
    },
    
	[1] = {
        .start  = PIC_IRQ_WTG,
        .end    = PIC_IRQ_WTG,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct platform_device silan_wtg_device ={
    .name          = "silan-watchdog",
    .id            = -1,
    .num_resources = ARRAY_SIZE(silan_wtg_resource),
    .resource      = silan_wtg_resource,
};

static struct resource silan_dsp_resource[] = {
    {
        .start = SILAN_DSPCFG_PHY_BASE,
        .end   = SILAN_DSPCFG_PHY_BASE + SILAN_DSPCFG_SIZE ,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SILAN_CXC_PHY_BASE,
        .end   = SILAN_CXC_PHY_BASE + SILAN_CXC_SIZE,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = MIPS_DSP_CXC_IRQ,
        .end   = MIPS_DSP_CXC_IRQ,
        .flags = IORESOURCE_IRQ,
    }
};

static struct platform_device silan_dsp_device ={
    .name          = "silan-dsp",
    .id            = -1,
    .num_resources = ARRAY_SIZE(silan_dsp_resource),
    .resource      = silan_dsp_resource,
};

static struct resource silan_mmc_resource0[] = 
{
    [0] = {
        .start  = SILAN_SD_PHY_BASE,
        .end    = SILAN_SD_PHY_BASE + SILAN_SD_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    
    [1] = {
        .start  = PIC_IRQ_SDMMC,
        .end    = PIC_IRQ_SDMMC,
        .flags  = IORESOURCE_IRQ,
    }
};

static struct resource silan_mmc_resource1[] = 
{
    [0] = {
        .start  = SILAN_MMC_PHY_BASE,
        .end    = SILAN_MMC_PHY_BASE + SILAN_MMC_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    
    [1] = {
        .start  = PIC_IRQ_SDIO,
        .end    = PIC_IRQ_SDIO,
        .flags  = IORESOURCE_IRQ,
    }    
};

static struct dw_mci_board mmc_plat_data =
{
    .quirks = 0,
    //.bus_hz = 74250000,
    .bus_hz = SUV_II_BUS_CLK,
    .detect_delay_ms = 10,
    .num_slots = 1,
};

static struct platform_device silan_mmc_device0 = 
{
    .dev = {
        .platform_data = &mmc_plat_data,    
    },
    .name              = "silan-mmc0",
    .id                = 0,
    .num_resources     = ARRAY_SIZE(silan_mmc_resource0),
    .resource          = silan_mmc_resource0,
};

static struct platform_device silan_mmc_device1 = 
{
    .dev = {
        .platform_data = &mmc_plat_data,    
    },
    .name            = "silan-mmc1",
    .id              = 1,
    .num_resources   = ARRAY_SIZE(silan_mmc_resource1),
    .resource        = silan_mmc_resource1,
};

//GPIO1
static struct resource silan_gpio1_resource[] = 
{
    [0] = {
        .start    = SILAN_GPIO1_PHY_BASE,
        .end      = SILAN_GPIO1_PHY_BASE + SILAN_GPIO1_SIZE - 1,
        .flags    = IORESOURCE_MEM,
    },
    
    [1] = {
        .start    = PIC_IRQ_GPIO1,
        .end      = PIC_IRQ_GPIO1,
        .flags    = IORESOURCE_IRQ,
    }    
};

static struct pl061_platform_data gpio1_plat_data = 
{ 
    .gpio_base      = SILAN_GPIO1_BASENUM,
    .irq_base       = PIC_IRQ_GPIO1,
    .gpio_irqbase   = SILAN_GPIO1_IRQBASE,
    .gpio_num       = SILAN_GPIO1_GPIONUM,
    .padmux_gpio    = SILAN_PADMUX_END-1, 
};

static struct platform_device silan_gpio1_device = 
{
    .dev = { 
                .init_name = "gpio1",
                .platform_data = &gpio1_plat_data,
        },  
    .name            = "silan-gpio1",
    .id              = -1,
    .num_resources   = ARRAY_SIZE(silan_gpio1_resource),
    .resource        = silan_gpio1_resource,
};

//GPIO2
static struct resource silan_gpio2_resource[] = 
{
    [0] = {
        .start  = SILAN_GPIO2_PHY_BASE,
        .end    = SILAN_GPIO2_PHY_BASE + SILAN_GPIO2_SIZE - 1,
        .flags  = IORESOURCE_MEM,
    },
    
    [1] = {
        .start  = PIC_IRQ_GPIO2,
        .end    = PIC_IRQ_GPIO2,
        .flags  = IORESOURCE_IRQ,
    }    
};


static struct pl061_platform_data gpio2_plat_data = 
{ 
    .gpio_base      = SILAN_GPIO2_BASENUM,
    .irq_base       = PIC_IRQ_GPIO2,
    .gpio_irqbase   = SILAN_GPIO2_IRQBASE,
    .gpio_num       = SILAN_GPIO2_GPIONUM,
    .padmux_gpio    = SILAN_PADMUX_END-1,
};

static struct platform_device silan_gpio2_device = 
{
    .dev = { 
                .init_name = "gpio2",
                .platform_data = &gpio2_plat_data,
        },  
    .name            = "silan-gpio2",
    .id              = -1,
    .num_resources   = ARRAY_SIZE(silan_gpio2_resource),
    .resource        = silan_gpio2_resource,
};

static struct platform_device silan_testio_device = 
{
    .name   = "silan_testio",
    .id     = -1,
};

static struct platform_device silan_padmux_device = 
{
    .name   = "silan-padmux",
    .id     = -1,
};

#if defined(CONFIG_SND_SILAN_SOC) && !defined(CONFIG_SPI_SILAN_GPIO)
static struct platform_device silan_wmcodec_device = 
{
    .name            = "silan-wmcodec",
    .id                = -1,
};
#endif
/*
static struct i2c_gpio_platform_data i2c_gpio_adapter_data = 
{
    .sda_pin = 32+12,
    .scl_pin = 32+11,
    .udelay  = 50,
    .timeout = 100,
    .sda_is_open_drain = 1,
    .scl_is_open_drain = 1,    
};

static struct platform_device silan_i2c_gpio = 
{
    .name = "i2c-gpio",
    .id = -1,
    .dev = {
        .platform_data = &i2c_gpio_adapter_data,
    },
};
*/
static struct timed_gpio vibrator = 
{
    .name    = "vibrator",
    .gpio    = 61,
    .max_timeout = 10000,
    .active_low  = 0,
};

static struct timed_gpio_platform_data timed_gpio_data =
{
    .num_gpios =1,
    .gpios = &vibrator,
};

static struct platform_device silan_timed_gpio =
{    
    .name    = "timed-gpio",
    .id        = -1,
    .dev     = {
            .platform_data = &timed_gpio_data,
    },
};

static struct resource slinner_codec_resource[] = 
{
    {
        .start    = SILAN_CODEC_PHY_BASE,
        .end      = SILAN_CODEC_PHY_BASE + SILAN_CODEC_SIZE,
        .flags    = IORESOURCE_MEM,
    },
	{
        .start    = SILAN_CR_PHY_BASE,
        .end      = SILAN_CR_PHY_BASE + SILAN_CR_SIZE,
        .flags    = IORESOURCE_MEM,
    },
	{
        .start  = PIC_IRQ_CODEC,
        .end    = PIC_IRQ_CODEC,
        .flags  = IORESOURCE_IRQ,
    }    

};

static struct platform_device slinner_codec_device = 
{
    .name             = "silan-inner",
    .id               = -1,
    .num_resources    = ARRAY_SIZE(slinner_codec_resource),
    .resource         = slinner_codec_resource,
};

static struct platform_device silan_lcd = 
{
	.name = "silan-lcd",
	.id = -1,
};

static struct platform_device silan_bt = 
{
	.name = "silan-bt",
	.id = -1,
};

static struct resource silan_adkey_resource[] =
{
    {
        .start  = SILAN_ADC_PHY_BASE,
        .end    = SILAN_ADC_PHY_BASE + SILAN_ADC_SIZE,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = PIC_IRQ_ADC,
        .end    = PIC_IRQ_ADC,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct platform_device silan_ad_keys =
{
	.name = "sl_ad-keys",
	.id = -1,
    .num_resources   = ARRAY_SIZE(silan_adkey_resource),
    .resource        = silan_adkey_resource,
};

static struct gpio_led gpio_leds[]={
	{
		.name = "wifi-led",
		.default_trigger = "timer",
		.gpio = 21,
		.active_low = 1,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	}
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds = gpio_leds,
	.num_leds = ARRAY_SIZE(gpio_leds),
};

static struct platform_device silan_leds = {
	.name = "leds-gpio",
	.id = -1,
	.dev = {
		.platform_data = &gpio_led_info,
	},

};

static struct platform_device *silan_devices[] __initdata = 
{
    &silan_uart_device,
    &silan_uart3_device,
    &silan_uart4_device,
//    &silan_phy_device,    
//    &silan_mac_device,    
    &silan_hostusb_device,
//    &silan_otgusb_device,
//    &silan_i2c_device,
    &silan_spi_device,
    &silan_spi_ctrl_device,
    &silan_dsp_device,
    &silan_mmc_device0,
    &silan_mmc_device1,
    &silan_gpio1_device,    
    &silan_gpio2_device,
#ifdef CONFIG_SND_SILAN_SOC
    &silan_soc_dma,
	&slinner_codec_device,
    &silan_iis_device,
    &silan_spdif_device,
    &silan_spdcodec_device,
    &silan_spdif_in_device,
    &silan_wmcodec_device,
#endif
//    &silan_testio_device,
    &silan_padmux_device,
    //&silan_pwm_device,
    //&silan_backlight,
//    &silan_i2c_gpio,    
//    &silan_timed_gpio,
    &silan_rtc_device,
    //&spi_silan_gpio_device,
    &silan_wtg_device,
    &silan_drm_device,
//    &silan_lcd,
//    &silan_ad_keys,
#ifdef CONFIG_SILAN_BLUETOOTH
    &silan_bt,
#endif
	//&silan_leds,
};

static int __init silan_add_devices(void)
{
    int err;
#if defined(CONFIG_SILAN_DLNA_DMA) || defined(CONFIG_SILAN_DSP_DMA)
    int i;
#endif
    err = platform_add_devices(silan_devices, ARRAY_SIZE(silan_devices));
    if (err)
        return err;
    
#if defined(CONFIG_SILAN_DLNA_DMA) || defined(CONFIG_SILAN_DSP_DMA)
    for(i = 0; i < ARRAY_SIZE(silan_amba_devs); i++) {
        struct amba_device *d = silan_amba_devs[i];
        amba_device_register(d, &iomem_resource);
    }
#endif

    return 0;
}

device_initcall(silan_add_devices);

