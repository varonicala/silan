/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
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
 *
 * Reset the MIPS boards.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/reboot.h>
#include <silan_resources.h>
#include <silan_generic.h>
#include <silan_reset.h>

#define SILAN_SYS_REG12		(SILAN_CR_BASE+0x18)

static void mips_machine_restart(char *command);
static void mips_machine_halt(void);

int silan_module_rst(RSTMOD module)
{
	unsigned int value = 0;

	if(module < SILAN_SR_START || module >= SILAN_SR_END)
		return -1;

	value = sl_readl(SILAN_SYS_REG12);

	value &= ~(1 << (module % 32));
	
	sl_writel(value, SILAN_SYS_REG12);

	value |= (1 << (module % 32));

	sl_writel(value, SILAN_SYS_REG12);

	return 0;
}
EXPORT_SYMBOL(silan_module_rst);

static void mips_machine_restart(char *command)
{
	printk ( KERN_NOTICE "\n** Stop vpe1 if exists here\n");
	printk ( KERN_NOTICE "\n** Close all Integrated Peripherals here\n");

	set_c0_status(ST0_BEV | ST0_ERL);
	set_c0_config(CONF_CM_UNCACHED);

	//silan_module_rst(SILAN_SR_G);

	/* Jump to the beggining in case board_reset() is empty
	__asm__ __volatile__("jr\t%0"::"r"(0xbfc00000));
	*/
}

static void mips_machine_halt(void)
{
	local_irq_disable();
	clear_c0_status(ST0_IM);
	printk ( KERN_NOTICE "\n** Stop vpe1 if exists here\n");
	printk ( KERN_NOTICE "\n** Close all Integrated Peripherals here\n");
	printk(KERN_NOTICE "\n** You can safely turn off the power\n");
	while(1);
}

void mips_reboot_setup(void)
{
	_machine_restart = mips_machine_restart;
	_machine_halt = mips_machine_halt;
	pm_power_off = mips_machine_halt;
}
