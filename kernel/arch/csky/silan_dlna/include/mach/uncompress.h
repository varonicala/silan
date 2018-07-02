/*
 *  arch/arm/mach-aaec2000/include/mach/uncompress.h
 *
 *  Copyright (c) 2005 Nicolas Bellido Y Ortega
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_UNCOMPRESS_H
#define __ASM_ARCH_UNCOMPRESS_H

#include <mach/silan_resources.h>

#define UART_THR ((volatile unsigned char *)(SILAN_UART2_BASE + 0x0))
#define UART_LSR ((volatile unsigned char *)(SILAN_UART2_BASE + 0x14))

#define LSR_THRE	0x20

//#include "ckuart.h"
void inline uart_init(void)
{
}

void inline putcu(unsigned char c)
{
	int i;

	for (i = 0; i < 0x1000; i++) {
		/* Transmit fifo not full? */
		if (*UART_LSR & LSR_THRE)
			break;
	}

	*UART_THR = c;

}
static inline void flush(void)
{
}


#define arch_decomp_setup()
#define arch_decomp_wdog()

#endif /* __ASM_ARCH_UNCOMPRESS_H */
