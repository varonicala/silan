/*
 * arch/csky/dioscuri/include/mach/board.h
 *
 *  Copyright (C) 2010 Hu Junshan<junshan_hu@c-sky.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * These are data structures found in platform_device.dev.platform_data,
 * and describing board-specific data needed by drivers.  
 */

#ifndef __ASM_ARCH_BOARD_H
#define __ASM_ARCH_BOARD_H

#include <linux/mtd/partitions.h>


/* NAND / SmartMedia */
struct csky_nand_data {
//	int     enable_pin; /* chip enable */
//	int     det_pin;    /* card detect */
//	int     rdy_pin;    /* ready/busy */
//	u8      rdy_pin_active_low; /* rdy_pin value is inverted */
//	u8      ale;        /* address line number connected to ALE */
//	u8      cle;        /* address line number connected to CLE */
//	u8      bus_width_16;   /* buswidth is 16 bit */
	int     nr_chips;
	int     nr_partitions;
	char    *name;
	struct  mtd_partition *partitions;
};

/* cskymci platform config */
/* SDHC */
struct csky_mmc_data {
        unsigned int            card_present;
        unsigned int            no_wp;                  /* (SD) writeprotect detect */
        unsigned int            no_detect;              /* power switching (high == on) */
        unsigned int            use_dma;                /* use dma */
};


/* keypad */

struct csky_keypad_platform_data
{
        unsigned int *keypadmap;
        unsigned int keycodemax;
        int irq;
};

struct gt801_platform_data {

        u16             model;                  /* 801. */
        bool    swap_xy;                /* swap x and y axes */
        u16             x_min, x_max;
        u16             y_min, y_max;
    int         gpio_reset;
    int     gpio_reset_active_low;
        int             gpio_pendown;           /* the GPIO used to decide the pendown */

//        char    pendown_iomux_name[IOMUX_NAME_SIZE];
//        char    resetpin_iomux_name[IOMUX_NAME_SIZE];
        int             pendown_iomux_mode;
        int             resetpin_iomux_mode;

        int         (*get_pendown_state)(void);
};


#endif /* __ASM_ARCH_BOARD_H */

