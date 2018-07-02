/*--------------------------------------------------------------------
 *copyright (c) 2012-2013, Hangzhou silan Microelectronics CO.,LTD
 *					All rights reserved
 *
 *FileName:	uart.c
 *Author:		Dong DongSheng
 *created:	2012-7-18
 *Edition:		1.0
 *Function:	uart driver
 *Note:
 *History:
 *--------------------------------------------------------------------*/
//#include "sys_cfg.h"
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include "type_def.h"
#define __SUPPORT_HS_UART__
#ifdef __SUPPORT_HS_UART__

#include "uart.h"
#include "dma.h"
//#include "sys_comm.h"
#include "uart_buf.h"
//#include "buffer.h"
#include "silan_irq.h"
#include "silan_addrspace.h"
#include <linux/delay.h>

/*============================================*/
/* 				porting						  */
/*============================================*/

#define CK610_CACHE

#define	get_sys_freq()			(150*1000*1000)
#define vaddr2paddr(a)			((a)&0x1fffffff)	
#define min(a, b)  (a) < (b) ? a : b

//void SLEEP(size_t ms)
//{
//	usleep(1000*ms);
//}
/*============================================*/

//#define UART_TRANSMIT_USE_INTERRUPT

#define PRINT_INFO(a)	printk a

uart_rcb_t uart_rcb;
UINT8 uart_rcv_buf[UART_RCV_BLOCKS][UART_RCV_BLOCK_LEN];
request_sDMA uart_rx_request;
apDMA_sLLI	uart_rx_lli[UART_RCV_BLOCKS];

static UINT32 uart_baudrate = 0;
static UINT32 uart_parity = 0;

/*
 *	parity: bit0->enable. bit1-> 0:odd, 1:even
 */
void init_uart(UINT32 baudrate, UINT32 parity)
{
	UINT32 ibrd, fbrd;
	UINT32 freq = get_sys_freq();
	UART0_CR = 0;			//  设置前必须先关掉
	UART0_LCRH = 0;
	uart_baudrate = baudrate;
	ibrd = get_sys_freq()/16/baudrate;
	fbrd = (get_sys_freq()*8/baudrate+1)/2 - ibrd*64;		// (freq/(16*baudrate)+0.5 - ibrd)*64
	UART0_IBRD = ibrd;
	UART0_FBRD = fbrd;
	//UART0_LCRH = 0x60;		// 8bit 1stop 无校验
	UART0_LCRH = 0x70|((parity<<1)&0x6);	// 8bit 1stop fifo enable
	uart_parity = parity;
#if 0
#if (TRANSPORT_LAYER_PROTOCOL == HCI_CSR_BCSP)	
	UART0_LCRH = 0x76;		// 8bit 1stop fifo enable 偶校验
#else
	UART0_LCRH = 0x70;		// 8bit 1stop 无校验fifo enable
#endif
#endif
	UART0_CR = 0x301; 		// enable Tx&Rx
	UART0_DMACR = 0x3;		// enable dma
	UART0_IMSC= 0x0;		// mask all interrupte
	PRINT_INFO(("set hs uart baudrate = %d\n", get_sys_freq()*4/(ibrd*64+fbrd)));
}

#ifdef UART_TRANSMIT_USE_INTERRUPT
buf_cb_t * uart_tb_p = NULL;

void enable_uart_transmit_interrupt(void)
{
	UART0_IFLS &= (~(0x7));
	UART0_IMSC |= (1<<5);
	
}

void disable_uart_transmit_interrupt(void)
{
	UART0_IMSC &= (~(1<<5));
}

BOOL create_uart_transmit()
{
	uart_tb_p = creat_buffer(UART_TRAN_BUF_LEN);
	return (uart_tb_p != NULL);
}

UINT8 * get_uart_transmit_buf_ptr()
{
	return get_output_buf_ptr(uart_tb_p);
}

void destroy_uart_transmit(void)
{
	destory_buffer(uart_tb_p);
	uart_tb_p = NULL;
}

void uart_tx_buf_to_fifo(void)
{
	size_t len = get_used_buf_len_con(uart_tb_p);
	UINT8 * data = get_uart_transmit_buf_ptr();
	int i;
	for(i=0; i<len; i++)
	{
		if((UART0_FR &(1<<5)))
		{
			release_out_buffer_irq(uart_tb_p, i);
			return;
		}
		URAT0_DR = data[i];
	}
	release_out_buffer_irq(uart_tb_p, len);
	len = get_used_buf_len_con(uart_tb_p);
	data = get_uart_transmit_buf_ptr();
	for(i=0; i<len; i++)
	{
		if((UART0_FR &(1<<5)))
		{
			release_out_buffer_irq(uart_tb_p, i);
			return;
		}
		URAT0_DR = data[i];
	}
	release_out_buffer_irq(uart_tb_p, len);
}

#endif

/*
 * Output a single byte to the serial port.
 */
void uart_send_byte (UINT8 data)
{
	
	while((UART0_FR &(1<<5)));
	URAT0_DR = data;
	//timer_wait_us(1000);
}

void restore_uart_baudrate(void)
{
	if(uart_baudrate)
		init_uart(uart_baudrate, uart_parity);
}

void wait_uart_idle(void)
{
	while(UART0_FR&(1<<3));
}

int uart_transmit_sync(UINT8 * buf, size_t len)
{
	int i;
	for(i=0; i<len; i++)
	{
		uart_send_byte(buf[i]);
	}
}

int uart_transmit(UINT8 * buf, size_t len)
{
#if 1
	int i;
#ifdef UART_TRANSMIT_USE_INTERRUPT
	if(uart_tb_p!=NULL)
	{
		while(get_free_buf_len(uart_tb_p)<len)
		{
			enable_uart_transmit_interrupt();
			SLEEP(10);
		}
		write_buffer(uart_tb_p, buf, len);
		disable_interrupts();
		uart_tx_buf_to_fifo();
		enable_interrupts();
		enable_uart_transmit_interrupt();
	}
	else
#else
	{
		for(i=0; i<len; i++)
		{
			uart_send_byte(buf[i]);
			//sie_printf("%x ", buf[i]);
		}
	}
#endif
	return TRUE;
#else
	#if 0
	request_sDMA tx_request;
	apDMA_sLLI tx_LLI;
	tx_request.ch = 0;
	tx_request.dma_dir = DMA_DIR_M2UART;
	tx_request.dma_LLI = &tx_LLI;
	tx_request.width = DMA_TS_WIDTH_8BIT;
	tx_LLI.NumTransfers = len;
	tx_LLI.pDstAddr = &URAT0_DR;
	tx_LLI.pSrcAddr = buf;
	tx_LLI.pNextLLI = NULL;
	//request_dma_transfer(&tx_request);
	dma_request_sync(&tx_request);
	#else
	dma_transceive_eays(DMA_DIR_M2UART, buf, &URAT0_DR, len);
	#endif
	return TRUE;
#endif
}

extern int free_dma(int ch);
void close_uart_rcv()
{
	free_dma(DMA_CHANNEL_UART_RX);
}

extern int request_dma_transfer(request_sDMA * dma_request);
void open_uart_rcv(uart_rcb_t * uart_rcb_p, request_sDMA * dma_req)
{
	int i;
	memset(uart_rcb_p, 0, sizeof(uart_rcb_t));
	uart_rcb_p->buf = &uart_rcv_buf[0][0];
	dma_req->callback 	= NULL;//uart_receive_callback;
	dma_req->ch 		= DMA_CHANNEL_UART_RX;
	dma_req->dma_dir 	= DMA_DIR_UART_TO_M;
	dma_req->dma_LLI	= &uart_rx_lli;
	dma_req->int_enable	= 1;
	dma_req->lli_num	= UART_RCV_BLOCKS;
	dma_req->lli_num_int= UART_RCV_BLOCKS;
	dma_req->result 	= -1;
	dma_req->use_once	= 0;
	dma_req->width		= DMA_TS_WIDTH_8BIT;
	for(i=0; i<UART_RCV_BLOCKS; i++)
	{
		uart_rx_lli[i].NumTransfers = UART_RCV_BLOCK_LEN;
		uart_rx_lli[i].pDstAddr		= (UINT32)uart_rcb_p->buf + (i*UART_RCV_BLOCK_LEN);
		uart_rx_lli[i].pSrcAddr		= &URAT0_DR;
		if(i==(UART_RCV_BLOCKS-1))
			uart_rx_lli[i].pNextLLI	= &uart_rx_lli[0];
		else
			uart_rx_lli[i].pNextLLI	= &uart_rx_lli[i+1];
	}
	request_dma_transfer(dma_req);
	PRINT_INFO(("open uart rcv\n"));
}


void uart_interrupt_service(void)
{
#ifdef UART_TRANSMIT_USE_INTERRUPT
	if(UART0_RIS & (1<<5))
	{
		uart_tx_buf_to_fifo();
		if(get_used_buf_len(uart_tb_p)==0)
			disable_uart_transmit_interrupt();
	}
#endif
}

void init_bt_uart(UINT32 baudrate, UINT32 parity)
{
	printk("\open uart %d %d\n", baudrate, parity);
	init_uart(baudrate, parity);
	open_uart_rcv(&uart_rcb, &uart_rx_request);
#ifdef UART_TRANSMIT_USE_INTERRUPT
	create_uart_transmit();
	if(request_irq(IRQ_UART0, uart_interrupt_service, SA_INTERRUPT, "UART0", (void *)0))
	{
		PRINT_INFO(("Request UART0 IRQ Failed!\n\r"));
		//while(1);
	}
#endif
}

void deinit_bt_uart(void)
{
	close_uart_rcv();
#ifdef UART_TRANSMIT_USE_INTERRUPT
	destroy_uart_transmit();
	free_irq(IRQ_UART0, NULL);
#endif
}

extern UINT32 get_dma_destaddr_now(UINT8 ch);
size_t get_uart_rcv_data_len_in_buf()
{
	UINT32 size;
	uart_rcb.inp = get_dma_destaddr_now(DMA_CHANNEL_UART_RX) - vaddr2paddr((UINT32)uart_rcb.buf);
	if(uart_rcb.inp>=uart_rcb.outp)
		uart_rcb.cap = (uart_rcb.inp - uart_rcb.outp);
	else
		uart_rcb.cap = (UART_RCV_BUF_LEN + uart_rcb.inp - uart_rcb.outp);
	return uart_rcb.cap;
}

size_t get_uart_rcv_data_len_in_buf_con()
{
	UINT32 size;
	uart_rcb.inp = get_dma_destaddr_now(DMA_CHANNEL_UART_RX) - vaddr2paddr((UINT32)uart_rcb.buf);
	if(uart_rcb.inp>=uart_rcb.outp)
	{
		uart_rcb.cap = (uart_rcb.inp - uart_rcb.outp);
		size = uart_rcb.cap;
	}
	else
	{
		uart_rcb.cap = (UART_RCV_BUF_LEN + uart_rcb.inp - uart_rcb.outp);
		size = UART_RCV_BUF_LEN - uart_rcb.outp;
	}
	return size;
}

UINT8 * get_uart_rcv_buf_ptr_lock()
{
	uart_rcb.lock = 1;
	return (uart_rcb.outp + uart_rcb.buf);
}

void free_uart_rcv_buf_lock(size_t len)
{
	uart_rcb.outp += len;
	if(uart_rcb.outp>=UART_RCV_BUF_LEN)
		uart_rcb.outp -= UART_RCV_BUF_LEN;
	uart_rcb.lock = 0;
}

//extern void udelay(unsigned long usecs);
int uart_read(UINT8 * buf, size_t len)
{
	size_t cpy_len = len;
	size_t temp_len;
	size_t cpy_pos = 0;
	while(cpy_len)
	{
		if(0!=get_uart_rcv_data_len_in_buf())
		{
			if(uart_rcb.inp>=uart_rcb.outp)
				temp_len = (uart_rcb.inp - uart_rcb.outp);
			else
				temp_len = UART_RCV_BUF_LEN - uart_rcb.outp;
			temp_len = min(temp_len, cpy_len);
#ifdef CK610_CACHE
			memcpy(buf+cpy_pos, (UINT32)(uart_rcb.buf + uart_rcb.outp) + 0x20000000, temp_len);
#else
			Dcache_invalidate(uart_rcb.buf + uart_rcb.outp, temp_len);
			memcpy(buf+cpy_pos, uart_rcb.buf + uart_rcb.outp, temp_len);
#endif
			#if 0
			//sie_printf("cpy %x %x %d %d\n", buf, uart_rcb.buf + uart_rcb.outp, temp_len, uart_rcb.cap);
			int i;
			for(i=0; i<temp_len; i++)
			{
				sie_printf("%x ", buf[i]);
			}
			sie_printf("\n");
			#endif
			cpy_pos += temp_len;
			uart_rcb.outp += temp_len;
			if(uart_rcb.outp >= UART_RCV_BUF_LEN)
				uart_rcb.outp -= UART_RCV_BUF_LEN;
			uart_rcb.cap -= temp_len;
			cpy_len -= temp_len;
		}
		else
		{
			//usleep(2*1000);
			msleep(2);
		}
	}
}

/*=========================================
 * for test
 =========================================*/
#if 0
#define UART_MAX_TX_BUF_LEN	4096
UINT8 uart_tx_buf[UART_MAX_TX_BUF_LEN];
UINT8 uart_rx_buf[UART_MAX_TX_BUF_LEN];
volatile int uart_receive_ok = 0;
int uart_receive_callback(void)
{
	uart_receive_ok = 1;
	return 0;
}

/*
int uart_read(UINT8 * buf, size_t len)
{
	request_sDMA rx_request;
	apDMA_sLLI rx_LLI;
	rx_request.ch = 6;
	rx_request.dma_dir = DMA_DIR_UART2M;
	rx_request.dma_LLI = &rx_LLI;
	rx_request.callback =NULL;
	rx_request.width = DMA_TS_WIDTH_8BIT;
	rx_LLI.NumTransfers = len;
	rx_LLI.pDstAddr = buf;
	rx_LLI.pSrcAddr = &URAT0_DR;
	rx_LLI.pNextLLI = NULL;
	return request_dma_transfer(&rx_request);
	//dma_request_sync(&rx_request);
}*/

int test_hs_uart(void)
{
	request_sDMA rx_request;
	apDMA_sLLI rx_LLI;
	int i;
	for(i=0; i<4096; i++)
	{
		uart_tx_buf[i] = i;
	}
	memset(uart_rx_buf, 0, 4096);
	init_uart(921600*4, 0);
	rx_request.ch = DMA_DIR_UART2M;
	rx_request.dma_dir = DMA_DIR_UART2M;
	rx_request.dma_LLI = &rx_LLI;
	rx_request.callback = uart_receive_callback;
	rx_request.width = DMA_TS_WIDTH_8BIT;
	rx_request.int_enable = 1;
	rx_request.lli_num = 1;
	rx_request.lli_num_int = 1;
	rx_request.use_once = 1;
	rx_LLI.NumTransfers = 4095;
	rx_LLI.pDstAddr = uart_rx_buf;
	rx_LLI.pSrcAddr = &URAT0_DR;
	rx_LLI.pNextLLI = NULL;
	request_dma_transfer(&rx_request);
	//uart_transmit(uart_tx_buf, 4095);
	for(i=0; i<4095/16; i++)
	{
		uart_transmit(uart_tx_buf+ i*16, 16);
		PRINT_INFO(("cnt=%d\n", i*16));
		timer_wait_ms(200);
	}
	uart_transmit(uart_tx_buf+ 4080, 15);
	//wait_dma_done(6);
	PRINT_INFO(("uart %d\n", uart_receive_ok));
	while(uart_receive_ok==0);
	Dcache_invalidate(uart_rx_buf, 4095);
	for(i=0; i<4095; i++)
	{
		if(uart_rx_buf[i]!=uart_tx_buf[i])
		{
			PRINT_INFO(("uart dma err\n"));
			while(1);
			return -1;
		}
	}
	PRINT_INFO(("uart dma ok\n"));
	while(1);
	return 0;
}


void temp_test()
{
	int cnt=0;
	int i;
	enable_interrupts();
	init_bt_uart(57600, 0);
	for(i=0; i<4096; i++)
	{
		uart_tx_buf[i] = i;
	}
	while(cnt++<10)
	{
		uart_transmit(uart_tx_buf, 256);
		timer_wait_ms(1000);
	}
	PRINT_INFO(("test end %d\n", tx_cnt));
}

#endif

#endif /*__SUPPORT_HS_UART__*/
 
