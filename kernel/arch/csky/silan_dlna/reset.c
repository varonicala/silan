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
#include <asm/csky.h>
#include <asm/delay.h>
#include <asm/mmu_context.h>
#include <asm/io.h>
#include <silan_resources.h>
#include <silan_regs.h>
#include <silan_generic.h>
#include <silan_reset.h>

int silan_module_rst(RSTMOD module)
{
	unsigned int value = 0;

	if(module < SILAN_SR_START || module >= SILAN_SR_END)
		return -1;

	if(module < 32)
		value = sl_readl(SILAN_SYS_REG12);
	else if(module < 64)
		value = sl_readl(SILAN_SYS_REG13);
	else if(module < 96)
		value = sl_readl(SILAN_SYS_REG14);
	else
		value = sl_readl(SILAN_SYS_REG15);

	value &= ~(1 << (module % 32));
	
	if(module < 32)
		sl_writel(value, SILAN_SYS_REG12);
	else if(module < 64)
		sl_writel(value, SILAN_SYS_REG13);
	else if(module < 96)
		sl_writel(value, SILAN_SYS_REG14);
	else
		sl_writel(value, SILAN_SYS_REG15);

	value |= (1 << (module % 32));

	if(module < 32)
		sl_writel(value, SILAN_SYS_REG12);
	else if(module < 64)
		sl_writel(value, SILAN_SYS_REG13);
	else if(module < 96)
		sl_writel(value, SILAN_SYS_REG14);
	else
		sl_writel(value, SILAN_SYS_REG15);

	return 0;
}
EXPORT_SYMBOL(silan_module_rst);

void csky_machine_restart(void)
{

	printk("csky_machine_restart \n");

	//watchdog
	//writel(100000000, 0xba0d1000);
	//writel(3, 0xba0d1008);

	//software reset
	writel(1, 0xba0d2000);
	
	//silan_module_rst(SILAN_SR_G);

	/* Jump to the beggining in case board_reset() is empty
	__asm__ __volatile__("jr\t%0"::"r"(0xbfc00000));
	*/
}

void csky_machine_halt(void)
{
	local_irq_disable();
	while(1);
}

