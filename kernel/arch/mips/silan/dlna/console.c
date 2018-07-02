/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
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
 * Putting things on the screen/serial line using YAMONs facilities.
 */
#include <linux/types.h>
#include <linux/console.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/amba/serial.h>
#include <silan_generic.h>
#include <silan_resources.h>

#define PORT(offset) (SILAN_UART2_BASE + (offset))

static inline u32 serial_in(int offset)
{
	return sl_readl(PORT(offset));
}

static inline void serial_out(int offset, u32 value)
{
	sl_writel(value, PORT(offset));
}

int prom_putchar(char c)
{
    while(serial_in(UART01x_FR) & UART01x_FR_TXFF);

    serial_out(UART01x_DR, (u32)c);

    return 1;
}
