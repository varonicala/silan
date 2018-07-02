/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  (C) Copyright 2011, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
 *  Author: Hu junshan <junshan_hu@c-sky.com>
 *  
 */

#ifndef __ASM_CSKY_IRQFLAGS_H
#define __ASM_CSKY_IRQFLAGS_H

#include <asm/entry.h>

#ifdef __KERNEL__

/*
 * CPU interrupt mask handling.
 */
static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;
	asm volatile(
		"mfcr   %0, psr	\n"
		"psrclr ie      \n"
		:"=r"(flags): :"memory");
	return flags;
}

static inline void arch_local_irq_enable(void)
{
	asm volatile(
		"psrset ee, ie \n"
		: : :"memory" );
}

static inline void arch_local_irq_disable(void)
{
	asm volatile(
		"psrclr ie     \n"
		: : :"memory");
}

/*
 * Save the current interrupt enable state.
 */
static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;
	asm volatile(
		"mfcr   %0, psr	 \n"
		:"=r"(flags) : :"memory" );
	return flags;
}

/*
 * restore saved IRQ state
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
	asm volatile(
		"mtcr    %0, psr  \n"
		: :"r" (flags) :"memory" );
}

static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return !((flags) & ALLOWINT); 
}

#endif
#endif
