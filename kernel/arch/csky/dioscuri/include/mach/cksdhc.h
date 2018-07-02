/* arch/csky/dioscuri/include/mach/cksdhc.h
 *
 * Copyright (c) 2012 zhang wenmeng <wenmeng_zhang@c-sky.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#ifndef _DIOSCURI_CKSDHC_H_
#define _DIOSCURI_CKSDHC_H_

/* SDHC register memory map */
#define	SDHC_CONTROL			(0x00)	// control
#define SDHC_POWER_ENABLE		(0x04)	// power enable
#define SDHC_CLOCK_DIVIDER		(0x08)	// clock divider
#define SDHC_CLOCK_SOURCE		(0x0c)	// SD clock source
#define SDHC_CLOCK_ENABLE		(0x10)	// clock enable
#define SDHC_TIMEOUT			(0x14)	// timeout
#define SDHC_CARD_TYPE			(0x18)	// card type
#define SDHC_BLOCK_SIZE			(0x1c)	// block size
#define SDHC_BYTE_COUNT			(0x20)	// byte count
#define SDHC_INT_MASK			(0x24)	// interrupt mask
#define SDHC_ARGUMENT			(0x28)	// command argument
#define SDHC_COMMAND			(0x2c)	// command
#define SDHC_RESPONSE_0			(0x30)	// response 0
#define SDHC_RESPONSE_1			(0x34)	// response 1
#define SDHC_RESPONSE_2			(0x38)	// response 2
#define SDHC_RESPONSE_3			(0x3c)	// response 3
#define SDHC_MASKED_INT_STATUS		(0x40)	// masked interrupt status
#define SDHC_RAW_INT_STATUS		(0x44)	// raw interrupt status
#define SDHC_STATUS			(0x48)	// status
#define SDHC_FIFO_WATERMARK		(0x4c)	// FIFO threshold watermark
#define SDHC_CARD_DETECT		(0x50)	// card detect
#define SDHC_WRITE_PROTECT		(0x54)	// write protect
#define SDHC_IO_REG			(0x58)	// general purpose input/output register
#define SDHC_CIU_BYTE_COUNT		(0x5c)	// transferred CIU card byte count
#define SDHC_BIU_BYTE_COUNT		(0x60)	// transferred host to BIU-FIFO byte count
#define SDHC_DEBOUNCE_COUNT		(0x64)	// debounce count
#define SDHC_USER_ID			(0x68)	// user ID
#define SDHC_VERSION_ID			(0x6c)	// version ID
#define SDHC_HARDWARE_CONFIG		(0x70)	// hardware configuration
#define SDHC_BUS_MODE			(0x80)	// bus mode
#define SDHC_POLL_DEMAND		(0x84)	// poll demand
#define SDHC_LIST_BASE_ADDR		(0x88)	// descriptor list base address
#define SDHC_IDMAC_STATUS		(0x8c)	// internal DMAC status
#define SDHC_IDMAC_INT_ENABLE		(0x90)	// internal DMAC interrupt enable
#define SDHC_HOST_ADDR			(0x94)	// current host descriptor address
#define SDHC_BUFFER_ADDR		(0x98)	// current buffer descriptor address
#define SDHC_FIFO       		(0x100)

/* control register 0x00h */
#define USE_INTERNAL_DMAC		(1 << 25)
#define ENABLE_OD_PULLUP		(1 << 24)
//#define CARD_VOLTAGE_B
//#define CARD_VOLTAGE_A
#define CEATE_INT_STATUS		(1 << 11)
#define AUTO_STOP_CCSD			(1 << 10)
#define SEND_CCSD			(1 << 9)
#define ABORT_READ_DATE			(1 << 8)
#define SEND_IRQ_RESPOSE		(1 << 7)
#define READ_WAIT			(1 << 6)
#define DMA_ENABLE			(1 << 5)
#define INT_ENABLE			(1 << 4)
#define DMA_RESET			(1 << 2)
#define FIFO_RESET			(1 << 1)
#define CONTROLLER_RESET		(1 << 0)

/* power enable register 0x04h */
#define POWER_ON			(1)

/* clock div 0x08h */
#define CLOCK_DIV2			(1 << 1)

/* SD clock source register 0x0ch */
#define CLOCK_DIVIDER_0			(0x00)
#define CLOCK_DIVIDER_1			(0x01)
#define CLOCK_DIVIDER_2			(0x10)
#define CLOCK_DIVIDER_3			(0x11)

/* clock enable register 0x10h */
#define	LOW_POWER_MODE			(1)
#define CLOCK_ENABLED			(1)

/* card type register 0x18h */
#define BIT_MODE_8				(1)
#define BIT_MODE_4				(1)
#define BIT_MODE_1				(0)

/* interrupt mask register 0x24h */
#define ENABLE_INT				(1)

/* for interrupt mask register and 
 * masked interrupt status register 
 * and raw interrupt status register
 */
#define MASK_ALL				(0xFFFF)
#define	END_BIT_ERR				(1 << 15)
#define AUTO_CMD_DONE			(1 << 14)
#define START_BIT_ERR			(1 << 13)
#define HARDWARE_LOCKED_ERR		(1 << 12)	
#define FIFO_UNDERRUN_ERR		(1 << 11)
#define DATA_STARVATION_ERR		(1 << 10)
#define DATA_READ_TO			(1 << 9)
#define RESPONSE_TO			(1 << 8)
#define DATA_CRC_ERR			(1 << 7)
#define RESPONSE_CRC_ERR		(1 << 6)
#define RECEIVE_FIFO_DATA_REQ		(1 << 5)
#define TRANSMIT_FIFO_DATA_REQ		(1 << 4)
#define DATA_TRANSFER_OVER		(1 << 3)
#define COMMAND_DONE			(1 << 2)
#define RESPONSE_ERR			(1 << 1)
#define CARD_DETECT_INT			(1 << 0)

/* command regsiter 0x2c */
#define START_CMD			(1 << 31)
#define BOOT_MODE			(1 << 27)
#define DISABLE_BOOT			(1 << 26)
#define EXPECT_BOOT_ACK			(1 << 25)
#define ENABLE_BOOT			(1 << 24)
#define CCS_EXPECTED			(1 << 23)
#define READ_CEATE_DIVICE		(1 << 22)
#define UPDATE_CLOCK_ONLY		(1 << 21)
//#define CARD_NUMBER			
#define SEND_INIT			(1 << 15)
#define STOP_ABORT_CMD			(1 << 14)
#define WAIT_PRVDATA_COMPLETE		(1 << 13)
#define SEND_AUTO_STOP			(1 << 12)
#define TRANSFER_MODE			(1 << 11)
#define READ_WRITE			(1 << 10)
#define DATA_EXPECTED			(1 << 9)
#define CHECK_RESPOSE_CRC		(1 << 8)
#define REPOSE_LENGTH			(1 << 7)
#define RESPOSE_EXPECT			(1 << 6)

/* masked interrupt status register 0x40h */
#define ENABLE_INT_STATUS		(1)

/* raw interrupt status register 0x40h */
#define ENABLE_RAW_INT_STATUS		(1)

/* status register 0x48 */
#define DMA_REQ				(1 << 31)
#define DMA_ACK				(1 << 30)
// #define FIFO_COUNT
// #define RESPOSE_INDEX
#define DATA_STATE_BUSY			(1 << 10)
#define DATA_BUSY			(1 << 9)
#define DATA_3_STATUS			(1 << 8)
// #define CMD_FSM_STATES
#define FIFO_FULL			(1 << 3)
#define FIFO_EMPTY			(1 << 2)
#define FIFO_TX_WATERMARK		(1 << 1)
#define FIFO_RX_WATERMARK		(1 << 0)

/* card detect register 0x50 */
#define CARD_DETECT				(1)

/* write protect register 0x54 */
#define WRITE_PROTECT			(1)

/* hardware configuration register 0x70 */
#define CARD_TYPE				(1 << 0)
// #define NUM_CARDS				(0x3E)
#define H_BUS_TYPE				(1 << 6)
// #define H_DATA_WIDTH
// #define H_ADDR_WIDTH
// #define DMA_INTERFACE
// #define GE_DMA_DATA_WIDTH
#define FIFO_RAM_INSIDE			(1 << 21)
#define IMPLEMENT_HOLD_REG		(1 << 22)
#define SET_CLK_FALSE_PATH		(1 << 23)
// #define NUM_CLK_DIVIDER
#define AREA_OPTIMIZED			(1 << 26)

/* bus mode register 0x80 */
#define IDMA_ENABLE			(1 << 7)
#define FIXED_BURST			(1 << 1)
#define SOFTWARE_RESET			(1 << 0)

/* for internal DMAC status register 0x8c 
 * and internal DMAC interrupt enable register
 */
#define ABNORMAL_INT_SUMMARY		(1 << 9)
#define NORMAL_INT_SUMMARY		(1 << 8)
#define CARD_ERROR_SUMMARY		(1 << 5)
#define DESCRIPTOR_UNAV_INT		(1 << 4)
#define FATAL_BUS_ERR_INT		(1 << 2)
#define RECEIVE_INT			(1 << 1)
#define TRANSMIT_INT			(1 << 0)

#endif
