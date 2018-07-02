#ifndef _DSP_IO_H_
#define _DSP_IO_H_

#include "dsp_pp.h"

typedef enum
{
	DSP_CMD_TADD = 0x100,
	DSP_CMD_TDEL,
	DSP_CMD_TCFG,
	DSP_CMD_TSTOP,
	DSP_CMD_CLOSE,

	DSP_CMD_BKSEND = 0x200,
	DSP_CMD_BKREQ,
	DSP_CMD_BKSEND_ASYNC,
	DSP_CMD_PP,

	DSP_CMD_CODEC_TYPE = 0x300,
	DSP_CMD_CODEC_CFG,
	DSP_CMD_CODEC_START,
	DSP_CMD_CODEC_STOP,
	DSP_CMD_CODEC_PAUSE,
	DSP_CMD_CODEC_INFO,

	DSP_CMD_SYSTEM_ATTR = 0x400,

	DSP_CMD_RUNLEVEL = 0x500,

	DSP_CMD_DEBUG = 0x600,

	DSP_CMD_POLL = 0x700,

	DSP_CMD_FLUSH_BUF = 0x800,

	DSP_CMD_SUSPEND,
	DSP_CMD_RESUME,

	DSP_LOAD_FIRMWARE = 0x900,
#ifdef CONFIG_MIPS_SILAN_DLNA
	DSP_CMD_DLNA_RING_BUFF_CFG = 0xa00,
	DSP_CMD_DLNA_SET_FILE_SIZE,
	DSP_CMD_DLNA_RING_BUFF_RESET,
	DSP_CMD_DLNA_DECODE_SUSPEND,
	DSP_CMD_DLNA_DECODE_RESUME,
    DSP_CMD_DLNA_SET_SEEK_FLAG,
    DSP_CMD_DLNA_RING_BUFF_FREE,
    DSP_CMD_DLNA_DECODE_STOP,
    DSP_CMD_DLNA_DECODE_START,
    DSP_CMD_DLNA_SET_AIFF_PARAM,
	DSP_CMD_DLNA_SET_CUR_USER,
	DSP_CMD_DLNA_GET_CUR_USER,

	DSP_CMD_DLNA_GET_IN_RIDX_INFO,
    DSP_CMD_DLNA_GET_IN_WIDX_INFO,
    DSP_CMD_DLNA_GET_OUT_RIDX_INFO,
    DSP_CMD_DLNA_GET_OUT_WIDX_INFO,
    DSP_CMD_DLNA_SET_IN_RIDX_INFO,
    DSP_CMD_DLNA_SET_IN_WIDX_INFO,
    DSP_CMD_DLNA_SET_OUT_RIDX_INFO,
    DSP_CMD_DLNA_SET_OUT_WIDX_INFO,
    DSP_CMD_DLNA_SET_END_FLAG,
    DSP_CMD_DLNA_GET_DECODE_STATE,
    DSP_CMD_DLNA_CLEAR_DECODE_STATE,
#endif
    DSP_CMD_PRINT_STAT_ENABLE = 0xb00,
    DSP_CMD_PRINT_STAT_DISABLE,

}DSPCMD;

typedef enum
{
	DSP_CODEC_PCM = 0,
	DSP_CODEC_MP3,
	DSP_CODEC_AAC,
	DSP_CODEC_AC3,
	DSP_CODEC_WMA,
	DSP_CODEC_DST,
	DSP_CODEC_FLAC,
	DSP_CODEC_COOK,
	DSP_CODEC_OGG,
	DSP_CODEC_APE,
	DSP_CODEC_FDIP,
	DSP_CODEC_VTK,
	DSP_CODEC_HOT_FDIP,
	DSP_CODEC_MAX,
}DSP_CODEC_TYPE;

typedef struct _DSP_AC3_INFO
{
	int lfe;
	int mode;
	int numchans;
}DSP_CFG_AC3;

typedef struct _DSP_PCM_INFO
{
	int channels;
	int rate;
	int bitpersample;
}DSP_CFG_PCM;

typedef struct _DSP_WMA_INFO
{
	int packet_size;
	int codec_id;
	int channels;
	int rate;
	int bitrate;
	int blockalign;
	int bitpersample;
	int datalen;
	int numpackets;
	char data[46];
}DSP_CFG_WMA;

typedef struct _DSP_FLAC_INFO
{
	int channels;
	int rate;
	int bitpersample;
}DSP_CFG_FLAC;

typedef struct _DSP_COOK_INFO
{
	int channels;
	int rate;
	int bitrate;
	int bitpersample;
	int blockalign;
	int datalen;
	char data[16];
}DSP_CFG_COOK;

typedef struct _DSP_VTK_INFO
{
    int   width;
    int   height;
}DSP_VTK_FLAC;

typedef struct _DSP_HOT_CFG
{
    u32    width;
    u32    height;
}DSP_HOT_CFG;

typedef struct _DSP_CFG_INFO
{
	u32 taskid;
	DSP_CODEC_TYPE codec_type;
	union
	{
		DSP_CFG_PCM pcm_cfg;
		DSP_CFG_WMA wma_cfg;
		DSP_CFG_FLAC flac_cfg;
		DSP_CFG_COOK cook_cfg;
		DSP_VTK_FLAC vtk_cfg;
		DSP_HOT_CFG fdif_cfg;
	}para;
}DSP_CFG_INFO;

typedef struct taskID
{
	u32 id;
}TASKID;

typedef struct BKIO
{
	u32 taskid;
	u8* in_addr;
	u32 in_size;
	u8* out_addr;
	u32 out_size;
	u32 used_size;
	u32 out_pcm_size;
	u8* remain_addr;
	u32 remain_len;
}BKIO_TYPE;

typedef struct _DECEXT
{
	u32 flag;
}DEC_EXT;

typedef struct _CODECCFG
{
	u32 taskid;
	DSP_CODEC_TYPE codec_type;
	union
	{
		DSP_CFG_PCM  pcm_cfg;
		DSP_CFG_WMA  wma_cfg;
		DSP_CFG_FLAC flac_cfg;
		DSP_CFG_COOK cook_cfg;
		DSP_VTK_FLAC vtk_cfg;
		DSP_HOT_CFG fdif_cfg;
	}u;
}CODEC_CFG;

typedef struct _CODECPP
{
	u32 taskid;
	SLAUD_PostProcess_t pp;
}CODEC_PP;

typedef struct _CODECTYPE
{
	u32 id;
	DSP_CODEC_TYPE codec;
}CODEC_TYPE;

typedef struct _CODECSTART
{
	u32 id;
}CODEC_START;

typedef struct _CODECSTOP
{
	u32 id;
}CODEC_STOP;

typedef struct _CODECPAUSE
{
	u32 id;
}CODEC_PAUSE;

typedef struct _CODECINFO
{
	u32 id;
	u32 channels;
	u32 rate;
	u32 bitpersample;
	u32 framelen;
}CODEC_INFO;
#endif

