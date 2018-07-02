/*
 * linux/arch/csky/ck6408evb/config.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1999-2002, Greg Ungerer (gerg@snapgear.com)
 * Copyright (C) 2000, Lineo (www.lineo.com)
 * Copyright (C) 2004, Kang Sun (sunk@vlsi.zju.edu.cn)
 * Copyright (C) 2004, Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009, Hu Junshan (junshan_hu@c-sky.com)
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

extern void csky_tick(void);
extern int csky_hwclk(int set, struct rtc_time *t);
extern void __init csky_timer_init(void);
extern unsigned long csky_timer_offset(void); 
extern void __init csky_init_IRQ(void); 
extern void csky_machine_restart(void);
extern void csky_machine_halt(void);
void __init prom_meminit(void);

extern unsigned int csky_get_auto_irqno(void); 
#ifdef CONFIG_CPU_USE_FIQ
extern void __init csky_init_FIQ(void); 
#endif

void __init config_BSP(void)
{
	printk("C-SKY SILAN_DLNA Board port by panjianguang\n");

	mach_time_init = csky_timer_init;
	mach_init_IRQ = csky_init_IRQ;
	mach_get_auto_irqno = csky_get_auto_irqno;
	mach_reset = csky_machine_restart;
#ifndef CONFIG_GENERIC_CLOCKEVENTS
	mach_tick = csky_tick;
	mach_hwclk = csky_hwclk;
	mach_gettimeoffset = csky_timer_offset;
#endif
#ifdef CONFIG_CPU_USE_FIQ
	mach_init_FIQ = csky_init_FIQ;
#endif
	
    system_rev = 0x09;  // varsion 0.9
	
    prom_meminit();
}

const char *get_machine_type(void)
{
	return "DLNA";
}
