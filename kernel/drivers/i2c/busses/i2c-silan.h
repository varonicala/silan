#ifndef _SILAN_I2C_H_
#define _SILAN_I2C_H_

#define WRDATA				0x01
#define MULWRDATA			0x02
#define RDDATA				0x03

/*******TI2C***************/
#define CPU_PCLK(x)			((x)<<0)
#define SCL_DUTY_RATIO(x)	((x)<<16)   //scl low/scl high
#define HD_DAT_CNT			(1<<20)     //hold time
/*******MBCR***********/
#define MBCR_MASK(x)		(x<<0)
#define MBCR_RSTA			(1<<16)		//repeat start
#define MBCR_TXAK			(1<<17)		//enable ack
#define MBCR_MTX_TX			(1<<18)		//master transmit flag
#define MBCR_MTX_RX			(0<<18)
#define MBCR_MSTA			(1<<19)		//Master/Slave mode select ,1:master 0:slave
#define MBCR_MIEN			(1<<20)		//interrupt enable
#define MBCR_MEN			(1<<21)		//i2c core enable bit
#define MBCR_MBEN			(1<<22)		//mult-byte transmit enable
#define MBCR_GCD			(1<<23)		//generate call address disable, 0:enable 1: disable

/*********MBNR**** *********/
#define MBNR_SINGLE			(1<<0)
#define MBNR_MULT			(1<<1)

/**********MBSR***************/
#define MBSR_ARBITR			(1<<0)	//Arbitration is lost(only for master)
#define MBSR_NOACK			(1<<3)
#define MBSR_MINT			(1<<4)		//Master transmitter has completed current transaction

#define MBSR_W_CLEAR		(1<<4)|(1<<24)
#define MBSR_R_CLEAR		(1<<5)|(1<<24)

#define ADDR_SEND_NR(x)		((x)>>0x18)
#define WRITE_DONE_BIT		(0x1<<0x4)
#define I2C_BUSY			(0x1<<0x13)
#define	NO_START			(0x1<<0x11)
#define BIT_COMPLETE(x)		((x)<<24)

#define CLK_EN_REG		0xbfba9000
#define I2C_EN_CLK		(0x1<<30)

#define CLEAR_INT_BIT		 0xFFFF
#define MAX_NUM_SEND_ONCE 	 0x10
#define FIFO_LEN				 4 
#define RECV_LAST_BYTES(x)   ((0x1<<8)|(x))
#define READ_FIFO_FULL		 0x1<<5
#define READ_HAVE_DATA  	 0x1<<5

#define START_CONFIG		MBCR_MASK(0xFFFF)|MBCR_MTX_TX|MBCR_MSTA|MBCR_MIEN|MBCR_MEN|MBCR_MBEN	
#define RESTART_CONFIG 		MBCR_MASK(0xFFFF)|MBCR_RSTA|MBCR_MIEN|MBCR_MEN|MBCR_MBEN|MBCR_MSTA
#define STOP_CONFIG			MBCR_MASK(0xFFFF)|MBCR_TXAK|MBCR_MIEN|MBCR_MEN|MBCR_MBEN

enum silan_i2c_state 
{
	STATE_IDLE,
	STATE_START,
	STATE_READ,
	STATE_WRITE,
	STATE_MULT_WRITE,
	STATE_STOP
};

typedef struct  _I2C_REG
{
	 volatile u32 TI2C;
	 volatile u32 MADR;
	 volatile u32 MBCR;
	 volatile u32 MBSR;
	 volatile u32 MBNR;
	 volatile u32 MBDR0;
	 volatile u32 MBDR1;
	 volatile u32 MBDR2;
	 volatile u32 MBDR3;
	 volatile u32 MBMR;
	 volatile u32 MBRR0;
	 volatile u32 MBRR1;
	 volatile u32 MBRR2;
	 volatile u32 MBRR3;
}I2CREG;

#endif
