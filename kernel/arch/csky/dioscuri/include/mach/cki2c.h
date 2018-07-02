/*
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012  Hangzhou C-SKY Microsystems co.,ltd.
 */

#ifndef __CKI2C_H__
#define __CKI2C_H__

#include <asm/addrspace.h>
#include <mach/ck_iomap.h>

/* slave address for CK_IC_TAR */
#define SLAVE_ADDRESS	0x38

/* I2C registers */
#define CK_IC_CON	0x00
#define CK_IC_TAR	0x04
#define CK_IC_SAR	0x08
#define CK_IC_DATA_CMD		0x10
#define CK_IC_SS_SCL_HCNT	0x14
#define CK_IC_SS_SCL_LCNT	0x18
#define CK_IC_FS_SCL_HCNT	0x1c
#define CK_IC_FS_SCL_LCNT	0x20
#define CK_IC_INTR_STAT		0x2c
#define CK_IC_INTR_MASK		0x30
#define CK_IC_RAW_INTR_STAT	0x34
#define CK_IC_RX_TL	0x38
#define CK_IC_TX_TL	0x3c
#define CK_IC_CLR_INTR	0x40
#define CK_IC_CLR_RX_UNDER	0x44
#define CK_IC_CLR_RX_OVER	0x48
#define CK_IC_CLR_TX_OVER	0x4c
#define CK_IC_CLR_RD_REQ	0x50
#define CK_IC_CLR_TX_ABRT	0x54
#define CK_IC_CLR_TX_DONE	0x58
#define CK_IC_CLR_ACTIVITY	0x5c
#define CK_IC_CLR_STOP_DET	0x60
#define CK_IC_CLR_START_DET	0x64
#define CK_IC_CLR_GEN_CALL	0x68
#define CK_IC_ENABLE	0x6c
#define CK_IC_STATUS	0x70
#define CK_IC_TXFLR	0x74
#define CK_IC_RXFLR	0x78
#define CK_IC_TX_ABRT_SOURCE	0x80
#define CK_IC_DMA_CR	0x88
#define CK_IC_DMA_TDLR	0x8c
#define CK_IC_DMA_RDLR	0x90

#define CK_I2C_SLAVE		0
#define CK_I2C_MASTER		1
#define CK_I2C_SLAVEMASTER	2

#define CK_I2C_STANDARD_SPEED	0
#define CK_I2C_FAST_SPEED	1

#define CK_I2C_RXFIFO_FULL	(0x1 << 4)
#define CK_I2C_RXFIFO_NOT_EMPTY	(0x1 << 3)
#define CK_I2C_TXFIFO_EMPTY	(0x1 << 2)
#define CK_I2C_TXFIFO_NOT_FULL	(0x1 << 1)
#define CK_I2C_ACTIVITY		(0x1 << 0)

#define CK_I2C_ADDRESS_7BITS	0
#define CK_I2C_ADDRESS_10BITS	1

#endif
