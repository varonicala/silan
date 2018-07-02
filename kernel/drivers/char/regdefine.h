//------------------------------------------------------------------------------
// File: RegDefine.h
//
// Copyright (c) 2006~2010, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef REGDEFINE_H_INCLUDED
#define REGDEFINE_H_INCLUDED

//------------------------------------------------------------------------------
// REGISTER BASE
//------------------------------------------------------------------------------

#define BIT_REG_SIZE			0x2000	// include GDI register range
#define BIT_REG_BASE			0x10000000

#define BIT_BASE            0x00
#define GDMA_BASE          	0x1000
#define MBC_BASE            0x400
#define MC_BASE				0xC00

//------------------------------------------------------------------------------
// HARDWARE REGISTER
//------------------------------------------------------------------------------
#define BIT_CODE_RUN                (BIT_BASE + 0x000)
#define BIT_CODE_DOWN               (BIT_BASE + 0x004)
#define BIT_INT_REQ                 (BIT_BASE + 0x008)
#define BIT_INT_CLEAR               (BIT_BASE + 0x00C)
#define BIT_INT_STS					(BIT_BASE + 0x010)
#define BIT_CODE_RESET				(BIT_BASE + 0x014)
#define BIT_CUR_PC                  (BIT_BASE + 0x018)
#define BIT_SW_RESET				(BIT_BASE + 0x024)

//------------------------------------------------------------------------------
// GLOBAL REGISTER
//------------------------------------------------------------------------------
#define BIT_CODE_BUF_ADDR           (BIT_BASE + 0x100)
#define BIT_WORK_BUF_ADDR           (BIT_BASE + 0x104)
#define BIT_PARA_BUF_ADDR           (BIT_BASE + 0x108)
#define BIT_BIT_STREAM_CTRL         (BIT_BASE + 0x10C)
#define BIT_FRAME_MEM_CTRL          (BIT_BASE + 0x110)
#define	BIT_BIT_STREAM_PARAM		(BIT_BASE + 0x114)

#define BIT_RD_PTR_0                (BIT_BASE + 0x120)
#define BIT_WR_PTR_0                (BIT_BASE + 0x124)
#define BIT_RD_PTR_1                (BIT_BASE + 0x128)
#define BIT_WR_PTR_1                (BIT_BASE + 0x12C)
#define BIT_RD_PTR_2                (BIT_BASE + 0x130)
#define BIT_WR_PTR_2                (BIT_BASE + 0x134)
#define BIT_RD_PTR_3                (BIT_BASE + 0x138)
#define BIT_WR_PTR_3                (BIT_BASE + 0x13C)

#define BIT_AXI_SRAM_USE            (BIT_BASE + 0x140)

#define	BIT_FRM_DIS_FLG_0			(BIT_BASE + 0x150)
#define	BIT_FRM_DIS_FLG_1			(BIT_BASE + 0x154)
#define	BIT_FRM_DIS_FLG_2			(BIT_BASE + 0x158)
#define	BIT_FRM_DIS_FLG_3			(BIT_BASE + 0x15C)

#define BIT_BUSY_FLAG               (BIT_BASE + 0x160)
#define BIT_RUN_COMMAND             (BIT_BASE + 0x164)
#define BIT_RUN_INDEX               (BIT_BASE + 0x168)
#define BIT_RUN_COD_STD             (BIT_BASE + 0x16C)
#define BIT_INT_ENABLE              (BIT_BASE + 0x170)
#define BIT_INT_REASON              (BIT_BASE + 0x174)
#define BIT_RUN_AUX_STD             (BIT_BASE + 0x178)


#define BIT_MSG_0                   (BIT_BASE + 0x1F0)
#define BIT_MSG_1                   (BIT_BASE + 0x1F4)
#define BIT_MSG_2                   (BIT_BASE + 0x1F8)
#define BIT_MSG_3                   (BIT_BASE + 0x1FC)

#define MBC_BUSY                	(MBC_BASE + 0x040)
#define MC_BUSY                 	(MC_BASE + 0x004)
//------------------------------------------------------------------------------
// [ENC SEQ INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_SEQ_BB_START        (BIT_BASE + 0x180)
#define CMD_ENC_SEQ_BB_SIZE         (BIT_BASE + 0x184)
#define CMD_ENC_SEQ_OPTION          (BIT_BASE + 0x188)          // HecEnable,ConstIntraQp, FMO, QPREP, AUD, SLICE, MB BIT
#define CMD_ENC_SEQ_COD_STD         (BIT_BASE + 0x18C)
#define CMD_ENC_SEQ_SRC_SIZE        (BIT_BASE + 0x190)
#define CMD_ENC_SEQ_SRC_F_RATE      (BIT_BASE + 0x194)
#define CMD_ENC_SEQ_MP4_PARA        (BIT_BASE + 0x198)
#define CMD_ENC_SEQ_263_PARA        (BIT_BASE + 0x19C)
#define CMD_ENC_SEQ_264_PARA        (BIT_BASE + 0x1A0)
#define CMD_ENC_SEQ_SLICE_MODE      (BIT_BASE + 0x1A4)
#define CMD_ENC_SEQ_GOP_NUM         (BIT_BASE + 0x1A8)
#define CMD_ENC_SEQ_RC_PARA         (BIT_BASE + 0x1AC)
#define CMD_ENC_SEQ_RC_BUF_SIZE     (BIT_BASE + 0x1B0)
#define CMD_ENC_SEQ_INTRA_REFRESH   (BIT_BASE + 0x1B4)
#define CMD_ENC_SEQ_INTRA_QP		(BIT_BASE + 0x1C4)
#define CMD_ENC_SEQ_RC_QP_MAX			(BIT_BASE + 0x1C8)		
#define CMD_ENC_SEQ_RC_GAMMA			(BIT_BASE + 0x1CC)		
#define CMD_ENC_SEQ_RC_INTERVAL_MODE	(BIT_BASE + 0x1D0)		// MbInterval[32:2], RcIntervalMode[1:0]
#define CMD_ENC_SEQ_INTRA_WEIGHT		(BIT_BASE + 0x1D4)
#define CMD_ENC_SEQ_ME_OPTION			(BIT_BASE + 0x1D8)


#define CMD_ENC_SEQ_JPG_PARA			(BIT_BASE + 0x198)
#define CMD_ENC_SEQ_JPG_RST_INTERVAL	(BIT_BASE + 0x19C)
#define CMD_ENC_SEQ_JPG_THUMB_EN		(BIT_BASE + 0x1A0)
#define CMD_ENC_SEQ_JPG_THUMB_SIZE		(BIT_BASE + 0x1A4)
#define CMD_ENC_SEQ_JPG_THUMB_OFFSET	(BIT_BASE + 0x1A8)

#define RET_ENC_SEQ_SUCCESS         (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [ENC PIC RUN] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_PIC_SRC_INDEX       (BIT_BASE + 0x180)
#define CMD_ENC_PIC_SRC_STRIDE      (BIT_BASE + 0x184)
#define CMD_ENC_PIC_SRC_ADDR_Y		(BIT_BASE + 0x1A8)
#define CMD_ENC_PIC_SRC_ADDR_CB		(BIT_BASE + 0x1AC)
#define CMD_ENC_PIC_SRC_ADDR_CR		(BIT_BASE + 0x1B0)

#define CMD_ENC_PIC_QS              (BIT_BASE + 0x18C)
#define CMD_ENC_PIC_ROT_MODE        (BIT_BASE + 0x190)
#define CMD_ENC_PIC_OPTION          (BIT_BASE + 0x194)
#define CMD_ENC_PIC_BB_START        (BIT_BASE + 0x198)
#define CMD_ENC_PIC_BB_SIZE        	(BIT_BASE + 0x19C)
#define CMD_ENC_PIC_PARA_BASE_ADDR	(BIT_BASE + 0x1A0)



#define RET_ENC_PIC_FRAME_NUM       (BIT_BASE + 0x1C0)
#define RET_ENC_PIC_TYPE            (BIT_BASE + 0x1C4)
#define RET_ENC_PIC_FRAME_IDX       (BIT_BASE + 0x1C8)
#define RET_ENC_PIC_SLICE_NUM       (BIT_BASE + 0x1CC)
#define RET_ENC_PIC_FLAG			(BIT_BASE + 0x1D0)


//------------------------------------------------------------------------------
// [ENC SET FRAME BUF] COMMAND
//------------------------------------------------------------------------------
// Magellan ENCODER ONLY
#define CMD_SET_FRAME_SUBSAMP_A    (BIT_BASE + 0x188)
#define CMD_SET_FRAME_SUBSAMP_B    (BIT_BASE + 0x18C)


//------------------------------------------------------------------------------
// [ENC HEADER] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_HEADER_CODE         (BIT_BASE + 0x180)
#define CMD_ENC_HEADER_BB_START     (BIT_BASE + 0x184)
#define CMD_ENC_HEADER_BB_SIZE      (BIT_BASE + 0x188)
#define CMD_ENC_HEADER_FRAME_CROP_H (BIT_BASE + 0x18C)
#define CMD_ENC_HEADER_FRAME_CROP_V (BIT_BASE + 0x190)


//------------------------------------------------------------------------------
// [ENC_PARA_SET] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_PARA_SET_TYPE       (BIT_BASE + 0x180)
#define RET_ENC_PARA_SET_SIZE       (BIT_BASE + 0x1c0)

//------------------------------------------------------------------------------
// [ENC PARA CHANGE] COMMAND :
//------------------------------------------------------------------------------
#define CMD_ENC_SEQ_PARA_CHANGE_ENABLE	(BIT_BASE + 0x180)		// FrameRateEn[3], BitRateEn[2], IntraQpEn[1], GopEn[0]
#define CMD_ENC_SEQ_PARA_RC_GOP			(BIT_BASE + 0x184)
#define CMD_ENC_SEQ_PARA_RC_INTRA_QP	(BIT_BASE + 0x188)      
#define CMD_ENC_SEQ_PARA_RC_BITRATE     (BIT_BASE + 0x18C)
#define CMD_ENC_SEQ_PARA_RC_FRAME_RATE  (BIT_BASE + 0x190)
#define	CMD_ENC_SEQ_PARA_INTRA_MB_NUM	(BIT_BASE + 0x194)		// update param
#define CMD_ENC_SEQ_PARA_SLICE_MODE		(BIT_BASE + 0x198)		// update param
#define CMD_ENC_SEQ_PARA_HEC_MODE		(BIT_BASE + 0x19C)		// update param

#define RET_ENC_SEQ_PARA_CHANGE_SECCESS	(BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC SEQ INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_SEQ_BB_START        (BIT_BASE + 0x180)
#define CMD_DEC_SEQ_BB_SIZE         (BIT_BASE + 0x184)
#define CMD_DEC_SEQ_OPTION          (BIT_BASE + 0x188)
#define CMD_DEC_SEQ_SRC_SIZE        (BIT_BASE + 0x18C)
#define CMD_DEC_SEQ_PS_BB_START     (BIT_BASE + 0x194)
#define CMD_DEC_SEQ_PS_BB_SIZE      (BIT_BASE + 0x198)
#define CMD_DEC_SEQ_START_BYTE		(BIT_BASE + 0x190)
#define CMD_DEC_SEQ_JPG_THUMB_EN    (BIT_BASE + 0x19C) // JPEG only
#define CMD_DEC_SEQ_MP4_ASP_CLASS   (BIT_BASE + 0x19C)
#define CMD_DEC_SEQ_VC1_STREAM_FMT  (BIT_BASE + 0x19C)
#define CMD_DEC_SEQ_X264_MV_EN		(BIT_BASE + 0x19C)
// For MPEG2 and JPEG only
#define CMD_DEC_SEQ_USER_DATA_OPTION		(BIT_BASE + 0x194)
#define CMD_DEC_SEQ_USER_DATA_BASE_ADDR		(BIT_BASE + 0x1AC)
#define CMD_DEC_SEQ_USER_DATA_BUF_SIZE		(BIT_BASE + 0x1B0)	// Share with RET_DEC_SEQ_ASPECT


#define CMD_DEC_SEQ_INIT_ESCAPE		(BIT_BASE + 0x114)

#define RET_DEC_SEQ_ASPECT			(BIT_BASE + 0x1B0)
#define RET_DEC_SEQ_BIT_RATE		(BIT_BASE + 0x1B4)
#define RET_DEC_SEQ_SUCCESS         (BIT_BASE + 0x1C0)
#define RET_DEC_SEQ_SRC_SIZE        (BIT_BASE + 0x1C4)
#define RET_DEC_SEQ_SRC_F_RATE      (BIT_BASE + 0x1C8)
#define RET_DEC_SEQ_FRAME_NEED      (BIT_BASE + 0x1CC)
#define RET_DEC_SEQ_FRAME_DELAY     (BIT_BASE + 0x1D0)
#define RET_DEC_SEQ_INFO            (BIT_BASE + 0x1D4)
#define RET_DEC_SEQ_CROP_LEFT_RIGHT (BIT_BASE + 0x1D8)
#define RET_DEC_SEQ_CROP_TOP_BOTTOM (BIT_BASE + 0x1DC)
#define	RET_DEC_SEQ_NUM_UNITS_IN_TICK   (BIT_BASE + 0x1E4)
#define RET_DEC_SEQ_TIME_SCALE          (BIT_BASE + 0x1E8)
#define	RET_DEC_SEQ_FRAME_FORMAT	(BIT_BASE + 0x1E4)
#define RET_DEC_SEQ_JPG_THUMB_IND	(BIT_BASE + 0x1E8)

#define RET_DEC_SEQ_HEADER_REPORT   (BIT_BASE + 0x1EC)

//------------------------------------------------------------------------------
// [DEC PIC RUN] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_PIC_CLIP_ADDR_Y		(BIT_BASE + 0x1E0)
#define CMD_DEC_PIC_CLIP_ADDR_CB	(BIT_BASE + 0x1E4)
#define CMD_DEC_PIC_CLIP_ADDR_CR	(BIT_BASE + 0x1E8)

#define CMD_DEC_PIC_ROT_MODE        (BIT_BASE + 0x180)
#define CMD_DEC_PIC_ROT_INDEX       (BIT_BASE + 0x184)
#define CMD_DEC_PIC_ROT_STRIDE      (BIT_BASE + 0x1B8)
#define CMD_DEC_PIC_ROT_ADDR_Y      (BIT_BASE + 0x188)
#define CMD_DEC_PIC_ROT_ADDR_CB     (BIT_BASE + 0x18C)
#define CMD_DEC_PIC_ROT_ADDR_CR     (BIT_BASE + 0x190)
#define CMD_DEC_PIC_OPTION			(BIT_BASE + 0x194)
#define	CMD_DEC_PIC_SKIP_NUM		(BIT_BASE + 0x198)
#define	CMD_DEC_PIC_CHUNK_SIZE		(BIT_BASE + 0x19C)
#define	CMD_DEC_PIC_BB_START		(BIT_BASE + 0x1A0)
#define CMD_DEC_PIC_START_BYTE		(BIT_BASE + 0x1A4)
#define CMD_DEC_PIC_PARA_BASE_ADDR			(BIT_BASE + 0x1A8)
#define CMD_DEC_PIC_USER_DATA_BASE_ADDR		(BIT_BASE + 0x1AC)
#define CMD_DEC_PIC_USER_DATA_BUF_SIZE		(BIT_BASE + 0x1B0)


#define CMD_DEC_PIC_SAM_XY		    (BIT_BASE + 0x1E0) // JPEG down-sampling only
#define CMD_DEC_PIC_JPG_THUMB_EN    (BIT_BASE + 0x1E4) // JPEG only
#define	CMD_DEC_PIC_CLIP_MODE		(BIT_BASE + 0x1E8) // JPEG clipping only
#define	CMD_DEC_PIC_CLIP_FROM		(BIT_BASE + 0x1F0) // JPEG clipping only. share with BIT_MSG_0 but it is stored to memory at the beginning of JpgDecPic
#define	CMD_DEC_PIC_CLIP_TO			(BIT_BASE + 0x1F4) // JPEG clipping only. share with BIT_MSG_1 but it is stored to memory at the beginning of JpgDecPic
#define	CMD_DEC_PIC_CLIP_CNT		(BIT_BASE + 0x1F8) // JPEG clipping only. share with BIT_MSG_2 but it is stored to memory at the beginning of JpgDecPic


#define RET_DEC_PIC_SIZE			(BIT_BASE + 0x1BC)
#define RET_DEC_PIC_FRAME_NUM       (BIT_BASE + 0x1C0)
#define RET_DEC_PIC_FRAME_IDX       (BIT_BASE + 0x1C4)
#define RET_DEC_PIC_ERR_MB          (BIT_BASE + 0x1C8)
#define RET_DEC_PIC_TYPE            (BIT_BASE + 0x1CC)
#define RET_DEC_PIC_POST			(BIT_BASE + 0x1D0)
#define RET_DEC_PIC_OPTION			(BIT_BASE + 0x1D4)
#define RET_DEC_PIC_SUCCESS			(BIT_BASE + 0x1D8)
#define RET_DEC_PIC_CUR_IDX			(BIT_BASE + 0x1DC)
#define	RET_DEC_PIC_CROP_LEFT_RIGHT	(BIT_BASE + 0x1E0)
#define RET_DEC_PIC_CROP_TOP_BOTTOM	(BIT_BASE + 0x1E4)
#define RET_DEC_PIC_ASPECT          (BIT_BASE + 0x1F0)

//------------------------------------------------------------------------------
// [DEC SET FRAME BUF] COMMAND
//------------------------------------------------------------------------------
#define CMD_SET_FRAME_BUF_NUM       (BIT_BASE + 0x180)
#define CMD_SET_FRAME_BUF_STRIDE    (BIT_BASE + 0x184)
#define CMD_SET_FRAME_SLICE_BB_START	(BIT_BASE + 0x188)
#define CMD_SET_FRAME_SLICE_BB_SIZE		(BIT_BASE + 0x18C)
#define CMD_SET_FRAME_AXI_BIT_ADDR		(BIT_BASE + 0x190)
#define CMD_SET_FRAME_AXI_IPACDC_ADDR	(BIT_BASE + 0x194)
#define CMD_SET_FRAME_AXI_DBKY_ADDR		(BIT_BASE + 0x198)
#define CMD_SET_FRAME_AXI_DBKC_ADDR		(BIT_BASE + 0x19C)
#define CMD_SET_FRAME_AXI_OVL_ADDR		(BIT_BASE + 0x1A0)
#if defined(FALCONSE) || defined(MACH2PE) || defined(VEGASLE)
#else
#define CMD_SET_FRAME_AXI_BTP_ADDR		(BIT_BASE + 0x1A4)
#endif
#if defined(FALCONLE) || defined(FALCONSE) || defined(VEGASLE)
#define CMD_SET_FRAME_AXI_ME_ADDR		(BIT_BASE + 0x1A8)
#endif

#ifdef VEGASLE
#define CMD_SET_FRAME_MAX_DEC_SIZE   	(BIT_BASE + 0x1A4)
#define CMD_SET_FRAME_SOURCE_BUF_STRIDE   	(BIT_BASE + 0x1A8)
#else
#define CMD_SET_FRAME_MAX_DEC_SIZE   	(BIT_BASE + 0x1A8)
#endif

//------------------------------------------------------------------------------
// [DEC_PARA_SET] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_PARA_SET_TYPE       (BIT_BASE + 0x180)
#define CMD_DEC_PARA_SET_SIZE       (BIT_BASE + 0x184)

//------------------------------------------------------------------------------
// GDMA Register definition
//------------------------------------------------------------------------------
#define GDI_PRI_RD_PRIO_L   		(GDMA_BASE + 0x000)
#define GDI_PRI_RD_PRIO_H   		(GDMA_BASE + 0x004)
#define GDI_PRI_WR_PRIO_L   		(GDMA_BASE + 0x008)
#define GDI_PRI_WR_PRIO_H   		(GDMA_BASE + 0x00c)
#define GDI_PRI_RD_LOCK_CNT 		(GDMA_BASE + 0x010)
#define GDI_PRI_WR_LOCK_CNT 		(GDMA_BASE + 0x014)
#define GDI_SEC_RD_PRIO_L   		(GDMA_BASE + 0x018)
#define GDI_SEC_RD_PRIO_H   		(GDMA_BASE + 0x01c)
#define GDI_SEC_WR_PRIO_L   		(GDMA_BASE + 0x020)
#define GDI_SEC_WR_PRIO_H   		(GDMA_BASE + 0x024)
#define GDI_SEC_RD_LOCK_CNT 		(GDMA_BASE + 0x028)
#define GDI_SEC_WR_LOCK_CNT 		(GDMA_BASE + 0x02c)
#define GDI_SEC_CLIENT_EN   		(GDMA_BASE + 0x030)
#define GDI_CONTROL                 (GDMA_BASE + 0x034)
#define GDI_PIC_INIT_HOST           (GDMA_BASE + 0x038)


#define GDI_PINFO_REQ               (GDMA_BASE + 0x060)
#define GDI_PINFO_ACK               (GDMA_BASE + 0x064)
#define GDI_PINFO_ADDR              (GDMA_BASE + 0x068)
#define GDI_PINFO_DATA              (GDMA_BASE + 0x06c)
#define GDI_BWB_ENABLE              (GDMA_BASE + 0x070)
#define GDI_BWB_SIZE                (GDMA_BASE + 0x074)
#define GDI_BWB_STD_STRUCT          (GDMA_BASE + 0x078)
#define GDI_BWB_STATUS              (GDMA_BASE + 0x07c)

#define GDI_STATUS					(GDMA_BASE + 0x080)

#define GDI_DEBUG_0                 (GDMA_BASE + 0x084)
#define GDI_DEBUG_1                 (GDMA_BASE + 0x088)
#define GDI_DEBUG_2                 (GDMA_BASE + 0x08c)
#define GDI_DEBUG_3                 (GDMA_BASE + 0x090)
#define GDI_DEBUG_PROBE_ADDR        (GDMA_BASE + 0x094)
#define GDI_DEBUG_PROBE_DATA        (GDMA_BASE + 0x098)
#define GDI_BUS_CTRL				(GDMA_BASE + 0x0F0)
#define GDI_BUS_STATUS				(GDMA_BASE + 0x0F4)


#define GDI_DCU_PIC_SIZE            (GDMA_BASE + 0x0a8)
#define GDI_SIZE_ERR_FLAG           (GDMA_BASE + 0x0e0)
#define GDI_INFO_CONTROL			(GDMA_BASE + 0x400)
#define GDI_INFO_PIC_SIZE			(GDMA_BASE + 0x404)
#define GDI_INFO_BASE_Y				(GDMA_BASE + 0x408)
#define GDI_INFO_BASE_CB			(GDMA_BASE + 0x40C)
#define GDI_INFO_BASE_CR			(GDMA_BASE + 0x410)
//------------------------------------------------------------------------------
// [FIRMWARE VERSION] COMMAND  
// [32:16] project number => 
// [16:0]  version => xxxx.xxxx.xxxxxxxx 
//------------------------------------------------------------------------------
#define RET_VER_NUM					(BIT_BASE + 0x1c0)



#endif

