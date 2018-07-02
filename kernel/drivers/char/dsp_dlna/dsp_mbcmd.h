#ifndef _DSP_MBCMD_H_
#define _DSP_MBCMD_H_

#include "dsp_io.h"
#include "dsp_pp.h"

#define DSP_NOERR			0
#define DSP_FRAME_ERR		1
#define DSP_ARG_ERR			2

#define TASK_ID_FREE		0xff
#define TASK_ID_ANON		0xfe

#define DSP_SYNC_CODE		0xff
#define MIPS_TO_DSP			0
#define DSP_TO_MIPS			1

typedef enum
{
	MBOX_CMD_TADD = 0,
	MBOX_CMD_TDEL,
	MBOX_CMD_TCFG,
	MBOX_CMD_TSTOP,

	MBOX_CMD_BKSEND,
	MBOX_CMD_BKREQ,
	MBOX_CMD_BKSEND_ASYNC,
	MBOX_CMD_PP,

	MBOX_CMD_CODEC_TYPE,
	MBOX_CMD_CODEC_CFG,
	MBOX_CMD_CODEC_START,
	MBOX_CMD_CODEC_STOP,
	MBOX_CMD_CODEC_PAUSE,
	MBOX_CMD_CODEC_INFO,

	MBOX_CMD_SYSTEM_ATTR,

	MBOX_DSP_RUNLEVEL,

	MBOX_DSP_DEBUG,

	MBOX_DSP_POLL,
	
	MBOX_DSP_SUSPEND,
	MBOX_DSP_RESUME,
#ifdef CONFIG_MIPS_SILAN_DLNA
	MBOX_DSP_DLNA_RING_BUFF_CFG,
	MBOX_DSP_DLNA_SET_FILE_SIZE,
	MBOX_DSP_DLNA_RING_BUFF_RESET,
	MBOX_DSP_DLNA_DECODE_SUSPEND,
	MBOX_DSP_DLNA_DECODE_RESUME,
	MBOX_DSP_DLNA_SET_SEEK_FLAG,
	MBOX_DSP_DLNA_DECODE_STOP,
	MBOX_DSP_DLNA_DECODE_START,
	MBOX_DSP_DLNA_SET_AIFF,
#endif
    MBOX_DSP_PRINT_STAT_ENABLE,
    MBOX_DSP_PRINT_STAT_DISABLE,
    
	MBOX_CMD_MAX,
}MBCMD_TYPE;

typedef struct 
{
	u32 data;
	u32 addr;
	u32 size;
}DSP_BUF;

typedef struct _DSP_BKIO_TYPE
{
	DSP_BUF in_buf;
	u32 in_size;
	DSP_BUF out_buf;
	u32 out_size;
	u32 used_size;	
	u32 out_pcm_size;
}DSP_BKIO_TYPE;

typedef struct _DSP_READ_FIFO_ATTR
{
	dma_addr_t addr;
	u32 size;
}DSP_READ_FIFO;

#ifdef CONFIG_MIPS_SILAN_DLNA
/*
---------------------------------------------------------------------------------------------------
|sys exchange   | 1 | 2 | 3 | 4 | 	      IN				|  OUT(0)  | OUT(1)  |...     |OUT(N)  |
----------------------------------------------------------------------------------------------------
								^	   						^
								|			    			|		
1:in_r_index (32 bits, it is the offset in the buff)
2:in_w_index (32 bits, it is the offset in the buff)
3:out_r_index (32 bits, it is the offset in the buff)
4:out_w_index (32 bits, it is the offset in the buff)

*/
typedef struct _DSP_DLNA_RING_BUFF_CFG
{
	u8 *phy_base_addr;
	u8 *vir_base_addr; // vir_base_addr + DSP_MMAP_AREA1_OFFSET
	u32 size;
	
	//sys exchange size
	u32 sys_exchange_size;
    
	//bar size
    u32 index_size;
    
	//in ring buff info
	u32 in_buff_size;

	//out buff info
	u32 out_buff_frame_num;
	u32 out_buff_frame_size;
}DSP_DLNA_RING_BUFF_CFG_S;

typedef struct _DSP_DLNA_FILE_SIZE
{
	u32 file_size;
}DSP_DLNA_FILE_SIZE_S;

typedef struct _AIFFCtx
{
	s32 channels;
	s32 sample_rate;
	s32 bit_rate;
	s32 block_align;
	s32 block_duration;
	s32 data_size;
	s32 offset;
	s32 duration;
	s32 version;
	u32 nb_frames;
	u32 codec_id;       
}AIFFCTX_S;

#endif

struct mbcmd
{
	u32 sync;
	u32 seq;
	u32 cmd;
	u32 taskid;
	union
	{
		DSP_CODEC_TYPE 		 codec;
		DSP_BKIO_TYPE		 io;
		DSP_READ_FIFO 		 read_fifo;
		DSP_CFG_INFO		 cfg;
		SLAUD_PostProcess_t	 pp;
#ifdef CONFIG_MIPS_SILAN_DLNA
		DSP_DLNA_RING_BUFF_CFG_S dlna_ring_buff_cfg;
		DSP_DLNA_FILE_SIZE_S dlna_file_size;
        AIFFCTX_S aiff_ctx;
#endif
	}u;
};

typedef struct _BKIO_INFO
{
	u32 used_size;	
	u32 out_pcm_size;
}BKIO_INFO;

#define DSP2MIPS_FIFO_FULL	(1<<0)

typedef struct
{
	int channels;
	int rate;
	int bitpersample;
} DSP_AUDIO_INFO;

struct mbcmd_st
{
	int errno;
	u32 cmd;
	u32 taskid;
	union
	{
		BKIO_INFO io_info;
		DSP_AUDIO_INFO au_info;
	}u;
	int flags; 
	int framelen;
};
/*
struct mbcmd_st
{
	int errno;
	u32 cmd;
	union
	{
		TASKID taskid;
		BKIO_INFO io_info;
	}u;
	int flags; 
};
*/
#endif

