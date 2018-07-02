#ifndef _COMMON_H
#define _COMMON_H

#define DEV_DRV
//#define DEV_DSP
//#define DEV_RISC

#ifdef DEV_DRV
#define LOGD printk
#define LOGE printk

#elif defined(DEV_DSP)
#include "log/printf.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#define LOGD sl_printf
#define LOGE sl_printf

#elif defined(DEV_RISC)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

//#define DSP_DATA_BUF_SIZE   1024
//#define DSP_IN_BUF_SIZE     786432 //512*1536
//#define DSP_OUT_BUF_SIZE    122880
//#define DSP_TOTAL_BUF_SIZE  0x200000//(DSP_DATA_BUF_SIZE + DSP_IN_BUF_SIZE + DSP_OUT_BUF_SIZE)

typedef enum {
    DSP_ERROR_NONE = 0,
    DSP_ERROR_CMD = -1,
    DSP_ERROR_CXC = -2,
    DSP_ERROR_DATA = -3,
    DSP_ERROR_DECODE = -4,
    DSP_ERROR_DEVICE = -5,
}ERROR_TYPE;

typedef enum {
    DSP_CMD_START = 0,
    DSP_CMD_PLAY,
    DSP_CMD_STOP,
    DSP_CMD_FLUSH,
    DSP_CMD_RELOAD,
    DSP_CMD_END,
}CMD_TYPE;

typedef enum {
    DSP_IDLE = 0,
    DSP_DECODE_ERROR,
    DSP_DATA_END,
    DSP_DECODE_END,
    DSP_PLAY_END,
}DECODE_STATUS;

typedef enum {
    AUDIO_UNKNOWN,
    AUDIO_MP3,
    AUDIO_ALAC,
    AUDIO_AAC,
    AUDIO_WMA,
    AUDIO_FLAC,
    AUDIO_APE,
    AUDIO_AC3,
    AUDIO_DTS,
    AUDIO_WAV,
    AUDIO_SPDAC3,
    AUDIO_SPDDTS,
    AUDIO_MAX
}AUDIO_TYPE;

typedef struct {
    int sample_rate;
    int channels;
    int bps;
}AUDIO_FORMAT;

typedef struct {
    AUDIO_TYPE audio_type;
    DECODE_STATUS status;
    AUDIO_FORMAT inAudioFormat;
    AUDIO_FORMAT outAudioFormat;
    AUDIO_FORMAT reqAudioFormat;
    int bit_rate;
    int cur_frame_index;
    int duration;
    int first_frame_offset;
    int framelen;
    int nframes;
    int channel_layout;
    int file_size;
}AUDIO_INFO;

struct mbcmd
{
    int cmd;
};

struct mbcmd_st
{
    int ret;
    int cmd;
};

struct dsp_buf_s
{
    int mbv_start;
    int dbv_start;
    int dbv_size;
    int abv_size;
    int pcm_size;
    int abv_wrptr;
    int abv_rdptr;
    int pcm_wrptr;
    int pcm_rdptr;
};

#ifdef DEV_RISC
typedef struct __alacinfo
{
    int channels;
    int bitpersample;
    int samplerate;
    int codecdata_len;
    int frames;
    int *framesize_table;
    void *codecdata;

    int *offset_table;
    int num_offset_byte_sizes;
    int totaltime;
    //int internal_bps;
    //int request_bps;
    //int request_channels;
}ALACINFO_S;

#elif defined(DEV_DSP)

/* Audio channel masks */
#define AV_CH_FRONT_LEFT             0x00000001
#define AV_CH_FRONT_RIGHT            0x00000002
#define AV_CH_FRONT_CENTER           0x00000004
#define AV_CH_LOW_FREQUENCY          0x00000008
#define AV_CH_BACK_LEFT              0x00000010
#define AV_CH_BACK_RIGHT             0x00000020
#define AV_CH_FRONT_LEFT_OF_CENTER   0x00000040
#define AV_CH_FRONT_RIGHT_OF_CENTER  0x00000080
#define AV_CH_BACK_CENTER            0x00000100
#define AV_CH_SIDE_LEFT              0x00000200
#define AV_CH_SIDE_RIGHT             0x00000400
#define AV_CH_TOP_CENTER             0x00000800
#define AV_CH_TOP_FRONT_LEFT         0x00001000
#define AV_CH_TOP_FRONT_CENTER       0x00002000
#define AV_CH_TOP_FRONT_RIGHT        0x00004000
#define AV_CH_TOP_BACK_LEFT          0x00008000
#define AV_CH_TOP_BACK_CENTER        0x00010000
#define AV_CH_TOP_BACK_RIGHT         0x00020000
#define AV_CH_STEREO_LEFT            0x20000000  ///< Stereo downmix.
#define AV_CH_STEREO_RIGHT           0x40000000  ///< See AV_CH_STEREO_LEFT.

/** Channel mask value used for AVCodecContext.request_channel_layout
    to indicate that the user requests the channel order of the decoder output
    to be the native codec channel order. */
#define AV_CH_LAYOUT_NATIVE          0x8000000000000000LL

/* Audio channel convenience macros */
#define AV_CH_LAYOUT_MONO            (AV_CH_FRONT_CENTER)
#define AV_CH_LAYOUT_STEREO          (AV_CH_FRONT_LEFT|AV_CH_FRONT_RIGHT)
#define AV_CH_LAYOUT_2_1             (AV_CH_LAYOUT_STEREO|AV_CH_BACK_CENTER)
#define AV_CH_LAYOUT_SURROUND        (AV_CH_LAYOUT_STEREO|AV_CH_FRONT_CENTER)
#define AV_CH_LAYOUT_4POINT0         (AV_CH_LAYOUT_SURROUND|AV_CH_BACK_CENTER)
#define AV_CH_LAYOUT_2_2             (AV_CH_LAYOUT_STEREO|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT)
#define AV_CH_LAYOUT_QUAD            (AV_CH_LAYOUT_STEREO|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT)
#define AV_CH_LAYOUT_5POINT0         (AV_CH_LAYOUT_SURROUND|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT)
#define AV_CH_LAYOUT_5POINT1         (AV_CH_LAYOUT_5POINT0|AV_CH_LOW_FREQUENCY)
#define AV_CH_LAYOUT_5POINT0_BACK    (AV_CH_LAYOUT_SURROUND|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT)
#define AV_CH_LAYOUT_5POINT1_BACK    (AV_CH_LAYOUT_5POINT0_BACK|AV_CH_LOW_FREQUENCY)
#define AV_CH_LAYOUT_7POINT0         (AV_CH_LAYOUT_5POINT0|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT)
#define AV_CH_LAYOUT_7POINT1         (AV_CH_LAYOUT_5POINT1|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT)
#define AV_CH_LAYOUT_7POINT1_WIDE    (AV_CH_LAYOUT_5POINT1_BACK|AV_CH_FRONT_LEFT_OF_CENTER|AV_CH_FRONT_RIGHT_OF_CENTER)
#define AV_CH_LAYOUT_STEREO_DOWNMIX  (AV_CH_STEREO_LEFT|AV_CH_STEREO_RIGHT)

#define DSP_ASSERT(cond) do {                                          \
    if (!(cond)) {                                                      \
        dsp_log_err("Assertion %s failed at %s:%d\n",    				\
               		 #cond, __FILE__, __LINE__);                        \
        abort();                                                        \
    }                                                                   \
} while (0)

#define DSP_FREE(x) if (NULL != (x)) { free((x)); (x) = NULL;}

static inline unsigned int dsp_read8(unsigned char * buff)
{
    return *buff;
}

static inline unsigned int dsp_read16L_ring(unsigned char * buff, int dt)
{
    unsigned int tmp = dsp_read8(buff);
    if(dt <= 1)
    {
        dt = dt + 1 - dsp_in_ringbuf_size();
        buff = buff + 1 - dsp_in_ringbuf_size();
    }
    else
    {
        dt -= 1;
        buff += 1;
    }
    tmp += dsp_read8(buff) << 8;
    return tmp;
}

static inline unsigned int dsp_read16L(unsigned char * buff)
{
    unsigned int tmp = dsp_read8(buff);
    tmp += dsp_read8(buff+1) << 8;
    return tmp;
}

static inline unsigned int dsp_read32L_ring(unsigned char * buff, int dt)
{
    unsigned int tmp = dsp_read16L_ring(buff, dt);
    if(dt <= 2)
    {
        dt = dt + 2 - dsp_in_ringbuf_size();
        buff = buff + 2 - dsp_in_ringbuf_size();
    }
    else
    {
        dt -= 2;
        buff += 2;
    }
    tmp +=  dsp_read16L_ring(buff,dt)<<16;
    return tmp;
}

static inline unsigned int dsp_read32L(unsigned char * buff)
{
    unsigned int tmp = dsp_read16L(buff);
    tmp +=  dsp_read16L(buff+2)<<16;
    return tmp;
}

static inline unsigned int dsp_read64L_ring(unsigned char * buff, int dt)
{
    unsigned long long tmp = dsp_read32L_ring(buff, dt);
    if(dt <= 4)
    {
        dt = dt + 4 - dsp_in_ringbuf_size();
        buff = buff + 4 - dsp_in_ringbuf_size();
    }
    else
    {
        dt -= 4;
        buff += 4;
    }
    tmp += ((unsigned long long)(dsp_read32L_ring(buff, dt)))<<32;
    return tmp;
}

static inline unsigned int dsp_read64L(unsigned char * buff)
{
    unsigned long long tmp = dsp_read32L(buff);
    tmp += ((unsigned long long)(dsp_read32L(buff+4)))<<32;
    return tmp;
}

static inline unsigned int dsp_read16_ring(unsigned char * buff, int dt)
{
    unsigned int tmp = dsp_read8(buff) << 8;
    if(dt <= 1)
    {
        dt = dt + 1 - dsp_in_ringbuf_size();
        buff = buff + 1 - dsp_in_ringbuf_size();
    }
    else
    {
        dt -= 1;
        buff += 1;
    }
    tmp += dsp_read8(buff);
    return tmp;
}

static inline unsigned int dsp_read16(unsigned char * buff)
{
    unsigned int tmp = dsp_read8(buff) << 8;
    tmp += dsp_read8(buff+1);
    return tmp;
}

static inline unsigned int dsp_read24_ring(unsigned char * buff, int dt)
{
    unsigned int tmp = dsp_read16_ring(buff, dt)<<16;
    if(dt <= 2)
    {
        dt = dt + 2 - dsp_in_ringbuf_size();
        buff = buff + 2 - dsp_in_ringbuf_size();
    }
    else
    {
        dt -= 2;
        buff += 2;
    }
    tmp += dsp_read8(buff);
    return tmp;
}

static inline unsigned int dsp_read24(unsigned char * buff)
{
    unsigned int tmp = dsp_read16(buff)<<16;
    tmp += dsp_read8(buff+2);
    return tmp;
}

static inline unsigned int dsp_read32_ring(unsigned char * buff, int dt)
{
    unsigned int tmp = dsp_read16_ring(buff, dt)<<16;
    if(dt <= 2)
    {
        dt = dt + 2 - dsp_in_ringbuf_size();
        buff = buff + 2 - dsp_in_ringbuf_size();
    }
    else
    {
        dt -= 2;
        buff += 2;
    }
    tmp +=  dsp_read16_ring(buff, dt);
    return tmp;
}

static inline unsigned int dsp_read32(unsigned char * buff)
{
    unsigned int tmp = dsp_read16(buff)<<16;
    tmp +=  dsp_read16(buff+2);
    return tmp;
}

static inline unsigned long long dsp_read64_ring(unsigned char * buff, int dt)
{
    unsigned long long tmp = ((unsigned long long)dsp_read32_ring(buff, dt))<<32;
    if(dt <= 4)
    {
        dt = dt + 4 - dsp_in_ringbuf_size();
        buff = buff + 4 - dsp_in_ringbuf_size();
    }
    else
    {
        dt -= 4;
        buff += 4;
    }
    tmp += ((unsigned long long)(dsp_read32_ring(buff, dt)));
    return tmp;
}

static inline unsigned long long dsp_read64(unsigned char * buff)
{
    unsigned long long tmp = ((unsigned long long)dsp_read32(buff))<<32;
    tmp += ((unsigned long long)(dsp_read32(buff+4)));
    return tmp;
}

#if 0
static inline unsigned int dsp_get_size(unsigned char * buff, unsigned int len)
{
    unsigned int v = 0;
    while (len--)
        v = (v << 7) + (dsp_read8(buff++) & 0x7F);
    return v;
}
#endif

static inline int dsp_tagcmp(const void *g1, const void *g2)
{
    if ((NULL == g1) || (NULL == g2))
        return -1;

    return memcmp(g1, g2, 4);
}

#define DSP_MIN(x,y) (x)>(y)?(y):(x)
#define DSP_MAX(x,y) (x)<(y)?(y):(x)

#endif

#endif

