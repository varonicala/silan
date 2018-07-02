
#ifndef VIU_H_INCLUDED
#define VIU_H_INCLUDED

#include <linux/interrupt.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-common.h>
#include "viu-export.h"
#define VIU_DEF_BUFS 2

/* buffer for one video frame */
struct viu_buffer {
	/* common v4l buffer stuff -- must be first */
	struct vb2_buffer	vb;
	struct list_head	list;
	struct viu_fmt        *fmt;
};

struct viu_dmaqueue {
	struct list_head       active;

};



struct lpvbits_t {
	unsigned int  base;
	unsigned int  phy_addr;
	unsigned int  size;
};



typedef struct frameInfo
{
unsigned int Y_addr;
unsigned int U_addr;
unsigned int V_addr;

}
frameInfo_t;

typedef struct VIU_handle
{
	CapMode_t capMode;
	int interlace;
	int stop_capture;
	//need to protect
	int fillBufferDone;
	//need to protect

	struct vb2_queue vb_q;
	struct viu_dmaqueue       dma_q;
	struct mutex		   mutex;
	struct viu_fmt            *fmt;

	struct tasklet_struct vidin_taskq;
	spinlock_t taskq_lock;

	struct lpvbits_t lpvbits[VIU_DEF_BUFS]; /* image data */
	int current_frame;
	int next_frame;

	enum v4l2_field		   field;
	unsigned int		   field_count;

	//window
	unsigned int win_left;
	unsigned int win_top;
	unsigned int width;//resulotion
	unsigned int height;//resolution
	unsigned int actual_width;//scale
	unsigned int actual_height;//scale
	unsigned int y_stride;
	unsigned int cb_stride;
	unsigned int cr_stride;
	unsigned int y_width_store;
	unsigned int cb_width_store;
	unsigned int cr_width_store;
	unsigned int height_store;
	frameInfo_t frameInfo[VIU_DEF_BUFS];
	unsigned int frameSize;
	unsigned int mirror;
	unsigned int flip;
	VIU_StoreMode_e storeFormat; //enum
	VIU_ScaleMode_e X_scaleMode;
	VIU_ScaleMode_e Y_scaleMode;
	VIU_ScaleMode_e scaleMode;
	VIU_Crop_Info_t crop_info;
	VIU_Sym_Info_t sym_info;
	VIU_Mask_Info_t mask_info;
	VIU_H_V_OFF_Info_t h_v_off_info;
	VIU_BT656_REF_NUM_Info_t bt656_ref_num_info;
}
VIU_handle_t;

typedef struct _viu_dev
{
	struct v4l2_device 	   v4l2_dev;
	//struct v4l2_ctrl_handler   ctrl_handler;

	spinlock_t                 slock;
	struct mutex		   mutex;

	/* various device info */
	struct video_device        *vfd;

	//struct viu_dmaqueue       vidq;

	/* Several counters */
	unsigned 		   ms;
	unsigned long              jiffies;
	unsigned		   button_pressed;

	int			   mv_count;	/* Controls bars movement */

	/* Input Number */
	int			   input;
	struct tasklet_struct viu_taskq;
	spinlock_t taskq_lock;
	int irq;
	s32 reg_base;
	s32 reg_size;
	u32 *viu_base;
	struct lpvbits_t lpvbits[VIU_DEF_BUFS]; /* image data */
	VIU_handle_t *handle[8];
}
viu_dev;

int VIU_StartCapture(CapMode_t capMode);
int VIU_StopCapture(CapMode_t capMode);
int VIU_ConfigCapMode(CapMode_t capMode,VIU_handle_t *handle);
int VIU_SetCH_BaseAddr(int ch_idx,int frameIndex,VIU_handle_t *handle);
int VIU_SetCH_PORT0_1_BaseAddr(int frameIndex,VIU_handle_t *handle);
int VIU_Reset(void);
int VIU_OpenCLK(void);
int VIU_CloseCLK(void);
int VIU_Restart(VIU_handle_t *handle);
int VIU_Pause(VIU_handle_t *handle);
#endif
