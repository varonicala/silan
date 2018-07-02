//------------------------------------------------------------------------------
// File: viuapi.c
//
// Copyright (c) 2006~2010, Silan Inc.  All rights reserved.
//------------------------------------------------------------------------------

#include "viuapi.h"
#include "regdefine.h"


#define PRINT_REGS
typedef unsigned int UINT;

inline unsigned int VIU_ReadReg(unsigned long reg)
{
	return (*(volatile unsigned int *)reg);
}

void inline VIU_WriteReg( unsigned long reg,unsigned int  val)
{
	*(volatile unsigned int *)(reg) = val;
}


int VI_PortCfg(int port_num, unsigned int value)
{

	VIU_WriteReg((VI0_PORT_CFG+port_num*0x2000), value);
	return 0;
}

int print_viu_values()
{

#ifdef PRINT_REGS
	printk("\nthe viu reg values :\n\n");
	printk("VI0_PORT_CFG:%X\n",VIU_ReadReg(VI0_PORT_CFG));
	printk("VI2_PORT_CFG:%X\n",VIU_ReadReg(VI2_PORT_CFG));

	printk("VI0_CH_CFG:%X\n",VIU_ReadReg(VI0_CH_CFG));
	printk("VI2_CH_CFG:%X\n",VIU_ReadReg(VI2_CH_CFG));


	printk("VI0_CH_CTRL:%X\n",VIU_ReadReg(VI0_CH_CTRL));
	printk("VI2_CH_CTRL:%X\n",VIU_ReadReg(VI2_CH_CTRL));


	printk("VI0_REG_NEWER:%X\n",VIU_ReadReg(VI0_REG_NEWER));
	printk("VI2_REG_NEWER:%X\n",VIU_ReadReg(VI2_REG_NEWER));


	printk("VI0_CAP_START:%X\n",VIU_ReadReg(VI0_CAP_START));
	printk("VI2_CAP_START:%X\n",VIU_ReadReg(VI2_CAP_START));


	printk("VI0_CAP_SIZE:%X\n",VIU_ReadReg(VI0_CAP_SIZE));
	printk("VI2_CAP_SIZE:%X\n",VIU_ReadReg(VI2_CAP_SIZE));


	printk("VI0_LINE_OFFSET:%X\n",VIU_ReadReg(VI0_LINE_OFFSET));
	printk("VI2_LINE_OFFSET:%X\n",VIU_ReadReg(VI2_LINE_OFFSET));


	printk("VI0_YBASE_ADDR:%X\n",VIU_ReadReg(VI0_YBASE_ADDR));
	printk("VI2_YBASE_ADDR:%X\n",VIU_ReadReg(VI2_YBASE_ADDR));


	printk("VI0_UBASE_ADDR:%X\n",VIU_ReadReg(VI0_UBASE_ADDR));
	printk("VI2_UBASE_ADDR:%X\n",VIU_ReadReg(VI2_UBASE_ADDR));

	printk("VI0_VBASE_ADDR:%X\n",VIU_ReadReg(VI0_VBASE_ADDR));
	printk("VI2_VBASE_ADDR:%X\n",VIU_ReadReg(VI2_VBASE_ADDR));


	printk("CH0_0_FIFO_DEPTH:%X\n",VIU_ReadReg(CH0_0_FIFO_DEPTH));
	printk("CH0_1_FIFO_DEPTH:%X\n",VIU_ReadReg(CH0_1_FIFO_DEPTH));
	printk("CH0_2_FIFO_DEPTH:%X\n",VIU_ReadReg(CH0_2_FIFO_DEPTH));


	printk("CH2_0_FIFO_DEPTH:%X\n",VIU_ReadReg(CH2_0_FIFO_DEPTH));
	printk("CH2_1_FIFO_DEPTH:%X\n",VIU_ReadReg(CH2_1_FIFO_DEPTH));
	printk("CH2_2_FIFO_DEPTH:%X\n",VIU_ReadReg(CH2_2_FIFO_DEPTH));

#endif
	return 0;


}



/*!
 ************************************************************************
 * \brief
 *    Video input ports reset control
 *    
 * \param
 *    reset_pattern : 
 *       bit[3] bit[2] bit[1] bit[0] 
 *       port3  port2  port1  port0  
 *       For example : reset all ports, reset_pattern = 0xf; //4'b1111
 *                     reset port 0,2,  reset_pattern = 0x5; //4'b0101
 *                     reset port 1,3,  reset_pattern = 0xa; //4'b1010
 *                     reset port 0,    reset_pattern = 0x1; //4'b0001
 *                     reset port 1,    reset_pattern = 0x2; //4'b0010
 *                     etc.
 *    
 *    reset_duration : duration of reset pulse
 * \return
 *    none
 ************************************************************************
 */
//todo yuliubing
int VI_PortReset(int port_num)
{
	int rd_data;

	//clear 0
	rd_data= VIU_ReadReg(PORT_RESET_CTRL);
	rd_data = rd_data &( ~(1<<port_num));
	VIU_WriteReg(PORT_RESET_CTRL, rd_data);

	//delay needed maybe

	//set 1
	rd_data= VIU_ReadReg(PORT_RESET_CTRL);
	rd_data = rd_data |(1<<port_num);
	VIU_WriteReg(PORT_RESET_CTRL, rd_data);

	return 0;



}

int VI_CHCfg(int addr, int cfg)
{
	VIU_WriteReg(addr, cfg);
	return 0;

}

int VI_GetIntrptIndicator(void)
{
	UINT intrpt_indicator;	
	intrpt_indicator = VIU_ReadReg(VI_INT_INDICATOR);	
	return intrpt_indicator;
}
//TODO
int VI_GetChIntrptStatus(int ch_idx)
{
	UINT intrpt_status;	
	unsigned int reg = VI0_INT_STATUS + (ch_idx*0x1000);
	intrpt_status = VIU_ReadReg(reg);	
	return intrpt_status;
}

int VI_ClearChIntrpt(int ch_idx,int value)
{
	unsigned int reg = VI0_INT_STATUS + (ch_idx*0x1000);
	VIU_WriteReg(reg, value);
	return 0;
	
}

int VI_SetCHFifoDepth(int ch_idx, int fifo_depth_0, int fifo_depth_1, int fifo_depth_2)
{
	int reg_addr_0, reg_addr_1, reg_addr_2;

	reg_addr_0 = CH0_0_FIFO_DEPTH + ch_idx * 0x1000;
	reg_addr_1 = CH0_1_FIFO_DEPTH + ch_idx * 0x1000;
	reg_addr_2 = CH0_2_FIFO_DEPTH + ch_idx * 0x1000;

	VI_CHCfg(reg_addr_0, fifo_depth_0);


	VI_CHCfg(reg_addr_1, fifo_depth_1);


	VI_CHCfg(reg_addr_2, fifo_depth_2);

	return 0;

}
//todo yuliubing
int VI_SetCH_PlanarStoreSize(int ch_idx, int cap_width, int cap_height)
{
	int reg_addr_0, reg_addr_1, reg_addr_2;
	int y_store_size, u_store_size, v_store_size;
	int y_width,cb_width,cr_width,y_height,cb_height,cr_height;
	//8 byte align
	y_width = (cap_width+7)&(~7);
	cb_width = (cap_width/2+7)&(~7);
	cr_width = (cap_width/2+7)&(~7);
	y_height =( cap_height + 1)&(~1);//2 byte align
	cb_height = y_height/2;
	cr_height = y_height/2;
	reg_addr_0 = VI0_Y_STORESIZE + ch_idx * 0x1000;
	reg_addr_1 = VI0_U_STORESIZE + ch_idx * 0x1000;
	reg_addr_2 = VI0_V_STORESIZE + ch_idx * 0x1000;   

	// Y channel store size
	y_store_size = ((y_height << 16) +y_width );
	VI_CHCfg(reg_addr_0, y_store_size);         


	// ch0 u channel store size
	u_store_size = (((cb_height) << 16) + (cb_width));
	VI_CHCfg(reg_addr_1, u_store_size);         


	// ch0 v channel store size
	v_store_size = (((cr_height) << 16) + (cr_width));
	VI_CHCfg(reg_addr_2, v_store_size);
	return 0;
}
int VI_SetCH_PackageStoreSize(int ch_idx, int cap_width, int cap_height)
{
	int reg_addr;
	int y_width,y_height;
	int y_store_size;
	//8 byte align
	y_width = (cap_width+7)&(~7);
	y_height =( cap_height + 1)&(~1);//2 byte align
	reg_addr = VI0_Y_STORESIZE + ch_idx * 0x1000;

	// Y channel store size
	//y_store_size = ((y_height << 16) +y_width );
	y_store_size = y_height*y_width*2 ;
	VI_CHCfg(reg_addr, y_store_size);         

	return 0;
}
//cbcr interleave
int VI_SetCH_SemiPlanarStoreSize(int ch_idx, int cap_width, int cap_height)
{
	int reg_addr_0, reg_addr_1;
	int y_store_size, u_store_size;
	int y_width,y_height,c_width,c_height;
	//8 byte align
	y_width = (cap_width+7)&(~7);
	c_width = (cap_width+7)&(~7);
	y_height =(cap_height + 1)&(~1);//2 byte align

	c_height = y_height/2;

	reg_addr_0 = VI0_Y_STORESIZE + ch_idx * 0x1000;
	reg_addr_1 = VI0_U_STORESIZE + ch_idx * 0x1000;

	// Y channel store size
	y_store_size = ((y_height << 16) +y_width );
	VI_CHCfg(reg_addr_0, y_store_size);         


	// ch0 u channel store size
	u_store_size = ((c_height) << 16) + (c_width);
	VI_CHCfg(reg_addr_1, u_store_size);         


	return 0;
}


#if 0
int VI_SetCH_PlanarBaseAddr(int ch_idx, int cap_width, int cap_height, int y_base_addr)
{
	int reg_addr_0, reg_addr_1, reg_addr_2;
	int u_base_addr, v_base_addr;
	int y_store_size,cb_store_size;
	int y_width,cb_width,cr_width,y_height,cb_height,cr_height;
	//8 byte align
	y_width = (cap_width+7)&(~7);
	cb_width = (cap_width/2+7)&(~7);
	cr_width = (cap_width/2+7)&(~7);
	y_height =( cap_height + 1)&(~1);//2 byte align
	cb_height = y_height/2;
	cr_height = y_height/2;

	y_store_size = y_width*y_height;
	cb_store_size = cb_width*cb_height;
	reg_addr_0 = VI0_YBASE_ADDR + ch_idx * 0x1000;
	reg_addr_1 = VI0_UBASE_ADDR + ch_idx * 0x1000;
	reg_addr_2 = VI0_VBASE_ADDR + ch_idx * 0x1000;  

	// Y channel base address
	VI_CHCfg(reg_addr_0, y_base_addr);


	u_base_addr = y_base_addr +y_store_size; 
	VI_CHCfg(reg_addr_1, u_base_addr);


	v_base_addr = u_base_addr + cb_store_size;
	VI_CHCfg(reg_addr_2, v_base_addr);

	return 0; 
}
int VI_SetCH_SemiPlanarBaseAddr(int ch_idx, int cap_width, int cap_height, int y_base_addr)
{
	int reg_addr_0, reg_addr_1, reg_addr_2;
	int u_base_addr, v_base_addr;
	int y_store_size,cb_store_size;
	int y_width,cb_width,cr_width,y_height,cb_height,cr_height;
	//8 byte align
	y_width = (cap_width+7)&(~7);
	cb_width = (cap_width/2+7)&(~7);
	cr_width = (cap_width/2+7)&(~7);
	y_height =( cap_height + 1)&(~1);//2 byte align
	cb_height = y_height/2;
	cr_height = y_height/2;

	y_store_size = y_width*y_height;
	cb_store_size = cb_width*cb_height;
	reg_addr_0 = VI0_YBASE_ADDR + ch_idx * 0x1000;
	reg_addr_1 = VI0_UBASE_ADDR + ch_idx * 0x1000;
	reg_addr_2 = VI0_VBASE_ADDR + ch_idx * 0x1000;  

	// Y channel base address
	VI_CHCfg(reg_addr_0, y_base_addr);


	u_base_addr = y_base_addr +y_store_size; 
	VI_CHCfg(reg_addr_1, u_base_addr);


	return 0; 
}


int VI_SetCH_PackageBaseAddr(int ch_idx, int cap_width, int cap_height, int y_base_addr)
{
	int reg_addr;
	reg_addr = VI0_YBASE_ADDR + ch_idx * 0x1000;
	// Y channel base address
	VI_CHCfg(reg_addr, y_base_addr);

	return 0; 
}

#endif

#if 0
int VI_Set_Planar_PORT0_1_CHBaseAddr( int cap_width, int cap_height, int y_base_addr)
{
	int u_base_addr, v_base_addr;
	int y_store_size,cb_store_size;
	int y_width,cb_width,cr_width,y_height,cb_height,cr_height;
	//8 byte align
	y_width = (cap_width+7)&(~7);
	cb_width = (cap_width/2+7)&(~7);
	cr_width = (cap_width/2+7)&(~7);
	y_height =( cap_height + 1)&(~1);//2 byte align
	cb_height = y_height/2;
	cr_height = y_height/2;

	y_store_size = y_width*y_height;
	cb_store_size = cb_width*cb_height;


	// Y channel base address chn0
	VI_CHCfg(VI0_YBASE_ADDR, y_base_addr);


	// Cb Cr channel base address chn2
	u_base_addr = y_base_addr + y_store_size;
	VI_CHCfg(VI2_UBASE_ADDR, u_base_addr);


	v_base_addr = u_base_addr + cb_store_size;
	VI_CHCfg(VI2_VBASE_ADDR, v_base_addr);

	return 0; 



}
int VI_Set_SemiPlanar_PORT0_1_CHBaseAddr( int cap_width, int cap_height, int y_base_addr)
{
	int u_base_addr, v_base_addr;
	int y_store_size,cb_store_size;
	int y_width,cb_width,cr_width,y_height,cb_height,cr_height;
	//8 byte align
	y_width = (cap_width+7)&(~7);
	cb_width = (cap_width/2+7)&(~7);
	cr_width = (cap_width/2+7)&(~7);
	y_height =( cap_height + 1)&(~1);//2 byte align
	cb_height = y_height/2;
	cr_height = y_height/2;

	y_store_size = y_width*y_height;
	cb_store_size = cb_width*cb_height;


	// Y channel base address chn0
	VI_CHCfg(VI0_YBASE_ADDR, y_base_addr);


	// Cb Cr channel base address chn2
	u_base_addr = y_base_addr + y_store_size;
	VI_CHCfg(VI2_UBASE_ADDR, u_base_addr);


	return 0; 



}
#endif
//todo like PORT0_1
int VI_Set_BT1120_PORT2_3_CHBaseAddr( int pic_width, int pic_height, int y_base_addr,int format)
{
	int u_base_addr, v_base_addr;


	// Y channel base address chn0
	VI_CHCfg(VI4_YBASE_ADDR, y_base_addr);


	// Cb Cr channel base address chn2
	u_base_addr = y_base_addr + pic_width * pic_height;
	VI_CHCfg(VI6_UBASE_ADDR, u_base_addr);


	v_base_addr = u_base_addr + pic_width * pic_height/4;
	VI_CHCfg(VI6_VBASE_ADDR, v_base_addr);

	return 0; 



}
int VI_SetCHCapInfo(int ch_idx, int cap_start, int cap_size)
{
	int reg_addr_0, reg_addr_1;

	reg_addr_0 = VI0_CAP_START + ch_idx * 0x1000;
	reg_addr_1 = VI0_CAP_SIZE  + ch_idx * 0x1000;

	VI_CHCfg(reg_addr_0, cap_start);         
	VI_CHCfg(reg_addr_1, cap_size);


	return 0;  
}


int VI_SetCH_PlanarStride(int ch_idx, int cap_width)
{
	int stride;
	int y_width_stride,cb_width_stride,cr_width_stride;
	int reg_addr;

#if 0
	//8 byte align
	y_width_stride = (cap_width+7)&(~7);
	cb_width_stride = (cap_width/2+7)&(~7);
	cr_width_stride = (cap_width/2+7)&(~7);
#endif
	//16 byte align
	y_width_stride = (cap_width+15)&(~15);
	cb_width_stride = y_width_stride/2;
	cr_width_stride = y_width_stride/2;

	stride = (((y_width_stride/8) << 20) + ((cb_width_stride/8) << 10) + (cr_width_stride/8));
	reg_addr = VI0_LINE_OFFSET + ch_idx * 0x1000;

	VI_CHCfg(reg_addr, stride);

	return 0; 
}

int VI_SetCH_SemiPlanarStride(int ch_idx, int cap_width)
{
	int stride;
	int y_width_stride,c_width_stride;

	int reg_addr;
	y_width_stride = (cap_width+7)&(~7);
	c_width_stride = (cap_width+7)&(~7);
	stride = (((y_width_stride/8) << 20) + ((c_width_stride/8) << 10));
	reg_addr = VI0_LINE_OFFSET + ch_idx * 0x1000;

	VI_CHCfg(reg_addr, stride);

	return 0; 
}

int VI_SetCH_PackageStride(int ch_idx, int cap_width)
{
	int stride;
	int y_width_stride;

	int reg_addr;
	//todo
	y_width_stride = ((cap_width+7)&(~7))*2;
	stride = ((y_width_stride/8) << 20);
	reg_addr = VI0_LINE_OFFSET + ch_idx * 0x1000;

	VI_CHCfg(reg_addr, stride);

	return 0; 
}

int	VI_SetCHStoreConfig(int ch_idx, int width, int height,VIU_StoreMode_e storeFormat,VIU_ScaleMode_e scaleMode, unsigned int y_addr)
{
	int cap_width=width;
	int cap_height=height;


	switch (storeFormat)
	{

		case Mode_Planar:
			VI_SetCH_PlanarStoreSize(ch_idx, cap_width, cap_height);

			VI_SetCH_PlanarStride(ch_idx,cap_width);

			break;
		case Mode_SemiPlanar:
			VI_SetCH_SemiPlanarStoreSize(ch_idx, cap_width, cap_height);

			VI_SetCH_SemiPlanarStride(ch_idx,cap_width);

			break;
		case Mode_Package:
		case Mode_Raw:
			VI_SetCH_PackageStoreSize(ch_idx, cap_width, cap_height);

			VI_SetCH_PackageStride(ch_idx,cap_width);

			break;


	}
	return 0;
}

int	VI_SetCH_PORT0_1_StoreConfig( int width, int height,VIU_StoreMode_e storeFormat,VIU_ScaleMode_e scaleMode, unsigned int y_addr)
{
	int cap_width=width;
	int cap_height=height;
	cap_width=cap_width-cap_width%2;

	switch (storeFormat)
	{

		case Mode_Planar:
			//todo
			VI_SetCH_PlanarStoreSize(0, cap_width, cap_height);
			VI_SetCH_PlanarStoreSize(2, cap_width, cap_height);
			VI_SetCH_PlanarStride(0,cap_width);
			VI_SetCH_PlanarStride(2,cap_width);


			break;
		case Mode_SemiPlanar:
			VI_SetCH_SemiPlanarStoreSize(0, cap_width, cap_height);
			VI_SetCH_SemiPlanarStoreSize(2, cap_width, cap_height);

			VI_SetCH_SemiPlanarStride(0,cap_width);
			VI_SetCH_SemiPlanarStride(2,cap_width);

			break;
		case Mode_Package:
		case Mode_Raw:
			printk("package and raw mode not supported\n");
			break;

	}
	return 0;
}

int VI_EnableCH_CTRL(int ch_idx)
{
	unsigned reg_addr;
	unsigned int tmp;


#if 0
	//1.regnew = 1 ,2.ch_ctrl= 1 ;
	VI_CHCfg(VI0_REG_NEWER+ ch_idx * 0x1000 , 0x1); 


	reg_addr = VI0_CH_CTRL + ch_idx * 0x1000;
	tmp = VIU_ReadReg(reg_addr);
	tmp |=0x1;

	VIU_WriteReg(reg_addr,tmp);
#else


	//1.ch_ctrl= 1,.regnew = 1 ;
	reg_addr = VI0_CH_CTRL + ch_idx* 0x1000;
	tmp = VIU_ReadReg(reg_addr);
	tmp |=0x1;

	VIU_WriteReg(reg_addr,tmp);

	VI_CHCfg(VI0_REG_NEWER+ ch_idx * 0x1000 , 0x1); 

#endif
	return 0;
}
int VI_DisableCH_CTRL(int ch_idx)
{
	unsigned reg_addr;
	unsigned int tmp;


	VI_CHCfg(VI0_INT_EN+ ch_idx * 0x1000 , 0x0);

	VI_CHCfg(VI0_REG_NEWER+ ch_idx * 0x1000 , 0x0); 
	reg_addr = VI0_CH_CTRL + ch_idx * 0x1000;
	tmp = VIU_ReadReg(reg_addr);
	tmp &=~0x1;

	VIU_WriteReg(reg_addr,tmp);
	return 0;
}
