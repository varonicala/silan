/* arch/csky/dioscuri/include/mach/ck_iomap.h
 *
 * Copyright (C) 2010 C-SKY Microsystems Co., Ltd.
 * Author: Hu junshan <junshan_hu@c-sky.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * The MSM peripherals are spread all over across 768MB of physical
 * space, which makes just having a simple IO_ADDRESS macro to slide
 * them into the right virtual location rough.  Instead, we will
 * provide a master phys->virt mapping for peripherals here.
 *
 */

#ifndef __ASM_ARCH_DIOSCURI_IOMAP_H
#define __ASM_ARCH_DIOSCURI_IOMAP_H

/* AHB arbiter */
#define DIOSCURI_AHB_PHYS           0x0C000000   /* Base address of AHB */
#define DIOSCURI_AHB_SIZE           0x1000       /* AHB Reg size is 4k */

/* MMCA */
#define DIOSCURI_MMCA_PHYS          0x0C001000   /* Base address of MMCA */
#define DIOSCURI_MMCA_SIZE          0x2000       /* MMCA Reg size is 4k */

/* Power Management */
#define DIOSCURI_PM_PHYS            0x0C002000
#define DIOSCURI_PM_SIZE            0x1000

/* DMA controller */
#define CSKY_DMA_PHYS           0x0C003000
#define CSKY_DMA_SIZE           0x1000

/* MAC controller -registers */
#define DIOSCURI_MACC_PHYS          0x0C006000
#define DIOSCURI_MACC_SIZE          0x2000

/* NFC controller */
#define DIOSCURI_NFC_PHYS           0x0C00A000   
#define DIOSCURI_NFC_SIZE           0x1000      

#define DIOSCURI_NFC_BUF0_PHYS      0x0C008000   /* Base address of NFC BUF1 */
#define DIOSCURI_NFC_BUF1_PHYS      0x0C009000   /* Base address of NFC BUF2 */ 
#define DIOSCURI_NFC_BUF_SIZE       0x1000       /* NFC BUF size is 4k */

/* USB host controller */
#define DIOSCURI_USB_HOST_PHYS      0x0C00C000
#define DIOSCURI_USB_HOST_SIZE      0x1000

/* Interrupt controller*/
#define DIOSCURI_INT_PHYS           0x0C013000
#define DIOSCURI_INT_SIZE           0x1000

/* Timer */
#define DIOSCURI_TIMER_PHYS         0x0C014000
#define DIOSCURI_TIMER_SIZE         0x1000
 
/* Real Time clock */
#define DIOSCURI_RTC_PHYS           0x0C01C000
#define DIOSCURI_RTC_SIZE           0x1000

/* Watchdog */
#define DIOSCURI_WTD_PHYS           0x0C015000
#define DIOSCURI_WTD_SIZE           0x1000

/* PWM */
#define DIOSCURI_PWM_PHYS           0x0C018000
#define DIOSCURI_PWM_SIZE           0x1000

/* UART */
#define DIOSCURI_UART0_PHYS         0x0C016000   /* Base address of UART1 */
#define DIOSCURI_UART1_PHYS         0x0C017000   /* Base address of UART2 */
#define DIOSCURI_UART_SIZE          0x1000       /* UART Reg size is 4k */

/* I2C bus controller */
#define DIOSCURI_IIC_PHYS           0x0C01B000
#define DIOSCURI_IIC_SIZE           0x1000

/* GPIO controller */
#define DIOSCURI_GPIO_PHYS          0x0C019000
#define DIOSCURI_GPIO_SIZE          0x1000 

/* SPI Maste */
#define DIOSCURI_SPI_PHYS           0x0C01A000
#define DIOSCURI_SPI_SIZE           0x1000 

/* SDHC */
#define DIOSCURI_SDHC_PHYS          0x0C00B000
#define DIOSCURI_SDHC_SIZE          0x1000

/* I2C bus controller1 */
#define DIOSCURI_IIC1_PHYS           0x0C01E000
#define DIOSCURI_IIC1_SIZE           0x1000

/* IIS */
#define DIOSCURI_IIS_PHYS           0x0C01F000
#define DIOSCURI_IIS_SIZE           0x1000

/* LCD */
#define DIOSCURI_LCD_PHYS           0x0C00E000
#define DIOSCURI_LCD_SIZE           0x1000

#endif /* __ASM_ARCH_DIOSCURI_IOMAP_H */
