/*
 * Setting up the clock on MSP SOCs.  No RTC typically.
 *
 * Wang Zheng, wangzheng@silan.com.cn
 * Copyright (C) 1999,2010 Silan.  All rights reserved.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/ptrace.h>

#include <asm/mipsregs.h>
#include <asm/time.h>
#include <silan_resources.h>
#include <silan_def.h>


static int mips_cpu_timer_irq;
static int mips_cpu_perf_irq;

static void mips_timer_dispatch(void)
{
	do_IRQ(mips_cpu_timer_irq);
}

static void mips_perf_dispatch(void)
{
	do_IRQ(mips_cpu_perf_irq);
}

static void __init plat_perf_setup(void)
{
	if (cp0_perfcount_irq >= 0) {
		if (cpu_has_vint)
			set_vi_handler(cp0_perfcount_irq, mips_perf_dispatch);
		mips_cpu_perf_irq = MIPS_CPU_IRQ_BASE + cp0_perfcount_irq;
#ifdef CONFIG_SMP
		irq_set_handler(mips_cpu_perf_irq, handle_percpu_irq);
#endif
	}
}

unsigned __cpuinit get_c0_compare_int(void)
{
	if (cpu_has_vint)
		set_vi_handler(cp0_compare_irq, mips_timer_dispatch);

	mips_cpu_timer_irq = MIPS_CPU_IRQ_BASE + cp0_compare_irq;
    
	return mips_cpu_timer_irq;
}

void __init plat_time_init(void)
{
	ulong cpu_speed;

	cpu_speed	= get_silan_pllclk();
	
	mips_hpt_frequency = cpu_speed / 2;
	
	plat_perf_setup();
}
