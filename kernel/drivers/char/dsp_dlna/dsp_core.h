#ifndef _DSP_CORE_H_
#define _DSP_CORE_H_
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_def.h>
#include <silan_gpio.h>
#include <silan_reset.h>
#include <linux/dma-mapping.h>


#include "ringbuffer.h"
#include "dsp_io.h"

#define DSP_INDATA_SIZE 0x20000
#ifndef CONFIG_MIPS_SILAN_DLNA	
#define DSP_R_RINGBUFFER_SIZE 0x100000
#define DSP_W_RINGBUFFER_SIZE 0x100000
#else
#define DSP_R_RINGBUFFER_SIZE 0x10
#define DSP_W_RINGBUFFER_SIZE 0x8
#endif
#define DSP_FILTER_NUM		0x2

typedef struct
{
	int filternum;
	struct dspdev_filter *filter;
	int initialized;
	int users;
	struct mutex mutex;
	struct list_head	queue;
}dspdev_info_t;

enum dspdev_state 
{
	DSPDEV_STATE_FREE,
	DSPDEV_STATE_BUSY,
};

struct dspdev_filter
{
	enum dspdev_state state;
	struct mutex rmutex;
	struct mutex wmutex;
	struct dsp_ringbuffer rbuffer;
	struct dsp_ringbuffer wbuffer;
	struct timer_list timer;
	dspdev_info_t *pdspdev_info;

	u32 taskid;
	DSP_CODEC_TYPE type;
	struct list_head	queue;
	u32 rdma_addr;
	u32 wdma_addr;
	u32 frame_sync;
	u8* sysex;
	u8* bufstart;
	u32 update;
};

#endif

