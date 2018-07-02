/* linux/arch/csky/dioscuri/devices.c
 *
 * Copyright (C) 2010 , Hu junshan<junshan_hu@c-sky.com>.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <mach/irqs.h>
#include "devices.h"
#include <mach/ck_iomap.h>
#include <mach/board.h>
#include <mach/ckuart.h>
#include <mach/fb.h>

#include <mach/ckgpio.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/i2c.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/partitions.h>
#include <linux/stmmac.h>
#include <linux/phy.h> 

static struct resource resources_uart1[] = {
	{
		.start  = DIOSCURI_UART0_IRQ,
		.end    = DIOSCURI_UART0_IRQ,
		.flags  = IORESOURCE_IRQ,
	},
	{
		.start  = DIOSCURI_UART0_PHYS,
		.end    = DIOSCURI_UART0_PHYS + DIOSCURI_UART_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct resource resources_uart2[] = {
	{
		.start  = DIOSCURI_UART1_IRQ,
		.end    = DIOSCURI_UART1_IRQ,
		.flags  = IORESOURCE_IRQ,
	},
	{
		.start  = DIOSCURI_UART1_PHYS,
		.end    = DIOSCURI_UART1_PHYS + DIOSCURI_UART_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

struct platform_device csky_device_uart1 = {
	.name   = "cskyuart",
	.id = 0,
	.num_resources  = ARRAY_SIZE(resources_uart1),
	.resource   = resources_uart1,
};

struct platform_device csky_device_uart2 = {
	.name   = "cskyuart",
	.id = 1,
	.num_resources  = ARRAY_SIZE(resources_uart2),
	.resource   = resources_uart2,
};

#ifdef CONFIG_SERIAL_CSKY_CONSOLE
static struct platform_device *__initdata dioscuri_uarts[CSKY_MAX_UART]= {
	&csky_device_uart1,
	&csky_device_uart2,
};
struct platform_device *csky_default_console_device;	/* the serial console device */
void __init dioscuri_set_serial_console(unsigned portnr)
{
	if (portnr < CSKY_MAX_UART)
		csky_default_console_device = dioscuri_uarts[portnr];
}
#else
void __init dioscuri_set_serial_console(unsigned portnr) {}
#endif

/* NAND parititon */
static struct mtd_partition default_nand_part[] = {
	[0] = {
		.name	= "Boot Load",
		.size	= 0x100000,
		.offset	= 0,
	},
	[1] = {
		.name	= "Kernel",
		.offset = 0x100000,
		.size	= 0x700000,
	},
	[2] = {
		.name	= "YAFFS2 FS",
		.offset = 0x800000,
		.size	= 0x2000000,
	},
	[3] = {
		.name	= "DIOSCURI flash partition 3",
		.offset	= 0x2800000,
		.size	= 0x1800000,
	},
	[4] = {
	    .name   = "DIOSCURI flash partition 4",
	    .offset = 0x4000000,
	    .size   = 0x1000000,
	},
	[5] = {
	    .name   = "DIOSCURI flash partition 5",
	    .offset = 0x5000000,
	    .size   = 0x1000000,
	},
	[6] = {
	    .name   = "DIOSCURI flash partition 6",
	    .offset = 0x6000000,
	    .size   = 0x1000000,
	}
};

static struct csky_nand_data csky_nand_pdata[] = {
	[0] = {
		.name		= "NAND",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(default_nand_part),
		.partitions	= default_nand_part,
	},
};

static struct resource resources_nand[] = {
	[0] = {
	      .start  = DIOSCURI_NFC_PHYS,
	      .end    = DIOSCURI_NFC_PHYS + DIOSCURI_NFC_SIZE - 1,
	      .flags  = IORESOURCE_MEM,
	},
	[1] = {
	      .start  = DIOSCURI_NFC_BUF0_PHYS,
	      .end    = DIOSCURI_NFC_BUF0_PHYS + DIOSCURI_NFC_BUF_SIZE - 1,
	      .flags  = IORESOURCE_MEM,
	},
};

struct platform_device csky_device_nand = {
	.name   = "csky_nand",
	.id = -1,
		.dev		= {
			.platform_data = &csky_nand_pdata,
		},
	.num_resources  = ARRAY_SIZE(resources_nand),
	.resource   = resources_nand,
};

EXPORT_SYMBOL(csky_device_nand);

/* SD Host Controller */

static struct resource csky_sdhc_resource[] = {
        [0] = {
            .start = DIOSCURI_SDHC_PHYS,
            .end   = DIOSCURI_SDHC_PHYS + DIOSCURI_SDHC_SIZE - 1,
            .flags = IORESOURCE_MEM,
        },
        [1] = {
            .start = DIOSCURI_SDHC_IRQ,
            .end   = DIOSCURI_SDHC_IRQ,
            .flags = IORESOURCE_IRQ,
        }
};

struct platform_device csky_device_sdhc = {
        .name          = "csky_mci",
        .id            = -1,
        .num_resources = ARRAY_SIZE(csky_sdhc_resource),
        .resource      = csky_sdhc_resource,
}; 
 
EXPORT_SYMBOL(csky_device_sdhc);

/* USB Host Controller */

static struct resource csky_usb_resource[] = {
	[0] = {
	    .start = DIOSCURI_USB_HOST_PHYS,
	    .end   = DIOSCURI_USB_HOST_PHYS + DIOSCURI_USB_HOST_SIZE - 1,
	    .flags = IORESOURCE_MEM,
	},
	[1] = {
	    .start = DIOSCURI_USBD_IRQ,
	    .end   = DIOSCURI_USBD_IRQ,
	    .flags = IORESOURCE_IRQ,
	}
};

static u64 csky_device_usb_dmamask = 0xffffffffUL;

struct platform_device csky_device_usb = {
	.name          = "csky-ohci",
	.id            = -1,
	.num_resources = ARRAY_SIZE(csky_usb_resource),
	.resource      = csky_usb_resource,
	.dev           = {
	    .dma_mask          = &csky_device_usb_dmamask,
	    .coherent_dma_mask = 0xffffffffUL
	}
};

EXPORT_SYMBOL(csky_device_usb);

/* LCD Controller */
static struct resource csky_lcd_resource[] = {
	[0] = {
	    .start = DIOSCURI_LCD_PHYS,
	    .end   = DIOSCURI_LCD_PHYS + DIOSCURI_LCD_SIZE - 1,
	    .flags = IORESOURCE_MEM,
	},
	[1] = {
	    .start = DIOSCURI_LCD_IRQ,
	    .end   = DIOSCURI_LCD_IRQ,
	    .flags = IORESOURCE_IRQ,
	}
};

static u64 csky_device_lcd_dmamask = 0xffffffffUL;

struct platform_device csky_device_lcd = {
	.name          = "csky-lcd",
	.id            = -1,
	.num_resources = ARRAY_SIZE(csky_lcd_resource),
	.resource      = csky_lcd_resource,
	.dev           = {
	    .dma_mask          = &csky_device_lcd_dmamask,
	    .coherent_dma_mask = 0xffffffffUL
	}
};
EXPORT_SYMBOL(csky_device_lcd);

struct csky_fb_hw ck6408evb_fb_regs = {
	.control = CSKY_LCDCON_OUT_24BIT | CSKY_LCDCON_VBL_RESERVED | \
		CSKY_LCDCON_PBS_16BITS | CSKY_LCDCON_PAS_TFT,
	.timing0 = (CSKY_HBP << 24) | (CSKY_HFP << 16) | \
		((CSKY_HLW & 0x3f) << 10) | CSKY_LCDTIM0_PPL_800,
	.timing1 = (CSKY_VBP << 24) | ((CSKY_VFP << 16)) | \
		((CSKY_VLW & 0x3f) << 10)| CSKY_LCDTIM1_LPP_480,
	.timing2 = CSKY_LCDTIM2_OEP_ACT_LOW | CSKY_LCDTIM2_HSP_ACT_LOW | \
		CSKY_LCDTIM2_VSP_ACT_LOW | CSKY_LCDTIM2_PCD_20,
};

struct csky_fb_display dioscuri_lcd_cfg __initdata = {
	.timing2 = CSKY_LCDTIM2_OEP_ACT_LOW | CSKY_LCDTIM2_HSP_ACT_LOW | \
		CSKY_LCDTIM2_VSP_ACT_LOW | CSKY_LCDTIM2_PCD_20,
	.type           = CSKY_LCDCON_PAS_TFT,

	.width          = 800,
	.height         = 480,
	
	.pixclock       = 500000, // HCLK 40 MHz, divisor 20
	.xres           = 800,
	.yres           = 480,
	.bpp            = 32,     // if 24, bpp 888 and 8 dummy
	.left_margin    = 0,
	.right_margin   = 0,
	.hsync_len      = 16,     // unit is pixel clock period
	.upper_margin   = 0,
	.lower_margin   = 2,      // unit is line clock period
	.vsync_len      = 2,
};

struct csky_fb_mach_info ck6408evb_fb_info __initdata = {
	.displays       = &dioscuri_lcd_cfg,
	.num_displays   = 1,
	.default_display = 0,
};

//------------lcdc pulse generation------------
void lcdc_pulse_gen(){
  volatile unsigned long *pwm_baddr;
   pwm_baddr = (volatile unsigned long*) (0xac018000);
   //mask all interrupt
   pwm_baddr[7] = 0x0;
   //data source ODR, mono voice, pwm0's width is 16
   pwm_baddr[1] = 0xaa4c;
   //pclk/2*240 for pwm period
   pwm_baddr[2] = 0x00fa00fa;
   //store the data into ODR
   pwm_baddr[3] = 0x007d007d; //the bar:pulse = 1:1;
   //enable the pwm
   pwm_baddr[0] = 0x3;
}

/* Set platform data(csky_fb_mach_info) for csky_device_lcd */
void __init csky_fb_set_platdata(struct csky_fb_mach_info *pd)
{
	struct csky_fb_mach_info *npd;

	npd = kmemdup(pd, sizeof(struct csky_fb_mach_info), GFP_KERNEL);
	if (npd) {
		csky_device_lcd.dev.platform_data = npd;
        npd->displays = kmemdup(pd->displays,
            sizeof(struct csky_fb_display) * npd->num_displays, GFP_KERNEL);
        if (!npd->displays)
            printk(KERN_ERR "no memory for LCD display data\n");
	} else {
		printk(KERN_ERR "no memory for LCD platform data\n");
	}
	lcdc_pulse_gen();
}

static struct mtd_partition csky_partitions[] = {
        {
            .name = "Bootloader",
            .offset = 0,
            .size = 0x080000
        },
        {
            .name = "kernel",
            .offset = 0x080000,
            .size = 0x0280000,
        },
        {
            .name = "fs",
            .offset = 0x300000,
            .size = 0x500000
        }
};

static struct physmap_flash_data csky_nor_pdata = {
        .width          = 4,
	.parts          = csky_partitions,
        .nr_parts       = ARRAY_SIZE(csky_partitions),
};

static struct resource csky_nor_resource[] = {
        [0] = {
                .start = 0x0, 
                .end   = 0x1000000 - 1,
                .flags = IORESOURCE_MEM,
        }
};

struct platform_device csky_device_nor = {
        .name           = "physmap-flash",
        .id             = 0,
        .num_resources  = ARRAY_SIZE(csky_nor_resource),
        .resource       = csky_nor_resource,
        .dev            = {
                .platform_data = &csky_nor_pdata,
        },
};

EXPORT_SYMBOL(csky_device_nor);

static struct gpio_keys_button csky_gpio_keys[] = {
	{
		.gpio	= CK_GPIOA4,
		.code	= KEY_1,
		.desc	= "key4",
		.type	= EV_KEY,
		.active_low	= 1,
		.wakeup	=1,
		.debounce_interval	= 1,
		.can_disable	= 1,
	}
};

static struct gpio_keys_platform_data csky_gpio_keys_data = {
	.buttons  = csky_gpio_keys,
	.nbuttons = 1,
};

struct platform_device csky_gpio_keys_device = {
	.name  = "gpio-keys",
	.id    = -1,
	.dev   = {
		.platform_data  = &csky_gpio_keys_data,
	}
};

static struct plat_stmmacphy_data csky_mac_phy_data = {
        .bus_id = 0,
        .phy_addr = 1,
        .phy_mask = 0,
        .interface = PHY_INTERFACE_MODE_MII,
};

struct platform_device csky_macphy_device = {
        .name  = "stmmacphy",
        .id    = 0,
//      .num_resources = ARRAY_SIZE(csky_mac_phy_resource),
//      .resource      = csky_mac_phy_resource,
        .dev   = {
                .platform_data  = &csky_mac_phy_data,
        }
};

static struct plat_stmmacenet_data csky_mac_eth_data = {
        .bus_id = 0,
        .has_gmac = 1,
        .enh_desc = 0,
        .pbl = 32,
//      .fix_mac_speed = fix_mac_speed,
//      .init = stmmacinit,
};

static struct resource csky_mac_eth_resource[] = {
        [0] = {
            .start = DIOSCURI_MACC_PHYS,
            .end   = DIOSCURI_MACC_PHYS + DIOSCURI_MACC_SIZE - 1,
            .flags = IORESOURCE_MEM,
        },
        [1] = {
            .start = DIOSCURI_MAC_IRQ,
            .end   = DIOSCURI_MAC_IRQ,
            .flags = IORESOURCE_IRQ,
            .name = "macirq",
        }
};

struct platform_device csky_maceth_device = {
        .name  = "stmmaceth",
        .id    = 0,
        .num_resources = ARRAY_SIZE(csky_mac_eth_resource),
        .resource      = csky_mac_eth_resource,
        .dev   = {
                .platform_data  = &csky_mac_eth_data,
        }
};

/* RTC */
static struct resource csky_rtc_resource[] = {
        [0] = {
                .start  = DIOSCURI_RTC_PHYS,
                .end    = DIOSCURI_RTC_PHYS + DIOSCURI_RTC_SIZE - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = DIOSCURI_RTC_IRQ,
                .end    = DIOSCURI_RTC_IRQ,
                .flags  = IORESOURCE_IRQ,
        }
};

struct platform_device csky_device_rtc = {
        .name           = "ck-rtc",
        .id             = -1,
        .num_resources  = ARRAY_SIZE(csky_rtc_resource),
        .resource       = csky_rtc_resource,
};
EXPORT_SYMBOL(csky_device_rtc);
/* i2c */
/*
static struct resource csky_i2c_resources1[] = {
	[0] = {
		.start  = DIOSCURI_IIC_PHYS,
		.end    = DIOSCURI_IIC_PHYS + DIOSCURI_IIC_SIZE -1,
		.flags  = IORESOURCE_MEM,
	}
};
*/
static struct resource csky_i2c_resources2[] = {
        [0] = {
                .start  = DIOSCURI_IIC1_PHYS,
                .end    = DIOSCURI_IIC1_PHYS + DIOSCURI_IIC1_SIZE -1,
                .flags  = IORESOURCE_MEM,
        }
};
/*
struct platform_device csky_device_i2c1 = {
        .name           = "ck-i2c",
        .id             = 0,
        .num_resources  = ARRAY_SIZE(csky_i2c_resources1),
        .resource       = csky_i2c_resources1,
};
EXPORT_SYMBOL(csky_device_i2c1);
*/


struct platform_device csky_device_i2c2 = {
        .name           = "ck-i2c",
        .id             = 1,
        .num_resources  = ARRAY_SIZE(csky_i2c_resources2),
        .resource       = csky_i2c_resources2,
};
EXPORT_SYMBOL(csky_device_i2c2);

/* i2c devices */
/*
static const unsigned int ck_keypadmap[] = {
        KEY_ESC, KEY_1, KEY_2, KEY_3,
        KEY_4, KEY_5, KEY_6, KEY_7,
        KEY_8, KEY_9, KEY_0, KEY_MINUS,
        KEY_EQUAL, KEY_BACKSPACE, KEY_TAB, KEY_Q,
};
static struct csky_keypad_platform_data ck_keypad_data = {
        .keycodemax     = 16,
        .keypadmap      = ck_keypadmap,
};
*/

static struct gt801_platform_data gt801_info = {
        .model                  = 801,
        .swap_xy                = 0,
        .x_min                  = 0,
        .x_max                  = 480,
        .y_min                  = 0,
        .y_max                  = 800,
//        .gpio_reset     = GT801_GPIO_RESET,
//        .gpio_reset_active_low = 0,
//        .gpio_pendown           = GT801_GPIO_INT,
//    .pendown_iomux_name = GPIO4D5_CPUTRACECTL_NAME,
//    .resetpin_iomux_name = NULL,
//    .pendown_iomux_mode = GPIO4H_GPIO4D5,
//    .resetpin_iomux_mode = 0,
};
/*
struct i2c_board_info ck_i2c_devices1[] __initdata = {
        {
                I2C_BOARD_INFO("ck-keypad", 0x38),
                .platform_data  = &ck_keypad_data,
                .irq            = DIOSCURI_GPIOA0_IRQ,
                .flags          = I2C_CLIENT_WAKE,
        },
};
*/
struct i2c_board_info ck_i2c_devices2[] __initdata = {
	{
		I2C_BOARD_INFO("gt801_ts", 0x55),
		.platform_data	= &gt801_info,
		.irq    = DIOSCURI_GPIOA8_IRQ,
		.flags  = 0,
	}
};

void __init ck_i2c_devices_init(void)
{
//	i2c_register_board_info(0, ck_i2c_devices1, ARRAY_SIZE(ck_i2c_devices1));
	i2c_register_board_info(0, ck_i2c_devices2, ARRAY_SIZE(ck_i2c_devices2));

}

