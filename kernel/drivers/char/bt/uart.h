#ifndef __UART_H__
#define __UART_H__
//#include "chip_regs.h"
#define UART0_BASE_ADDR		0xBE000000
/*
 *  UART Register Offsets.
 */
#define UART_DR				0x00	/* Data read or written from the interface. */
#define UART_RSR			0x04	/* Receive status register (Read). */
#define UART_ECR			0x04	/* Error clear register (Write). */
#define UART_FR				0x18	/* Flag register (Read only). */
#define UART_ILPR			0x20	/* IrDA low power counter register. */
#define UART_IBRD			0x24	/* Integer baud rate divisor register. */
#define UART_FBRD			0x28	/* Fractional baud rate divisor register. */
#define UART_LCRH			0x2c	/* Line control register. */
#define UART_CR				0x30	/* Control register. */
#define UART_IFLS			0x34	/* Interrupt fifo level select. */
#define UART_IMSC			0x38	/* Interrupt mask. */
#define UART_RIS			0x3c	/* Raw interrupt status. */
#define UART_MIS			0x40	/* Masked interrupt status. */
#define UART_ICR			0x44	/* Interrupt clear register. */
#define UART_DMACR			0x48	/* DMA control register. */

#define URAT0_DR			__REG(UART0_BASE_ADDR + UART_DR)
#define UART0_RSR			__REG(UART0_BASE_ADDR + 0x04)
#define UART0_ECR			__REG(UART0_BASE_ADDR + 0x04)
#define UART0_FR			__REG(UART0_BASE_ADDR + 0x18)
#define UART0_ILPR			__REG(UART0_BASE_ADDR + 0x20)
#define UART0_IBRD			__REG(UART0_BASE_ADDR + 0x24)
#define UART0_FBRD			__REG(UART0_BASE_ADDR + 0x28)
#define UART0_LCRH			__REG(UART0_BASE_ADDR + 0x2c)
#define UART0_CR			__REG(UART0_BASE_ADDR + 0x30)
#define UART0_IFLS			__REG(UART0_BASE_ADDR + 0x34)
#define UART0_IMSC			__REG(UART0_BASE_ADDR + 0x38)
#define UART0_RIS			__REG(UART0_BASE_ADDR + 0x3c)
#define UART0_MIS			__REG(UART0_BASE_ADDR + 0x40)
#define UART0_ICR			__REG(UART0_BASE_ADDR + 0x44)
#define UART0_DMACR		__REG(UART0_BASE_ADDR + 0x48)



#define UART1_DR			__REG(UART1_BASE_ADDR + UART_DR)
#define UART1_RSR			__REG(UART1_BASE_ADDR + 0x04)
#define UART1_ECR			__REG(UART1_BASE_ADDR + 0x04)
#define UART1_FR			__REG(UART1_BASE_ADDR + 0x18)
#define UART1_ILPR			__REG(UART1_BASE_ADDR + 0x20)
#define UART1_IBRD			__REG(UART1_BASE_ADDR + 0x24)
#define UART1_FBRD			__REG(UART1_BASE_ADDR + 0x28)
#define UART1_LCRH			__REG(UART1_BASE_ADDR + 0x2c)
#define UART1_CR			__REG(UART1_BASE_ADDR + 0x30)
#define UART1_IFLS			__REG(UART1_BASE_ADDR + 0x34)
#define UART1_IMSC			__REG(UART1_BASE_ADDR + 0x38)
#define UART1_RIS			__REG(UART1_BASE_ADDR + 0x3c)
#define UART1_MIS			__REG(UART1_BASE_ADDR + 0x40)
#define UART1_ICR			__REG(UART1_BASE_ADDR + 0x44)
#define UART1_DMACR		__REG(UART1_BASE_ADDR + 0x48)

#endif /*__UART_H__*/
