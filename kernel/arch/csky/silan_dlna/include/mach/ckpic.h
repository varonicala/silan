/*
 * ckpic.h -- Programmable Interrupt Controller support.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Hu Junshan (junshan_hu@c-sky.com)
 */

#ifndef	_CSKY_PIC_H_
#define	_CSKY_PIC_H_

#include <asm/addrspace.h>
#include <mach/silan_resources.h>

/*
 *	Define the base address of the C-SKY interrupt controller
 */
#ifdef CONFIG_MMU
#define CKPIC_BASE       KSEG1ADDR(SILAN_INTL_PHY_BASE)
#else
#define CKPIC_BASE       SILAN_INTL_PHY_BASE
#endif

/*
 *	Define the offset(Index) of the Programmable Interrupt Controller registers
 */
#define IRQ_INTEN		0x0
#define IRQ_INTMASK		0x8
#define IRQ_INTFORCE	0x10
#define IRQ_RAWSTATUS	0x18
#define IRQ_STATUS		0x20
#define IRQ_MASKSTATUS	0x28
#define IRQ_FINALSTATUS	0x30

/*
 *  Bit Definition for the PIC Interrupt control register
 */
#define CKPIC_ICR_AVE    0x80000000 // F/N auto-interrupt requests enable
#define CKPIC_ICR_FVE    0x40000000 // F interrupt request to have vector number
#define CKPIC_ICR_ME     0x20000000 // Interrupt masking enable
#define	CKPIC_ICR_MFI	 0x10000000	// Masking of fast interrupt rquests enable

/*
 * Bit definition for the PIC Normal Interrup Enable register
 */
#define CKPIC_NIER_NIE(x)   (1 << x)   // Prioity=x normal interrupt enable bit

/*
 * Bit definition for the PIC Fast Interrup Enable register
 */
#define CKPIC_FIER_FIE(x)   (1 << x)   // Prioity=x Fast interrupt enable bit

#define csky_irq_wake NULL

#endif /* _CSKY_PIC_H_ */
