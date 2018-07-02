#ifndef VIU_EXPORT_INCLUDE
#define VIU_EXPORT_INCLUDE

#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-common.h>

#define V4L2_DEF_BUFFERS 9
/******** user control ********************/

#define V4L2_CID_PRIVATE_SET_CAP_MODE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_SET_STORE_FORMAT  (V4L2_CID_PRIVATE_BASE + 1)

#define V4L2_CID_PRIVATE_INFO_RESOLUTION  (V4L2_CID_PRIVATE_BASE + 2)
#define V4L2_CID_PRIVATE_SET_CBCR_INTERLEAVE  (V4L2_CID_PRIVATE_BASE + 3)

#define V4L2_CID_PRIVATE_OPEN_CLK  (V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_CID_PRIVATE_CLOSE_CLK  (V4L2_CID_PRIVATE_BASE + 5)

#define V4L2_CID_PRIVATE_INFO_CROP  (V4L2_CID_PRIVATE_BASE + 6)

#define V4L2_CID_PRIVATE_INFO_MASK  (V4L2_CID_PRIVATE_BASE + 7)

#define V4L2_CID_PRIVATE_INFO_SCALE  (V4L2_CID_PRIVATE_BASE + 8)

#define V4L2_CID_PRIVATE_INFO_H_V_OFF  (V4L2_CID_PRIVATE_BASE + 9)

#define V4L2_CID_PRIVATE_INFO_BT656_REF_NUM  (V4L2_CID_PRIVATE_BASE + 10)

#define V4L2_CID_PRIVATE_INFO_SYM  (V4L2_CID_PRIVATE_BASE + 11)
#define V4L2_CID_PRIVATE_CONFIG_REGS  (V4L2_CID_PRIVATE_BASE + 0xfe)

#define V4L2_CID_PRIVATE_END (V4L2_CID_PRIVATE_BASE + 0xff)

#define MASK_NUM 4



//crop
typedef struct VIU_Crop_Info
{
unsigned int  startX;
unsigned int  startY;
unsigned int  width;
unsigned int  height;

}
VIU_Crop_Info_t;

typedef struct VIU_Sym_Info
{
unsigned int  mirror;
unsigned int  flip;
}
VIU_Sym_Info_t;

//mask
typedef struct VIU_Mask_Att
{
	unsigned int index;//0~3
	int enable;
	unsigned int  startX;
	unsigned int  startY;
	unsigned int  width;
	unsigned int  height;
	unsigned int color;//y cb cr

}
VIU_Mask_Att_t;


typedef struct VIU_Mask_Info
{
VIU_Mask_Att_t mask_attrs[MASK_NUM];

}
VIU_Mask_Info_t;

typedef struct VIU_H_V_OFF_Info
{
int user_define;
unsigned int hoff;
unsigned int voff1;
unsigned int voff2;
}
VIU_H_V_OFF_Info_t;
//TODO
typedef struct VIU_BT656_REF_NUM_Info
{
int user_define;
unsigned int voff1;
unsigned int voff2;
}

VIU_BT656_REF_NUM_Info_t;


typedef enum VIU_StoreMode
{
	Mode_Planar=0,
	Mode_SemiPlanar,
	Mode_Package,
	Mode_Raw
}
VIU_StoreMode_e;

typedef enum VIU_ScaleMode
{
SCALE_NO=0,
SCALE_1_2,
SCALE_1_4,
SCALE_1_8
#if 0
SCALE_Y_NO_C_1_2,
SCALE_Y_NO_C_1_4,
SCALE_Y_NO_C_1_8,

SCALE_Y_1_2_C_NO,
SCALE_Y_1_2_C_1_2,
SCALE_Y_1_2_C_1_4,
SCALE_Y_1_2_C_1_8
#endif
}
VIU_ScaleMode_e;
#define SCALE_Y_NO_C_1_2 ((SCALE_NO << 4) | SCALE_1_2 )
#define SCALE_Y_NO_C_1_4 ((SCALE_NO << 4) | SCALE_1_4 )
#define SCALE_Y_NO_C_1_8 ((SCALE_NO << 4) | SCALE_1_8 )

#define SCALE_Y_1_2_C_NO ((SCALE_1_2 << 4) | SCALE_NO )

#define SCALE_Y_1_2_C_1_2 ((SCALE_1_2 << 4) | SCALE_1_2 )
#define SCALE_Y_1_2_C_1_4 ((SCALE_1_2 << 4) | SCALE_1_4 )
#define SCALE_Y_1_2_C_1_8 ((SCALE_1_2 << 4) | SCALE_1_8 )

typedef enum CapMode
{
MODE_INTERLACE_EMBBED_PORT0 = 0, //bt656
MODE_INTERLACE_SEPERATE_PORT0,//BT601
MODE_PROGRESSIVE_EMBBED_PORT0, //480p(embedded),720p(embedded),1080p(embedded)
MODE_PROGRESSIVE_SEPERATE_PORT0, //480p(embedded),720p(embedded),1080p(embedded)
MODE_CAM_PORT0,
MODE_INTERLACE_EMBBED_PORT1,//bt656
MODE_PROGRESSIVE_EMBBED_PORT1, //480p(embedded),720p(embedded),1080p(embedded)
MODE_INTERLACE_EMBBED_PORT0_1, //BT1120
MODE_INTERLACE_SEPERATE_PORT0_1, 
MODE_PROGRESSIVE_EMBBED_PORT0_1, 


MODE_PROGRESSIVE_SEPERATE_PORT0_1, 

}
CapMode_t;
#endif
