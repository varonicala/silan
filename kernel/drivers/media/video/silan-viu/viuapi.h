//------------------------------------------------------------------------------
// File: VidinApi.h
//
// Copyright (c) 2006~2010, Silan Inc.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef VIAPI_H_INCLUDED
#define VIAPI_H_INCLUDED

#include"viu-export.h"

typedef unsigned char	Uint8;
typedef unsigned int	Uint32;
typedef unsigned short	Uint16;


#ifdef __cplusplus
extern "C" {
#endif

inline unsigned int VIU_ReadReg(unsigned long reg);
void inline VIU_WriteReg( unsigned long reg,unsigned int  val);
int VI_PortCfg(int port_num, unsigned int value);
int VI_PortReset(int port_num);
int VI_CHCfg(int addr, int cfg);
int VI_GetIntrptIndicator(void);
int VI_GetChIntrptStatus(int ch_idx);
int VI_ClearChIntrpt(int ch_idx,int value);
int VI_SetCHFifoDepth(int ch_idx, int fifo_depth_0, int fifo_depth_1, int fifo_depth_2);
#if 0
int VI_SetCHStoreSize(int ch_idx, int pic_width, int pic_height,int format);
int VI_SetCHBaseAddr(int ch_idx, int pic_width, int pic_height, int y_base_addr,int format);

int VI_Set_BT1120_PORT0_1_CHBaseAddr( int pic_width, int pic_height, int y_base_addr,int format);
int VI_Set_BT1120_PORT2_3_CHBaseAddr( int pic_width, int pic_height, int y_base_addr,int format);
#endif

int	VI_SetCHStoreConfig(int ch_idx, int width, int height,VIU_StoreMode_e storeFormat,VIU_ScaleMode_e scaleMode, unsigned int y_addr);
int	VI_SetCH_PORT0_1_StoreConfig(int width, int height,VIU_StoreMode_e storeFormat,VIU_ScaleMode_e scaleMode, unsigned int y_addr);
int VI_SetCHCapInfo(int ch_idx, int cap_start, int cap_size);


int print_viu_values(void);
int VI_EnableCH_CTRL(int ch_idx);
int VI_DisableCH_CTRL(int ch_idx);
#ifdef __cplusplus
}
#endif

#endif

