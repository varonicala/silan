#ifndef SPI_CTRL_H 
#define SPI_CTRL_H

#define SPI_SR      0x80
#define SPI_CR      0x84
#define SPI_TR_CNT  0x88
#define SPI_TR_DMA  0x90
#define SPI_RR_DMA  0x94
#define SPI_TR_PIO  0x98
#define SPI_RR_PIO  0x9c

/*SPI Status Register Bit*/
#define	SPI_TFIFO_FULL      (0x1<<26)
#define	SPI_TFIFO_EMPTY     (0x1<<25)
#define	SPI_TFIFO_HALF      (0x1<<24)
#define	SPI_RFIFO_FULL      (0x1<<18)
#define	SPI_RFIFO_EMPTY     (0x1<<17)
#define	SPI_RFIFO_HALF      (0x1<<16)
#define SPI_CNT_DONE        (0x1<<15)
#define SPISR_DONE          (0x1<<7)
#define SPISR_SPIERR        (0x1<<6)
#define SPISR_BUSY          (0x1<<5)
#define SPISR_INT_N         (0x1<<4)
#define SPISR_XMIT_EMPTY    (0x1<<3)
#define SPISR_RCV_FULL      (0x1<<2)

/*SPI Control Register Bit*/
#define	SPICR_DMA_TX_EN         (0x1<<31)
#define	SPICR_DMA_RX_EN	        (0x1<<30)
#define	SPICR_FIFO_TX_DISABLE   (0x1<<29)
#define	SPICR_FIFO_RX_DISABLE   (0x1<<28)
#define	SPICR_FIFO_RST          (0x1<<27)
#define	SPICR_CS_SEL(a)         ((a)<<24)
#define	SPICR_TC                (0x1<<21)
#define	SPICR_BIT_NUM(a)        ((a)<<15)
#define	SPICR_WR_EN             (0x1<<14)
#define	SPICR_RD_EN             (0x1<<13)
#define	SPICR_CLK_OE            (0x1<<12)
#define	SPICR_CLK_EN            (0x1<<11)
#define	SPICR_DIV(a)            (((a&0x4)<<6)|((a&0x3)<<3))
#define SPICR_SPIEN             (0x1<<7)
#define SPICR_INTEN             (0x1<<6)
#define SPICR_START             (0x1<<5)


#define SPI_TRMODE_WR       0x3
#define SPI_TRMODE_RONLY    0x1
#define SPI_TRMODE_WONLY    0x2
#define SPI_TRMODE_DISABLE  0x0

#endif















