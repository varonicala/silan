#ifndef _UART_BUF_H_
#define _UART_BUF_H_

typedef struct
{
	//UINT16	baudrate;
	UINT32	inp;
	UINT32	outp;
	UINT32	cap;
	UINT8 * buf;
	UINT8	lock;
}uart_rcb_t;

#define UART_RCV_BLOCKS		(2)
#define UART_RCV_BLOCK_LEN	(3*1024)
#define UART_RCV_BUF_LEN	(UART_RCV_BLOCKS*UART_RCV_BLOCK_LEN)
#define UART_TRAN_BUF_LEN	(4096)

#endif /*_UART_BUF_H_*/

