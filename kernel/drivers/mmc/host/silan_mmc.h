/*
 * Silan Multimedia Card Interface driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _SILAN_MMC_H_
#define _SILAN_MMC_H_

#define SDMMC_CTRL			0x000
#define SDMMC_PWREN			0x004
#define SDMMC_CLKDIV		0x008
#define SDMMC_CLKSRC		0x00c
#define SDMMC_CLKENA		0x010
#define SDMMC_TMOUT			0x014
#define SDMMC_CTYPE			0x018
#define SDMMC_BLKSIZ		0x01c
#define SDMMC_BYTCNT		0x020
#define SDMMC_INTMASK		0x024
#define SDMMC_CMDARG		0x028
#define SDMMC_CMD			0x02c
#define SDMMC_RESP0			0x030
#define SDMMC_RESP1			0x034
#define SDMMC_RESP2			0x038
#define SDMMC_RESP3			0x03c
#define SDMMC_MINTSTS		0x040
#define SDMMC_RINTSTS		0x044
#define SDMMC_STATUS		0x048
#define SDMMC_FIFOTH		0x04c
#define SDMMC_CDETECT		0x050
#define SDMMC_WRTPRT		0x054
#define SDMMC_GPIO			0x058
#define SDMMC_FIFO_DAT_CNT	0x060
#define SDMMC_DEBNCE		0x064
#define SDMMC_USRID			0x068
#define SDMMC_DMAADDR		0x070
#define SDMMC_DMACTRL		0x074
#define SDMMC_RSP_SPI		0x078
#define SDMMC_WR_DATA		0x080
#define SDMMC_RD_DATA		0x084

/* shift bit field */
#define _SBF(f, v)		((v) << (f))

/* Control register defines */
#define SDMMC_CTRL_O_DRAIN			BIT(24)
#define SDMMC_CTRL_ABRT_READ_DATA	BIT(8)
#define SDMMC_CTRL_SEND_IRQ_RESP	BIT(7)
#define SDMMC_CTRL_READ_WAIT		BIT(6)
#define SDMMC_CTRL_DMA_ENABLE		BIT(5)
#define SDMMC_CTRL_INT_ENABLE		BIT(4)
#define SDMMC_CTRL_DMA_RESET		BIT(2)
#define SDMMC_CTRL_FIFO_RESET		BIT(1)
#define SDMMC_CTRL_RESET			BIT(0)
/* Clock Enable register defines */
#define SDMMC_CLKEN_LOW_PWR			BIT(16)
#define SDMMC_CLKEN_ENABLE			BIT(0)
/* time-out register defines */
#define SDMMC_TMOUT_DATA(n)			_SBF(8, (n))
#define SDMMC_TMOUT_DATA_MSK		0xFFFFFF00
#define SDMMC_TMOUT_RESP(n)			((n) & 0xFF)
#define SDMMC_TMOUT_RESP_MSK		0xFF
/* card-type register defines */
#define SDMMC_CTYPE_8BIT			BIT(1)
#define SDMMC_CTYPE_4BIT			BIT(0)
#define SDMMC_CTYPE_1BIT			0
/* Interrupt status & mask register defines */
#define SDMMC_INT_INX_ERR			BIT(26)
#define SDMMC_INT_CIU_DAT_DONE		BIT(25)
#define SDMMC_INT_DMA_DONE			BIT(24)
#define SDMMC_INT_SDIO				BIT(16)
#define SDMMC_INT_EBE				BIT(15)
#define SDMMC_INT_ACD				BIT(14)
#define SDMMC_INT_SBE				BIT(13)
#define SDMMC_INT_HLE				BIT(12)
#define SDMMC_INT_FRUN				BIT(11)
#define SDMMC_INT_HTO				BIT(10)
#define SDMMC_INT_DTO				BIT(9)
#define SDMMC_INT_RTO				BIT(8)
#define SDMMC_INT_DCRC				BIT(7)
#define SDMMC_INT_RCRC				BIT(6)
#define SDMMC_INT_RXDR				BIT(5)
#define SDMMC_INT_TXDR				BIT(4)
#define SDMMC_INT_DATA_OVER			BIT(3)
#define SDMMC_INT_CMD_DONE			BIT(2)
#define SDMMC_INT_RESP_ERR			BIT(1)
#define SDMMC_INT_CD				BIT(0)
#define SDMMC_INT_ERROR				0xbfc2
/* Command register defines */
#define SDMMC_CMD_START				BIT(31)
#define SDMMC_CMD_UPD_CLK			BIT(21)
#define SDMMC_CMD_CRC_OFF			BIT(20)
#define SDMMC_CMD_INIT				BIT(15)
#define SDMMC_CMD_STOP				BIT(14)
#define SDMMC_CMD_PRV_DAT_WAIT		BIT(13)
#define SDMMC_CMD_SEND_STOP			BIT(12)
#define SDMMC_CMD_STRM_MODE			BIT(11)
#define SDMMC_CMD_DAT_WR			BIT(10)
#define SDMMC_CMD_DAT_EXP			BIT(9)
#define SDMMC_CMD_RESP_CRC			BIT(8)
#define SDMMC_CMD_RESP_LONG			BIT(7)
#define SDMMC_CMD_RESP_EXP			BIT(6)
#define SDMMC_CMD_INDX(n)			((n) & 0x1F)
/* Status register defines */
#define SDMMC_GET_FCNT(x)			(((x)>>17) & 0x1FF)
#define SDMMC_FIFO_FL_SZ			96
#define SDMMC_FIFO_EP_SZ			32
#define SDMMC_FIFO_SZ				128

/* DMA control*/
#define SDMMC_DMA_START				BIT(31)
#define DMA_BURST_SING				0
#define DMA_BURST_INC4				3
#define DMA_BURST_INC8				5
#define DMA_BURST_INC16				7
#define SDMMC_DMA_BURST_SZ(x)		((x)<<28)
#define DMA_FIFO_INC32				0
#define DMA_FIFO_INC64				1
#define DMA_FIFO_INC128				2
#define DMA_FIFO_INC256				3
#define SDMMC_DMA_FIFO_SZ(x)		((x)<<26)	
#define SDMMC_DMA_LEN(x)			((x)<<0)

/* Register access macros */
#define mci_readl(dev, reg)			\
	readl(dev->regs + SDMMC_##reg)
#define mci_writel(dev, reg, value)			\
	writel((value), dev->regs + SDMMC_##reg)

/* 16-bit FIFO access macros */
#define mci_readw(dev, reg)			\
	readw(dev->regs + SDMMC_##reg)
#define mci_writew(dev, reg, value)			\
	writew((value), dev->regs + SDMMC_##reg)

/* 64-bit FIFO access macros */
#ifdef readq
#define mci_readq(dev, reg)			\
	__raw_readq(dev->regs + SDMMC_##reg)
#define mci_writeq(dev, reg, value)			\
	__raw_writeq((value), dev->regs + SDMMC_##reg)
#else
/*
 * Dummy readq implementation for architectures that don't define it.
 *
 * We would assume that none of these architectures would configure
 * the IP block with a 64bit FIFO width, so this code will never be
 * executed on those machines. Defining these macros here keeps the
 * rest of the code free from ifdefs.
 */
#define mci_readq(dev, reg)			\
	(*(volatile u64 __force *)(dev->regs + SDMMC_##reg))
#define mci_writeq(dev, reg, value)			\
	(*(volatile u64 __force *)(dev->regs + SDMMC_##reg) = value)
#endif

#endif /* _DW_MMC_H_ */
