/* arch/csky/dioscuri/include/mach/irqs.h
 *
 * Copyright (c) 20010 Simtec Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_MACH_IRQS_H
#define __ASM_MACH_IRQS_H

#define DIOSCURI_GPIOA0_IRQ      0
#define DIOSCURI_GPIOA1_IRQ      1
#define DIOSCURI_GPIOA2_IRQ      2
#define DIOSCURI_GPIOA3_IRQ      3
#define DIOSCURI_GPIOA4_IRQ      4
#define DIOSCURI_GPIOA5_IRQ      5
#define DIOSCURI_GPIOA6_IRQ      6
#define DIOSCURI_GPIOA7_IRQ      7
#define DIOSCURI_GPIOA8_IRQ      8
#define DIOSCURI_GPIOA9_IRQ      9
#define DIOSCURI_GPIOA10_IRQ     10
#define DIOSCURI_USBD_IRQ        11
#define DIOSCURI_TIMER0_IRQ      12
#define DIOSCURI_TIMER1_IRQ      13
#define DIOSCURI_TIMER2_IRQ      14
#define DIOSCURI_TIMER3_IRQ      15
#define DIOSCURI_UART0_IRQ       16
#define DIOSCURI_UART1_IRQ       17
#define DIOSCURI_TIMER4_IRQ      18
#define DIOSCURI_TIMER5_IRQ      19
#define DIOSCURI_RTC_IRQ         20
#define DIOSCURI_SPI_IRQ         21
#define CSKY_IIC_IRQ         22
#define DIOSCURI_PWM_IRQ         23
#define DIOSCURI_WTD_IRQ         24
#define DIOSCURI_USBH_IRQ        25
#define DIOSCURI_MAC_IRQ         26
#define DIOSCURI_SDHC_IRQ        27
#define DIOSCURI_IIS_IRQ         28
#define CSKY_DMAC_IRQ        29
#define DIOSCURI_POWM_IRQ        30
#define DIOSCURI_NFC_IRQ         31
#define DIOSCURI_LCD_IRQ         31      // FIXME: no but need define

#define NR_IRQS              32
#define FIQ_START            32  /* first vector number of fast interrupt */ 
#define NR_FIQS              32

#endif   /* __ASM_MACH_IRQS_H */
