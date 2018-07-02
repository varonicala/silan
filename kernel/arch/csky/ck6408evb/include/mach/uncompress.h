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

#include "ckuart.h"
void inline uart_init(void)
{
        int divisor;
        volatile unsigned int *addr= (unsigned int *)CSKY_UART_BASE0;
	divisor = ((CONFIG_APB_FREQ/CONFIG_SERIAL_CSKY_BAUDRATE) >> 4);
	addr[CSKY_UART_LCR] |= CSKY_UART_LCR_DLAEN;
	addr[CSKY_UART_DLL] = divisor & 0xff;
	addr[CSKY_UART_DLH] = (divisor >> 8) & 0xff;
	addr[CSKY_UART_LCR] &= (~CSKY_UART_LCR_DLAEN);
	addr[CSKY_UART_LCR] &= (~CSKY_UART_LCR_PEN);
	addr[CSKY_UART_LCR] &= 0xfb;
	addr[CSKY_UART_LCR]|= CSKY_UART_LCR_WLEN8;
	addr[CSKY_UART_IER] &= (~CSKY_UART_IER_ERDAI);
	addr[CSKY_UART_IER] &= (~CSKY_UART_IER_ETHEI);
}

void inline putcu(unsigned char c)
{
        volatile unsigned int *addr= (unsigned int *)CSKY_UART_BASE0;
	// wait for space in the UART's transmitter
	while ((addr[CSKY_UART_USR] & 0x1))
		barrier();

	// send the character out. 
	addr[CSKY_UART_DLL] = c;
}
static inline void flush(void)
{
}

#define arch_decomp_setup()
#define arch_decomp_wdog()

#endif /* __ASM_ARCH_UNCOMPRESS_H */
