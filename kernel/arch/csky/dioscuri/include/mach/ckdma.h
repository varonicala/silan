/*
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2011  Hangzhou C-SKY Microsystems co.,ltd.
 */

#ifndef __CKDMA_H__
#define __CKDMA_H__


#include <asm/addrspace.h>
#include <mach/ck_iomap.h>

/*
 * define the dma control register base address
 */
#ifdef CONFIG_MMU
#define CK_DMA_BASE     KSEG1ADDR(CSKY_DMA_PHYS)
#else
#define CK_DMA_BASE	CSKY_DMA_PHYS
#endif


/* DMA channel registers */
#define SAR	0x000
#define DAR	0x008
#define CTRLa	0x018
#define CTRLb	0x01c
#define CFGa	0x040
#define CFGb	0x044

/* DMA interrupt registers */
#define RawTfr		0x2c0
#define RawBlock	0x2c8
#define RawSrcTran	0x2d0
#define RawDstTran	0x2d8
#define RawErr		0x2e0

#define StatusTfr	0x2e8
#define StatusBlock	0x2f0
#define StatusSrcTran	0x2f8
#define StatusDstTran	0x300
#define StatusErr	0x308

#define MaskTfr		0x310
#define MaskBlock	0x318
#define MaskSrcTran	0x320
#define MaskDstTran	0x328
#define MaskErr		0x330

#define ClearTfr	0x338
#define ClearBlock	0x340
#define ClearSrcTran	0x348
#define ClearDstTran	0x350
#define ClearErr	0x358
#define StatusInt	0x360

/* DMA software handshaking register */
#define ReqSrcReg	0x368
#define ReqDstReg	0x370
#define SglReqSrcReg	0x378
#define SglReqDstReg	0x380
#define LstSrcReg	0x388
#define LstDstReg	0x390

/* DMA miscellaneous register */
#define DmaCfgReg	0x398
#define ChEnReg		0x3a0
#define DmaTestReg	0x3b0


/* number of DMA channels */
#define DMA_CHANNEL_NUM	4

/* type of interrupts */
enum dma_inttype
{
	CK_DMA_TFR	= 1,
	CK_DMA_BLOCK	= 2,
	CK_DMA_ERR	= 16
};


/* struct of a channel's information */
struct ck_dma_chan
{
	unsigned int number;
	void __iomem* regbase;
	unsigned int in_use;	
	unsigned int is_complete;
	int src_dev_id;
	int dst_dev_id;
	unsigned int numbytes;
	char *src_device_name;
	char *dst_device_name;
	void (*callback)(unsigned int channel, unsigned int type);
};



/* DMA hardware handshaking interfaces */
enum dev_id
{
	UART0_TX	= 0,
	UART0_RX	= 1,	
	UART1_TX	= 2,
	UART1_RX	= 3,
	I2C_TX		= 4,
	I2C_RX		= 5,
	SPI_TX		= 6,
	SPI_RX		= 7,
	SDHC_TX		= 8,
	SDHC_RX		= 9,
	AC97_LEFT_TX	= 10,
	AC97_RIGHT_TX	= 11,
	AC97_LEFT_IN_RX	= 12,
	AC97_RIGHT_IN_RX	= 13,
	AC97_MIC_RX	= 14,
	PWM		= 15		
};

/* DMA source and dest master select */
enum master_select
{
        AHB_MASTER_1    = 0,
        AHB_MASTER_2    = 1,
        AHB_MASTER_3    = 2,
        AHB_MASTER_4    = 3
};

/* DMA transfer mode*/
enum transfer_mode
{
	MEM_TO_MEM   = 0,
	MEM_TO_PERI  = 1,
	PERI_TO_MEM  = 2,
	PERI_TO_PERI = 3
};

/* DMA transfer width */
enum transfer_width
{
	WIDTH_8	 = 0,
	WIDTH_16 = 1,
	WIDTH_32 = 2
};

/* DMA burst transaction length */
enum burst_length
{
	LENGTH_1 = 0,
	LENGTH_4 = 1
};

/* DMA source and destination address's change */
enum addr_inc
{
	ADDR_INCREASE = 0,
	ADDR_DECREASE = 1,
	ADDR_CONSTANT = 2
};

/* DMA handshake*/
enum handshake
{
	HARDWARE_MODE = 0,
	SOFTWARE_MODE = 1
};

/* request a dma channel */
int ck_request_dma(int src_dev_id, int dst_dev_id, char *src_device_name, char *dst_device_name, void (*callback)(unsigned int channel, unsigned int type));

/* free a dma channel */
int ck_free_dma(unsigned int channel);

/* source and master slect */
int ck_dma_masterSelect(unsigned int channel, unsigned int src_master, unsigned int dst_master);

/* config source and destination's addresses */
int ck_dma_addrconfig(unsigned int channel, unsigned long srcaddr, unsigned long dstaddr, size_t numbytes);

/* config transfer mode */
int ck_dma_transfermodeconfig(unsigned int channel, enum transfer_mode mode);

/* config source and destination's transfer width */
int ck_dma_widthconfig(unsigned int channel, enum transfer_width srcwidth, enum transfer_width dstwidth);

/* config source and destination's burst length */
int ck_dma_burstlenconfig(unsigned int channel, enum burst_length srclen, enum burst_length dstlen);

/* config source and destination's address_increase mode */
int ck_dma_addrincconfig(unsigned int channel, enum addr_inc srcinc, enum addr_inc dstinc);

/* config source and destination's handshake mode */
int ck_dma_handshakeconfig(unsigned int channel, enum handshake srchs, enum handshake dsths);

/* start a channel */
int ck_dma_start(unsigned int channel);

/* stop a channel before transfering completed */
int ck_dma_stop(unsigned int channel);

/* get the addresses in source and destination's registers */
int ck_dma_getposition(unsigned int channel, dma_addr_t *src, dma_addr_t *dst);

/* return a channel is closed or not */
int ck_dma_ischannelclose(int channel);


#endif
