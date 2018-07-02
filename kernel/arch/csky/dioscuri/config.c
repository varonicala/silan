/*
 * linux/arch/csky/dioscuri/config.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009, Hu Junshan (junshan_hu@c-sky.com)
 * Copyright (C) 2012, zhang wenmeng (wenmeng_zhang@c-sky.com)
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <asm/machdep.h>
#include <asm/csky.h>
#include <asm/delay.h>
#include <asm/mmu_context.h>

#include <mach/fb.h>
#include "devices.h"

extern void csky_tick(void);
extern int csky_hwclk(int set, struct rtc_time *t);
extern void __init csky_timer_init(void);
extern unsigned long csky_timer_offset(void); 
extern void __init csky_init_IRQ(void); 
extern void __init ck_gpio_init(void);

extern void __init ck_i2c_devices_init(void);

extern unsigned int csky_get_auto_irqno(void); 
extern void __init dioscuri_set_serial_console(unsigned portnr); 
#ifdef CONFIG_CPU_USE_FIQ
extern void __init csky_init_FIQ(void); 
#endif

static struct platform_device *devices[] __initdata = {
	&csky_device_uart1,
	&csky_device_uart2,
	&csky_device_nand,
	&csky_device_usb,
	&csky_device_lcd,
	&csky_device_nor,
//	&csky_gpio_keys_device,
	&csky_device_sdhc,
	&csky_macphy_device,
	&csky_maceth_device,
	&csky_device_rtc,
//	&csky_device_i2c1,
	&csky_device_i2c2,
};

static int __init board_devices_init(void)
{
	csky_fb_set_platdata(&ck6408evb_fb_info); //configure mach_info for LCD
	platform_add_devices(devices, ARRAY_SIZE(devices));

	ck_i2c_devices_init();

	return 0;
}


void __init config_BSP(void)
{
	printk("C-SKY DIOSCURI Board port by zhang wenmeng,  wenmeng_zhang@c-sky.com\n");

	mach_time_init = csky_timer_init;
	mach_tick = csky_tick;
	mach_hwclk = csky_hwclk;
	mach_init_IRQ = csky_init_IRQ;
	mach_get_auto_irqno = csky_get_auto_irqno;
	mach_gettimeoffset = csky_timer_offset;
#ifdef CONFIG_CPU_USE_FIQ
	mach_init_FIQ = csky_init_FIQ;
#endif

	system_rev = 0x10;  // varsion 1.0
	dioscuri_set_serial_console(0);
	ck_gpio_init();
}

const char *get_machine_type(void)
{
	return "DIOSCURI";
}

arch_initcall(board_devices_init);
