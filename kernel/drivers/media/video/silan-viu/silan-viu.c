/*
 * Virtual Video driver - This code emulates a real video device with v4l2 api
 *
 * Copyright (c) 2006 by:
 *   
 *      http://v4l.videotechnology.com/
 *
 *      Conversion to videobuf2 by Pawel Osciak & Marek Szyprowski
 *      Copyright (c) 2010 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the BSD Licence, GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-common.h>
#include "viuapi.h"
#include "viu-export.h"
#include "viu.h"
#include "regdefine.h"

#define MODULE_NAME "silan-vidin"

/* Wake up at about 30 fps */
#define WAKE_NUMERATOR 30
#define WAKE_DENOMINATOR 1001
#define BUFFER_TIMEOUT     msecs_to_jiffies(500)  /* 0.5 seconds */


#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080
#define MAX_FRAME_SIZE (MAX_WIDTH*MAX_HEIGHT*3/2)
#define VIU_MAJOR_VERSION 0
#define VIU_MINOR_VERSION 8
#define VIU_RELEASE 0
#define VIU_VERSION \
	KERNEL_VERSION(VIU_MAJOR_VERSION, VIU_MINOR_VERSION, VIU_RELEASE)

//#define CAP_WIDTH 640
#define CAP_WIDTH 720
#define CAP_HEIGHT 480

MODULE_DESCRIPTION("SILAN Video Capture Board");
MODULE_AUTHOR("SILAN");
MODULE_LICENSE("Dual BSD/GPL");

static unsigned video_nr = -1;
module_param(video_nr, uint, 0644);
MODULE_PARM_DESC(video_nr, "videoX start number, -1 is autodetect");

static unsigned n_devs = 1;
module_param(n_devs, uint, 0644);
MODULE_PARM_DESC(n_devs, "number of video devices to create");

//static unsigned debug=0xffffffff;//print all messages
static unsigned debug=0;//no print 
module_param(debug, uint, 0644);
MODULE_PARM_DESC(debug, "activates debug info");

static unsigned int vid_limit = 16;
module_param(vid_limit, uint, 0644);
MODULE_PARM_DESC(vid_limit, "capture memory limit in megabytes");
#define dprintk(dev, level, fmt, arg...) v4l2_dbg(level, debug, &dev->v4l2_dev, fmt, ## arg)


//#define VIU_DBG

#ifdef VIU_DBG

#define VIU_printk(fmt, arg...) ;printk(fmt, ## arg);    

#else

#define VIU_printk(fmt, arg...) ;

#endif

viu_dev *g_dev_info;

/* ------------------------------------------------------------------
	Basic structures
   ------------------------------------------------------------------*/

struct viu_fmt {
	char  *name;
	u32   fourcc;          /* v4l2 format id */
	int   depth;
	VIU_StoreMode_e storeFormat; 
};


/* Bars and Colors should match positions */

enum colors {
	WHITE,
	AMBER,
	CYAN,
	GREEN,
	MAGENTA,
	RED,
	BLUE,
	BLACK,
	TEXT_BLACK,
};

/* R   G   B */
#define COLOR_WHITE	{204, 204, 204}
#define COLOR_AMBER	{208, 208,   0}
#define COLOR_CYAN	{  0, 206, 206}
#define	COLOR_GREEN	{  0, 239,   0}
#define COLOR_MAGENTA	{239,   0, 239}
#define COLOR_RED	{205,   0,   0}
#define COLOR_BLUE	{  0,   0, 255}
#define COLOR_BLACK	{  0,   0,   0}

struct bar_std {
	u8 bar[9][3];
};

/* Maximum number of bars are 10 - otherwise, the input print code
   should be modified */
static struct bar_std bars[] = {
	{	/* Standard ITU-R color bar sequence */
		{ COLOR_WHITE, COLOR_AMBER, COLOR_CYAN, COLOR_GREEN,
		  COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_BLACK, COLOR_BLACK }
	}, {
		{ COLOR_WHITE, COLOR_AMBER, COLOR_BLACK, COLOR_WHITE,
		  COLOR_AMBER, COLOR_BLACK, COLOR_WHITE, COLOR_AMBER, COLOR_BLACK }
	}, {
		{ COLOR_WHITE, COLOR_CYAN, COLOR_BLACK, COLOR_WHITE,
		  COLOR_CYAN, COLOR_BLACK, COLOR_WHITE, COLOR_CYAN, COLOR_BLACK }
	}, {
		{ COLOR_WHITE, COLOR_GREEN, COLOR_BLACK, COLOR_WHITE,
		  COLOR_GREEN, COLOR_BLACK, COLOR_WHITE, COLOR_GREEN, COLOR_BLACK }
	},
};

#define NUM_INPUTS ARRAY_SIZE(bars)


static struct viu_fmt formats[] = {

	/*TODO: add viu support formats*/
	//planar 420
	{
		.name = "Y/Cb/Cr 4:2:0",
		.fourcc = V4L2_PIX_FMT_YUV420,
		.depth = 12,
		.storeFormat =Mode_Planar,
	},
#if 0
#if 0	
	/* two planes -- one Y, one Cr + Cb interleaved  */
#define V4L2_PIX_FMT_NV12    v4l2_fourcc('N', 'V', '1', '2') /* 12  Y/CbCr 4:2:0  */
#endif
	//semi-planar 420
	{
		.name = "Y/CbCr 4:2:0",
		.fourcc =V4L2_PIX_FMT_NV12,
		.depth = 12,
		.storeFormat =Mode_SemiPlanar,
	},
	{
		.name     = "4:2:2, packed, YUYV",
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.depth    = 16,
		.storeFormat =Mode_Package,
	},
	//package
	{
		.name     = "4:2:2, packed, UYVY",
		.fourcc   = V4L2_PIX_FMT_UYVY,
		.depth    = 16,
		.storeFormat =Mode_Package,
	},
#endif
#if 0
	{
		.name     = "RGB565 (LE)",
		.fourcc   = V4L2_PIX_FMT_RGB565, /* gggbbbbb rrrrrggg */
		.depth    = 16,
	},
	{
		.name     = "RGB565 (BE)",
		.fourcc   = V4L2_PIX_FMT_RGB565X, /* rrrrrggg gggbbbbb */
		.depth    = 16,
	},
	{
		.name     = "RGB555 (LE)",
		.fourcc   = V4L2_PIX_FMT_RGB555, /* gggbbbbb arrrrrgg */
		.depth    = 16,
	},
	{
		.name     = "RGB555 (BE)",
		.fourcc   = V4L2_PIX_FMT_RGB555X, /* arrrrrgg gggbbbbb */
		.depth    = 16,
	},
#endif
};

static const struct v4l2_queryctrl no_ctl = {
	.name  = "42",
	.flags = V4L2_CTRL_FLAG_DISABLED,
};
static const struct v4l2_queryctrl bttv_ctls[] = {
	/* --- video --- */
	{
		.id            = V4L2_CID_BRIGHTNESS,
		.name          = "Brightness",
		.minimum       = 0,
		.maximum       = 65535,
		.step          = 256,
		.default_value = 32768,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
#if 0
	{
		.id            = V4L2_CID_CONTRAST,
		.name          = "Contrast",
		.minimum       = 0,
		.maximum       = 65535,
		.step          = 128,
		.default_value = 32768,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},{
		.id            = V4L2_CID_SATURATION,
		.name          = "Saturation",
		.minimum       = 0,
		.maximum       = 65535,
		.step          = 128,
		.default_value = 32768,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},{
		.id            = V4L2_CID_HUE,
		.name          = "Hue",
		.minimum       = 0,
		.maximum       = 65535,
		.step          = 256,
		.default_value = 32768,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
#endif
	/* --- private --- */
	{
		.id            = V4L2_CID_PRIVATE_SET_CAP_MODE,
		.name          = "CapMode",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
	{
		.id            = V4L2_CID_PRIVATE_SET_STORE_FORMAT,
		.name          = "StoreFormat",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
	{
		.id            = V4L2_CID_PRIVATE_OPEN_CLK,
		.name          = "openclk",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
	{
		.id            = V4L2_CID_PRIVATE_CLOSE_CLK,
		.name          = "closeclk",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},

	{
		.id            = V4L2_CID_PRIVATE_INFO_MASK,
		.name          = "mask",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
#if 0
	{
		.id            = V4L2_CID_PRIVATE_INFO_SCALE,
		.name          = "scale",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
#endif
	{
		.id            = V4L2_CID_PRIVATE_INFO_H_V_OFF,
		.name          = "h_v_off",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
	{
		.id            = V4L2_CID_PRIVATE_INFO_BT656_REF_NUM,//jizhun ma
		.name          = "bt656_ref_num",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
#if 0
	{
		.id            = V4L2_CID_PRIVATE_INFO_CROP,
		.name          = "crop",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
	{
		.id            = V4L2_CID_PRIVATE_CONFIG_REGS,
		.name          = "config_regs",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	},
#endif

	{
		.id            =V4L2_CID_PRIVATE_INFO_RESOLUTION,
		.name          = "Resolution",
		.minimum       = 0,
		.maximum       = 0XFFFFFFFF,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	}


};

static const struct v4l2_queryctrl *ctrl_by_id(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bttv_ctls); i++)
		if (bttv_ctls[i].id == id)
			return bttv_ctls+i;

	return NULL;
}


/*************************/
static struct viu_fmt *get_format(struct v4l2_format *f)
{
	struct viu_fmt *fmt;
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(formats); k++) {
		fmt = &formats[k];
		if (fmt->fourcc == f->fmt.pix.pixelformat)
			break;
	}

	if (k == ARRAY_SIZE(formats))
		return NULL;

	return &formats[k];
}



/* ------------------------------------------------------------------
	Videobuf operations
   ------------------------------------------------------------------*/
static int queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				unsigned int *nplanes, unsigned long sizes[],
				void *alloc_ctxs[])
{
	VIU_handle_t *handle = vb2_get_drv_priv(vq);
	viu_dev *dev =(viu_dev *)g_dev_info;
	unsigned long size;


	//size = (handle->frameSize);
	size = (unsigned int)(handle->actual_width * handle->actual_height * handle->fmt->depth/ 8);
	//size +=2048;
	//size =(size/4096 +1)*4096;



#if 0

	size = dev->width * dev->height * 2;

	if (0 == *nbuffers)
		*nbuffers = 32;

	while (size * *nbuffers > vid_limit * 1024 * 1024)
		(*nbuffers)--;
#endif
	printk("*nbuffers in queue_setup:%d\n", (unsigned int)(*nbuffers));
	//*nbuffers = V4L2_DEF_BUFFERS;

	*nplanes = 1;

	sizes[0] = size;

	/*
	 * videobuf2-vmalloc allocator is context-less so no need to set
	 * alloc_ctxs array.
	 */

	dprintk(dev, 1, "%s, count=%d, size=%ld\n", __func__,
			*nbuffers, size);

	return 0;
}

static int buffer_init(struct vb2_buffer *vb)
{
	VIU_handle_t *handle = vb2_get_drv_priv(vb->vb2_queue);
	//viu_dev *dev = g_dev_info;
	//todo yuliubing
	BUG_ON(NULL == handle->fmt);

	/*
	 * This callback is called once per buffer, after its allocation.
	 *
	 * Vivi does not allow changing format during streaming, but it is
	 * possible to do so when streaming is paused (i.e. in streamoff state).
	 * Buffers however are not freed when going into streamoff and so
	 * buffer size verification has to be done in buffer_prepare, on each
	 * qbuf.
	 * It would be best to move verification code here to buf_init and
	 * s_fmt though.
	 */

	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	VIU_handle_t *handle = vb2_get_drv_priv(vb->vb2_queue);
	viu_dev *dev = g_dev_info;
	struct viu_buffer *buf = container_of(vb, struct viu_buffer, vb);
	unsigned long size;

	dprintk(dev, 1, "%s, field=%d\n", __func__, vb->v4l2_buf.field);
	//todo yuliubing
	BUG_ON(NULL == handle->fmt);

	/*
	 * Theses properties only change when queue is idle, see s_fmt.
	 * The below checks should not be performed here, on each
	 * buffer_prepare (i.e. on each qbuf). Most of the code in this function
	 * should thus be moved to buffer_init and s_fmt.
	 */
	if (handle->width  < 48 || handle->width  > MAX_WIDTH ||
			handle->height < 32 || handle->height > MAX_HEIGHT)
		return -EINVAL;

	size = (unsigned int)(handle->actual_width * handle->actual_height * handle->fmt->depth/ 8);
	//size = handle->frameSize;
	handle->frameSize = size;
	if (vb2_plane_size(vb, 0) < size) {
		dprintk(dev, 1, "%s data will not fit into plane (%lu < %lu)\n",
				__func__, vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	vb2_set_plane_payload(&buf->vb, 0, size);

	buf->fmt = handle->fmt;

	return 0;
}

static int buffer_finish(struct vb2_buffer *vb)
{
#if 0
	VIU_handle_t *handle = vb2_get_drv_priv(vb->vb2_queue);
	viu_dev *dev = g_dev_info;
	dprintk(dev, 1, "%s\n", __func__);
#endif
	return 0;
}

static void buffer_cleanup(struct vb2_buffer *vb)
{
#if 0
	struct viu_dev *dev = g_dev_info;
	VIU_handle_t *handle = vb2_get_drv_priv(vb->vb2_queue);
	dprintk(dev, 1, "%s\n", __func__);
#endif

}

static void buffer_queue(struct vb2_buffer *vb)
{
	viu_dev *dev = g_dev_info;
	VIU_handle_t *handle = vb2_get_drv_priv(vb->vb2_queue);
	struct viu_buffer *buf = container_of(vb, struct viu_buffer, vb);
	struct viu_dmaqueue *vidq = &handle->dma_q;
	dprintk(dev, 1, "%s\n", __func__);
	//spin_lock_irqsave(&dev->slock, flags);
	list_add_tail(&buf->list, &vidq->active);
	//spin_unlock_irqrestore(&dev->slock, flags);
}

static int viu_start_generating(VIU_handle_t *handle)
{
	handle->stop_capture = 0;
printk("************viu_start_generating\n");
#if 0
	VIU_Reset();
#endif
	VIU_OpenCLK();//you need to open clk before config registers
	//config registers
	//VIU_ConfigCapMode(handle->capMode,handle);
	VIU_ConfigCapMode(0,handle);
	//VIU_StartCapture(handle->capMode);
	VIU_StartCapture(0);

	return 0;
}


//todo
static void viu_stop_generating(VIU_handle_t *handle)
{
	struct viu_dmaqueue *dma_q = &handle->dma_q;

	handle->stop_capture = 1;
	//	VIU_StopCapture(handle->capMode);
	VIU_Pause(handle);//wait cc and stopCapture

	/*
	 * Typical driver might need to wait here until dma engine stops.
	 * In this case we can abort imiedetly, so it's just a noop.
	 */

	/* Release all active buffers */
	while (!list_empty(&dma_q->active)) {
		struct viu_buffer *buf;
		buf = list_entry(dma_q->active.next, struct viu_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
		//	dprintk(dev, 2, "[%p/%d] done\n", buf, buf->vb.v4l2_buf.index);
	}
}
static int start_streaming(struct vb2_queue *vq)
{
	viu_dev *dev = g_dev_info;
	VIU_handle_t *handle = vb2_get_drv_priv(vq);
	VIU_printk("**********start_streaming\n");
	printk("**********start_streaming\n");
	dprintk(dev, 1, "%s\n", __func__);
	return viu_start_generating(handle);
}

/* abort streaming and wait for last buffer */
static int stop_streaming(struct vb2_queue *vq)
{
	viu_dev *dev = g_dev_info;
	VIU_handle_t *handle = vb2_get_drv_priv(vq);
	VIU_printk("**********stop_streaming\n");
	printk("**********stop_streaming\n");
	dprintk(dev, 1, "%s\n", __func__);
	viu_stop_generating(handle);
	return 0;
}
#if 1
static void viu_lock(struct vb2_queue *vq)
{
	VIU_handle_t *handle  = vb2_get_drv_priv(vq);
	mutex_lock(&handle->mutex);
}

static void viu_unlock(struct vb2_queue *vq)
{
	VIU_handle_t *handle = vb2_get_drv_priv(vq);
	mutex_unlock(&handle->mutex);
}
#endif

static struct vb2_ops viu_video_qops = {
	.queue_setup		= queue_setup,
	.buf_init		= buffer_init,
	.buf_prepare		= buffer_prepare,
	.buf_finish		= buffer_finish,
	.buf_cleanup		= buffer_cleanup,
	.buf_queue		= buffer_queue,
	.start_streaming	= start_streaming,
	.stop_streaming		= stop_streaming,
	.wait_prepare		= viu_unlock,
	.wait_finish		= viu_lock,
};

/* ------------------------------------------------------------------
	IOCTL vidioc handling
   ------------------------------------------------------------------*/
static int vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	viu_dev *dev = video_drvdata(file);

	strcpy(cap->driver, "viu");
	strcpy(cap->card, "viu");
	strlcpy(cap->bus_info, dev->v4l2_dev.name, sizeof(cap->bus_info));
	cap->version = VIU_VERSION;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | \
						V4L2_CAP_READWRITE;
	return 0;
}

static int vidioc_queryctrl(struct file *file, void *priv,
					struct v4l2_queryctrl *c)
{
	const struct v4l2_queryctrl *ctrl;

	if ((c->id <  V4L2_CID_BASE ||
				c->id >= V4L2_CID_LASTP1) &&
			(c->id <  V4L2_CID_PRIVATE_BASE ||
			 c->id >= V4L2_CID_PRIVATE_END))
		return -EINVAL;

	ctrl = ctrl_by_id(c->id);

	*c = (NULL != ctrl) ? *ctrl : no_ctl;

	return 0;
}
static int vidioc_g_ctrl(struct file *file, void *priv,
					struct v4l2_control *c)
{

	VIU_handle_t *handle = file->private_data;
	switch (c->id) {
		case V4L2_CID_BRIGHTNESS:
			//c->value = handle->bright;
			break;

		case V4L2_CID_PRIVATE_SET_CAP_MODE:
			c->value = handle->capMode;
			break;

		case V4L2_CID_PRIVATE_INFO_RESOLUTION:
			c->value=(handle->width<<16)|(handle->height);
			break;
//to complement other ctrl

		default:
			return -EINVAL;
	}
	return 0;
}
static int vidioc_s_ctrl(struct file *file, void *f,
					struct v4l2_control *c)
{
	VIU_handle_t *handle = file->private_data;


	switch (c->id) {
#if 0
		case V4L2_CID_BRIGHTNESS:
			//handle->bright = c->value
			//bt848_bright(btv, c->value);
			break;

		case V4L2_CID_PRIVATE_SET_CAP_MODE:
			handle->capMode = c->value;

			break;

		case V4L2_CID_PRIVATE_SET_STORE_FORMAT:
			handle->storeFormat = c->value;

			break;

		case V4L2_CID_PRIVATE_OPEN_CLK:
			//VIU_OpenCLK();
			//VIU_Restart(handle);
			break;

		case V4L2_CID_PRIVATE_CLOSE_CLK:
			//VIU_CloseCLK();
			//VIU_Pause(handle);
			break;
		case V4L2_CID_PRIVATE_INFO_MASK:

			if(copy_from_user(&handle->mask_info,(unsigned int *)c->value,sizeof(VIU_Mask_Info_t)))
			{
				return -EFAULT;
			}

			break;
		case V4L2_CID_PRIVATE_INFO_SYM:

			if(copy_from_user(&handle->sym_info,(unsigned int *)c->value,sizeof(VIU_Sym_Info_t)))
			{
				return -EFAULT;
			}

			break;

		case V4L2_CID_PRIVATE_INFO_H_V_OFF:

			if(copy_from_user(&handle->h_v_off_info,(unsigned int *)c->value,sizeof(VIU_H_V_OFF_Info_t)))
			{
				return -EFAULT;
			}

			break;
		case V4L2_CID_PRIVATE_INFO_BT656_REF_NUM:

			if(copy_from_user(&handle->bt656_ref_num_info,(unsigned int *)c->value,sizeof(VIU_BT656_REF_NUM_Info_t)))
			{
				return -EFAULT;
			}

			break;
#if 0
		case V4L2_CID_PRIVATE_INFO_SCALE:

			handle->scaleMode = c->value;
			printk("scaleMode:%d\n",handle->scaleMode);
			break;


		case V4L2_CID_PRIVATE_INFO_CROP:

			if(copy_from_user(&handle->crop_info,(unsigned int *)c->value,sizeof(VIU_Crop_Info_t)))
			{
				return -EFAULT;
			}
			printk("cap width:%d",handle->crop_info.width);
			printk("cap height:%d",handle->crop_info.height);
			//	VIU_ConfigCapMode(handle->capMode,handle);

			break;
#endif
		case V4L2_CID_PRIVATE_INFO_RESOLUTION:
			handle->width =((c->value)>>16)&0xffff; 
			handle->height =((c->value))&0xffff;

			handle->crop_info.startX = 0;
			handle->crop_info.startY = 0;
			handle->crop_info.width =handle->width;
			handle->crop_info.height =handle->height;

			printk("resolution width:%d",handle->width);
			printk("resolution heght:%d",handle->height);

			break;
#if 0
		case V4L2_CID_PRIVATE_CONFIG_REGS:

			VIU_ConfigCapMode(handle->capMode,handle);

			break;
#endif
#endif
		default:
			//return -EINVAL;
			break;
	}
	return 0;
}



static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	struct viu_fmt *fmt;

	if (f->index >= ARRAY_SIZE(formats))
		return -EINVAL;

	fmt = &formats[f->index];

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{

	VIU_handle_t *handle = file->private_data;
	f->fmt.pix.width        = handle->actual_width;
	f->fmt.pix.height       = handle->actual_height;
	f->fmt.pix.field        = handle->field;//todo
	f->fmt.pix.pixelformat  = handle->fmt->fourcc;
	f->fmt.pix.bytesperline =
		(f->fmt.pix.width * handle->fmt->depth) >> 3;
	f->fmt.pix.sizeimage =
		f->fmt.pix.height * f->fmt.pix.bytesperline;
	return 0;


}
#if 1
static int vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	struct viu_dev *dev = video_drvdata(file);
	struct viu_fmt *fmt;
	enum v4l2_field field;

	fmt = get_format(f);
	if (!fmt) {
	//	dprintk(dev, 1, "Fourcc format (0x%08x) invalid.\n",
	//		f->fmt.pix.pixelformat);
		printk("Fourcc format (0x%08x) invalid.\n",
			f->fmt.pix.pixelformat);
		return -EINVAL;
	}

	field = f->fmt.pix.field;
#if 0
	if (field == V4L2_FIELD_ANY) {
		field = V4L2_FIELD_INTERLACED;
	} else if (V4L2_FIELD_INTERLACED != field) {
		dprintk(dev, 1, "Field type invalid.\n");
		return -EINVAL;
	}
#endif
	f->fmt.pix.field = field;
	//TODO
	v4l_bound_align_image(&f->fmt.pix.width, 48, MAX_WIDTH, 2,
			      &f->fmt.pix.height, 32, MAX_HEIGHT, 0, 0);
	f->fmt.pix.bytesperline =
		(f->fmt.pix.width * fmt->depth) >> 3;
	f->fmt.pix.sizeimage =
		f->fmt.pix.height * f->fmt.pix.bytesperline;
	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	int X_scale,Y_scale;
	int ret = vidioc_try_fmt_vid_cap(file, priv, f);
	if (ret < 0)
		return ret;
#if 0
	if (vb2_is_streaming(q)) {
		dprintk(dev, 1, "%s device busy\n", __func__);
		return -EBUSY;
	}
#endif
	handle->fmt = get_format(f);
	//FIXME
	handle->storeFormat = handle->fmt->storeFormat;
	printk("handle->storeFormat:%d\n",handle->storeFormat);
	handle->actual_width = f->fmt.pix.width;
	handle->actual_height = f->fmt.pix.height;
//	handle->actual_width = 720;
//	handle->actual_height =480;
	handle->field = f->fmt.pix.field;
	X_scale = handle->crop_info.width/handle->actual_width;
	Y_scale = handle->crop_info.height/handle->actual_height;
	printk("handle->actual_width:%d\n",handle->actual_width);
	printk("handle->actual_height:%d\n",handle->actual_height);

#if 0

	if(X_scale==1)
	{
		handle->actual_width =((handle->crop_info.width+7)&(~7) );
		handle->X_scaleMode =SCALE_NO;  

	}
	else if(X_scale==2)
	{
		handle->actual_width =((handle->crop_info.width/2+7)&(~7)); 
		handle->X_scaleMode =SCALE_1_2;  

	}
	else if(X_scale==3)
	{
		handle->actual_width =((handle->crop_info.width/4+7)&(~7)); 
		handle->X_scaleMode =SCALE_1_4;  

	}
	else if(X_scale==4)
	{
		handle->actual_width =((handle->crop_info.width/4+7)&(~7)); 
		handle->X_scaleMode =SCALE_1_4;  

	}
	else
	{

		handle->actual_width =((handle->crop_info.width/8+7)&(~7)); 
		handle->X_scaleMode =SCALE_1_8;  

	}
#else
	if(X_scale==1)
	{
		handle->actual_width =((handle->crop_info.width+15)&(~15) );
		handle->X_scaleMode =SCALE_NO;  

	}
	else if(X_scale==2)
	{
		handle->actual_width =((handle->crop_info.width/2+15)&(~15)); 
		handle->X_scaleMode =SCALE_1_2;  

	}
	else if(X_scale==3)
	{
		handle->actual_width =((handle->crop_info.width/4+15)&(~15)); 
		handle->X_scaleMode =SCALE_1_4;  

	}
	else if(X_scale==4)
	{
		handle->actual_width =((handle->crop_info.width/4+15)&(~15)); 
		handle->X_scaleMode =SCALE_1_4;  

	}
	else
	{

		handle->actual_width =((handle->crop_info.width/8+15)&(~15)); 
		handle->X_scaleMode =SCALE_1_8;  

	}

#endif
	if(Y_scale==1)
	{
		handle->actual_height =((handle->crop_info.height+1)&(~1)); 
		handle->Y_scaleMode =SCALE_NO;  

	}
	else if(Y_scale==2)
	{
		handle->actual_height =((handle->crop_info.height/2+1)&(~1)); 
		handle->Y_scaleMode =SCALE_1_2;  

	}
	else 
	{
		handle->actual_height =((handle->crop_info.height/2+1)&(~1)); 

		handle->Y_scaleMode =SCALE_1_2;  
	}

	handle->scaleMode = (handle->Y_scaleMode << 4) | handle->X_scaleMode;


	printk("handle->scaleMode:%d\n",handle->scaleMode);	
	return 0;
}
#endif

static int viu_cropcap(struct file *file, void *priv,
				struct v4l2_cropcap *cap)
{

	struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	if (cap->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
			cap->type != V4L2_BUF_TYPE_VIDEO_OVERLAY)
		return -EINVAL;

	//	*cap = bttv_tvnorms[btv->tvnorm].cropcap;

	return 0;
}

static int viu_g_crop(struct file *file, void *f, struct v4l2_crop *crop)
{
	struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;

	if (crop->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
			crop->type != V4L2_BUF_TYPE_VIDEO_OVERLAY)
		return -EINVAL;

	/* No fh->do_crop = 1; because btv->crop[1] may be
	   inconsistent with fh->width or fh->height and apps
	   do not expect a change here. */

	//	crop->c = btv->crop[!!fh->do_crop].rect;
	crop->c.top = handle ->crop_info.startY;
	crop->c.left = handle ->crop_info.startX;
	crop->c.width = handle ->crop_info.width;
	crop->c.height = handle ->crop_info.height;

	return 0;
}

static int viu_s_crop(struct file *file, void *f, struct v4l2_crop *crop)
{

	struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;

	handle ->crop_info.startY = crop->c.top;
	handle ->crop_info.startX = crop->c.left;
	handle ->crop_info.width = crop->c.width;
	handle ->crop_info.height = crop->c.height;
	return 0;
}
static int vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{
	//struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	return vb2_reqbufs(&handle->vb_q, p);
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	//struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	return vb2_querybuf(&handle->vb_q, p);
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
//	struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	return vb2_qbuf(&handle->vb_q, p);
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	//	struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	return vb2_dqbuf(&handle->vb_q, p, file->f_flags & O_NONBLOCK);
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	//	struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	return vb2_streamon(&handle->vb_q, i);
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	//	struct viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	return vb2_streamoff(&handle->vb_q, i);
}

//to complete yuliubing
#if 1
static int vidioc_s_std(struct file *file, void *priv, v4l2_std_id *i)
{
	return 0;
}

/* only one input in this sample driver */
static int vidioc_enum_input(struct file *file, void *priv,
				struct v4l2_input *inp)
{
	if (inp->index >= NUM_INPUTS)
		return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	inp->std = V4L2_STD_525_60;
	sprintf(inp->name, "Camera %u", inp->index);
	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	viu_dev *dev = video_drvdata(file);

	*i = dev->input;
	return 0;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	viu_dev *dev = video_drvdata(file);

	if (i >= NUM_INPUTS)
		return -EINVAL;

	dev->input = i;
	//precalculate_bars(dev);
	//precalculate_line(dev);
	return 0;
}
#endif

/* ------------------------------------------------------------------
	File operations for the device
   ------------------------------------------------------------------*/
static int viu_alloc_dma_buffer(struct lpvbits_t *vb)
{
	vb->base = (unsigned long)dma_alloc_coherent(NULL, PAGE_ALIGN(vb->size), &vb->phy_addr, GFP_DMA | GFP_KERNEL);
	if ((void *)(vb->base) == NULL) 
	{
		printk(KERN_ERR "[VIU] Physical memory allocation error size=%d\n", vb->size);
		return -1;
	}
	return 0;
}

static void viu_free_dma_buffer(struct lpvbits_t *vb)
{
	if (vb->base) 
	{
		dma_free_coherent(NULL, PAGE_ALIGN(vb->size), (void *)vb->base, vb->phy_addr);
	}
}

static ssize_t
viu_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;

	dprintk(dev, 1, "read called\n");
	return vb2_read(&handle->vb_q, data, count, ppos,
			file->f_flags & O_NONBLOCK);
}

static unsigned int
viu_poll(struct file *file, struct poll_table_struct *wait)
{
	viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	struct vb2_queue *q = &handle->vb_q;

	dprintk(dev, 1, "%s\n", __func__);
	return vb2_poll(q, file, wait);
}

static int viu_close(struct file *file)
{
	struct video_device  *vdev = video_devdata(file);
	viu_dev *dev = video_drvdata(file);
	VIU_handle_t *handle = file->private_data;
	int i;
	dprintk(dev, 1, "close called (dev=%s), file %p\n",
			video_device_node_name(vdev), file);
printk("viu_close\n");

	vb2_queue_release(&handle->vb_q);

	if(!handle->stop_capture)
		viu_stop_generating(handle);

	for(i = 0; i < VIU_DEF_BUFS; i++) 
		if(handle->lpvbits[i].base != 0 && handle->lpvbits[i].phy_addr != 0) {
			viu_free_dma_buffer(handle->lpvbits + i);
			handle->lpvbits[i].base = 0;
			handle->lpvbits[i].phy_addr = 0;
		}
	kfree(handle);
	return 0;
}

static int viu_mmap(struct file *file, struct vm_area_struct *vma)
{
	viu_dev *dev = video_drvdata(file);

	VIU_handle_t *handle = file->private_data;
	int ret;

	dprintk(dev, 1, "mmap called, vma=0x%08lx\n", (unsigned long)vma);
	printk(" lxz mmap called, vma=0x%08lx\n", (unsigned long)vma);

	ret = vb2_mmap(&handle->vb_q, vma);
	dprintk(dev, 1, "vma start=0x%08lx, size=%ld, ret=%d\n",
			(unsigned long)vma->vm_start,
			(unsigned long)vma->vm_end - (unsigned long)vma->vm_start,
			ret);
	return ret;
}

	
int v4l2_open(struct file *filp)
{     
	struct vb2_queue *q;
	int retval;
	int i;
	//struct video_device *vdev = video_devdata(filp);
	viu_dev *dev = video_drvdata(filp);
	VIU_handle_t *handle = kzalloc(sizeof(VIU_handle_t), GFP_KERNEL);
	dprintk(dev, 1, "%s\n", __func__);
	handle->fmt = &formats[0]; //need dynamic got TODO

	handle->storeFormat = handle->fmt->storeFormat;
	handle->fillBufferDone=1;

	/* initialize queue */
	q = &handle->vb_q;
	memset(q, 0, sizeof(struct vb2_queue));
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
	q->drv_priv = handle;
	q->buf_struct_size = sizeof(struct viu_buffer);
	q->ops = &viu_video_qops;
	q->mem_ops = &vb2_vmalloc_memops;

	vb2_queue_init(q);



	/* init video dma queues */
	INIT_LIST_HEAD(&handle->dma_q.active);

	/* start create system buffers */
	for(i = 0; i < VIU_DEF_BUFS; i++) {
		handle->lpvbits[i].base = 0;
		handle->lpvbits[i].phy_addr = 0;
		handle->lpvbits[i].size = MAX_FRAME_SIZE;
		//handle->lpvbits[i].size =(handle->width * handle->height * handle->fmt->depth/ 8) ;
		//yuliubing todo
		//retval = viu_alloc_dma_buffer(handle->lpvbits + i);
		retval = viu_alloc_dma_buffer(&(handle->lpvbits[i]));
		printk("handle->lpvbits[%d].phy_addr:%p\n",i,(void *)handle->lpvbits[i].phy_addr);
		if(retval) {
			printk(KERN_ERR KBUILD_MODNAME ":----------------- %s: out of memory!\n", __func__);
			goto err;
		}
	}


			handle->width =720;
			handle->height =480;
	handle->crop_info.startX = 0;
			handle->crop_info.startY = 0;
			handle->crop_info.width =CAP_WIDTH;
			handle->crop_info.height =CAP_HEIGHT;


	/* end create system buffers */
	filp->private_data = handle;




	//todo
	g_dev_info->handle[0]= handle;

	dprintk(dev, 1, "leave %s\n", __func__);
	return 0;
err:
	for(i = 0; i < VIU_DEF_BUFS; i++) {

		if(handle->lpvbits[i].base)
		{
			//viu_free_dma_buffer(handle->lpvbits + i);
			viu_free_dma_buffer(&(handle->lpvbits[i]));
		}

	}
	return -1;
}


static const struct v4l2_file_operations viu_fops = {
	.owner		= THIS_MODULE,
	.open		= v4l2_open,
	.release        = viu_close,
	.read           = viu_read,
	.poll		= viu_poll,
	.unlocked_ioctl = video_ioctl2, /* V4L2 ioctl handler */
	.mmap           = viu_mmap,
};

static const struct v4l2_ioctl_ops viu_ioctl_ops = {
	.vidioc_querycap      = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap  = vidioc_enum_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = vidioc_s_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = vidioc_g_fmt_vid_cap,
	.vidioc_reqbufs       = vidioc_reqbufs,
	.vidioc_querybuf      = vidioc_querybuf,
	.vidioc_qbuf          = vidioc_qbuf,
	.vidioc_dqbuf         = vidioc_dqbuf,
	#if 1
	.vidioc_s_std         = vidioc_s_std,
	.vidioc_enum_input    = vidioc_enum_input,
	.vidioc_g_input       = vidioc_g_input,
	.vidioc_s_input       = vidioc_s_input,
	#endif
	.vidioc_cropcap       = viu_cropcap,
	.vidioc_g_crop        = viu_g_crop,
	.vidioc_s_crop        = viu_s_crop,
	.vidioc_queryctrl     = vidioc_queryctrl,
	.vidioc_g_ctrl        = vidioc_g_ctrl,
	.vidioc_s_ctrl        = vidioc_s_ctrl,
	.vidioc_streamon      = vidioc_streamon,
	.vidioc_streamoff     = vidioc_streamoff,
};

static struct video_device viu_template = {
	.name		= "viu",
	.fops           = &viu_fops,
	.ioctl_ops 	= &viu_ioctl_ops,
	.release	= video_device_release,
//todo	
	.tvnorms              = V4L2_STD_525_60,
	.current_norm         = V4L2_STD_NTSC_M,
};




void fillbuffer_tasklet(unsigned long arg)
{
	VIU_handle_t *handle ;
	struct viu_dmaqueue *dma_q;

	struct viu_buffer *buf;
	char *vbuf;
	int frame_index;
	unsigned char *src;
	unsigned char *dst;
	unsigned int size;
	int i;
	viu_dev *dev = g_dev_info;
	//	handle= (VIU_handle_t *)arg;
	handle= dev->handle[0];

	dma_q = &handle->dma_q;

	frame_index = handle->current_frame;

	if(handle->stop_capture) {
		printk("cam b_acquire is zero!\n");
		goto err;
	}


	if (list_empty(&dma_q->active)) {
		dprintk(dev, 1, "No active queue to serve\n");
		goto err;
	}

	buf = list_entry(dma_q->active.next, struct viu_buffer, list);
	list_del(&buf->list);
//printk("get and fill buffer\n");
	vbuf = vb2_plane_vaddr(&buf->vb, 0);
	dprintk(dev, 1, "filled buffer %p\n", buf);
	//	printk("memcpy frame_index:%d vbuf:%p\n",frame_index,vbuf);
//printk("handle->frameSize:%d\n",handle->frameSize);
		//memcpy(vbuf, (void *)handle->lpvbits[frame_index].base,handle->frameSize);
#if 0
		memcpy(vbuf, (void *)handle->lpvbits[frame_index].base,720*480*5/4);
		memset(vbuf+720*480*5/4, 0x00,720*480/4);
#endif
#if 1
#if 1
	if(handle->storeFormat==Mode_Planar)
	{

		//planar case
		src =(unsigned char *)( handle->lpvbits[frame_index].base);
		dst = vbuf;
		size =handle->y_width_store*handle->height_store;
		memcpy(dst,src,size);
		dst +=size;
		src +=size;
		//cb copy
		for(i=0;i<handle->height_store/2;i++)
		{

			memcpy(dst,src,handle->cb_width_store);
			src +=handle->cb_stride;
			dst += handle->cb_width_store;

		}
		//cr copy
		for(i=0;i<handle->height_store/2;i++)
		{

			memcpy(dst,src,handle->cr_width_store);
			src +=handle->cr_stride;
			dst += handle->cr_width_store;

		}

	}
	else
	{
		memcpy(vbuf, (void *)handle->lpvbits[frame_index].base,handle->frameSize);
	}
#endif
#endif


#if 0
	//#define COPY_SIZE_ONE_TIME 0xff 
	//#define COPY_SIZE_ONE_TIME 0x50 
#define COPY_SIZE_ONE_TIME 0x20 
	unsigned int dst,src;
	int totalsize=handle->lpvbits[frame_index].size;
	dst=vbuf;
	src = handle->lpvbits[frame_index].base;
	while(totalsize>0)
	{
		if(totalsize>=COPY_SIZE_ONE_TIME)
		{
			memcpy(dst, src,COPY_SIZE_ONE_TIME);
		}
		else
		{

			memcpy(dst, src,totalsize);
		}
		udelay(1000);
		totalsize-=COPY_SIZE_ONE_TIME;
		dst+=COPY_SIZE_ONE_TIME;
		src+=COPY_SIZE_ONE_TIME;
	}
#endif

	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
	dprintk(dev, 2, "[%p/%d] done\n", buf, buf->vb.v4l2_buf.index);

err:
	handle->fillBufferDone =1;
	VIU_printk("set regnew : 1\n");
	VI_ClearChIntrpt(0,0xffff);
	VI_ClearChIntrpt(2,0xffff);
	VI_CHCfg(VI0_REG_NEWER , 0x1);
	VI_CHCfg(VI2_REG_NEWER , 0x1);
#if 0
	VIU_WriteReg(VI0_CH_CTRL + 0 * 0x1000,0x1);
	VIU_WriteReg(VI0_CH_CTRL + 2 * 0x1000,0x1);
#endif
	VIU_printk("leave %s\n",__func__);
	return;
}

static irqreturn_t VIU_IrqFunc(int irq, void *device_id)
{
	int intrpt_indicator;
	int ch_int_status[8];
	static int m=0 ,n=2;

	VIU_handle_t *handle;
	static volatile int frameBufferChanged=0; 
	intrpt_indicator = VI_GetIntrptIndicator();

	//  handle = VIU_GetHandleFromChnIrq(intrpt_indicator);
	handle = g_dev_info->handle[0];
	VIU_printk("in %s \n",__func__);
	//bt1120 case
#if 0
	if (((intrpt_indicator & 0x00000001) == 0x00000001) ||
			((intrpt_indicator & 0x00000004) == 0x00000004)) 
	{
		ch_int_status[m] = VI_GetChIntrptStatus(m);
		VI_ClearChIntrpt(m,ch_int_status[m]);
		ch_int_status[n] = VI_GetChIntrptStatus(n);
		VI_ClearChIntrpt(n,ch_int_status[n]);


		//	VIU_printk("ch_int_status[0]:%x\n",ch_int_status[m]);
		//	VIU_printk("ch_int_status[2]:%x\n",ch_int_status[n]);

		if(handle->fillBufferDone)
		{

			if (((ch_int_status[m] & 0x00000001) == 0x00000001) && ((ch_int_status[n] & 0x00000001) == 0x00000001)) // cc intrpt
			{
				VIU_printk("ch%d,chn%d cc irq  happen\n",m,n);
				if(frameBufferChanged) 
				{
#if 0

					VIU_WriteReg(VI0_CH_CTRL + 0 * 0x1000,0x0);
					VIU_WriteReg(VI0_CH_CTRL + 2 * 0x1000,0x0);
#endif

					handle->fillBufferDone = 0;
					tasklet_schedule(&(g_dev_info->viu_taskq));
					//tasklet_schedule(&(handle->viu_taskq));
					frameBufferChanged=0;


				}

			}

			if (((ch_int_status[m] & 0x00000060) == 0x00000060) && ((ch_int_status[n] & 0x00000060) == 0x00000060)) // frame pulse and reg_update
			{
				//printk("reg update and field irq  happen\n");

				handle->current_frame=handle->next_frame;
				if(handle->next_frame == 0)
				{

					handle->next_frame = 1;
				}
				else
				{
					handle->next_frame = 0;
				}
				// chx base address
				VIU_printk("change yuv address\n");
				VIU_SetCH_PORT0_1_BaseAddr(handle->next_frame,handle);
				frameBufferChanged=1;


#if 1
				VI_CHCfg(VI0_REG_NEWER , 0x0);
				VI_CHCfg(VI2_REG_NEWER , 0x0);
#endif
			}
		}

#if 1
		if ((ch_int_status[m] & (1<<9)) == (1<<9))
		{
			printk("chn:%d underflow\n",m);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
		if ((ch_int_status[n] & (1<<9)) == (1<<9))
		{
			printk("chn:%d underflow\n",n);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}

		if ((ch_int_status[m] & (1<<3)) == (1<<3))
		{
			printk("chn:%d bus error\n",m);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
		if ((ch_int_status[n] & (1<<3)) == (1<<3))
		{
			printk("chn:%d bus error\n",n);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}

		if ((ch_int_status[m] & (1<<4)) == (1<<4))
		{
			printk("chn:%d protect bit error\n",m);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
		if ((ch_int_status[n] & (1<<4)) == (1<<4))
		{
			printk("chn:%d protect bit error\n",n);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}

		if ((ch_int_status[m] & 0x00000400) == 0x0000400)
		{
			//VIU_printk("chn:%d overlap\n",m);
			printk("chn:%d overlap\n",m);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}

		if ((ch_int_status[n] & 0x000400) == 0x000400)
		{     
			//VIU_printk("chn:%d overlap\n",n);
			printk("chn:%d overlap\n",n);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;

		}

		if ((ch_int_status[m] & 0x00000002) == 0x00000002)
		{
			//	VIU_printk("chn:%d overflow happen\n",m);
			printk("chn:%d overflow happen\n",m);
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}

		if ((ch_int_status[n] & 0x00000002) == 0x00000002)
		{     
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			//VIU_printk("chn:%d overflow happen\n",n);
			printk("chn:%d overflow happen\n",n);
			frameBufferChanged=0;

		}
#endif

	}


	//bt656 case

#else
	if ((intrpt_indicator & 0x00000004) == 0x00000004)
	{


		ch_int_status[2] = VI_GetChIntrptStatus(2);

			VIU_printk("ch_int_status[2]:%x\n",ch_int_status[2]);

		VI_ClearChIntrpt(2,ch_int_status[2] );
		


		if(handle->fillBufferDone)
		{


			if ((ch_int_status[2] & 0x00000001) == 0x00000001) // cc intrpt
			{

				VIU_printk("cc irq  happen\n");
				//printk("cc irq  happen\n");


				//	VI_CHCfg(VI0_CH_CTRL, 0x0);


				if(frameBufferChanged) 
				{

					handle->fillBufferDone = 0;
					VIU_printk("fillbuffer_tasklet in irq\n");
					tasklet_schedule(&(g_dev_info->viu_taskq));
					frameBufferChanged=0;

				}
			}
			if ((ch_int_status[2] & 0x00000060) == 0x00000060) // frame pulse and reg_update
			{


				//		printk("reg update and field irq  happen\n");
				{
					handle->current_frame=handle->next_frame;
					if(handle->next_frame == 0)
					{

						handle->next_frame = 1;
					}
					else
					{
						handle->next_frame = 0;
					}
					// chx base address
						VIU_printk("change yuv address handle->next_frame :%d\n",handle->next_frame);
					VIU_SetCH_BaseAddr(2,handle->next_frame,handle);
					frameBufferChanged=1;
					//	VIU_printk("set regnew : 0\n");
					VI_CHCfg(VI2_REG_NEWER , 0x0);
				}

			}


		}

//TODO
#if 0
		if ((ch_int_status[0] & (1<<9)) == (1<<9))
		{
			printk("ch0 underflow\n");
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
		

		if ((ch_int_status[0] & (1<<3)) == (1<<3))
		{
			printk("ch0 bus error\n");
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
	
		if ((ch_int_status[m] & (1<<4)) == (1<<4))
		{
			printk("ch0 protect bit error\n");
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}


		if ((ch_int_status[0] & 0x00000400) == 0x0000400)
		{
			printk("ch0 overlap\n");
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
		if ((ch_int_status[0] & 0x00000002) == 0x00000002)
		{
			printk("ch0 overflow happen\n");
			VI_CHCfg(VI0_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
#else
#if 1
			if ((ch_int_status[2] & (1<<9)) == (1<<9))
		{
			printk("ch2 underflow\n");
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
		

		if ((ch_int_status[2] & (1<<3)) == (1<<3))
		{
			printk("ch2 bus error\n");
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
	
		if ((ch_int_status[2] & (1<<4)) == (1<<4))
		{
			printk("ch2 protect bit error\n");
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}


		if ((ch_int_status[2] & 0x00000400) == 0x0000400)
		{
			printk("ch2 overlap\n");
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
		if ((ch_int_status[2] & 0x00000002) == 0x00000002)
		{
			printk("ch2 overflow happen\n");
			VI_CHCfg(VI2_REG_NEWER , 0x1);
			frameBufferChanged=0;
		}
#endif
#endif





	}


#endif

	VIU_printk("leave %s \n",__func__);
	return IRQ_RETVAL(1);
}


/***********************/
/***********************/
/***********************/


static int viu_probe(struct platform_device *pdev) 
{
	viu_dev *dev;
	struct video_device *vfd;
	int ret;
	struct resource *irq;
	struct resource *reg_mem;
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	ret = v4l2_device_register(NULL, &dev->v4l2_dev);
	if (ret)
		goto free_dev;


	dev->v4l2_dev.ctrl_handler = NULL;

	/* initialize locks */
	spin_lock_init(&dev->slock);
	mutex_init(&dev->mutex);

	ret = -ENOMEM;
	vfd = video_device_alloc();
	if (!vfd)
		goto unreg_dev;

	*vfd = viu_template;
	vfd->debug = debug;
	vfd->v4l2_dev = &dev->v4l2_dev;
	//set_bit(V4L2_FL_USE_FH_PRIO, &vfd->flags); //yuliubing

	/*
	 * Provide a mutex to v4l2 core. It will be used to protect
	 * all fops and v4l2 ioctls.
	 */
	vfd->lock = &dev->mutex;

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, video_nr);
	if (ret < 0)
		goto rel_vdev;

	video_set_drvdata(vfd, dev);



	dev->vfd = vfd;
	v4l2_info(&dev->v4l2_dev, "V4L2 device registered as %s\n",
			video_device_node_name(vfd));

#if 0
	videobuf_queue_vmalloc_init(&cam->vb_vidq, &viu_video_qops,
			NULL, &cam->slock,
			cam->type,
			V4L2_FIELD_INTERLACED,
			sizeof(struct viu_buffer), cam, NULL);
#endif
	/*get platform resource*/
	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0); 
	reg_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	dev->reg_base = reg_mem->start;
	dev->reg_size = reg_mem->end - reg_mem->start;
	dev->irq = irq->start; 
	dev->viu_base = (u32*)ioremap_nocache(dev->reg_base, dev->reg_size);

	ret = request_irq(dev->irq, VIU_IrqFunc, 0, MODULE_NAME, dev);
	if(ret) {
		printk(KERN_INFO KBUILD_MODNAME ": %s request irq: %d error!\n", __func__, dev->irq);
		goto err;		
	}
//need to move to v4l2_open
	tasklet_init(&dev->viu_taskq, fillbuffer_tasklet, (unsigned long)dev);

	g_dev_info=dev;

	//TODO
	unsigned int tmp;
	//pad mux
	//
#if 0
	//port 0
	tmp =*(volatile unsigned int *)0xbfba9044;
	tmp |= (1<<5);
	*(volatile unsigned int *)0xbfba9044 = tmp; 
#else
//port 0
	tmp =*(volatile unsigned int *)0xbfba9044;
	tmp &= ~(1<<5);
	*(volatile unsigned int *)0xbfba9044 = tmp; 

#endif

	//port 1
	tmp =*(volatile unsigned int *)0xbfba9048;
	tmp |= (1<<19);
	*(volatile unsigned int *)0xbfba9048 = tmp; 

	//port 2
	tmp =*(volatile unsigned int *)0xbfba90a8;
	tmp |= (1<<28);
	*(volatile unsigned int *)0xbfba90a8 = tmp; 

	//port 3
	tmp =*(volatile unsigned int *)0xbfba90a8;
	tmp |= (1<<29);
	*(volatile unsigned int *)0xbfba90a8 = tmp; 




	VIU_Reset();//viu module reset
	return 0;
err:

rel_vdev:
	video_device_release(vfd);
unreg_dev:
	//v4l2_ctrl_handler_free(hdl);
	v4l2_device_unregister(&dev->v4l2_dev);
free_dev:
	kfree(dev);
	return ret;
}




static void viu_destroy(viu_dev *dev )
{
	if(!dev) {
		printk(KERN_ERR KBUILD_MODNAME ": %s no device!\n", __func__);
		return ;
	}

	video_unregister_device(dev->vfd);
	v4l2_device_unregister(&dev->v4l2_dev);
	iounmap((void __iomem*)dev->reg_base);
	kfree(dev);
	dev = NULL;

	return ;
}

static int viu_remove(struct platform_device *pdev)
{
	viu_dev *dev = platform_get_drvdata(pdev);
	viu_destroy(dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver silan_viu_driver = 
{
	.probe = viu_probe,
	.remove = viu_remove,
	.driver = 
	{
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init silan_viu_init(void)
{
	int retval;

	retval = platform_driver_register(&silan_viu_driver);
	if(retval)
		printk(KERN_ERR KBUILD_MODNAME ": %s platform_driver_register failed!\n", __func__);
	else 
		printk(KERN_INFO KBUILD_MODNAME ": %s platform_driver_register succeed!\n", __func__);
	

	
	return retval;
}

static void __exit silan_viu_exit(void)
{
	platform_driver_unregister(&silan_viu_driver);
	printk(KERN_INFO KBUILD_MODNAME ": %s silan viu module removed!\n", __func__);
}

module_init(silan_viu_init);
module_exit(silan_viu_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

