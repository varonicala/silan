/*--------------------------------------------------------------------
 *copyright (c) 2012-2016, Hangzhou silan Microelectronics CO.,LTD
 *					All rights reserved
 *
 *FileName:	dma.c
 *Author:		Dong DongSheng
 *created:	2012-7-18
 *Edition:		1.0
 *Function:	dma driver
 *Note:
 *History:	2016-02-24为6138做了适当修改 by dds
 *--------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include "type_def.h"
#include "dmac.h"
#include "dma.h"
#include "silan_irq.h"
#include "cache.h"
#include "silan_io.h"

DMA_sPort * dma_reg_p = DMAC_BASE_ADDR;

#define PRINT_INFO(a)	printk a
#define DMA_PRINT(a)	printk a
#define Virtualaddr_2_physicaladdr(a)		(a&0x1fffffff)
//static irqreturn_t dma_interrupt_service(int irq, void *dev);
#define MAX_DMA_CHANNELS			(8)

DMA_CHS_sCB dma_chs_cb[MAX_DMA_CHANNELS];

void stop_dma(DMA_sChannel * dma_chs)
{
	dma_chs->Configuration = 0;
	dma_chs->Control = 0;
}
void init_dmac(void)
{
	dma_reg_p->IntTCClear = 0xff; // clear the terminal count
	dma_reg_p->IntErrorClear = 0xff; // clear err
	dma_reg_p->Configuration = 0x1; // disable reduce power
	stop_dma(&dma_reg_p->sDmaChannels[0]);
	stop_dma(&dma_reg_p->sDmaChannels[1]);
	stop_dma(&dma_reg_p->sDmaChannels[2]);
	stop_dma(&dma_reg_p->sDmaChannels[3]);
	stop_dma(&dma_reg_p->sDmaChannels[6]);
	memset(dma_chs_cb, 0, sizeof(dma_chs_cb[0])*MAX_DMA_CHANNELS);
	dma_chs_cb[4].active = 1;
	dma_chs_cb[5].active = 1;
	dma_chs_cb[7].active = 1;

    int irq = readl(0xba090000);
    irq |= 1<<PIC_IRQ_DMAC0;
    writel(irq, 0xba090000);

#if 0
	if(0!=request_irq(IRQ_DMA, dma_interrupt_service, SA_INTERRUPT, "DMAC", NULL))
	{
		DMA_PRINT(("request irq of DMA failed!\n"));
	}
#else
	//int retval = request_irq(PIC_IRQ_DMAC0, dma_interrupt_service, 0, "bt-dma", NULL);
	//if (retval)
	//{
	//	DMA_PRINT(("request irq of DMA failed!\n"));
	//}
	//up_enable_irq(PIC_IRQ_DMAC0);
#endif
}


int start_dma(DMA_sChannel * dma_chs, UINT8 dma_dir, apDMA_sRawLLI * DMA_LLI, UINT8 itc_enable)
{
	UINT32 cfg=0;
	switch(dma_dir)
	{
		case DMA_DIR_M_TO_M:
			cfg |= (apDMA_MEM_TO_MEM_DMA_CTRL<<11);
			break;
		case DMA_DIR_UART_TO_M:
			cfg |= (apDMA_PERIPHERAL_TO_MEM_DMA_CTRL<<11);
			cfg |= DMA_PER_ID_UART_RX<<1;
			break;
		case DMA_DIR_M_TO_UART:
			cfg |= (apDMA_MEM_TO_PERIPHERAL_DMA_CTRL<<11);
			cfg |= DMA_PER_ID_UART_TX<<6;
			break;
		case DMA_DIR_SPI_TO_M:
			cfg |= (apDMA_PERIPHERAL_TO_MEM_DMA_CTRL<<11);
			cfg |= DMA_PER_ID_SPI_IN<<1;
			break;
		case DMA_DIR_M_TO_SPI:
			cfg |= (apDMA_MEM_TO_PERIPHERAL_DMA_CTRL<<11);
			cfg |= DMA_PER_ID_SPI_OUT<<6;
			break;
		case DMA_DIR_IIS_TO_M:
			cfg |= (apDMA_PERIPHERAL_TO_MEM_DMA_CTRL<<11);
			cfg |= DMA_PER_ID_IIS51_IN0<<1;
			break;
		case DMA_DIR_M_TO_IIS:
			cfg |= (apDMA_MEM_TO_PERIPHERAL_DMA_CTRL<<11);
			cfg |= DMA_PER_ID_IIS51_OUT0<<6;
			break;
		case DMA_DIR_M_TO_SPDIF:
			cfg |= (apDMA_MEM_TO_PERIPHERAL_DMA_CTRL<<11);
			cfg |= DMA_PER_ID_SPDIF_OUT<<6;
			break;
		case DMA_DIR_SPDIF_TO_M:
			cfg |= (apDMA_PERIPHERAL_TO_MEM_PERIPHERAL_CTRL<<11);
			cfg |= DMA_PER_ID_SPDIF_IN<<1;
			break;
		default:
			break;
	}
	cfg |= DMA_ERR_INT_EN;
	if(itc_enable)
	{
		cfg |= DMA_TC_INT_EN;
	}
	dma_chs->SrcAddr = DMA_LLI->SrcAddr;
	dma_chs->DestAddr = DMA_LLI->DstAddr;
	dma_chs->LLI = DMA_LLI->NextLLI;
	dma_chs->Control = DMA_LLI->TransferCtrl;
	dma_chs->Configuration = cfg|0x1;
	return 0;
}

UINT32 get_dma_destaddr_now(UINT8 ch)
{
	return dma_reg_p->sDmaChannels[ch].DestAddr;
}

int request_dma(UINT8 ch)
{
	if(ch>7)
		return -1;
	if((dma_reg_p->ActiveChannels&(1<<ch))||(dma_chs_cb[ch].active))
	{
		DMA_PRINT(("the channel %d is actived\n", ch));
		return -1;
	}
	return 0;
}

int free_dma(int ch)
{
	dma_chs_cb[ch].active = 0;
	if(dma_reg_p->ActiveChannels&(1<<ch))
	{
		stop_dma(&dma_reg_p->sDmaChannels[ch]);
	}
}

/*
 *	得到通道寄存器的配置内容
 */
UINT32 prepare_dma_cfg(UINT8 dma_mode, UINT8 width, size_t NumTransfers)
{
	UINT32 cfg=0;
	switch(dma_mode)
	{
		case DMA_DIR_M_TO_M:
			cfg = DMA_DST_ADDR_INC|DMA_SRC_ADDR_INC |(width<<21)|(width<<18)|(DMA_BURST_SIZE_4<<15)|(DMA_BURST_SIZE_4<<12)|(NumTransfers);
			break;
		case DMA_DIR_UART_TO_M:
			cfg = DMA_DST_ADDR_INC | DMA_SRC_MASTER_AHB |(width<<21)|(width<<18)|(DMA_BURST_SIZE_1<<15)|(DMA_BURST_SIZE_1<<12)|(NumTransfers);
			break;
		case DMA_DIR_M_TO_UART:
			cfg = DMA_SRC_ADDR_INC | DMA_DST_MASTER_AHB |(width<<21)|(width<<18)|(DMA_BURST_SIZE_32<<15)|(DMA_BURST_SIZE_32<<12)|(NumTransfers);
			break;
		case DMA_DIR_SPI_TO_M:
			cfg = DMA_DST_ADDR_INC | DMA_SRC_MASTER_AHB |(width<<21)|(width<<18)|(DMA_BURST_SIZE_4<<15)|(DMA_BURST_SIZE_4<<12)|(NumTransfers);
			break;
		case DMA_DIR_M_TO_SPI:
			cfg = DMA_SRC_ADDR_INC | DMA_DST_MASTER_AHB |(width<<21)|(width<<18)|(DMA_BURST_SIZE_4<<15)|(DMA_BURST_SIZE_4<<12)|(NumTransfers);
			break;
		case DMA_DIR_IIS_TO_M:
		case DMA_DIR_IIS0_TO_M:
		case DMA_DIR_IIS1_TO_M:
		case DMA_DIR_IIS2_TO_M:
			cfg = DMA_DST_ADDR_INC | DMA_SRC_MASTER_AHB |(width<<21)|(width<<18)|(DMA_BURST_SIZE_8<<15)|(DMA_BURST_SIZE_8<<12)|(NumTransfers);
			break;
		case DMA_DIR_M_TO_IIS:
			cfg = DMA_SRC_ADDR_INC | DMA_DST_MASTER_AHB |(width<<21)|(width<<18)|(DMA_BURST_SIZE_8<<15)|(DMA_BURST_SIZE_8<<12)|(NumTransfers);
			break;
		case DMA_DIR_M_TO_SPDIF:
			cfg = DMA_SRC_ADDR_INC | DMA_DST_MASTER_AHB |(width<<21)|(width<<18)|(DMA_BURST_SIZE_8<<15)|(DMA_BURST_SIZE_16<<12)|(NumTransfers);
			break;
		case DMA_DIR_SPDIF_TO_M:
			cfg = DMA_DST_ADDR_INC | DMA_SRC_MASTER_AHB |(width<<21)|(width<<18)|(DMA_BURST_SIZE_8<<15)|(DMA_BURST_SIZE_8<<12)|(NumTransfers);
			break;
			break;
		default:
			break;
	}
	return cfg;
}

int update_dma_lli(request_sDMA * dma_request)
{
	UINT32 lli_num = dma_request->lli_num;
	UINT32 lli_num_int = dma_request->lli_num_int-1;
	UINT32 i;
	apDMA_sLLI * cur_LLI = dma_request->dma_LLI;
	if(cur_LLI!=NULL)
	{
		for(i=0; i<lli_num; i++)
		{
			cur_LLI->sRaw.DstAddr = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pDstAddr);
			cur_LLI->sRaw.SrcAddr = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pSrcAddr);
			cur_LLI->sRaw.TransferCtrl = prepare_dma_cfg(dma_request->dma_dir, dma_request->width, cur_LLI->NumTransfers);
			//cur_LLI->sRaw.NextLLI = (0x1fffffff&((UINT32)cur_LLI->pNextLLI))|0x1;
			cur_LLI->sRaw.NextLLI = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pNextLLI);
			if(cur_LLI->pNextLLI==NULL)
			{
				//cur_LLI->sRaw.TransferCtrl |= DMA_LLI_TC_EN;
				break;
			}
			if(i==lli_num_int)
			{
				cur_LLI->sRaw.TransferCtrl |= DMA_LLI_TC_EN;
			}
			cur_LLI = cur_LLI->pNextLLI;
		}
	}
	return 0;
}

int restart_dma_lli_update(request_sDMA * dma_request)
{
	UINT32 lli_num = dma_request->lli_num;
	UINT32 lli_num_int = dma_request->lli_num_int-1;
	UINT32 i;
	apDMA_sLLI * cur_LLI = dma_request->dma_LLI;
	if(cur_LLI!=NULL)
	{
		for(i=0; i<lli_num; i++)
		{
			cur_LLI->sRaw.DstAddr = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pDstAddr);
			cur_LLI->sRaw.SrcAddr = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pSrcAddr);
			cur_LLI->sRaw.TransferCtrl = prepare_dma_cfg(dma_request->dma_dir, dma_request->width, cur_LLI->NumTransfers);
			//cur_LLI->sRaw.NextLLI = (0x1fffffff&((UINT32)cur_LLI->pNextLLI))|0x1;
			cur_LLI->sRaw.NextLLI = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pNextLLI);
			if(cur_LLI->pNextLLI==NULL)
			{
				//cur_LLI->sRaw.TransferCtrl |= DMA_LLI_TC_EN;
				break;
			}
			if(i==lli_num_int)
			{
				cur_LLI->sRaw.TransferCtrl |= DMA_LLI_TC_EN;
			}
			cur_LLI = cur_LLI->pNextLLI;
		}
		start_dma(&dma_reg_p->sDmaChannels[dma_request->ch], dma_request->dma_dir, &dma_request->dma_LLI->sRaw, dma_request->int_enable);
	}
	return 0;
}

int request_dma_transfer(request_sDMA * dma_request)
{
	UINT8 ch = dma_request->ch;
	UINT32 lli_num = dma_request->lli_num;
	UINT32 lli_num_int = dma_request->lli_num_int -1;
	UINT32 i;
	apDMA_sLLI * cur_LLI = dma_request->dma_LLI;
	if(request_dma(ch)!=0)
	{
		return -1;
	}
	if(cur_LLI!=NULL)
	{
		//while(1)
		for(i=0; i<lli_num; i++)
		{
			cur_LLI->sRaw.DstAddr = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pDstAddr);
			cur_LLI->sRaw.SrcAddr = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pSrcAddr);
			cur_LLI->sRaw.TransferCtrl = prepare_dma_cfg(dma_request->dma_dir, dma_request->width, cur_LLI->NumTransfers);
			cur_LLI->sRaw.NextLLI = Virtualaddr_2_physicaladdr((UINT32)cur_LLI->pNextLLI);
			if(i==lli_num_int)
			{
				cur_LLI->sRaw.TransferCtrl |= DMA_LLI_TC_EN;
			}
			if(cur_LLI->pNextLLI==NULL)
			{
				//cur_LLI->sRaw.TransferCtrl |= DMA_LLI_TC_EN;
				break;
			}
			cur_LLI = cur_LLI->pNextLLI;
		}
	}
	dma_chs_cb[ch].callback = dma_request->callback;
	dma_chs_cb[ch].request = dma_request;
	start_dma(&dma_reg_p->sDmaChannels[ch], dma_request->dma_dir, &dma_request->dma_LLI->sRaw, dma_request->int_enable);
	return 0;
}



void restart_dma(request_sDMA * dma_request)
{
	stop_dma(&dma_reg_p->sDmaChannels[dma_request->ch]);
	request_dma_transfer(dma_request);
}
/*
//static irqreturn_t pl08x_irq(int irq, void *dev)
static irqreturn_t dma_interrupt_service(int irq, void *dev)
{
	UINT32 status;
	int i;
	status = dma_reg_p->RawIntErrorStatus;
	if(status)
	{
		DMA_PRINT(("DMA err %x\n", status));
		dma_reg_p->IntErrorClear = status;
	}
	status = dma_reg_p->IntTCStatus;
	//printk("###### dma_interrupt_service %x\n", status);
	if(status)
	{
		dma_reg_p->IntTCClear = status;
		for(i=0; i<8; i++)
		{
			if(status&(0x1<<i))
			{
				if(0){
//				if(i == DMA_CHANNEL_UART_RX){
					dma_chs_cb[i].request->result = 0;
					if(NULL!=dma_chs_cb[i].callback)
					{
						dma_chs_cb[i].callback(dma_chs_cb[i].request);
					}
					if(dma_chs_cb[i].request->use_once)
					{
						free_dma(i);
					}
				}else{
					writel(0x1<<(i+1), 0xba0b0004);
					writel(0x1<<(i+1), 0xba0b0008);
				}
			}
		}
	}
	//printk("111###### dma_interrupt_service\n");
	//writel(0, dma_reg_p->IntTCClear);
	//printk("222###### dma_interrupt_service\n");
	return IRQ_HANDLED;
}
*/

int dma_request_sync(request_sDMA * dma_request)
{
	UINT32 status;
	UINT8 ch;
	ch = dma_request->ch;
	request_dma_transfer(dma_request);
	while(1)
	{
		status = dma_reg_p->RawIntErrorStatus;
		if(status&(1<<ch))
		{
			dma_reg_p->RawIntErrorStatus = (1<<ch);
			DMA_PRINT(("DMA ch %d err\n", ch));
			return -1;
		}
		status = dma_reg_p->RawIntTCStatus;
		if(status&(1<<ch))
		{
			free_dma(ch);
			dma_reg_p->IntTCClear = (1<<ch);
			break;
		}
	}
	return 0;
}


int dma_transceive_easy(UINT8 dir, void * src, void * dest, size_t num)
{
	UINT8 width = DMA_TS_WIDTH_8BIT;
	request_sDMA easy_dma_request;
	apDMA_sLLI easy_dma_LLI;
	if((dir==DMA_DIR_M_TO_SPI)||(dir==DMA_DIR_SPI_TO_M))
		width = DMA_TS_WIDTH_32BIT;
	easy_dma_request.callback = NULL;
	easy_dma_request.ch = DMA_CHANNEL_TEMP;					
	easy_dma_request.dma_dir = dir;
	easy_dma_request.result = -1;
	easy_dma_request.dma_LLI = &easy_dma_LLI;
	easy_dma_request.lli_num = 1;
	easy_dma_request.lli_num_int = 1;
	easy_dma_request.int_enable = 0;
	easy_dma_request.width = width;
	easy_dma_LLI.NumTransfers = num;
	easy_dma_LLI.pDstAddr = dest;
	easy_dma_LLI.pSrcAddr = src;
	easy_dma_LLI.pNextLLI = NULL;
	return dma_request_sync(&easy_dma_request);
}

void Dcache_invalidate(size_t addr, size_t size)
{
	flush_dcache_range(addr, addr+size);
}

void memcpy_dma32(UINT32 * dest, UINT32 * src,UINT32 len)
{
	dma_transceive_easy(DMA_DIR_M_TO_M, src, dest, len*4);
	Dcache_invalidate(dest, len*4);
}

void test_dma_cpy()
{
	UINT32 dma_src[128], dma_dest[128];
	memcpy_dma32(dma_dest, dma_src,128);
}

int wait_dma_done(int ch)
{
	UINT32 status;
	while(1)
	{
		status = dma_reg_p->RawIntErrorStatus;
		if(status&(1<<ch))
		{
			dma_reg_p->RawIntErrorStatus = (1<<ch);
			DMA_PRINT(("DMA ch %d err\n", ch));
			return -1;
		}
		status = dma_reg_p->RawIntTCStatus;
		if(status&(1<<ch))
		{
			free_dma(ch);
			dma_reg_p->IntTCClear = (1<<ch);
			break;
		}
	}
	return 0;
}

#if 0
//==============================================
// for test

UINT8 dma_done = 0;
int dma_callback(request_sDMA * dma_request)
{
	if(dma_request->result==0)
	{
		dma_done = 1;
		PRINT_INFO(("DMA done\n"));
	}
	return 0;
}

UINT8 * chip_ram_p = 0xbfc00000;
int test_dma(void)
{
	int i;
	UINT32 dma_src[128], dma_dest[128];
	apDMA_sLLI DMA_sLLI[2];
	request_sDMA dma_request;
	UINT8 * temp_buf;
	UINT8 * src_buf;
	UINT8 * dest_buf;
	UINT32 status;
	src_buf = (UINT8 *)dma_src;
	//src_buf = (UINT8 *)chip_ram_p;
	dest_buf = (UINT8 *)dma_dest;
	//dest_buf = chip_ram_p+512;
	for(i=0; i<256; i++)
	{
		src_buf[i] = i;
	}
	temp_buf = src_buf + 256;
	for(i=0; i<256; i++)
	{
		temp_buf[i] = 255-i;
	}
	memset(dest_buf, 0xaa, 512);
	dma_request.dma_LLI = &DMA_sLLI[0];
	DMA_sLLI[0].pSrcAddr = src_buf;
	DMA_sLLI[0].pDstAddr = dest_buf;
	DMA_sLLI[0].NumTransfers = 64;
	DMA_sLLI[0].pNextLLI = &DMA_sLLI[1];
	//DMA_sLLI[0].pNextLLI = NULL;
	DMA_sLLI[1].pSrcAddr = src_buf + 256;
	DMA_sLLI[1].pDstAddr = dest_buf + 256;
	DMA_sLLI[1].NumTransfers = 64;
	DMA_sLLI[1].pNextLLI = NULL;
	dma_request.callback = dma_callback;
	dma_request.ch = 6;
	dma_request.lli_num = 2;
	dma_request.lli_num_int = 2;
	dma_request.int_enable = 0;
	dma_request.dma_dir = DMA_DIR_M2M;
	dma_request.width = DMA_TS_WIDTH_32BIT;
	/*
	request_dma_transfer(&dma_request);
	while(1)
	{
		status = dma_reg_p->RawIntStatus;
		if(status)
		{
			dma_interrupt_service();
			break;
		}
	}
	*/
	dma_request_sync(&dma_request);
	while(1)
	{
		//if(dma_done)
		if(1)
		{
			Dcache_invalidate(dest_buf, 256*sizeof(UINT32));
			temp_buf = (UINT8 *)dest_buf;
			for(i=0; i<256; i++)
			{
				if(temp_buf[i] != i)
				{
					PRINT_INFO(("data err1\n"));
					goto faied;
				}
			}
			temp_buf = dest_buf+256;
			for(i=0; i<256; i++)
			{
				if(temp_buf[i] != (255-i))
				{
					PRINT_INFO(("data err2\n"));
					goto faied;
				}
			}
			break;
		}
	}
	PRINT_INFO(("dma test ok\n"));
	return 0;
faied:
	return -1;
}

#endif
 
