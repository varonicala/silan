#include "viuapi.h"
#include <linux/delay.h>
#include "viu-export.h"
#include "viu.h"
#include "regdefine.h"
/******************* bt656:port0 ********/

#define MODE_INTERLACE_EMBBED_PORT0_CFG_PORT0 0x00000001
#if 0
#define MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0_STOREMODE_PLANAR 0x00002084
#define MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0_STOREMODE_SEMI_PLANAR 0x00002184
#define MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0_STOREMODE_PACKAGE  0x00002284
#define MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0_STOREMODE_RAW 0x00002384
#endif

#define MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0 0x00002000  //base config


#define MODE_INTERLACE_EMBBED_PORT0_CH_CTRL_CH0 (0x0)

#define MODE_INTERLACE_EMBBED_PORT0_INT_EN_CH0 0x000007ef

#define MODE_INTERLACE_EMBBED_PORT0_FIFO0_DEPTH_CH0 511
#define MODE_INTERLACE_EMBBED_PORT0_FIFO1_DEPTH_CH0 255
#define MODE_INTERLACE_EMBBED_PORT0_FIFO2_DEPTH_CH0 255

/*********************BT601:PORT0******************/


#define MODE_INTERLACE_SEPERATE_PORT0_CFG_PORT0 0x0000011f

#if 0
#define MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0_STOREMODE_PLANAR 0x00002084
#define MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0_STOREMODE_SEMI_PLANAR 0x00002184
#define MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0_STOREMODE_PACKAGE 0x00002284
#define MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0_STOREMODE_RAW 0x00002384
#endif
#define MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0 0x00002000

#define MODE_INTERLACE_SEPERATE_PORT0_CH_CTRL_CH0 (0x0)

#define MODE_INTERLACE_SEPERATE_PORT0_INT_EN_CH0 0x000007ef

#define MODE_INTERLACE_SEPERATE_PORT0_FIFO0_DEPTH_CH0 511
#define MODE_INTERLACE_SEPERATE_PORT0_FIFO1_DEPTH_CH0 255
#define MODE_INTERLACE_SEPERATE_PORT0_FIFO2_DEPTH_CH0 255
	//480i_NTSC
#define MODE_INTERLACE_SEPERATE_PORT0_HOFF   238
#define MODE_INTERLACE_SEPERATE_PORT0_VOFF1  18
#define MODE_INTERLACE_SEPERATE_PORT0_VOFF2  18

	//576I_PAL
#if 0
#define MODE_INTERLACE_SEPERATE_PORT0_HOFF   132
#define MODE_INTERLACE_SEPERATE_PORT0_VOFF1  22
#define MODE_INTERLACE_SEPERATE_PORT0_VOFF2  23
#endif




/*************************port0_480p (embed)*: port0**************/


#define MODE_PROGRESSIVE_EMBBED_PORT0_CFG_PORT0  0x401

#if 0
#define MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0_STOREMODE_PLANAR 0x00002094
#define MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0_STOREMODE_SEMI_PLANAR 0x00002194
#define MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0_STOREMODE_PACKAGE     0x00002294
#define MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0_STOREMODE_RAW    0x00002394
#endif

#define MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0 0x00002000

#define MODE_PROGRESSIVE_EMBBED_PORT0_CH_CTRL_CH0 (0x0)

#define MODE_PROGRESSIVE_EMBBED_PORT0_INT_EN_CH0  0x7ef

	//todo
#define MODE_PROGRESSIVE_EMBBED_PORT0_FIFO0_DEPTH_CH0 511
#define MODE_PROGRESSIVE_EMBBED_PORT0_FIFO1_DEPTH_CH0 255
#define MODE_PROGRESSIVE_EMBBED_PORT0_FIFO2_DEPTH_CH0 255




/***************port0_480p (seperate) :port 0******************/

#define MODE_PROGRESSIVE_SEPERATE_PORT0_CFG_PORT0 0x51f

#if 0
#define MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0_STOREMODE_PLANAR 0x00002080
#define MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0_STOREMODE_SEMI_PLANAR 0x00002180
#define MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0_STOREMODE_PACKAGE     0x00002280
#define MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0_STOREMODE_RAW    0x00002380
#endif
#define MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0 0x00002000

#define MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CTRL_CH0 (0x0)
#define MODE_PROGRESSIVE_SEPERATE_PORT0_INT_EN_CH0 0x7ef
#if 1
#define MODE_PROGRESSIVE_SEPERATE_PORT0_FIFO0_DEPTH_CH0 511
#define MODE_PROGRESSIVE_SEPERATE_PORT0_FIFO1_DEPTH_CH0 255
#define MODE_PROGRESSIVE_SEPERATE_PORT0_FIFO2_DEPTH_CH0 255
#endif
	//480p
#define MODE_PROGRESSIVE_SEPERATE_PORT0_HOFF   244
//#define MODE_PROGRESSIVE_SEPERATE_PORT0_VOFF1  35
#define MODE_PROGRESSIVE_SEPERATE_PORT0_VOFF1  36
#define MODE_PROGRESSIVE_SEPERATE_PORT0_VOFF2  19

	//576p
#if 0
#define MODE_PROGRESSIVE_SEPERATE_PORT0_HOFF   132
#define MODE_PROGRESSIVE_SEPERATE_PORT0_VOFF1  22
#define MODE_PROGRESSIVE_SEPERATE_PORT0_VOFF2  23
#endif


/******************BT1120 embedded : port0,1***************/

#define MODE_INTERLACE_EMBBED_PORT0_1_CFG_PORT0 0x00000801

#define MODE_INTERLACE_EMBBED_PORT0_1_CFG_PORT1 0x00000801

#define MODE_INTERLACE_EMBBED_PORT0_1_CH_CTRL_CH0 0x0
#define MODE_INTERLACE_EMBBED_PORT0_1_CH_CTRL_CH2 0x0

#if 0
#define MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH0_STOREMODE_PLANAR   0x00002094
#define MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH0_STOREMODE_SEMI_PLANAR   0x00002194

#define MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH2_STOREMODE_PLANAR  0x02002094
#define MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH2_STOREMODE_SEMI_PLANAR 0x02002194
#endif
#define MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH0   0x00002000
#define MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH2  0x02002000

#define MODE_INTERLACE_EMBBED_PORT0_1_INT_EN_CH0 0x000007ef
#define MODE_INTERLACE_EMBBED_PORT0_1_INT_EN_CH2 0x000007ef


#define MODE_INTERLACE_EMBBED_PORT0_1_FIFO0_DEPTH_CH0 511
#define MODE_INTERLACE_EMBBED_PORT0_1_FIFO1_DEPTH_CH0  0
#define MODE_INTERLACE_EMBBED_PORT0_1_FIFO2_DEPTH_CH0  0


#define MODE_INTERLACE_EMBBED_PORT0_1_FIFO0_DEPTH_CH2 0
#define MODE_INTERLACE_EMBBED_PORT0_1_FIFO1_DEPTH_CH2  255
#define MODE_INTERLACE_EMBBED_PORT0_1_FIFO2_DEPTH_CH2  255






/***************BT1120 SEPERATE:port0,1*********/
#if 1
#define MODE_INTERLACE_SEPERATE_PORT0_1_CFG_PORT0 0x00000915

#define MODE_INTERLACE_SEPERATE_PORT0_1_CFG_PORT1 0x00000915
#else
#define MODE_INTERLACE_SEPERATE_PORT0_1_CFG_PORT0 0x0000091f

#define MODE_INTERLACE_SEPERATE_PORT0_1_CFG_PORT1 0x0000091f
#endif

#define MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH0   0x00002000
#define MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH2  0x02002000
//TODO

#if 0
#define MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH0_STOREMODE_PLANAR   0x00002094
#define MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH0_STOREMODE_SEMI_PLANAR   0x02002194

#define MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH2_STOREMODE_PLANAR  0x02002094
#define MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH2_STOREMODE_SEMI_PLANAR 0x02002194
#endif
#define MODE_INTERLACE_SEPERATE_PORT0_1_CH_CTRL_CH0 (0x0)
#define MODE_INTERLACE_SEPERATE_PORT0_1_CH_CTRL_CH2 (0x0)

#define MODE_INTERLACE_SEPERATE_PORT0_1_INT_EN_CH0 0x000007ef
#define MODE_INTERLACE_SEPERATE_PORT0_1_INT_EN_CH2 0x000007ef
	//todo
#define MODE_INTERLACE_SEPERATE_PORT0_1_FIFO0_DEPTH_CH0 511
#define MODE_INTERLACE_SEPERATE_PORT0_1_FIFO1_DEPTH_CH0  0
#define MODE_INTERLACE_SEPERATE_PORT0_1_FIFO2_DEPTH_CH0  0

#define MODE_INTERLACE_SEPERATE_PORT0_1_FIFO0_DEPTH_CH2 0
#define MODE_INTERLACE_SEPERATE_PORT0_1_FIFO1_DEPTH_CH2  255
#define MODE_INTERLACE_SEPERATE_PORT0_1_FIFO2_DEPTH_CH2  255

#if 1
#define MODE_INTERLACE_SEPERATE_PORT0_1_HOFF  192
#define MODE_INTERLACE_SEPERATE_PORT0_1_VOFF1 20
#define MODE_INTERLACE_SEPERATE_PORT0_1_VOFF2 20
#else
#define MODE_INTERLACE_SEPERATE_PORT0_1_HOFF  149
#define MODE_INTERLACE_SEPERATE_PORT0_1_VOFF1 15
#define MODE_INTERLACE_SEPERATE_PORT0_1_VOFF2 15
#endif
/******************720p embedded : port0,1***************/

#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CFG_PORT0 0x00000C01

#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CFG_PORT1 0x00000C01


#if 0
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH0   0x00002090
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH2  0x02002090
#endif

#if 0
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH0   0x00002010
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH2  0x02002010
#endif
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH0   0x00002000
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH2  0x02002000
#if 0
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH0_STOREMODE_PLANAR   0x00002094
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH0_STOREMODE_SEMI_PLANAR   0x00002194

#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH2_STOREMODE_PLANAR  0x02002094
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH2_STOREMODE_SEMI_PLANAR 0x02002194
#endif


#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CTRL_CH0 (0x0)
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CTRL_CH2 (0x0)


#define MODE_PROGRESSIVE_EMBBED_PORT0_1_INT_EN_CH0 0x000007ef
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_INT_EN_CH2 0x000007ef


#define MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO0_DEPTH_CH0 511
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO1_DEPTH_CH0  0
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO2_DEPTH_CH0  0


#define MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO0_DEPTH_CH2 0
#define MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO1_DEPTH_CH2  255

#define MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO2_DEPTH_CH2  255






/***************720p SEPERATE:port0,1*********/
#if 1
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CFG_PORT0 0x00000D15

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CFG_PORT1 0x00000D15
#else

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CFG_PORT0 0x00000D1f

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CFG_PORT1 0x00000D1f
#endif
#if 0
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH0_STOREMODE_PLANAR   0x00002094
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH0_STOREMODE_SEMI_PLANAR   0x02002194

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH2_STOREMODE_PLANAR  0x02002094
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH2_STOREMODE_SEMI_PLANAR 0x02002194

#endif

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH0   0x00002000
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH2  0x02002000

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CTRL_CH0 (0x0)
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CTRL_CH2 (0x0)

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_INT_EN_CH0 0x000007ef

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_INT_EN_CH2 0x000007ef

//todo
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO0_DEPTH_CH0 511
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO1_DEPTH_CH0  0
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO2_DEPTH_CH0  0

#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO0_DEPTH_CH2 0
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO1_DEPTH_CH2  255
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO2_DEPTH_CH2  255

#if 1
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_HOFF  260
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_VOFF1 25
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_VOFF2 25
#else
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_HOFF  221
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_VOFF1 20
#define MODE_PROGRESSIVE_SEPERATE_PORT0_1_VOFF2 20
#endif

int VIU_SetCH_BaseAddr(int ch_idx,int frameIndex,VIU_handle_t* handle)
{
	unsigned int reg_addr_0,reg_addr_1,reg_addr_2;
	reg_addr_0 = VI0_YBASE_ADDR + ch_idx * 0x1000;
	reg_addr_1 = VI0_UBASE_ADDR + ch_idx * 0x1000;
	reg_addr_2 = VI0_VBASE_ADDR + ch_idx * 0x1000;  
	switch(handle->storeFormat)
	{
		case Mode_Planar:
			// Y channel base address
			VI_CHCfg(reg_addr_0, handle->frameInfo[frameIndex].Y_addr);
//printk("Mode_Planar Y_addr:%p\n",handle->frameInfo[frameIndex].Y_addr);

			VI_CHCfg(reg_addr_1, handle->frameInfo[frameIndex].U_addr);


//printk("Mode_Planar U_addr:%p\n",handle->frameInfo[frameIndex].U_addr);
			VI_CHCfg(reg_addr_2, handle->frameInfo[frameIndex].V_addr);
//printk("Mode_Planar V_addr:%p\n",handle->frameInfo[frameIndex].V_addr);
			break;
		case Mode_SemiPlanar:
			// Y channel base address
			VI_CHCfg(reg_addr_0, handle->frameInfo[frameIndex].Y_addr);


			VI_CHCfg(reg_addr_1, handle->frameInfo[frameIndex].U_addr);


			break;
		case Mode_Package:
		case Mode_Raw:
			// Y channel base address
			VI_CHCfg(reg_addr_0, handle->frameInfo[frameIndex].Y_addr);


			break;


	}
	return 0;
}

int VIU_SetCH_PORT0_1_BaseAddr(int frameIndex,VIU_handle_t *handle)
{

	switch(handle->storeFormat)
	{
		case Mode_Planar:
			// Y channel base address
			VI_CHCfg(VI0_YBASE_ADDR, handle->frameInfo[frameIndex].Y_addr);


			VI_CHCfg(VI2_UBASE_ADDR, handle->frameInfo[frameIndex].U_addr);


			VI_CHCfg(VI2_VBASE_ADDR, handle->frameInfo[frameIndex].V_addr);
			break;
		case Mode_SemiPlanar:
			// Y channel base address
			VI_CHCfg(VI0_YBASE_ADDR, handle->frameInfo[frameIndex].Y_addr);


			VI_CHCfg(VI2_UBASE_ADDR, handle->frameInfo[frameIndex].U_addr);

			break;
		case Mode_Package:
		case Mode_Raw:
			printk("Mode_Package and Mode_Raw NOT supported \n");

			break;
	}
	return 0;
}

int	VI_Caculate_YUV_Offset(VIU_handle_t *handle,int cap_width,int cap_height,VIU_StoreMode_e format,unsigned int *u_offset,unsigned int *v_offset,unsigned int *totalSize)
{
	int y_store_size,cb_store_size;
	int y_width,cb_width,cr_width,y_height,cb_height,cr_height;
	//8 byte align
#if 0
	y_width = (cap_width+7)&(~7);
	cb_width = (cap_width/2+7)&(~7);
	cr_width = (cap_width/2+7)&(~7);
#endif
	y_width = (cap_width+15)&(~15);
	cb_width = y_width/2;
	cr_width = y_width/2;

	handle->y_width_store =y_width;
	handle->cb_stride =cb_width;
	handle->cr_stride =cr_width;
	handle->cb_width_store = y_width/2;
	handle->cr_width_store = y_width/2;
	handle->height_store = cap_height; 
	y_height =( cap_height + 1)&(~1);//2 byte align
	cb_height = y_height/2;
	cr_height = y_height/2;

	printk("y_width:%d\n",y_width);
	printk("y_height:%d\n",y_height);
	printk("c_stride:%d\n",cb_width);
	printk("c_width:%d\n",y_width/2);
	y_store_size = y_width*y_height;
	cb_store_size = y_width*y_height/4;
	switch(format)
	{
		case Mode_Planar:
			printk("planar mode\n");
			*u_offset=y_store_size;
			*v_offset=cb_width*cb_height;
			*totalSize=y_store_size+2*cb_store_size; //y_width*y_height*1.5
			break;
		case Mode_SemiPlanar:
			printk("semiplanar mode\n");
			*u_offset=y_store_size;
			*v_offset=0;
			*totalSize=y_store_size+2*cb_store_size;//y_width*y_height*1.5
			break;
		case Mode_Package:
		case Mode_Raw:
			printk("package mode\n");
			y_store_size = y_width*y_height*2;
			*u_offset=y_store_size;
			*v_offset=0;
			*totalSize=y_store_size;//y_width*y_height*2
			break;

	}
	return 0;
}
//bt656
static int VIU_Config_MODE_INTERLACE_EMBBED_PORT0(VIU_handle_t *handle)
{
	int width,height;
	VIU_StoreMode_e format;
	VIU_ScaleMode_e scaleMode;
	VIU_Mask_Info_t *mask_info;

	unsigned int y_addr;
	int cap_start,cap_size;
	int  port_clk_sel_div;
	unsigned int start_x,start_y,width_align,height_align;

	unsigned int u_offset,v_offset,totalSize;
	unsigned int regValue;
	unsigned int mirror,flip;
	printk("****VIU_Config_MODE_INTERLACE_EMBBED_PORT0\n");

	width = handle->width;
	height = handle->height;
	format = handle ->storeFormat;
	scaleMode = handle ->scaleMode;
	mask_info = &(handle->mask_info);

	y_addr = handle->lpvbits[0].phy_addr;

	VI_PortCfg(1,MODE_INTERLACE_EMBBED_PORT0_CFG_PORT0);
	VI_PortReset(1);
#if 1
	port_clk_sel_div = 0x00084000;

	VI_CHCfg(PORT_CLK_SEL_DIV, port_clk_sel_div);
#endif
	mirror = handle->sym_info.mirror;
	flip = handle->sym_info.flip;
#if 0
	mirror = 1;
	flip = 1;
#endif
	if(format==Mode_Planar)
	{
		regValue = (MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0 | STORE_MODE_PLANAR);
	}
	else if(format==Mode_SemiPlanar)
	{
		regValue = (MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR);
	}
	else if(format==Mode_Package)
	{
		regValue = (MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0 | STORE_MODE_PACKAGE);
	}
	else
	{		regValue = (MODE_INTERLACE_EMBBED_PORT0_CH_CFG_CH0 | STORE_MODE_RAW);
	}
	printk("scale mode:%d\n",scaleMode);
	//scale configuration  TODO

	if(scaleMode==SCALE_Y_NO_C_1_2)
	{

		regValue |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FRAME ;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height;

	}
	else if(scaleMode==SCALE_Y_NO_C_1_4)
	{

		regValue |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_NO_C_1_8)
	{
		regValue |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_1_2_C_NO)
	{

		regValue |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height/2;
	}
	else if(scaleMode==SCALE_Y_1_2_C_1_2)
	{

		regValue |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_4)
	{

		regValue |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_8)
	{

		regValue |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height/2;
	}

	else
	{

		regValue |= CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height;

	}
	//mirror and flip

	if(mirror == 1)
	{
		regValue |= MIRROR_MODE;
	}
	if(flip == 1)
	{
		regValue |= FLIP_MODE;
	}

	VI_CHCfg(VI2_CH_CFG ,regValue);


	VI_CHCfg(VI2_INT_EN, MODE_INTERLACE_EMBBED_PORT0_INT_EN_CH0);
	VI_SetCHFifoDepth(2, MODE_INTERLACE_EMBBED_PORT0_FIFO0_DEPTH_CH0, MODE_INTERLACE_EMBBED_PORT0_FIFO1_DEPTH_CH0, MODE_INTERLACE_EMBBED_PORT0_FIFO2_DEPTH_CH0);

	start_x = handle->crop_info.startX*2;

	start_y = handle->crop_info.startY/2;

	printk("start_x:%d ,start_y:%d\n",start_x,start_y);

	cap_start =( (start_y)<<16)+(start_x);

	cap_size  = ((height_align << 16) +handle->crop_info.width);

	printk("cap_width:%d,cap_height:%d\n",handle->crop_info.width,height_align);
	printk("store_width:%d,store_height:%d\n",width_align,height_align);

	VI_SetCHCapInfo(2, cap_start, cap_size);


	regValue = 0;
	regValue = MODE_INTERLACE_EMBBED_PORT0_CH_CTRL_CH0;
//TODO
//change VI0 to VI2

#if 0
	if(mask_info->mask_attrs[0].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK0_START , (((mask_info->mask_attrs[0].startY)/2)<<14)+(mask_info->mask_attrs[0].startX)*2);
		VI_CHCfg(VI0_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height/2)<<14)+(mask_info->mask_attrs[0].width)*2);
		VI_CHCfg(VI0_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		regValue |= MASK_0; 

	}
	if(mask_info->mask_attrs[1].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK1_START , (((mask_info->mask_attrs[1].startY)/2)<<14)+(mask_info->mask_attrs[1].startX)*2);
		VI_CHCfg(VI0_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height/2)<<14)+(mask_info->mask_attrs[1].width)*2);
		VI_CHCfg(VI0_BLOCK1_COLOR , mask_info->mask_attrs[1].color);

		regValue |= MASK_1; 

	}

	if(mask_info->mask_attrs[2].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK2_START , (((mask_info->mask_attrs[2].startY)/2)<<14)+(mask_info->mask_attrs[2].startX)*2);
		VI_CHCfg(VI0_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height/2)<<14)+(mask_info->mask_attrs[2].width)*2);
		VI_CHCfg(VI0_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		regValue |= MASK_2; 

	}

	if(mask_info->mask_attrs[3].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK3_START , (((mask_info->mask_attrs[3].startY)/2)<<14)+(mask_info->mask_attrs[3].startX)*2);
		VI_CHCfg(VI0_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height/2)<<14)+(mask_info->mask_attrs[3].width)*2);
		VI_CHCfg(VI0_BLOCK3_COLOR , mask_info->mask_attrs[3].color);

		regValue |= MASK_3; 

	}
#endif
	VI_CHCfg(VI2_CH_CTRL,regValue);


	VI_SetCHStoreConfig(2, width_align, height_align,format,scaleMode,y_addr);


	VI_Caculate_YUV_Offset(handle,width_align,height_align,format,&u_offset,&v_offset,&totalSize);

	handle->frameSize = totalSize;
	printk("frameSize:%d\n",handle->frameSize);
	handle->frameInfo[0].Y_addr = handle->lpvbits[0].phy_addr;
	handle->frameInfo[0].U_addr = handle->lpvbits[0].phy_addr+u_offset;
	handle->frameInfo[0].V_addr = handle->lpvbits[0].phy_addr+u_offset+v_offset;

	printk("handle->frameInfo[0].Y_addr:%p\n",handle->frameInfo[0].Y_addr);
	handle->frameInfo[1].Y_addr = handle->lpvbits[1].phy_addr;
	handle->frameInfo[1].U_addr = handle->lpvbits[1].phy_addr+u_offset;
	handle->frameInfo[1].V_addr = handle->lpvbits[1].phy_addr+u_offset+v_offset;
	printk("handle->frameInfo[1].Y_addr:%p\n",handle->frameInfo[1].Y_addr);
	VIU_SetCH_BaseAddr(2,0,handle);

	//VI_CHCfg(VI0_REG_NEWER , 0x1);

	print_viu_values();

	return 0;

}

//BT601
static int VIU_Config_MODE_INTERLACE_SEPERATE_PORT0(VIU_handle_t *handle)
{
	int width,height;
	VIU_StoreMode_e format;
	VIU_ScaleMode_e scaleMode;
	unsigned int y_addr;
	int cap_start,cap_size;
	int  port_clk_sel_div;

	unsigned int u_offset,v_offset,totalSize;
	unsigned int start_x,start_y,width_align,height_align;
	int act1_voff, act1_height, act2_voff, act2_height, act_hoff, act_width;   
	int vi_p0_vsync1, vi_p0_vsync2, vi_p0_hsync;
	unsigned int regValue;
	unsigned int mirror,flip;
	VIU_Mask_Info_t *mask_info;


	printk("****VIU_Config_MODE_INTERLACE_SEPERATE_PORT0\n");

	mask_info = &(handle->mask_info);


	width = handle->width;
	height = handle->height;
	format = handle ->storeFormat;
	scaleMode= handle ->scaleMode;

	y_addr = handle->lpvbits[0].phy_addr;

	VI_PortCfg(0,MODE_INTERLACE_SEPERATE_PORT0_CFG_PORT0);
	VI_PortReset(0);
#if 1
	port_clk_sel_div = 0x000002a8;

	VI_CHCfg(PORT_CLK_SEL_DIV, port_clk_sel_div);
#endif
	mirror = handle->sym_info.mirror;
	flip = handle->sym_info.flip;
#if 0
	mirror = 1;
	flip = 1;
#endif

	if(format==Mode_Planar)
	{
		regValue = (MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0 | STORE_MODE_PLANAR);
	}
	else if(format==Mode_SemiPlanar)
	{
		regValue = (MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR);
	}
	else if(format==Mode_Package)
	{
		regValue = (MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0 | STORE_MODE_PACKAGE);
	}
	else
	{		regValue = (MODE_INTERLACE_SEPERATE_PORT0_CH_CFG_CH0 | STORE_MODE_RAW);
	}
	printk("scale mode:%d\n",scaleMode);
	//scale configuration  TODO


	if(scaleMode==SCALE_Y_NO_C_1_2)
	{

		regValue |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FRAME ;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height;

	}
	else if(scaleMode==SCALE_Y_NO_C_1_4)
	{

		regValue |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_NO_C_1_8)
	{
		regValue |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_1_2_C_NO)
	{

		regValue |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height/2;
	}
	else if(scaleMode==SCALE_Y_1_2_C_1_2)
	{

		regValue |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_4)
	{

		regValue |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_8)
	{

		regValue |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height/2;
	}

	else
	{

		regValue |= CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height;

	}

	if(mirror == 1)
	{
		regValue |= MIRROR_MODE;
	}
	if(flip == 1)
	{
		regValue |= FLIP_MODE;
	}




	VI_CHCfg(VI0_CH_CFG ,regValue);


	VI_CHCfg(VI0_INT_EN,MODE_INTERLACE_SEPERATE_PORT0_INT_EN_CH0 );

	VI_SetCHFifoDepth(0, MODE_INTERLACE_SEPERATE_PORT0_FIFO0_DEPTH_CH0, MODE_INTERLACE_SEPERATE_PORT0_FIFO1_DEPTH_CH0, MODE_INTERLACE_SEPERATE_PORT0_FIFO2_DEPTH_CH0);

	// VI_P0_VSYNC1, VI_P0_VSYNC2
	act1_voff   = MODE_INTERLACE_SEPERATE_PORT0_VOFF1-1;
	act1_height = height/2-1;

	act2_voff   = MODE_INTERLACE_SEPERATE_PORT0_VOFF2-1;
	act2_height = height/2-1;

	vi_p0_vsync1 = (act1_voff << 14) + act1_height;
	vi_p0_vsync2 = (act2_voff << 14) + act2_height;

	VI_CHCfg(VI_P0_VSYNC1, vi_p0_vsync1);


	VI_CHCfg(VI_P0_VSYNC2, vi_p0_vsync2);


	// VI_P0_HSYNC
	act_hoff  = MODE_INTERLACE_SEPERATE_PORT0_HOFF-1;
	act_width = width*2-1;
	vi_p0_hsync = (act_hoff << 14) + act_width;

	VI_CHCfg(VI_P0_HSYNC, vi_p0_hsync);


	start_x = handle->crop_info.startX*2;

	start_y = handle->crop_info.startY/2;

	printk("start_x:%d ,start_y:%d\n",start_x,start_y);
	cap_start =( (start_y)<<16)+(start_x);



	printk("cap_width:%d,cap_height:%d\n",handle->crop_info.width,height_align);
	printk("store_width:%d,store_height:%d\n",width_align,height_align);

	cap_size  = ((height_align << 16) +handle->crop_info.width);


	VI_SetCHCapInfo(0, cap_start, cap_size);


	regValue = 0;
	regValue = MODE_INTERLACE_SEPERATE_PORT0_CH_CTRL_CH0;

	if(mask_info->mask_attrs[0].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK0_START , (((mask_info->mask_attrs[0].startY)/2)<<14)+(mask_info->mask_attrs[0].startX)*2);
		VI_CHCfg(VI0_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height/2)<<14)+(mask_info->mask_attrs[0].width)*2);
		VI_CHCfg(VI0_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		regValue |= MASK_0; 

	}
	if(mask_info->mask_attrs[1].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK1_START , (((mask_info->mask_attrs[1].startY)/2)<<14)+(mask_info->mask_attrs[1].startX)*2);
		VI_CHCfg(VI0_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height/2)<<14)+(mask_info->mask_attrs[1].width)*2);
		VI_CHCfg(VI0_BLOCK1_COLOR , mask_info->mask_attrs[1].color);

		regValue |= MASK_1; 

	}

	if(mask_info->mask_attrs[2].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK2_START , (((mask_info->mask_attrs[2].startY)/2)<<14)+(mask_info->mask_attrs[2].startX)*2);
		VI_CHCfg(VI0_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height/2)<<14)+(mask_info->mask_attrs[2].width)*2);
		VI_CHCfg(VI0_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		regValue |= MASK_2; 

	}

	if(mask_info->mask_attrs[3].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK3_START , (((mask_info->mask_attrs[3].startY)/2)<<14)+(mask_info->mask_attrs[3].startX)*2);
		VI_CHCfg(VI0_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height/2)<<14)+(mask_info->mask_attrs[3].width)*2);
		VI_CHCfg(VI0_BLOCK3_COLOR , mask_info->mask_attrs[3].color);

		regValue |= MASK_3; 

	}
	VI_CHCfg(VI0_CH_CTRL,regValue);

	VI_SetCHStoreConfig(0, width_align, height_align,format,scaleMode,y_addr);

	VI_Caculate_YUV_Offset(handle,width_align,height_align,format,&u_offset,&v_offset,&totalSize);
	handle->frameSize = totalSize;
	printk("frameSize:%d\n",handle->frameSize);
	handle->frameInfo[0].Y_addr = handle->lpvbits[0].phy_addr;
	handle->frameInfo[0].U_addr = handle->lpvbits[0].phy_addr+u_offset;
	handle->frameInfo[0].V_addr = handle->lpvbits[0].phy_addr+u_offset+v_offset;

	handle->frameInfo[1].Y_addr = handle->lpvbits[1].phy_addr;
	handle->frameInfo[1].U_addr = handle->lpvbits[1].phy_addr+u_offset;
	handle->frameInfo[1].V_addr = handle->lpvbits[1].phy_addr+u_offset+v_offset;
	VIU_SetCH_BaseAddr(0,0,handle);


	//	VI_CHCfg(VI0_REG_NEWER , 0x1);

	print_viu_values();
	return 0;

}
//480p embedded
static int VIU_Config_MODE_PROGRESSIVE_EMBBED_PORT0(VIU_handle_t *handle)
{
	int width,height;
	VIU_StoreMode_e format;
	unsigned int y_addr;
	int cap_start,cap_size;
	unsigned int start_x,start_y,width_align,height_align;
	int  port_clk_sel_div;
	unsigned int regValue;
	unsigned int u_offset,v_offset,totalSize;
	unsigned int mirror,flip;
	VIU_Mask_Info_t *mask_info;
	VIU_ScaleMode_e scaleMode;

	printk("****VIU_Config_MODE_PROGRESSIVE_EMBBED_PORT0\n");
	width = handle->width;
	height = handle->height;
	format = handle ->storeFormat;


	mask_info = &(handle->mask_info);

	scaleMode= handle ->scaleMode;


	y_addr = handle->lpvbits[0].phy_addr;

	VI_PortCfg(0,MODE_PROGRESSIVE_EMBBED_PORT0_CFG_PORT0);
	VI_PortReset(0);
#if 1
	port_clk_sel_div = 0x000002a8;

	VI_CHCfg(PORT_CLK_SEL_DIV, port_clk_sel_div);
#endif
	mirror = handle->sym_info.mirror;
	flip = handle->sym_info.flip;
#if 0
	mirror = 1;
	flip = 1;
#endif
	if(format==Mode_Planar)
	{
		regValue = (MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0 | STORE_MODE_PLANAR  | INPUT_ORDER_CBYCRY);
	}
	else if(format==Mode_SemiPlanar)
	{
		regValue = (MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CBYCRY);
	}
	else if(format==Mode_Package)
	{
		regValue = (MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0 | STORE_MODE_PACKAGE | INPUT_ORDER_CBYCRY);
	}
	else
	{		regValue = (MODE_PROGRESSIVE_EMBBED_PORT0_CH_CFG_CH0 | STORE_MODE_RAW);
	}
	printk("scale mode:%d\n",scaleMode);
	//scale configuration  TODO

	if(scaleMode==SCALE_Y_NO_C_1_2)
	{

		regValue |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FIELD ;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height;

	}
	else if(scaleMode==SCALE_Y_NO_C_1_4)
	{

		regValue |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_NO_C_1_8)
	{
		regValue |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_1_2_C_NO)
	{

		regValue |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height/2;
	}
	else if(scaleMode==SCALE_Y_1_2_C_1_2)
	{

		regValue |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_4)
	{

		regValue |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_8)
	{

		regValue |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}

	else
	{

		regValue |= CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height;

	}
	if(mirror == 1)
	{
		regValue |= MIRROR_MODE;
	}
	if(flip == 1)
	{
		regValue |= FLIP_MODE;
	}
	VI_CHCfg(VI0_CH_CFG ,regValue);


	VI_CHCfg(VI0_INT_EN, MODE_PROGRESSIVE_EMBBED_PORT0_INT_EN_CH0);
	VI_SetCHFifoDepth(0, MODE_PROGRESSIVE_EMBBED_PORT0_FIFO0_DEPTH_CH0, MODE_PROGRESSIVE_EMBBED_PORT0_FIFO1_DEPTH_CH0, MODE_PROGRESSIVE_EMBBED_PORT0_FIFO2_DEPTH_CH0);

	start_x = handle->crop_info.startX*2;

	start_y = handle->crop_info.startY;

	printk("start_x:%d ,start_y:%d\n",start_x,start_y);
	cap_start =( (start_y)<<16)+(start_x);



	cap_size  = ((handle->crop_info.height << 16) +handle->crop_info.width);

	printk("cap_width:%d,cap_height:%d\n",handle->crop_info.width,handle->crop_info.width);
	printk("store_width:%d,store_height:%d\n",width_align,height_align);


	VI_SetCHCapInfo(0, cap_start, cap_size);

	regValue = 0;
	regValue = MODE_PROGRESSIVE_EMBBED_PORT0_CH_CTRL_CH0;

	if(mask_info->mask_attrs[0].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK0_START , (((mask_info->mask_attrs[0].startY))<<14)+(mask_info->mask_attrs[0].startX)*2);
		VI_CHCfg(VI0_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height)<<14)+(mask_info->mask_attrs[0].width)*2);
		VI_CHCfg(VI0_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		regValue |= MASK_0; 

	}
	if(mask_info->mask_attrs[1].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK1_START , (((mask_info->mask_attrs[1].startY))<<14)+(mask_info->mask_attrs[1].startX)*2);
		VI_CHCfg(VI0_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height)<<14)+(mask_info->mask_attrs[1].width)*2);
		VI_CHCfg(VI0_BLOCK1_COLOR , mask_info->mask_attrs[1].color);

		regValue |= MASK_1; 

	}

	if(mask_info->mask_attrs[2].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK2_START , (((mask_info->mask_attrs[2].startY))<<14)+(mask_info->mask_attrs[2].startX)*2);
		VI_CHCfg(VI0_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height)<<14)+(mask_info->mask_attrs[2].width)*2);
		VI_CHCfg(VI0_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		regValue |= MASK_2; 

	}

	if(mask_info->mask_attrs[3].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK3_START , (((mask_info->mask_attrs[3].startY))<<14)+(mask_info->mask_attrs[3].startX)*2);
		VI_CHCfg(VI0_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height)<<14)+(mask_info->mask_attrs[3].width)*2);
		VI_CHCfg(VI0_BLOCK3_COLOR , mask_info->mask_attrs[3].color);

		regValue |= MASK_3; 

	}
	VI_CHCfg(VI0_CH_CTRL,regValue);

	VI_SetCHStoreConfig(0, width_align, height_align,format,scaleMode,y_addr);

	VI_Caculate_YUV_Offset(handle,width_align,height_align,format,&u_offset,&v_offset,&totalSize);
	handle->frameSize = totalSize;
	printk("frameSize:%d\n",handle->frameSize);

	handle->frameInfo[0].Y_addr = handle->lpvbits[0].phy_addr;
	handle->frameInfo[0].U_addr = handle->lpvbits[0].phy_addr+u_offset;
	handle->frameInfo[0].V_addr = handle->lpvbits[0].phy_addr+u_offset+v_offset;

	handle->frameInfo[1].Y_addr = handle->lpvbits[1].phy_addr;
	handle->frameInfo[1].U_addr = handle->lpvbits[1].phy_addr+u_offset;
	handle->frameInfo[1].V_addr = handle->lpvbits[1].phy_addr+u_offset+v_offset;
	VIU_SetCH_BaseAddr(0,0,handle);

	//	VI_CHCfg(VI0_REG_NEWER , 0x1);

	print_viu_values();
	return 0;

}


//480p seperate
static int VIU_Config_MODE_PROGRESSIVE_SEPERATE_PORT0(VIU_handle_t *handle)
{
	int width,height;
	VIU_StoreMode_e format;
	unsigned int y_addr;
	int cap_start,cap_size;
	int  port_clk_sel_div;
	unsigned int start_x,start_y,width_align,height_align;
	int act1_voff, act1_height, act2_voff, act2_height, act_hoff, act_width;   
	int vi_p0_vsync1, vi_p0_vsync2, vi_p0_hsync;   

	VIU_Mask_Info_t *mask_info;
	unsigned int u_offset,v_offset,totalSize;
	unsigned int regValue;
	unsigned int mirror,flip;
	VIU_ScaleMode_e scaleMode;

	printk("****VIU_Config_MODE_PROGRESSIVE_SEPERATE_PORT0\n");
	scaleMode= handle ->scaleMode;

	mask_info = &(handle->mask_info);
	width = handle->width;
	height = handle->height;
	format = handle ->storeFormat;
	y_addr = handle->lpvbits[0].phy_addr;

	VI_PortCfg(0,MODE_PROGRESSIVE_SEPERATE_PORT0_CFG_PORT0);
	VI_PortReset(0);
#if 1
	port_clk_sel_div = 0x000002a8;

	VI_CHCfg(PORT_CLK_SEL_DIV, port_clk_sel_div);
#endif

	mirror = handle->sym_info.mirror;
	flip = handle->sym_info.flip;
#if 0
	mirror = 1;
	flip = 1;
#endif
	if(format==Mode_Planar)
	{
		regValue = (MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0 | STORE_MODE_PLANAR | INPUT_ORDER_CBYCRY);
	}
	else if(format==Mode_SemiPlanar)
	{
		regValue = (MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CBYCRY);
	}
	else if(format==Mode_Package)
	{
		regValue = (MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0 | STORE_MODE_PACKAGE | INPUT_ORDER_CBYCRY);
	}
	else
	{		regValue = (MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CFG_CH0 | STORE_MODE_RAW);
	}
	printk("scale mode:%d\n",scaleMode);
	//scale configuration  TODO

	if(scaleMode==SCALE_Y_NO_C_1_2)
	{

		regValue |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FIELD ;
		//	regValue |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FRAME ;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height;

	}
	else if(scaleMode==SCALE_Y_NO_C_1_4)
	{

		regValue |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_NO_C_1_8)
	{
		regValue |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_1_2_C_NO)
	{

		regValue |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height/2;
	}
	else if(scaleMode==SCALE_Y_1_2_C_1_2)
	{

		regValue |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_4)
	{

		regValue |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_8)
	{

		regValue |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height/2;
	}

	else
	{

		regValue |= CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height;

	}
	if(mirror == 1)
	{
		regValue |= MIRROR_MODE;
	}
	if(flip == 1)
	{
		regValue |= FLIP_MODE;
	}
	VI_CHCfg(VI0_CH_CFG ,regValue);


	VI_CHCfg(VI0_INT_EN, MODE_PROGRESSIVE_SEPERATE_PORT0_INT_EN_CH0);
	VI_SetCHFifoDepth(0, MODE_PROGRESSIVE_SEPERATE_PORT0_FIFO0_DEPTH_CH0, MODE_PROGRESSIVE_SEPERATE_PORT0_FIFO1_DEPTH_CH0, MODE_PROGRESSIVE_SEPERATE_PORT0_FIFO2_DEPTH_CH0);



	// VI_P0_VSYNC1, VI_P0_VSYNC2
	act1_voff   = MODE_PROGRESSIVE_SEPERATE_PORT0_VOFF1-1;
	act1_height = height-1;
	//act1_height = 480-1;

	act2_voff   = MODE_PROGRESSIVE_SEPERATE_PORT0_VOFF2-1;
	act2_height = height-1;
	//act2_height = 480-1;

	vi_p0_vsync1 = (act1_voff << 14) + act1_height;
	vi_p0_vsync2 = (act2_voff << 14) + act2_height;

	VI_CHCfg(VI_P0_VSYNC1, vi_p0_vsync1);

	VI_CHCfg(VI_P0_VSYNC2, vi_p0_vsync2);

	// VI_P0_HSYNC
	act_hoff  = MODE_PROGRESSIVE_SEPERATE_PORT0_HOFF-1;
	act_width = width*2-1;
	//act_width = 720*2-1;
	vi_p0_hsync = (act_hoff << 14) + act_width;

	VI_CHCfg(VI_P0_HSYNC, vi_p0_hsync);

	start_x = handle->crop_info.startX*2;

	start_y = handle->crop_info.startY;

	printk("start_x:%d ,start_y:%d\n",start_x,start_y);
	cap_start =( (start_y)<<16)+(start_x);

	cap_size  = ((handle->crop_info.height << 16) +handle->crop_info.width);


	printk("cap_width:%d,cap_height:%d\n",handle->crop_info.width,handle->crop_info.width);
	printk("store_width:%d,store_height:%d\n",width_align,height_align);


	VI_SetCHCapInfo(0, cap_start, cap_size);

	regValue = 0;
	regValue = MODE_PROGRESSIVE_SEPERATE_PORT0_CH_CTRL_CH0;

	if(mask_info->mask_attrs[0].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK0_START , (((mask_info->mask_attrs[0].startY))<<14)+(mask_info->mask_attrs[0].startX)*2);
		VI_CHCfg(VI0_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height)<<14)+(mask_info->mask_attrs[0].width)*2);
		VI_CHCfg(VI0_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		regValue |= MASK_0; 

	}
	if(mask_info->mask_attrs[1].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK1_START , (((mask_info->mask_attrs[1].startY))<<14)+(mask_info->mask_attrs[1].startX)*2);
		VI_CHCfg(VI0_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height)<<14)+(mask_info->mask_attrs[1].width)*2);
		VI_CHCfg(VI0_BLOCK1_COLOR , mask_info->mask_attrs[1].color);

		regValue |= MASK_1; 

	}

	if(mask_info->mask_attrs[2].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK2_START , (((mask_info->mask_attrs[2].startY))<<14)+(mask_info->mask_attrs[2].startX)*2);
		VI_CHCfg(VI0_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height)<<14)+(mask_info->mask_attrs[2].width)*2);
		VI_CHCfg(VI0_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		regValue |= MASK_2; 

	}

	if(mask_info->mask_attrs[3].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK3_START , (((mask_info->mask_attrs[3].startY))<<14)+(mask_info->mask_attrs[3].startX)*2);
		VI_CHCfg(VI0_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height)<<14)+(mask_info->mask_attrs[3].width)*2);
		VI_CHCfg(VI0_BLOCK3_COLOR , mask_info->mask_attrs[3].color);

		regValue |= MASK_3; 

	}
	VI_CHCfg(VI0_CH_CTRL,regValue);

	VI_SetCHStoreConfig(0, width_align, height_align,format,scaleMode,y_addr);

	VI_Caculate_YUV_Offset(handle,width_align,height_align,format,&u_offset,&v_offset,&totalSize);
	handle->frameSize = totalSize;
	printk("frameSize:%d\n",handle->frameSize);
	handle->frameInfo[0].Y_addr = handle->lpvbits[0].phy_addr;
	handle->frameInfo[0].U_addr = handle->lpvbits[0].phy_addr+u_offset;
	handle->frameInfo[0].V_addr = handle->lpvbits[0].phy_addr+u_offset+v_offset;

	handle->frameInfo[1].Y_addr = handle->lpvbits[1].phy_addr;
	handle->frameInfo[1].U_addr = handle->lpvbits[1].phy_addr+u_offset;
	handle->frameInfo[1].V_addr = handle->lpvbits[1].phy_addr+u_offset+v_offset;
	VIU_SetCH_BaseAddr(0,0,handle);

	//	VI_CHCfg(VI0_REG_NEWER , 0x1);

	print_viu_values();
	return 0;

}

//bt1120 yc sep  1080i(embedded)
static int VIU_Config_MODE_INTERLACE_EMBBED_PORT0_1(VIU_handle_t *handle)
{
	int width,height;
	VIU_StoreMode_e format;
	unsigned int y_addr;
	int cap_start,cap_size;
	int  port_clk_sel_div;
	unsigned int start_x,start_y,width_align,height_align;
	VIU_Mask_Info_t *mask_info;

	unsigned int u_offset,v_offset,totalSize;
	unsigned int regValue1,regValue2;

	unsigned int mirror,flip;
	VIU_ScaleMode_e scaleMode;
	printk("***MODE_INTERLACE_EMBBED_PORT0_1\n");
	width = handle->width;
	height = handle->height;
	format = handle ->storeFormat;
	mask_info = &(handle->mask_info);
	scaleMode= handle ->scaleMode;
	regValue2 = regValue1 = 0;
	y_addr = handle->lpvbits[0].phy_addr;
	VI_PortCfg(0,MODE_INTERLACE_EMBBED_PORT0_1_CFG_PORT0);
	VI_PortCfg(1,MODE_INTERLACE_EMBBED_PORT0_1_CFG_PORT1);

	VI_PortReset(0);
	VI_PortReset(1);
#if 1
	port_clk_sel_div = 0x000002a9;

	VI_CHCfg(PORT_CLK_SEL_DIV, port_clk_sel_div);
#endif
	mirror = handle->sym_info.mirror;
	flip = handle->sym_info.flip;
#if 0
	mirror = 1;
	flip = 1;
#endif
	if(format==Mode_Planar)
	{
		regValue1 = (MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH0 | STORE_MODE_PLANAR | INPUT_ORDER_CBYCRY);
		regValue2 = (MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH2 | STORE_MODE_PLANAR | INPUT_ORDER_CBYCRY);
	}
	else if(format==Mode_SemiPlanar)
	{

#if 1
		regValue1 = (MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CBYCRY);
		regValue2 = (MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH2 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CBYCRY);
#endif
#if 0
		regValue1 = (MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_YCRYCB);
		regValue2 = (MODE_INTERLACE_EMBBED_PORT0_1_CH_CFG_CH2 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_YCRYCB);
#endif
	}

	printk("scale mode:%d\n",scaleMode);
	//scale configuration  TODO




	if(scaleMode==SCALE_Y_NO_C_1_2)
	{

		regValue1 |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FRAME ;
		regValue2 |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FRAME ;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height;

	}
	else if(scaleMode==SCALE_Y_NO_C_1_4)
	{

		regValue1 |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		regValue2 |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_NO_C_1_8)
	{
		regValue1 |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		regValue2 |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_1_2_C_NO)
	{
#if 1
		regValue1 |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		regValue2 |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
#endif	

		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height/2;
	}
	else if(scaleMode==SCALE_Y_1_2_C_1_2)
	{

		regValue1 |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_4)
	{

		regValue1 |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_8)
	{

		regValue1 |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height/2;
	}

	else
	{

		regValue1 |= CAP_SEL_BOTH| STORE_MODE_FRAME;
		regValue2 |= CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height;

	}
	if(mirror == 1)
	{
		regValue1 |= MIRROR_MODE;
		regValue2 |= MIRROR_MODE;
	}
	if(flip == 1)
	{
		regValue1 |= FLIP_MODE;
		regValue2 |= FLIP_MODE;
	}
	VI_CHCfg(VI0_CH_CFG ,regValue1);
	VI_CHCfg(VI2_CH_CFG ,regValue2);

	VI_CHCfg(VI0_INT_EN, MODE_INTERLACE_EMBBED_PORT0_1_INT_EN_CH0);
	VI_CHCfg(VI2_INT_EN, MODE_INTERLACE_EMBBED_PORT0_1_INT_EN_CH2);


	//Ch0
	VI_SetCHFifoDepth(0, MODE_INTERLACE_EMBBED_PORT0_1_FIFO0_DEPTH_CH0,MODE_INTERLACE_EMBBED_PORT0_1_FIFO1_DEPTH_CH0,
			MODE_INTERLACE_EMBBED_PORT0_1_FIFO2_DEPTH_CH0);
	//ch2
	VI_SetCHFifoDepth(2,MODE_INTERLACE_EMBBED_PORT0_1_FIFO0_DEPTH_CH2,MODE_INTERLACE_EMBBED_PORT0_1_FIFO1_DEPTH_CH2,MODE_INTERLACE_EMBBED_PORT0_1_FIFO2_DEPTH_CH2);


	start_x = handle->crop_info.startX;

	start_y = handle->crop_info.startY/2;

	printk("start_x:%d ,start_y:%d\n",start_x,start_y);
	cap_start =( (start_y)<<16)+(start_x);

	printk("cap_width:%d,cap_height:%d\n",handle->crop_info.width,height_align);
	printk("store_width:%d,store_height:%d\n",width_align,height_align);

	cap_size  = ((height_align << 16) +handle->crop_info.width);



	regValue2=regValue1 = 0;
	regValue1 =MODE_INTERLACE_EMBBED_PORT0_1_CH_CTRL_CH0;
	regValue2 =MODE_INTERLACE_EMBBED_PORT0_1_CH_CTRL_CH2;

	if(mask_info->mask_attrs[0].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK0_START , (((mask_info->mask_attrs[0].startY)/2)<<14)+(mask_info->mask_attrs[0].startX));
		VI_CHCfg(VI0_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height/2)<<14)+(mask_info->mask_attrs[0].width));
		VI_CHCfg(VI0_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		VI_CHCfg(VI2_BLOCK0_START , (((mask_info->mask_attrs[0].startY)/2)<<14)+(mask_info->mask_attrs[0].startX));
		VI_CHCfg(VI2_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height/2)<<14)+(mask_info->mask_attrs[0].width));
		VI_CHCfg(VI2_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		regValue1 |= MASK_0; 
		regValue2 |= MASK_0; 

	}
	if(mask_info->mask_attrs[1].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK1_START , (((mask_info->mask_attrs[1].startY)/2)<<14)+(mask_info->mask_attrs[1].startX));
		VI_CHCfg(VI0_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height/2)<<14)+(mask_info->mask_attrs[1].width));
		VI_CHCfg(VI0_BLOCK1_COLOR , mask_info->mask_attrs[1].color);
		VI_CHCfg(VI2_BLOCK1_START , (((mask_info->mask_attrs[1].startY)/2)<<14)+(mask_info->mask_attrs[1].startX));
		VI_CHCfg(VI2_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height/2)<<14)+(mask_info->mask_attrs[1].width));
		VI_CHCfg(VI2_BLOCK1_COLOR , mask_info->mask_attrs[1].color);
		regValue1 |= MASK_1; 
		regValue2 |= MASK_1; 

	}

	if(mask_info->mask_attrs[2].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK2_START , (((mask_info->mask_attrs[2].startY)/2)<<14)+(mask_info->mask_attrs[2].startX));
		VI_CHCfg(VI0_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height/2)<<14)+(mask_info->mask_attrs[2].width));
		VI_CHCfg(VI0_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		VI_CHCfg(VI2_BLOCK2_START , (((mask_info->mask_attrs[2].startY)/2)<<14)+(mask_info->mask_attrs[2].startX));
		VI_CHCfg(VI2_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height/2)<<14)+(mask_info->mask_attrs[2].width));
		VI_CHCfg(VI2_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		regValue1 |= MASK_2; 
		regValue2 |= MASK_2; 

	}

	if(mask_info->mask_attrs[3].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK3_START , (((mask_info->mask_attrs[3].startY)/2)<<14)+(mask_info->mask_attrs[3].startX));
		VI_CHCfg(VI0_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height/2)<<14)+(mask_info->mask_attrs[3].width));
		VI_CHCfg(VI0_BLOCK3_COLOR , mask_info->mask_attrs[3].color);


		VI_CHCfg(VI2_BLOCK3_START , (((mask_info->mask_attrs[3].startY)/2)<<14)+(mask_info->mask_attrs[3].startX));
		VI_CHCfg(VI2_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height/2)<<14)+(mask_info->mask_attrs[3].width));
		VI_CHCfg(VI2_BLOCK3_COLOR , mask_info->mask_attrs[3].color);

		regValue1 |= MASK_3; 

		regValue2 |= MASK_3; 

	}




	VI_CHCfg(VI0_CH_CTRL,regValue1);
	VI_CHCfg(VI2_CH_CTRL,regValue2);
	VI_SetCHCapInfo(0, cap_start, cap_size);
	VI_SetCHCapInfo(2, cap_start, cap_size);
	VI_SetCH_PORT0_1_StoreConfig(width_align, height_align,format,scaleMode,y_addr);

	VI_Caculate_YUV_Offset(handle,width_align,height_align,format,&u_offset,&v_offset,&totalSize);
	handle->frameSize = totalSize;
	printk("frameSize:%d\n",handle->frameSize);

	handle->frameInfo[0].Y_addr = handle->lpvbits[0].phy_addr;
	handle->frameInfo[0].U_addr = handle->lpvbits[0].phy_addr+u_offset;
	handle->frameInfo[0].V_addr = handle->lpvbits[0].phy_addr+u_offset+v_offset;

	handle->frameInfo[1].Y_addr = handle->lpvbits[1].phy_addr;
	handle->frameInfo[1].U_addr = handle->lpvbits[1].phy_addr+u_offset;
	handle->frameInfo[1].V_addr = handle->lpvbits[1].phy_addr+u_offset+v_offset;
	VIU_SetCH_PORT0_1_BaseAddr(0,handle);

	//VI_CHCfg(VI0_REG_NEWER , 0x1);
	//VI_CHCfg(VI2_REG_NEWER , 0x1);

	print_viu_values();
	return 0;
}


//yc seperate interlace_sep
static int VIU_Config_MODE_INTERLACE_SEPERATE_PORT0_1(VIU_handle_t *handle)
{
	int width,height;
	VIU_StoreMode_e format;
	unsigned int y_addr;
	int cap_start,cap_size;
	int  port_clk_sel_div;
	unsigned int start_x,start_y,width_align,height_align;

	int act1_voff, act1_height, act2_voff, act2_height, act_hoff, act_width;
	int vi_p0_vsync1, vi_p0_vsync2, vi_p0_hsync;
	unsigned int regValue1,regValue2;

	unsigned int u_offset,v_offset,totalSize;

	unsigned int mirror,flip;
	VIU_ScaleMode_e scaleMode;
	VIU_Mask_Info_t *mask_info;

	printk("***MODE_INTERLACE_SEPERATE_PORT0_1\n");
	width = handle->width;
	height = handle->height;
	format = handle ->storeFormat;

	mask_info = &(handle->mask_info);


	scaleMode= handle ->scaleMode;
	regValue2 = regValue1 = 0;
	y_addr = handle->lpvbits[0].phy_addr;
	VI_PortCfg(0,MODE_INTERLACE_SEPERATE_PORT0_1_CFG_PORT0);
	VI_PortCfg(1,MODE_INTERLACE_SEPERATE_PORT0_1_CFG_PORT1);

	VI_PortReset(0);
	VI_PortReset(1);
#if 1
	port_clk_sel_div = 0x000002a9;

	VI_CHCfg(PORT_CLK_SEL_DIV, port_clk_sel_div);
#endif

	mirror = handle->sym_info.mirror;
	flip = handle->sym_info.flip;
#if 0
	mirror = 1;
	flip = 1;
#endif
	if(format==Mode_Planar)
	{
		regValue1 = (MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH0 | STORE_MODE_PLANAR);
		regValue2 = (MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH2 | STORE_MODE_PLANAR);
	}
	else if(format==Mode_SemiPlanar)
	{
		regValue1 = (MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR);
		regValue2 = (MODE_INTERLACE_SEPERATE_PORT0_1_CH_CFG_CH2 | STORE_MODE_SEMIPLANAR);
	}

	printk("scale mode:%d\n",scaleMode);
	//scale configuration  TODO



	if(scaleMode==SCALE_Y_NO_C_1_2)
	{

		regValue1 |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FRAME ;
		regValue2 |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FRAME ;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height;

	}
	else if(scaleMode==SCALE_Y_NO_C_1_4)
	{

		regValue1 |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		regValue2 |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_NO_C_1_8)
	{
		regValue1 |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		regValue2 |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_1_2_C_NO)
	{
#if 1
		regValue1 |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		regValue2 |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
#endif	

		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height/2;
	}
	else if(scaleMode==SCALE_Y_1_2_C_1_2)
	{

		regValue1 |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_4)
	{

		regValue1 |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_8)
	{

		regValue1 |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height/2;
	}

	else
	{

		regValue1 |= CAP_SEL_BOTH| STORE_MODE_FRAME;
		regValue2 |= CAP_SEL_BOTH| STORE_MODE_FRAME;
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height;

	}
	if(mirror == 1)
	{
		regValue1 |= MIRROR_MODE;
		regValue2 |= MIRROR_MODE;
	}
	if(flip == 1)
	{
		regValue1 |= FLIP_MODE;
		regValue2 |= FLIP_MODE;
	}
	VI_CHCfg(VI0_CH_CFG ,regValue1);
	VI_CHCfg(VI2_CH_CFG ,regValue2);

	VI_CHCfg(VI0_INT_EN,MODE_INTERLACE_SEPERATE_PORT0_1_INT_EN_CH0);
	VI_CHCfg(VI2_INT_EN, MODE_INTERLACE_SEPERATE_PORT0_1_INT_EN_CH2);
	//Ch0
	VI_SetCHFifoDepth(0, MODE_INTERLACE_SEPERATE_PORT0_1_FIFO0_DEPTH_CH0,MODE_INTERLACE_SEPERATE_PORT0_1_FIFO1_DEPTH_CH0,MODE_INTERLACE_SEPERATE_PORT0_1_FIFO2_DEPTH_CH0);
	//ch2
	VI_SetCHFifoDepth(2,MODE_INTERLACE_SEPERATE_PORT0_1_FIFO0_DEPTH_CH2,MODE_INTERLACE_SEPERATE_PORT0_1_FIFO1_DEPTH_CH2,MODE_INTERLACE_SEPERATE_PORT0_1_FIFO2_DEPTH_CH2);

	//TODO
#define SEPARATED_SYNC
#ifdef SEPARATED_SYNC
	// VI_P0_VSYNC1, VI_P0_VSYNC2
	act1_voff   = MODE_INTERLACE_SEPERATE_PORT0_1_VOFF1-1;
	act1_height = height/2-1;
	//	act1_height = height-1;

	act2_voff   =MODE_INTERLACE_SEPERATE_PORT0_1_VOFF2-1;
	act2_height = height/2-1;
	//	act2_height = height-1;

	vi_p0_vsync1 = (act1_voff << 14) + act1_height;
	vi_p0_vsync2 = (act2_voff << 14) + act2_height;

	VI_CHCfg(VI_P0_VSYNC1, vi_p0_vsync1);



	VI_CHCfg(VI_P0_VSYNC2, vi_p0_vsync2);


	// VI_P0_HSYNC
	act_hoff  = MODE_INTERLACE_SEPERATE_PORT0_1_HOFF-1;
	act_width = width-1;
	vi_p0_hsync = (act_hoff << 14) + act_width;

	VI_CHCfg(VI_P0_HSYNC, vi_p0_hsync);

#endif


	start_x = handle->crop_info.startX;

	start_y = handle->crop_info.startY/2;

	printk("start_x:%d ,start_y:%d\n",start_x,start_y);
	cap_start =( (start_y)<<16)+(start_x);


	printk("cap_width:%d,cap_height:%d\n",handle->crop_info.width,height_align);
	printk("store_width:%d,store_height:%d\n",width_align,height_align);

	cap_size  = ((height_align << 16) +handle->crop_info.width);

	regValue2=regValue1 = 0;
	regValue1 =MODE_INTERLACE_SEPERATE_PORT0_1_CH_CTRL_CH0;
	regValue2 =MODE_INTERLACE_SEPERATE_PORT0_1_CH_CTRL_CH2;

	if(mask_info->mask_attrs[0].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK0_START , (((mask_info->mask_attrs[0].startY)/2)<<14)+(mask_info->mask_attrs[0].startX));
		VI_CHCfg(VI0_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height/2)<<14)+(mask_info->mask_attrs[0].width));
		VI_CHCfg(VI0_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		VI_CHCfg(VI2_BLOCK0_START , (((mask_info->mask_attrs[0].startY)/2)<<14)+(mask_info->mask_attrs[0].startX));
		VI_CHCfg(VI2_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height/2)<<14)+(mask_info->mask_attrs[0].width));
		VI_CHCfg(VI2_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		regValue1 |= MASK_0; 
		regValue2 |= MASK_0; 

	}
	if(mask_info->mask_attrs[1].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK1_START , (((mask_info->mask_attrs[1].startY)/2)<<14)+(mask_info->mask_attrs[1].startX));
		VI_CHCfg(VI0_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height/2)<<14)+(mask_info->mask_attrs[1].width));
		VI_CHCfg(VI0_BLOCK1_COLOR , mask_info->mask_attrs[1].color);
		VI_CHCfg(VI2_BLOCK1_START , (((mask_info->mask_attrs[1].startY)/2)<<14)+(mask_info->mask_attrs[1].startX));
		VI_CHCfg(VI2_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height/2)<<14)+(mask_info->mask_attrs[1].width));
		VI_CHCfg(VI2_BLOCK1_COLOR , mask_info->mask_attrs[1].color);
		regValue1 |= MASK_1; 
		regValue2 |= MASK_1; 

	}

	if(mask_info->mask_attrs[2].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK2_START , (((mask_info->mask_attrs[2].startY)/2)<<14)+(mask_info->mask_attrs[2].startX));
		VI_CHCfg(VI0_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height/2)<<14)+(mask_info->mask_attrs[2].width));
		VI_CHCfg(VI0_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		VI_CHCfg(VI2_BLOCK2_START , (((mask_info->mask_attrs[2].startY)/2)<<14)+(mask_info->mask_attrs[2].startX));
		VI_CHCfg(VI2_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height/2)<<14)+(mask_info->mask_attrs[2].width));
		VI_CHCfg(VI2_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		regValue1 |= MASK_2; 
		regValue2 |= MASK_2; 

	}

	if(mask_info->mask_attrs[3].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK3_START , (((mask_info->mask_attrs[3].startY)/2)<<14)+(mask_info->mask_attrs[3].startX));
		VI_CHCfg(VI0_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height/2)<<14)+(mask_info->mask_attrs[3].width));
		VI_CHCfg(VI0_BLOCK3_COLOR , mask_info->mask_attrs[3].color);


		VI_CHCfg(VI2_BLOCK3_START , (((mask_info->mask_attrs[3].startY)/2)<<14)+(mask_info->mask_attrs[3].startX));
		VI_CHCfg(VI2_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height/2)<<14)+(mask_info->mask_attrs[3].width));
		VI_CHCfg(VI2_BLOCK3_COLOR , mask_info->mask_attrs[3].color);

		regValue1 |= MASK_3; 

		regValue2 |= MASK_3; 

	}

	VI_CHCfg(VI0_CH_CTRL,regValue1);
	VI_CHCfg(VI2_CH_CTRL,regValue2);


	VI_SetCHCapInfo(0, cap_start, cap_size);
	VI_SetCHCapInfo(2, cap_start, cap_size);
	VI_SetCH_PORT0_1_StoreConfig(width_align, height_align,format,scaleMode,y_addr);

	VI_Caculate_YUV_Offset(handle,width_align,height_align,format,&u_offset,&v_offset,&totalSize);
	handle->frameSize = totalSize;
	printk("frameSize:%d\n",handle->frameSize);
	handle->frameInfo[0].Y_addr = handle->lpvbits[0].phy_addr;
	handle->frameInfo[0].U_addr = handle->lpvbits[0].phy_addr+u_offset;
	handle->frameInfo[0].V_addr = handle->lpvbits[0].phy_addr+u_offset+v_offset;

	handle->frameInfo[1].Y_addr = handle->lpvbits[1].phy_addr;
	handle->frameInfo[1].U_addr = handle->lpvbits[1].phy_addr+u_offset;
	handle->frameInfo[1].V_addr = handle->lpvbits[1].phy_addr+u_offset+v_offset;
	VIU_SetCH_PORT0_1_BaseAddr(0,handle);
	//	VI_CHCfg(VI0_REG_NEWER , 0x1);
	//	VI_CHCfg(VI2_REG_NEWER , 0x1);

	print_viu_values();
	return 0;
}

// yc sep  720p(embedded)
static int VIU_Config_MODE_PROGRESSIVE_EMBBED_PORT0_1(VIU_handle_t *handle)
{
	int width,height;
	VIU_StoreMode_e format;
	unsigned int y_addr;
	int cap_start,cap_size;
	int  port_clk_sel_div;
	unsigned int start_x,start_y,width_align,height_align;
	unsigned int regValue1=0,regValue2=0;
	VIU_Mask_Info_t *mask_info;
	VIU_ScaleMode_e scaleMode;
	unsigned int u_offset,v_offset,totalSize;

unsigned int mirror,flip;
	printk("\n**MODE_PROGRESSIVE_EMBBED_PORT0_1\n");
	width = handle->width;
	height = handle->height;
	format = handle ->storeFormat;

	mask_info = &(handle->mask_info);
	scaleMode= handle ->scaleMode;
	y_addr = handle->lpvbits[0].phy_addr;
	VI_PortCfg(0,MODE_PROGRESSIVE_EMBBED_PORT0_1_CFG_PORT0);
	VI_PortCfg(1,MODE_PROGRESSIVE_EMBBED_PORT0_1_CFG_PORT1);

	VI_PortReset(0);
	VI_PortReset(1);
#if 1
	port_clk_sel_div = 0x000002a9;
	VI_CHCfg(PORT_CLK_SEL_DIV, port_clk_sel_div);
#endif
    mirror = handle->sym_info.mirror;
    flip = handle->sym_info.flip;
#if 0
    mirror = 1;
    flip = 1;
#endif
	if(format==Mode_Planar)
	{
		regValue1 = (MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH0 | STORE_MODE_PLANAR | INPUT_ORDER_CBYCRY);
		regValue2 = (MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH2 | STORE_MODE_PLANAR | INPUT_ORDER_CBYCRY);
	}
	else if(format==Mode_SemiPlanar)
	{
		regValue1 = (MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CBYCRY);
		regValue2 = (MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CFG_CH2 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CBYCRY);
	}

	printk("scale mode:%d\n",scaleMode);
	//scale configuration  TODO





	if(scaleMode==SCALE_Y_NO_C_1_2)
	{

		regValue1 |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FIELD ;
		regValue2 |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FIELD ;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height;

	}
	else if(scaleMode==SCALE_Y_NO_C_1_4)
	{

		regValue1 |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_NO_C_1_8)
	{
		regValue1 |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_1_2_C_NO)
	{
		regValue1 |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		regValue2 |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height/2;
	}
	else if(scaleMode==SCALE_Y_1_2_C_1_2)
	{

		regValue1 |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_4)
	{

		regValue1 |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_8)
	{

		regValue1 |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height/2;
	}

	else
	{

		regValue1 |= CAP_SEL_BOTH| STORE_MODE_FIELD;
		regValue2 |= CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height;

	}
if(mirror == 1)
{
		regValue1 |= MIRROR_MODE;
		regValue2 |= MIRROR_MODE;
}
if(flip == 1)
{
		regValue1 |= FLIP_MODE;
		regValue2 |= FLIP_MODE;
}

	VI_CHCfg(VI0_CH_CFG ,regValue1);
	VI_CHCfg(VI2_CH_CFG ,regValue2);


	VI_CHCfg(VI0_INT_EN, MODE_PROGRESSIVE_EMBBED_PORT0_1_INT_EN_CH0);
	VI_CHCfg(VI2_INT_EN, MODE_PROGRESSIVE_EMBBED_PORT0_1_INT_EN_CH2);
	//Ch0
	VI_SetCHFifoDepth(0, MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO0_DEPTH_CH0,MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO1_DEPTH_CH0,
			MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO2_DEPTH_CH0);
	//ch2
	VI_SetCHFifoDepth(2,MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO0_DEPTH_CH2,MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO1_DEPTH_CH2,MODE_PROGRESSIVE_EMBBED_PORT0_1_FIFO2_DEPTH_CH2);




	start_x = handle->crop_info.startX;

	start_y = handle->crop_info.startY;

	printk("start_x:%d ,start_y:%d\n",start_x,start_y);
	cap_start =( (start_y)<<16)+(start_x);


	printk("cap_width:%d,cap_height:%d\n",handle->crop_info.width,handle->crop_info.height);
	printk("store_width:%d,store_height:%d\n",width_align,height_align);

	cap_size  = ((handle->crop_info.height << 16) +handle->crop_info.width);



	regValue2=regValue1 = 0;
	regValue1 =MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CTRL_CH0;
	regValue2 =MODE_PROGRESSIVE_EMBBED_PORT0_1_CH_CTRL_CH2;

	if(mask_info->mask_attrs[0].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK0_START , (((mask_info->mask_attrs[0].startY))<<14)+(mask_info->mask_attrs[0].startX));
		VI_CHCfg(VI0_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height)<<14)+(mask_info->mask_attrs[0].width));
		VI_CHCfg(VI0_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		VI_CHCfg(VI2_BLOCK0_START , (((mask_info->mask_attrs[0].startY))<<14)+(mask_info->mask_attrs[0].startX));
		VI_CHCfg(VI2_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height)<<14)+(mask_info->mask_attrs[0].width));
		VI_CHCfg(VI2_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		regValue1 |= MASK_0; 
		regValue2 |= MASK_0; 

	}
	if(mask_info->mask_attrs[1].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK1_START , (((mask_info->mask_attrs[1].startY))<<14)+(mask_info->mask_attrs[1].startX));
		VI_CHCfg(VI0_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height)<<14)+(mask_info->mask_attrs[1].width));
		VI_CHCfg(VI0_BLOCK1_COLOR , mask_info->mask_attrs[1].color);
		VI_CHCfg(VI2_BLOCK1_START , (((mask_info->mask_attrs[1].startY))<<14)+(mask_info->mask_attrs[1].startX));
		VI_CHCfg(VI2_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height)<<14)+(mask_info->mask_attrs[1].width));
		VI_CHCfg(VI2_BLOCK1_COLOR , mask_info->mask_attrs[1].color);
		regValue1 |= MASK_1; 
		regValue2 |= MASK_1; 

	}

	if(mask_info->mask_attrs[2].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK2_START , (((mask_info->mask_attrs[2].startY))<<14)+(mask_info->mask_attrs[2].startX));
		VI_CHCfg(VI0_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height)<<14)+(mask_info->mask_attrs[2].width));
		VI_CHCfg(VI0_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		VI_CHCfg(VI2_BLOCK2_START , (((mask_info->mask_attrs[2].startY))<<14)+(mask_info->mask_attrs[2].startX));
		VI_CHCfg(VI2_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height)<<14)+(mask_info->mask_attrs[2].width));
		VI_CHCfg(VI2_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		regValue1 |= MASK_2; 
		regValue2 |= MASK_2; 

	}

	if(mask_info->mask_attrs[3].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK3_START , (((mask_info->mask_attrs[3].startY))<<14)+(mask_info->mask_attrs[3].startX));
		VI_CHCfg(VI0_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height)<<14)+(mask_info->mask_attrs[3].width));
		VI_CHCfg(VI0_BLOCK3_COLOR , mask_info->mask_attrs[3].color);


		VI_CHCfg(VI2_BLOCK3_START , (((mask_info->mask_attrs[3].startY))<<14)+(mask_info->mask_attrs[3].startX));
		VI_CHCfg(VI2_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height)<<14)+(mask_info->mask_attrs[3].width));
		VI_CHCfg(VI2_BLOCK3_COLOR , mask_info->mask_attrs[3].color);

		regValue1 |= MASK_3; 

		regValue2 |= MASK_3; 

	}




	VI_CHCfg(VI0_CH_CTRL,regValue1);
	VI_CHCfg(VI2_CH_CTRL,regValue2);


	VI_SetCHCapInfo(0, cap_start, cap_size);
	VI_SetCHCapInfo(2, cap_start, cap_size);

	VI_SetCH_PORT0_1_StoreConfig(width_align, height_align,format,scaleMode,y_addr);

	VI_Caculate_YUV_Offset(handle,width_align,height_align,format,&u_offset,&v_offset,&totalSize);
	handle->frameSize = totalSize;
	printk("frameSize:%d\n",handle->frameSize);
	handle->frameInfo[0].Y_addr = handle->lpvbits[0].phy_addr;
	handle->frameInfo[0].U_addr = handle->lpvbits[0].phy_addr+u_offset;
	handle->frameInfo[0].V_addr = handle->lpvbits[0].phy_addr+u_offset+v_offset;

	handle->frameInfo[1].Y_addr = handle->lpvbits[1].phy_addr;
	handle->frameInfo[1].U_addr = handle->lpvbits[1].phy_addr+u_offset;
	handle->frameInfo[1].V_addr = handle->lpvbits[1].phy_addr+u_offset+v_offset;
	VIU_SetCH_PORT0_1_BaseAddr(0,handle);


	//VI_CHCfg(VI0_REG_NEWER , 0x1);
	//VI_CHCfg(VI2_REG_NEWER , 0x1);

	print_viu_values();
	return 0;
}


//720P sep
static int VIU_Config_MODE_PROGRESSIVE_SEPERATE_PORT0_1(VIU_handle_t *handle)
{
	int width,height;
	VIU_StoreMode_e format;
	unsigned int y_addr;
	unsigned int start_x,start_y,width_align,height_align;
	int cap_start,cap_size;
	int  port_clk_sel_div;

	unsigned int regValue1,regValue2;
	VIU_ScaleMode_e scaleMode;

	VIU_Mask_Info_t *mask_info;

	unsigned int u_offset,v_offset,totalSize;



	int act1_voff, act1_height, act2_voff, act2_height, act_hoff, act_width;
	int vi_p0_vsync1, vi_p0_vsync2, vi_p0_hsync;
	unsigned int mirror,flip;
	printk("**MODE_PROGRESSIVE_SEPERATE_PORT0_1\n");
	mask_info = &(handle->mask_info);
	scaleMode= handle ->scaleMode;
	width = handle->width;
	height = handle->height;
	format = handle ->storeFormat;
	regValue2 = regValue1 = 0;
	y_addr = handle->lpvbits[0].phy_addr;

	VI_PortCfg(0,MODE_PROGRESSIVE_SEPERATE_PORT0_1_CFG_PORT0);
	VI_PortCfg(1,MODE_PROGRESSIVE_SEPERATE_PORT0_1_CFG_PORT1);

	VI_PortReset(0);
	VI_PortReset(1);
#if 1
	port_clk_sel_div = 0x000002a9;

	VI_CHCfg(PORT_CLK_SEL_DIV, port_clk_sel_div);
#endif
	mirror = handle->sym_info.mirror;
	flip = handle->sym_info.flip;
#if 0
	mirror = 1;
	flip = 1;
#endif
	if(format==Mode_Planar)
	{
		regValue1 = (MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH0 | STORE_MODE_PLANAR |INPUT_ORDER_CBYCRY);
		regValue2 = (MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH2 | STORE_MODE_PLANAR |INPUT_ORDER_CBYCRY);



	}
	else if(format==Mode_SemiPlanar)
	{

#if 1
		regValue1 = (MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CBYCRY);
		regValue2 = (MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH2 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CBYCRY);
#else
		regValue1 = (MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH0 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CRYCBY);
		regValue2 = (MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CFG_CH2 | STORE_MODE_SEMIPLANAR | INPUT_ORDER_CRYCBY);
#endif
	}

	printk("scale mode:%d\n",scaleMode);
	//scale configuration  TODO
	if(scaleMode==SCALE_Y_NO_C_1_2)
	{

		regValue1 |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FIELD ;
		regValue2 |= SCALE_MODE_1_2 | CAP_SEL_BOTH | STORE_MODE_FIELD ;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height;

	}
	else if(scaleMode==SCALE_Y_NO_C_1_4)
	{

		regValue1 |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_4 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_NO_C_1_8)
	{
		regValue1 |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_8 | CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height;
	}
	else if(scaleMode==SCALE_Y_1_2_C_NO)
	{
		printk("\n\nscaleMode==SCALE_Y_1_2_C_NO****\n");
#if 1
		regValue1 |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
		regValue2 |= CAP_SEL_ODD| STORE_MODE_FIELD;//only cap odd field
#endif	

		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height/2;
	}
	else if(scaleMode==SCALE_Y_1_2_C_1_2)
	{

		regValue1 |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_2 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/2;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_4)
	{

		regValue1 |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_4 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/4;
		height_align =handle->crop_info.height/2;
	}

	else if(scaleMode==SCALE_Y_1_2_C_1_8)
	{

		regValue1 |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		regValue2 |= SCALE_MODE_1_8 | CAP_SEL_ODD| STORE_MODE_FIELD;
		width_align =handle->crop_info.width/8;
		height_align =handle->crop_info.height/2;
	}

	else
	{

		regValue1 |= CAP_SEL_BOTH| STORE_MODE_FIELD;
		regValue2 |= CAP_SEL_BOTH| STORE_MODE_FIELD;
		width_align =handle->crop_info.width;
		height_align =handle->crop_info.height;

	}
	if(mirror == 1)
	{
		regValue1 |= MIRROR_MODE;
		regValue2 |= MIRROR_MODE;
	}
	if(flip == 1)
	{
		regValue1 |= FLIP_MODE;
		regValue2 |= FLIP_MODE;
	}
	VI_CHCfg(VI0_CH_CFG ,regValue1);
	VI_CHCfg(VI2_CH_CFG ,regValue2);


	VI_CHCfg(VI0_INT_EN,MODE_PROGRESSIVE_SEPERATE_PORT0_1_INT_EN_CH0);
	VI_CHCfg(VI2_INT_EN, MODE_PROGRESSIVE_SEPERATE_PORT0_1_INT_EN_CH2);
	//Ch0
	VI_SetCHFifoDepth(0, MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO0_DEPTH_CH0,MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO1_DEPTH_CH0,MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO2_DEPTH_CH0);
	//ch2
	VI_SetCHFifoDepth(2,MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO0_DEPTH_CH2,MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO1_DEPTH_CH2,MODE_PROGRESSIVE_SEPERATE_PORT0_1_FIFO2_DEPTH_CH2);


#define SEPARATED_SYNC
#ifdef SEPARATED_SYNC
	// VI_P0_VSYNC1, VI_P0_VSYNC2
	act1_voff   = MODE_PROGRESSIVE_SEPERATE_PORT0_1_VOFF1-1;
	act1_height = height-1;

	act2_voff   =MODE_PROGRESSIVE_SEPERATE_PORT0_1_VOFF2-1;
	act2_height = height-1;

	vi_p0_vsync1 = (act1_voff << 14) + act1_height;
	vi_p0_vsync2 = (act2_voff << 14) + act2_height;

	VI_CHCfg(VI_P0_VSYNC1, vi_p0_vsync1);



	VI_CHCfg(VI_P0_VSYNC2, vi_p0_vsync2);


	// VI_P0_HSYNC
	act_hoff  = MODE_PROGRESSIVE_SEPERATE_PORT0_1_HOFF-1;
	act_width = width-1;
	vi_p0_hsync = (act_hoff << 14) + act_width;

	VI_CHCfg(VI_P0_HSYNC, vi_p0_hsync);

#endif

	start_x = handle->crop_info.startX;

	start_y = handle->crop_info.startY;

	printk("start_x:%d ,start_y:%d\n",start_x,start_y);
	cap_start =( (start_y)<<16)+(start_x);


	printk("cap_width:%d,cap_height:%d\n",handle->crop_info.width,handle->crop_info.height);
	printk("store_width:%d,store_height:%d\n",width_align,height_align);


	cap_size  = ((handle->crop_info.height << 16) +handle->crop_info.width);
	regValue2=regValue1 = 0;
	regValue1 =MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CTRL_CH0;
	regValue2 =MODE_PROGRESSIVE_SEPERATE_PORT0_1_CH_CTRL_CH2;

	if(mask_info->mask_attrs[0].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK0_START , (((mask_info->mask_attrs[0].startY))<<14)+(mask_info->mask_attrs[0].startX));
		VI_CHCfg(VI0_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height)<<14)+(mask_info->mask_attrs[0].width));
		VI_CHCfg(VI0_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		VI_CHCfg(VI2_BLOCK0_START , (((mask_info->mask_attrs[0].startY))<<14)+(mask_info->mask_attrs[0].startX));
		VI_CHCfg(VI2_BLOCK0_SIZE , ((mask_info->mask_attrs[0].height)<<14)+(mask_info->mask_attrs[0].width));
		VI_CHCfg(VI2_BLOCK0_COLOR , mask_info->mask_attrs[0].color);

		regValue1 |= MASK_0; 
		regValue2 |= MASK_0; 

	}
	if(mask_info->mask_attrs[1].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK1_START , (((mask_info->mask_attrs[1].startY))<<14)+(mask_info->mask_attrs[1].startX));
		VI_CHCfg(VI0_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height)<<14)+(mask_info->mask_attrs[1].width));
		VI_CHCfg(VI0_BLOCK1_COLOR , mask_info->mask_attrs[1].color);
		VI_CHCfg(VI2_BLOCK1_START , (((mask_info->mask_attrs[1].startY))<<14)+(mask_info->mask_attrs[1].startX));
		VI_CHCfg(VI2_BLOCK1_SIZE , ((mask_info->mask_attrs[1].height)<<14)+(mask_info->mask_attrs[1].width));
		VI_CHCfg(VI2_BLOCK1_COLOR , mask_info->mask_attrs[1].color);
		regValue1 |= MASK_1; 
		regValue2 |= MASK_1; 

	}

	if(mask_info->mask_attrs[2].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK2_START , (((mask_info->mask_attrs[2].startY))<<14)+(mask_info->mask_attrs[2].startX));
		VI_CHCfg(VI0_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height)<<14)+(mask_info->mask_attrs[2].width));
		VI_CHCfg(VI0_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		VI_CHCfg(VI2_BLOCK2_START , (((mask_info->mask_attrs[2].startY))<<14)+(mask_info->mask_attrs[2].startX));
		VI_CHCfg(VI2_BLOCK2_SIZE , ((mask_info->mask_attrs[2].height)<<14)+(mask_info->mask_attrs[2].width));
		VI_CHCfg(VI2_BLOCK2_COLOR , mask_info->mask_attrs[2].color);

		regValue1 |= MASK_2; 
		regValue2 |= MASK_2; 

	}

	if(mask_info->mask_attrs[3].enable== 1)
	{
		VI_CHCfg(VI0_BLOCK3_START , (((mask_info->mask_attrs[3].startY))<<14)+(mask_info->mask_attrs[3].startX));
		VI_CHCfg(VI0_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height)<<14)+(mask_info->mask_attrs[3].width));
		VI_CHCfg(VI0_BLOCK3_COLOR , mask_info->mask_attrs[3].color);


		VI_CHCfg(VI2_BLOCK3_START , (((mask_info->mask_attrs[3].startY))<<14)+(mask_info->mask_attrs[3].startX));
		VI_CHCfg(VI2_BLOCK3_SIZE , ((mask_info->mask_attrs[3].height)<<14)+(mask_info->mask_attrs[3].width));
		VI_CHCfg(VI2_BLOCK3_COLOR , mask_info->mask_attrs[3].color);

		regValue1 |= MASK_3; 

		regValue2 |= MASK_3; 

	}




	VI_CHCfg(VI0_CH_CTRL,regValue1);
	VI_CHCfg(VI2_CH_CTRL,regValue2);


	VI_SetCHCapInfo(0, cap_start, cap_size);
	VI_SetCHCapInfo(2, cap_start, cap_size);
	VI_SetCH_PORT0_1_StoreConfig(width_align, height_align,format,scaleMode,y_addr);
	VI_Caculate_YUV_Offset(handle,width_align,height_align,format,&u_offset,&v_offset,&totalSize);
	handle->frameSize = totalSize;
	printk("frameSize:%d\n",handle->frameSize);
	handle->frameInfo[0].Y_addr = handle->lpvbits[0].phy_addr;
	handle->frameInfo[0].U_addr = handle->lpvbits[0].phy_addr+u_offset;
	handle->frameInfo[0].V_addr = handle->lpvbits[0].phy_addr+u_offset+v_offset;

	handle->frameInfo[1].Y_addr = handle->lpvbits[1].phy_addr;
	handle->frameInfo[1].U_addr = handle->lpvbits[1].phy_addr+u_offset;
	handle->frameInfo[1].V_addr = handle->lpvbits[1].phy_addr+u_offset+v_offset;
	VIU_SetCH_PORT0_1_BaseAddr(0,handle);
	//	VI_CHCfg(VI0_REG_NEWER , 0x1);
	//	VI_CHCfg(VI2_REG_NEWER , 0x1);

	print_viu_values();
	return 0;
}


//configure register for MODE_INTERLACE_EMBBED_PORT0 .etc
int VIU_ConfigCapMode(CapMode_t capMode,VIU_handle_t *handle)
{
	switch(capMode)
	{
		//bt656
		case MODE_INTERLACE_EMBBED_PORT0:
			VIU_Config_MODE_INTERLACE_EMBBED_PORT0(handle);
			break;
			//bt601
		case MODE_INTERLACE_SEPERATE_PORT0:
			VIU_Config_MODE_INTERLACE_SEPERATE_PORT0(handle);

			break;
			//480p embed
		case MODE_PROGRESSIVE_EMBBED_PORT0:
			VIU_Config_MODE_PROGRESSIVE_EMBBED_PORT0(handle);
			break;
			//480p seperate
		case MODE_PROGRESSIVE_SEPERATE_PORT0:
			VIU_Config_MODE_PROGRESSIVE_SEPERATE_PORT0(handle);
			break;
			//BT1120 EMBED
		case MODE_INTERLACE_EMBBED_PORT0_1:
			VIU_Config_MODE_INTERLACE_EMBBED_PORT0_1(handle);
			break;

			//BT1120 seperate
		case MODE_INTERLACE_SEPERATE_PORT0_1:
			VIU_Config_MODE_INTERLACE_SEPERATE_PORT0_1(handle);
			break;

			//720P EMBED
		case MODE_PROGRESSIVE_EMBBED_PORT0_1:
			VIU_Config_MODE_PROGRESSIVE_EMBBED_PORT0_1(handle);
			break;

			//720P seperate
		case MODE_PROGRESSIVE_SEPERATE_PORT0_1:
			VIU_Config_MODE_PROGRESSIVE_SEPERATE_PORT0_1(handle);
			break;


		default:
			printk("unsupport capmode\n");
			break;
	}
	return 0;
}
#if 0
void *VIU_GetHandleFromChnIrq(int irq_status)
{
//TODO
return NULL;
}

//reset the capmode  registers to the initial status
int VIU_ResetCapModeConfig(CapMode_t capMode)
{
return 0;
//TODO
}
#endif

int VIU_StartCapture(CapMode_t capMode)
{
	switch(capMode)
	{
		//bt656
		case MODE_INTERLACE_EMBBED_PORT0:
			//VI_EnableCH_CTRL(0);
			VI_EnableCH_CTRL(2);
			break;
			//bt601
		case MODE_INTERLACE_SEPERATE_PORT0:
			VI_EnableCH_CTRL(0);
			break;
			//480p embed
		case MODE_PROGRESSIVE_EMBBED_PORT0:
			VI_EnableCH_CTRL(0);
			break;
			//480p seperate
		case MODE_PROGRESSIVE_SEPERATE_PORT0:
			VI_EnableCH_CTRL(0);
			break;
			//BT1120 EMBED
		case MODE_INTERLACE_EMBBED_PORT0_1:
			VI_EnableCH_CTRL(0);
			VI_EnableCH_CTRL(2);
			break;

			//BT1120 seperate
		case MODE_INTERLACE_SEPERATE_PORT0_1:
			VI_EnableCH_CTRL(0);
			VI_EnableCH_CTRL(2);
			break;

			//720p EMBED
		case MODE_PROGRESSIVE_EMBBED_PORT0_1:
			VI_EnableCH_CTRL(0);
			VI_EnableCH_CTRL(2);
			break;

			//720p seperate
		case MODE_PROGRESSIVE_SEPERATE_PORT0_1:
			VI_EnableCH_CTRL(0);
			VI_EnableCH_CTRL(2);
			break;

		default:
			printk("no support capmode\n");
			break;
	}
	return 0;

}

int VIU_StopCapture(CapMode_t capMode)
{
	switch(capMode)
	{
		//bt656
		case MODE_INTERLACE_EMBBED_PORT0:
			//VI_DisableCH_CTRL(0);
			VI_DisableCH_CTRL(2);
			break;
			//bt601
		case MODE_INTERLACE_SEPERATE_PORT0:
			VI_DisableCH_CTRL(0);

			break;
			//480p embed
		case MODE_PROGRESSIVE_EMBBED_PORT0:
			VI_DisableCH_CTRL(0);
			break;
			//480p seperate
		case MODE_PROGRESSIVE_SEPERATE_PORT0:
			VI_DisableCH_CTRL(0);
			break;
			//BT1120 EMBED
		case MODE_INTERLACE_EMBBED_PORT0_1:
			VI_DisableCH_CTRL(0);
			VI_DisableCH_CTRL(2);
			break;

			//BT1120 seperate
		case MODE_INTERLACE_SEPERATE_PORT0_1:
			VI_DisableCH_CTRL(0);
			VI_DisableCH_CTRL(2);
			break;
			//720p EMBED
		case MODE_PROGRESSIVE_EMBBED_PORT0_1:
			VI_DisableCH_CTRL(0);
			VI_DisableCH_CTRL(2);
			break;

			//720p seperate
		case MODE_PROGRESSIVE_SEPERATE_PORT0_1:
			VI_DisableCH_CTRL(0);
			VI_DisableCH_CTRL(2);
			break;

		default:
			printk("no support capmode\n");
			break;

	}
	return 0;

}
int VIU_Reset(void)
{
	//vidin module software reset
	int tmp;
	int reg_newer;
	tmp = *(volatile unsigned int *)(0xbfba9034);
//	tmp &= ~(1<<18); //suv2
	tmp &= ~(1<<23);//suv3
	*(volatile unsigned int *)(0xbfba9034)= tmp;

	//	mdelay(2);	

	//software reset
	tmp = *(volatile unsigned int *)(0xbfba9034);
	//tmp |= (1<<18);//suv2
	tmp |= (1<<23);//suv3

	*(volatile unsigned int *)(0xbfba9034)= tmp;

	VI_PortReset(0);
	VI_PortReset(1);
	VI_PortReset(2);
	VI_PortReset(3);

	VIU_WriteReg(VI0_CH_CTRL,0x0);
	VIU_WriteReg(VI1_CH_CTRL,0x0);
	VIU_WriteReg(VI2_CH_CTRL,0x0);
	VIU_WriteReg(VI3_CH_CTRL,0x0);
	VIU_WriteReg(VI4_CH_CTRL,0x0);
	VIU_WriteReg(VI5_CH_CTRL,0x0);
	VIU_WriteReg(VI6_CH_CTRL,0x0);
	VIU_WriteReg(VI7_CH_CTRL,0x0);

	VIU_WriteReg(VI_INT_INDICATOR,0xffffffff); 

	VIU_WriteReg(VI0_INT_STATUS,0xffffffff); //only clear chn 0
	VIU_WriteReg(VI1_INT_STATUS,0xffffffff); //only clear chn 0
	VIU_WriteReg(VI2_INT_STATUS,0xffffffff); //only clear chn 0
	VIU_WriteReg(VI3_INT_STATUS,0xffffffff); //only clear chn 0
	VIU_WriteReg(VI4_INT_STATUS,0xffffffff); //only clear chn 0
	VIU_WriteReg(VI5_INT_STATUS,0xffffffff); //only clear chn 0
	VIU_WriteReg(VI6_INT_STATUS,0xffffffff); //only clear chn 0
	VIU_WriteReg(VI7_INT_STATUS,0xffffffff); //only clear chn 0

	reg_newer = 0x00000000;
	VIU_WriteReg(VI0_REG_NEWER,reg_newer);
	VIU_WriteReg(VI1_REG_NEWER,reg_newer);
	VIU_WriteReg(VI2_REG_NEWER,reg_newer);
	VIU_WriteReg(VI3_REG_NEWER,reg_newer);
	VIU_WriteReg(VI4_REG_NEWER,reg_newer);
	VIU_WriteReg(VI5_REG_NEWER,reg_newer);
	VIU_WriteReg(VI6_REG_NEWER,reg_newer);
	VIU_WriteReg(VI7_REG_NEWER,reg_newer);

	return 0;
}

//only open clk
int VIU_OpenCLK(void)
{

	printk("VIU_OpenCLK\n");
	//openclk
	int tmp;
	tmp = *(volatile unsigned int *)(0xbfba9000);
	tmp |= (1<<18);

	*(volatile unsigned int *)(0xbfba9000)= tmp;

	return 0;
}
//only close clk
int VIU_CloseCLK(void)
{
	int tmp;
	VIU_WriteReg(SUSPEND_REQ,0x1);
	tmp =VIU_ReadReg(SUSPEND_ACK); 
	while(!(tmp&0x1))
	{
		printk("waiting suspend ack\n");
		mdelay(10);
		tmp =VIU_ReadReg(SUSPEND_ACK); 
	}
	printk("got ACK\n");
	VIU_WriteReg(SUSPEND_REQ,0x0);
	//close clk
	tmp = *(volatile unsigned int *)(0xbfba9000);
	tmp &= ~(1<<18);
	*(volatile unsigned int *)(0xbfba9000)= tmp;
	return 0;
}
//open clk and enable regnew and ch_ctrl
int VIU_Restart(VIU_handle_t *handle)
{

	//openclk
	int tmp;
	tmp = *(volatile unsigned int *)(0xbfba9000);
	tmp |= (1<<18);

	*(volatile unsigned int *)(0xbfba9000)= tmp;
	VIU_StartCapture(handle->capMode);
	//printk("open vidin clk\n");
	return 0;
}

//when acl got,disable regnew and ch_ctrl and close clk
int VIU_Pause(VIU_handle_t *handle)
{
	int tmp;
	VIU_WriteReg(SUSPEND_REQ,0x1);
	tmp =VIU_ReadReg(SUSPEND_ACK); 
	while(!(tmp&0x1))
	{
		printk("waiting suspend ack\n");
		mdelay(10);
		tmp =VIU_ReadReg(SUSPEND_ACK); 
	}
	printk("got ACK\n");
	VIU_WriteReg(SUSPEND_REQ,0x0);
	VIU_StopCapture(handle->capMode);
	//close clk
	tmp = *(volatile unsigned int *)(0xbfba9000);
	tmp &= ~(1<<18);
	*(volatile unsigned int *)(0xbfba9000)= tmp;

	return 0;
}
