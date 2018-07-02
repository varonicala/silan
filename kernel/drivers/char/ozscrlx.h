/*
 *   OZSCR PCMCIA SmartCardBus Reader Driver Kernel module for 2.6 kernel
 *   Copyright (C) 2005-2006 O2Micro Inc.

 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.

 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *  O2Micro Inc., hereby disclaims all copyright interest in the
 *  library ozscrlx.ko written by Jeffrey Dai

 *    Module:       ozscrlx.h
 *    Author:       O2Micro Inc.,
 */

#ifndef OzSCRLx_H
#define OzSCRLx_H

/* Linux interface */

#include <linux/ioctl.h>

struct ozscr_apdu
{
   unsigned char    Command[4];
   unsigned short   LengthExpected;
   unsigned long    LengthIn;
   unsigned char    DataIn[300];
   unsigned long    LengthOut;
   unsigned char    DataOut[300];
   unsigned short   Status;
};

/* ioctl's for the OZSCR smartcardbus reader */
#define OZSCR_RESET         _IO('g', 0x01)  /* Reset CT */
#define OZSCR_PWROFF        _IO('g', 0x02)
#define OZSCR_STNDBY        _IO('g', 0x03)
#define OZSCR_OPEN          _IOR('g', 0x04, struct ozscr_apdu) /* Request ICC */
#define OZSCR_CLOSE         _IO('g', 0x05)  /* Eject ICC */
#define OZSCR_SELECT        _IO('g', 0x06)
#define OZSCR_STATUS        _IO('g', 0x07) /* Get Status */
#define OZSCR_RAM           _IOR('g', 0x08, READER_EXTENSION)
#define OZSCR_CMD           _IOWR('g', 0x09, struct ozscr_apdu)
#define OZSCR_PROTOCOL      _IOR('g', 0x10, struct ozscr_apdu)


/* Hardware information */

#define PTS_MAX_LEN                     128

#define SYSCLK                          33 /*Mhz*/
#define FRQ_DIV                         0x4

#define O2CLK                           (SYSCLK * 1000 )/(FRQ_DIV*2)
#define DELAY_RESET_O2_REG              1
#define O2_EXE_COMPLETE_POLLING_PERIOD  1
#define EXE_CMD_TIME_OUT_CNT            1000
#define MAX_T1_BLOCK_SIZE               270

/*Make sure the following delay is long enough
  Since we are doing polling mode for all cmd execution
  Some command may take longer for some reason.*/
#define CMD_MAX_DELAY EXE_CMD_TIME_OUT_CNT*50

/*Configuration registers:
  Note: Memory Mapping Registers
  Address Register    Function    Use    Read/Write    Size in byte*/
#define MANUAL_E_R      0x00
#define FRQ_MODE        0x02
#define MODE            0x04
#define CARD_MODE       0x06
#define PROTO           0x08
#define ETU_INI         0x0A
#define ETU_WRK         0x0C
#define CGT             0X0E
#define CWT_MSB         0x10
#define CWT_LSB         0x12
#define BWT_MSB         0x14
#define BWT_LSB         0x16
#define CLK_CNT         0x18
#define ETU_CNT         0x1A
#define MASK_IT         0x1C
#define FIFO_LEV        0x1E
#define EXE             0x20
#define STATUS_IT       0x22
#define DEVAL_IT        0x24
#define STATUS_EXCH     0x26
#define FIFO_NB         0x28

#define MANUAL          0x08
#define ATR_TO          0x10
#define EDC             0x40
#define CRD_DET         0x80
#define AS_SY           0x80
#define CP              0x80
#define RC              0x40
#define RC_Dur          0x20
#define EDC_TYP         0x10
#define SCP_MSK         0x80
#define SCI_MSK         0x40
#define IT_REC_MSK      0x02
#define POF_EXE         0x8000
#define PON_EXE         0x4000
#define RST_EXE         0x2000
#define EXCH_EXE        0x1000
#define CHG_ETU_EXE     0x0800
#define PTS_EXE         0x0400
#define S_TOC_EXE       0x0200
#define RESET_EXE       0x0100
#define CLK_SLEEP_EXE   0x0080
#define CLK_WAKE_EXE    0x0040
#define RST_FIFO_EXE    0x0020
#define SCP             0x80
#define SCI             0x40
#define IT_REC          0x02
#define END_EXE         0x01
#define END_EXE_CLR_B   0x01
#define BAD_TS          0x8000
#define BAD_PB          0x4000
#define ERR_PAR         0x2000
#define ERR_EXE         0x1000
#define TOC             0x0800
#define TOB             0x0400
#define TOR             0x0200
#define CRD_INS         0x0080
#define CRD_ON          0x0040
#define FIFO_FULL       0x0002
#define FIFO_EMPTY      0x0001
#define END_OF_CONFG_REG 0x2A

/*O2 Internal Register*/
#define O2_POWER_DELAY_REG 0XC4

#define VAL            0x08
#define RDY            0x04    /*RDYMOD pin of PC33560*/
#define PWR            0x02    /*PWRON pin of PC33560*/
#define CSM            0x01    /*CS pin of PC33560*/

/* XXX should be removed and once the boolean type is part of the kernel, 2.6.20 ?*/
/* Define BOOLEAN parameters */
typedef unsigned char    BOOLEAN;
#define TRUE  1
#define FALSE 0

/* PROTO REGISTER */
#define    PROTO_CP                    0x80
#define    PROTO_RC                    0x40
#define    PROTO_RC_DUR                0x20
#define    PROTO_EDC_TYP               0x10
#define    SCARD_PROTOCOL_T0           0x00
#define    SCARD_PROTOCOL_T1           0x01
#define    SCARD_PROTOCOL_RAW          0x03

/* T=1 checksum */
#define    SC_T1_CHECKSUM_LRC          0x00
#define    SC_T1_CHECKSUM_CRC          0x01

#define    SC_GENERAL_SHORT_DATA_SIZE  256
#define    SC_T1_MAX_BLKLEN            3+SC_GENERAL_SHORT_DATA_SIZE+2+2
#define    SC_T1_MAX_SBLKLEN           3+1+2

/* S-Block parameter */
#define    SC_T1_S_RESYNCH            0x00
#define    SC_T1_S_IFS                0x01
#define    SC_T1_S_ABORT              0x02
#define    SC_T1_S_WTX                0x03
#define    SC_T1_S_REQUEST            0x00
#define    SC_T1_S_RESPONSE           0x01
#define    SC_T1_S_IFS_MAX            0xFE

/* R-Block parameter */
#define    SC_T1_R_OK                 0x00
#define    SC_T1_R_EDC_ERROR          0x01
#define    SC_T1_R_OTHER_ERROR        0x02

/* APDU Casees */
/*
 * Case 1: lc=0, le=0
 * Case 2 Short: lc=0, le<=256
 * Case 3 Short: lc<=255, le=0
 * Case 4 Short: lc<=255, le<=256
 * Case 2 Extended: lc=0, le<=65536
 * Case 3 Extended: lc<=65535, le=0
 * Case 4 Extended: lc<=65535, le<=65536
 *
 * T=0: Case 4 becomes Case 3
 */
#define    SC_APDU_CASE_NONE           0
#define    SC_APDU_CASE_1              1
#define    SC_APDU_CASE_2_SHORT        2
#define    SC_APDU_CASE_3_SHORT        3
#define    SC_APDU_CASE_4_SHORT        4
#define    SC_APDU_CASE_2_EXT          5
#define    SC_APDU_CASE_3_EXT          6
#define    SC_APDU_CASE_4_EXT          7

/**********************************************************************
    The usage of the PSCR_REGISTERS struct is a little bit tricky:
    We set the address of that stucture to the IO Base Port, then
    the other reg's can be accessed by their address.
    p.E.    &PscrRegs = 0x320 --> &PscrRegs->CmdStatusReg = 0x321

    I/O registers :
**********************************************************************/
typedef struct _PSCR_REGISTERS {
    /*00 0000  MANUAL_IN  Apply on Scard contacts Synchronous cards    R/W    1*/
    u8  MANUAL_IN;
    u8 x1;

    /*00 0010  MANUAL_OUT Smart card contacts     Synchronous cards    R    1*/
    u8  MANUAL_OUT;
    u8 x2; /*x.. is a dummy*/

    /*00 0100  FIFO_IN    Enter data in FIFO            W    1*/
    u8  FIFO_IN;
    u8 x3; 

    /*00 0011  FIFO_OUT   Read data from FIFO            R    1*/
    u8  FIFO_OUT;
    u8 x4; /*x.. is a dummy*/

    /*00 0100  XOR_REG    Exclusive-or of data received in FIFO    R    1*/
    u8  XOR_REG;
    u8 x5; 

    /*00 0110 (msb)*/
    /*00 0111 (lsb)    CRC16 calculation of data received in FIFO    R    2*/
    u8  CRC16_MSB;
    u8 x7; 
    u8  CRC16_LSB;
    u8 x8; 

    /*00 1000  MOTO_CFG   MOTOROLA PC33560 manual configuration    R/W    1*/
    u8  MOTO_CFG;
}PSCR_REGISTERS;

typedef struct _T1DATA
{
    u8    nad;        /* NAD */
    u8    ns;        /* N(S) */
    u8    nr;        /* N(R) */
    u8    ifsc;        /* Information Field Size Card */
    u8    ifsd;        /* Information Field Size Device */
    BOOLEAN    ifsreq;        /* S(IFS Req) already sent? */
    unsigned long    cwt;        /* Character Waiting Time in etu -11 etu */
    unsigned long    bwt;        /* Block Waiting Time in us */
    u8    rc;        /* Redundancy Check (LRC/CRC) */
    u8    cse;

} T1_DATA;

typedef struct _SmartcardStatus
{
    u8     ATR[50];
    u16     Atr_len;
    u8     Protocol;
    u8     AvailableProtocol;
    u8     RqstProtocol;
    u8     WireProtocol;
    BOOLEAN  PowerOn;
    BOOLEAN  WakeUp;
    u16  AtrTA1, TA1, TA2;
    u16  TB1, TB2, TB3;
    u16  TC1, TC2, TC3;
    u16  IFSC; // XXX this is never used
    u8   EdcType;
    u16  Bwi;

} SMARTCARD;

typedef struct _SMARTCARD_DATA
{
    /* Buffer for received smart card data */
    u8 *    Buffer;

    /* Allocted size of this buffer */
    unsigned long     BufferSize; // XXX never used

    /* Number of bytes received from the card */
    unsigned long    BufferLength;

} SMARTCARD_DATA;

typedef struct _READER_EXTENSION 
{
    unsigned int  sci_base_addr;
    
    SMARTCARD     m_SCard;

    u8           bStatus_It;    // XXX never used
    u16          wStatus_Exch;    // XXX never used
    unsigned long       PowerTimeOut;    // XXX never used

    SMARTCARD_DATA      SmartcardReply;
    SMARTCARD_DATA      SmartcardRequest;

    /* Data for T=1 */
    T1_DATA        T1;

} READER_EXTENSION;

/* Exit codes */
#define    SC_EXIT_OK                   0
#define    SC_EXIT_UNKNOWN_ERROR        1    /* Error code for everything */
#define    SC_EXIT_IO_ERROR             2
#define    SC_EXIT_NOT_SUPPORTED        3
#define    SC_EXIT_NO_ACK               4
#define    SC_EXIT_BAD_ATR              5
#define    SC_EXIT_PROBE_ERROR          6
#define    SC_EXIT_BAD_CHECKSUM         7
#define    SC_EXIT_TIMEOUT              8
#define    SC_EXIT_CARD_CHANGED         9
#define    SC_EXIT_NO_CARD              10
#define    SC_EXIT_NOT_IMPLEMENTED      11
#define    SC_EXIT_CMD_TOO_SHORT        12
#define    SC_EXIT_MALLOC_ERROR         13
#define    SC_EXIT_BAD_SW               14
#define    SC_EXIT_BAD_PARAM            15
#define    SC_EXIT_NO_SLOT              16
#define    SC_EXIT_LIB_ERROR            17
#define    SC_EXIT_PROTOCOL_ERROR       18
#define    SC_EXIT_LOCKED               19
#define    SC_EXIT_NO_MATCH             20
#define    SC_EXIT_CMD_TOO_LONG         21
#define    SC_EXIT_RSP_TOO_LONG         22
#define    SC_EXIT_RETRY                23

/* Define Return Status */
#define STATUS_SUCCESS                   0
#define STATUS_IO_DEVICE_ERROR           1
#define STATUS_BUFFER_TOO_SMALL          2
#define STATUS_UNSUCCESSFUL              3
#define STATUS_IO_TIMEOUT                4
#define STATUS_INVALID_DEVICE_REQUEST    5
#define STATUS_CRC_ERROR                 6

/* STATUS_EXCH REGISTER */
#define STATUS_EXCH_BAD_TS         0x8000
#define STATUS_EXCH_BAD_PB         0x4000
#define STATUS_EXCH_ERR_PAR        0x2000
#define STATUS_EXCH_ERR_EXE        0x1000
#define STATUS_EXCH_TOC            0x0800
#define STATUS_EXCH_TOB            0x0400
#define STATUS_EXCH_TOR            0x0200
#define STATUS_EXCH_CRD_INS        0x0080
#define STATUS_EXCH_CRD_ON         0x0040
#define STATUS_EXCH_CRD_ACT        0x0020
#define STATUS_EXCH_CRD_SLEEP      0x0010
#define STATUS_EXCH_TR1            0x0008
#define STATUS_EXCH_FIFO_FULL      0x0002
#define STATUS_EXCH_FIFO_EMPTY     0x0001



/* Error Constants */

/*------------------------------------------------------------------------------
   Error codes between ICC and IFD (II).
      GE_II_COMM      (-100)                                                (13)
         Communication is not possible between ICC and IFD.
      GE_II_PARITY    (-101)                                                (A3)
         Character parity error between ICC and IFD.
      GE_II_EDC       (-102)
         Error detection code raised.

      GE_II_ATR       (-110) 
         Incoherence in ATR.
      GE_II_ATR_TS    (-111)                                                (10)
         Bad TS in ATR.
      GE_II_ATR_TCK   (-112)                                                (1D)
         Bad TCK in ATR.
      GE_II_ATR_READ  (-113)                                                (03)
         Impossible to read some ATR bytes.
      GE_II_PROTOCOL  (-120)
         Inconsistent protocol.
      GE_II_UNKNOWN   (-121)                                                (17)
         Unknown protocol.
      GE_II_PTS       (-122)                                                (18)
         PTS is required for this choice.
      GE_II_IFSD_LEN  (-123)
         Received block length > IFSD (T=1 only).
      GE_II_PROC_BYTE (-124)                                                (E4)
         Bad procedure byte from ICC (T=0 only).
      GE_II_INS       (-125)                                                (11)
         Bad INS in a command (6X or 9X) (T=0 only).
      GE_II_RES_LEN   (-126)
         Message length from ICC not supported.
      GE_II_RESYNCH   (-127)
         3 failures ! Resynch required by ICC.
------------------------------------------------------------------------------*/
#define GE_II_COMM             (-100)
#define GE_II_PARITY           (-101)
#define GE_II_EDC              (-102)
#define GE_II_ATR              (-110)

#define GE_II_ATR_PARM         (-110) 
#define GE_II_ATR_TS           (-111)
#define GE_II_ATR_TCK          (-112)
#define GE_II_ATR_READ         (-113)
#define GE_II_PROTOCOL         (-120)
#define GE_II_UNKNOWN          (-121)
#define GE_II_PTS              (-122)
#define GE_II_IFSD_LEN         (-123)
#define GE_II_PROC_BYTE        (-124)
#define GE_II_INS              (-125)
#define GE_II_RES_LEN          (-126)
#define GE_II_RESYNCH          (-127)

#endif /*OzSCRLx_H*/
