/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 * Copyright (C) 2008 Dmitri Vorobiev
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
 */
#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include <linux/screen_info.h>
#include <linux/time.h>

#include <asm/dma.h>
#include <asm/traps.h>
#ifdef CONFIG_VT
#include <linux/console.h>
#endif
#include <silan_def.h>

#define SILAN_SYS_TYPE_LEN	64

#define AR71XX_BASE_FREQ	40000000
#define AR724X_BASE_FREQ	5000000
#define AR913X_BASE_FREQ	5000000

extern void __init silan_clocks_init(void);

static char silan_sys_type[SILAN_SYS_TYPE_LEN];

static void __init silan_detect_sys_type(void)
{
	char *chip = "?????";
    char *dram = "????";

	u32 id;
	u32 major;
	u32 rev = 0;

	id = 3;
	major = id & 0x0003;

	switch (major) {
	case 0:
		chip = "8836C";
        dram = "8MB ";
        break;

	case 1:
		chip = "6138C";
        dram = "8MB ";
		break;

	case 2:
		chip = "8836A";
        dram = "32MB";
		break;

	case 3:
		chip = "6138A";
        dram = "32MB";
		break;
	}

	sprintf(silan_sys_type, "Silan SC%s @ %s rev %u", chip, dram, rev);
	pr_info("SoC: %s\n", silan_sys_type);
}

const char *get_system_type(void)
{
    return silan_sys_type;
}

void __init plat_chip_setup(void)
{
    silan_detect_sys_type();
    silan_clocks_init();
}

