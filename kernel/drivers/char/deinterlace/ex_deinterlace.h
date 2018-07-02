#ifndef __DO_DEINTERLACE_H__
#define __DO_DEINTERLACE_H__

#define DEINT_TRUE 1
#define DEINT_FALSE 0

#define EX_DMA_BOUNDARY_BYTES 8

#define FRAME_SIZE  (720*480)   
#define DEINT_Y_BUFFER (FRAME_SIZE)
#define DEINT_Y_BUFFER_SIZE (DEINT_Y_BUFFER*2)

#define DEINT_MV_BUFFER (FRAME_SIZE)
#define DEINT_MV_BUFFER_SIZE (DEINT_MV_BUFFER*2)

#define DEINT_Y_OUT_BUFFER (FRAME_SIZE)
#define DEINT_Y_OUT_BUFFER_SIZE (DEINT_Y_BUFFER*2)

typedef enum
{
    EX_DMA_CHANNEL_START = 0,
    EX_DMA_PV_TOP_FIELD_Y = EX_DMA_CHANNEL_START,
    EX_DMA_PV_BOTTOM_FIELD_Y,
    EX_DMA_PV_PRE_BOTTOM_FIELD_Y,
    EX_DMA_PV_FRAME_U,
    EX_DMA_PV_FRAME_V,
    EX_DMA_PV_PRE_BOTTOM_FIELD_MV,
    EX_DMA_PV_BOTTOM_FIELD_MV,
    EX_DMA_SV_TOP_FIELD_Y ,
    EX_DMA_SV_BOTTOM_FIELD_Y,
    EX_DMA_SV_PRE_BOTTOM_FIELD_Y,
    EX_DMA_SV_FRAME_U,
    EX_DMA_SV_FRAME_V,
    EX_DMA_DVE_PRE_FRAME_V,//EX_DMA_SV_PRE_BOTTOM_FIELD_MV,
    EX_DMA_PV_DEINT_Y,//EX_DMA_SV_BOTTOM_FIELD_MV,
    EX_DMA_PG_FRAME,
    EX_DMA_SG_FRAME,
    EX_DMA_OSD_FRAME,
    EX_DMA_DVE_FRAME_Y, // EX_DMA_CUR_FRAME
    EX_DMA_DVE_FRAME_U,
    EX_DMA_DVE_FRAME_V,
    EX_DMA_DVE_PRE_FRAME_Y,
    EX_DMA_DVE_PRE_FRAME_U,
    EX_DMA_CHANNEL_MAX,
}EX_DMA_CHANNEL;

typedef enum
{
    EX_ENDIAN_START = 0,
    /*byte order 76543210*/		
    EX_ENDIAN_MODE0 = EX_ENDIAN_START,
    /*byte order 01234567*/	
    EX_ENDIAN_MODE1,
    /*byte order 32107654*/	
    EX_ENDIAN_MODE2,
    /*byte order 45670123*/	
    EX_ENDIAN_MODE3,
    EX_ENDIAN_END,
}EX_ENDIAN_MODE;

typedef enum
{
    EX_DI_DENOISE_START = 0,
    EX_DI_DENOISE_TRI_REMOVE = EX_DI_DENOISE_START,
    EX_DI_DENOISE_ONE_PIXEL =1,
    EX_DI_DENOISE_TWO_PIXEL = 2,
    EX_DI_DENOISE_THREE_PIXEL = 3,
    EX_DI_DENOISE_END,
}EX_DENOISE_TOP;

typedef enum
{
    EX_DI_MOTION_START = 0,
    EX_DI_MOTION_1BIT = EX_DI_MOTION_START,
    EX_DI_MOTION_8BIT,
    EX_DI_MOTION_END,
}EX_MOTION_BIT;

typedef enum
{
    EX_INV_INPUT = 3,
    EX_INV_OUTPUT = 2,
    EX_INV_DISABLE = 0,
}EX_MSB_INV;

/*dit_cfg_00*/
#define EX_START_NEW_FRAME  (1<<0)

/*dit_cfg_01*/
#define EX_START_WORK (1<<0)
#define EX_AXI_ENABLE (1<<1)

/*dit_cfg_02*/
#define CLEAR_INT     (1<<0)

/*if_cfg_00*/
#define EX_SET_DI_SYNC (1<<0)

/*if_cfg_01*/
#define EX_SET_DI_PULLDOWN_FIELD (1<<2)
#define EX_SET_DI_WITH_PRE_FIELD (1<<4)
#define EX_SET_DI_WITH_PRE_MOTION (1<<5)
#define EX_SET_DI_WITH_CUR_MOTION (1<<6)
#define EX_SET_DI_1bit_MOTION (1<<7)
#define EX_DENOISE_TRIANGLE		(0<<8)
#define EX_DENOISE_ONEPIXEL		(1<<8)
#define EX_DENOISE_TWOPIXEL		(2<<8)
#define EX_DENOISE_THREEPIXEL	(3<<8)

/*if_cfg_02*/
#define EX_EDGE_THRESH			0x14
#define EX_MOTION_THRESH		0x8
#define EX_DENOISE_FILTER_THESH 0xa

/*if_cfg_03*/
#define EX_PD_THRESHOLD_0 0xf
#define EX_PD_THRESHOLD_1 0x12c

/*if_cfg_04*/
#define EX_DEFAULT_FRAME_ID (0)

/*if_cfg_05*/
#define EX_SET_DI_Y_DX(x) ((x)<<0)
#define EX_SET_DI_Y_DY(x) ((x)<<12)
#define EX_SET_DI_Y_DROP(x) ((x)<<24)

/*if_cfg_06*/


/*if_cfg_07*/
#define EX_Y_START 0xc
#define EX_Y_STOP 0x18
#define EX_Y_CRIT 0x6

/*if_cfg_08*/
#define EX_MV_START 0xc
#define EX_MV_STOP 0x18
#define EX_MV_CRIT 0x6

/*if_cfg_09*/


/*ctl cfg 00*/
#define EX_CLEAR_FCTL_INTERRUPT (1<<0)
#define EX_DEINT_CHANNEL_UPDATE (1<<8)
#define EX_Y_OUT_CHANNEL_UPDATE (1<<9)

typedef struct 
{
	volatile unsigned int dit_cfg_00;
	volatile unsigned int dit_cfg_01;
	volatile unsigned int dit_cfg_02;
    volatile unsigned int reserved[253];	
}DITCFG_REG;

typedef struct
{
    volatile unsigned int if_cfg_00;
    volatile unsigned int if_cfg_01;
    volatile unsigned int if_cfg_02;
    volatile unsigned int if_cfg_03;
    volatile unsigned int if_cfg_04;
    volatile unsigned int if_cfg_05;
    volatile unsigned int if_cfg_06;
    volatile unsigned int if_cfg_07;
    volatile unsigned int if_cfg_08;
    volatile unsigned int if_cfg_09;
    volatile unsigned int reserved[246];	
}IFCFG_REG;

typedef struct
{
    /*
    REG0:
    bit[15:0]:   current frame position x
    bit[27:16]: current frame position y
    bit[31:28]: current frame position z

    REG1:
    bit[15:0]:   frame picture size x
    bit[27:16]: frame picture size y
    bit[31:28]: frame picture size z

    REG2:
    bit[15:0]:   frame block size x
    bit[27:16]: frame block size y
    bit[28]: 0--no interleace sourcee ,1---interleave source
    bit[29]: 0--interlace source,1--pregressive source

    REG3:
    bit[15:0]:   frame offset x 
    bit[27:16]: frame offset y
    bit[31:28]: frame offset z
    */

    /* dma_ch0(pv_top_field_y)*/	
    volatile unsigned int fxyz_cfg_00;
    volatile unsigned int fxyz_cfg_01; 
    volatile unsigned int fxyz_cfg_02;
    volatile unsigned int fxyz_cfg_03;

    /* dma_ch1(pv_buttom_field_y)*/
    volatile unsigned int fxyz_cfg_04;
    volatile unsigned int fxyz_cfg_05;
    volatile unsigned int fxyz_cfg_06;
    volatile unsigned int fxyz_cfg_07;

    /* dma_ch2(pv_pre_buttom_field_y)*/
    volatile unsigned int fxyz_cfg_08;
    volatile unsigned int fxyz_cfg_09;
    volatile unsigned int fxyz_cfg_0a;
    volatile unsigned int fxyz_cfg_0b;

    /* dma_ch3(pv_frame_u)*/
    volatile unsigned int fxyz_cfg_0c;
    volatile unsigned int fxyz_cfg_0d;
    volatile unsigned int fxyz_cfg_0e;
    volatile unsigned int fxyz_cfg_0f;

    /* dma_ch4(pv_frame_v)*/
    volatile unsigned int fxyz_cfg_10;
    volatile unsigned int fxyz_cfg_11;
    volatile unsigned int fxyz_cfg_12;
    volatile unsigned int fxyz_cfg_13;

   /* dma_ch5(pv_pre_bottom_field_mv)*/
    volatile unsigned int fxyz_cfg_14;
    volatile unsigned int fxyz_cfg_15;
    volatile unsigned int fxyz_cfg_16;
    volatile unsigned int fxyz_cfg_17;

   /* dma_ch6(pv_bottom_field_mv)*/
    volatile unsigned int fxyz_cfg_18;
    volatile unsigned int fxyz_cfg_19;
    volatile unsigned int fxyz_cfg_1a;
    volatile unsigned int fxyz_cfg_1b;

    /* dma_ch7(sv_top_field_y)*/	
    volatile unsigned int fxyz_cfg_1c;
    volatile unsigned int fxyz_cfg_1d; 
    volatile unsigned int fxyz_cfg_1e;
    volatile unsigned int fxyz_cfg_1f;

    /* dma_ch8(sv_buttom_field_y)*/
    volatile unsigned int fxyz_cfg_20;
    volatile unsigned int fxyz_cfg_21;
    volatile unsigned int fxyz_cfg_22;
    volatile unsigned int fxyz_cfg_23;

    /* dma_ch9(sv_pre_buttom_field_y)*/
    volatile unsigned int fxyz_cfg_24;
    volatile unsigned int fxyz_cfg_25;
    volatile unsigned int fxyz_cfg_26;
    volatile unsigned int fxyz_cfg_27;

    /* dma_ch10(sv_frame_u)*/
    volatile unsigned int fxyz_cfg_28;
    volatile unsigned int fxyz_cfg_29;
    volatile unsigned int fxyz_cfg_2a;
    volatile unsigned int fxyz_cfg_2b;

    /* dma_ch11(sv_frame_v)*/
    volatile unsigned int fxyz_cfg_2c;
    volatile unsigned int fxyz_cfg_2d;
    volatile unsigned int fxyz_cfg_2e;
    volatile unsigned int fxyz_cfg_2f;

   /* dma_ch12(sv_pre_bottom_field_mv)*/
    volatile unsigned int fxyz_cfg_30;
    volatile unsigned int fxyz_cfg_31;
    volatile unsigned int fxyz_cfg_32;
    volatile unsigned int fxyz_cfg_33;

   /* dma_ch13(sv_bottom_field_mv)*/
    volatile unsigned int fxyz_cfg_34;
    volatile unsigned int fxyz_cfg_35;
    volatile unsigned int fxyz_cfg_36;
    volatile unsigned int fxyz_cfg_37;

   /* dma_ch14(pg_frame)*/
    volatile unsigned int fxyz_cfg_38;
    volatile unsigned int fxyz_cfg_39;
    volatile unsigned int fxyz_cfg_3a;
    volatile unsigned int fxyz_cfg_3b;

   /* dma_ch15(sg_frame)*/
    volatile unsigned int fxyz_cfg_3c;
    volatile unsigned int fxyz_cfg_3d;
    volatile unsigned int fxyz_cfg_3e;
    volatile unsigned int fxyz_cfg_3f;

   /* dma_ch16(osd_frame)*/
    volatile unsigned int fxyz_cfg_40;
    volatile unsigned int fxyz_cfg_41;
    volatile unsigned int fxyz_cfg_42;
    volatile unsigned int fxyz_cfg_43;

   /* dma_ch17(cursor_frame)*/
    volatile unsigned int fxyz_cfg_44;
    volatile unsigned int fxyz_cfg_45;
    volatile unsigned int fxyz_cfg_46;
    volatile unsigned int fxyz_cfg_47;

   /* dma_ch18(cur_dve_frame_y)*/
    volatile unsigned int fxyz_cfg_48;
    volatile unsigned int fxyz_cfg_49;
    volatile unsigned int fxyz_cfg_4a;
    volatile unsigned int fxyz_cfg_4b;

   /* dma_ch19(cur_dve_frame_uv)*/
    volatile unsigned int fxyz_cfg_4c;
    volatile unsigned int fxyz_cfg_4d;
    volatile unsigned int fxyz_cfg_4e;
    volatile unsigned int fxyz_cfg_4f;

   /* dma_ch20(pre_cur_dve_frame_y)*/
    volatile unsigned int fxyz_cfg_50;
    volatile unsigned int fxyz_cfg_51;
    volatile unsigned int fxyz_cfg_52;
    volatile unsigned int fxyz_cfg_53;

   /* dma_ch21(pre_cur_dve_frame_uv)*/
    volatile unsigned int fxyz_cfg_54;
    volatile unsigned int fxyz_cfg_55;
    volatile unsigned int fxyz_cfg_56;
    volatile unsigned int fxyz_cfg_57;
    volatile unsigned int reserved[40];	
}EX_DMAFXYZREG;

typedef struct
{
    /*
    bit[0] :dma start,it is self clean;
    bit[1] :dma interrupt clear,it is slef clean;
    */
    volatile unsigned int fctl_cfg_00;
    /*
    bit[21:0]:dma_en_ch[21:0],1---dma enable,0---dma disable
    bit[31]: 
    1--automatically start next frame dma after finishing current frame dma;
    0--need to wait dma starting command to start next frame dma after finishing current frame dma;            
    */
    volatile unsigned int fctl_cfg_01;
    volatile unsigned int fctl_cfg_02;
    volatile unsigned int fctl_cfg_03;
    volatile unsigned int fctl_cfg_04;
    volatile unsigned int reserved[123];
}EX_DMAFCTLREG;

typedef struct
{
    /*direction IN , frame 0,1, base address for dma_ch0 (pv_top_field_y) */
     volatile unsigned int fbas_cfg_00;
     volatile unsigned int fbas_cfg_01;
	
    /* direction IN , frame 0,1, base address for dma_ch1 (pv_bottom_field_y) */
     volatile unsigned int fbas_cfg_02;
     volatile unsigned int fbas_cfg_03;	

    /* direction IN , frame 0,1,  base address for dma_ch2 (pv_pre_bottom_field_y) */
     volatile unsigned int fbas_cfg_04;
     volatile unsigned int fbas_cfg_05;	

    /* direction IN , frame 0,1,  base address for dma_ch3 (pv_frame_u) */
     volatile unsigned int fbas_cfg_06;
     volatile unsigned int fbas_cfg_07;	

    /*direction IN ,  frame 0,1,  base address for dma_ch4 (pv_frame_v) */
     volatile unsigned int fbas_cfg_08;	
     volatile unsigned int fbas_cfg_09;

    /*direction IN ,  frame 0,1,  base address for dma_ch5 (pv_pre_bottom_field_mv) */
     volatile unsigned int fbas_cfg_0a;
     volatile unsigned int fbas_cfg_0b;	

    /* direction OUT , frame 0,1,  base address for dma_ch6 (pv_bottom_field_mv) */
     volatile unsigned int fbas_cfg_0c;
     volatile unsigned int fbas_cfg_0d;	
	 
    /*direction IN ,  frame 0,1,  base address for dma_ch7 (sv_top_field) */
     volatile unsigned int fbas_cfg_0e;	
     volatile unsigned int fbas_cfg_0f;

    /* direction IN , frame 0,1,   base address for dma_ch8 (sv_bottom_field_y) */
     volatile unsigned int fbas_cfg_10;	
     volatile unsigned int fbas_cfg_11;

    /*direction IN ,  frame 0,1,  base address for dma_ch9 (sv_pre_bottom_field_y) */
     volatile unsigned int fbas_cfg_12;	
     volatile unsigned int fbas_cfg_13;

    /* direction IN , frame 0,1,  base address for dma_ch10 (sv_frame_u) */
     volatile unsigned int fbas_cfg_14;	
     volatile unsigned int fbas_cfg_15;

    /* direction IN , frame 0,1,  base address for dma_ch11 (sv_frame_v) */
     volatile unsigned int fbas_cfg_16;	
     volatile unsigned int fbas_cfg_17;

    /*direction IN ,  frame 0,1,  base address for dma_ch12 (pre_dve_frame_u) */ // (sv_pre_bottom_field_mv)
     volatile unsigned int fbas_cfg_18;	
     volatile unsigned int fbas_cfg_19;

    /* direction OUT , frame 0,1,  base address for dma_ch13 (sv_bottom_field_mv) */
     volatile unsigned int fbas_cfg_1a;	
     volatile unsigned int fbas_cfg_1b;

   /* direction IN , frame 0,1,  base address for dma_ch14 (pg_frame)*/
     volatile unsigned int fbas_cfg_1c;	
     volatile unsigned int fbas_cfg_1d; 

  /* direction IN , frame 0,1,  base address for dma_ch15 (sg_frame)*/
     volatile unsigned int fbas_cfg_1e;	
     volatile unsigned int fbas_cfg_1f;

  /* direction IN , frame 0,1,  base address for dma_ch16 (osd_frame)*/
     volatile unsigned int fbas_cfg_20;
     volatile unsigned int fbas_cfg_21;	

  /* direction IN , frame 0,1,  base address for dma_ch17 (dve_frame_y)*/ //(cursor_frame)
     volatile unsigned int fbas_cfg_22;	
     volatile unsigned int fbas_cfg_23;

  /*direction IN ,  frame 0,1,  base address for dma_ch18 (dve_frame_u)*/ //(dve_frame_y)
     volatile unsigned int fbas_cfg_24;	
     volatile unsigned int fbas_cfg_25;

  /* direction IN , frame 0,1,  base address for dma_ch19 (dve_frame_v)*/ //(dve_frame_uv)
     volatile unsigned int fbas_cfg_26;	
     volatile unsigned int fbas_cfg_27;

  /* direction OUT , frame 0,1,  base address for dma_ch20 (pre_dve_frame_y)*/ //(pre_dve_frame_y)
     volatile unsigned int fbas_cfg_28;
     volatile unsigned int fbas_cfg_29;

  /* direction OUT , frame 0,1,  base address for dma_ch21 (pre_dve_frame_u)*/ //(pre_dve_frame_uv)
     volatile unsigned int fbas_cfg_2a;
     volatile unsigned int fbas_cfg_2b;	
     volatile unsigned int reserved[84];	 
}EX_DMABASREG;

typedef struct 
{
	/*  0h */
	volatile unsigned int reserved[5120];
	
	/*0x5000h*/
	DITCFG_REG dit_cfg_reg;
	
	/*0x5400h*/
	volatile unsigned int reserved1[256];
	
	/*0x5800h*/
	IFCFG_REG if_cfg_reg;

	/*0x5c00h*/
	volatile unsigned int reserved2[4096];

	/*0x9c00*/
	EX_DMAFXYZREG dma_xyz_reg;
	/*0x9e00*/
	EX_DMAFCTLREG dma_ctl_reg;
	/*0xa000*/
	EX_DMABASREG dma_bas_reg;
}EXDITREG;

struct ex_dit_cfg
{
	volatile unsigned int dit_cfg_00;
	volatile unsigned int dit_cfg_01;
	volatile unsigned int dit_cfg_02;	
};

struct ex_if_cfg
{
	volatile unsigned int if_cfg_00;
    volatile unsigned int if_cfg_01;
    volatile unsigned int if_cfg_02;
    volatile unsigned int if_cfg_03;
    volatile unsigned int if_cfg_04;
    volatile unsigned int if_cfg_05;
    volatile unsigned int if_cfg_06;
    volatile unsigned int if_cfg_07;
    volatile unsigned int if_cfg_08;
    volatile unsigned int if_cfg_09;
};


/*ctl cfg 00*/

/*ctl cfg 01*/
#define EX_SET_DMA_PV_TOP_FIELD_Y_ON              (1<<EX_DMA_PV_TOP_FIELD_Y)
#define EX_SET_DMA_PV_BUTTOM_FIELD_Y_ON           (1<<EX_DMA_PV_BOTTOM_FIELD_Y)
#define EX_SET_DMA_PV_PRE_BUTTOM_FIELD_Y_ON       (1<<EX_DMA_PV_PRE_BOTTOM_FIELD_Y)
#define EX_SET_DMA_PV_FRAME_U_ON                  (1<<EX_DMA_PV_FRAME_U)
#define EX_SET_DMA_PV_FRAME_V_ON                  (1<<EX_DMA_PV_FRAME_V)
#define EX_SET_DMA_PV_PRE_BUTTOM_FIELD_MV_ON      (1<<EX_DMA_PV_PRE_BOTTOM_FIELD_MV)
#define EX_SET_DMA_PV_BUTTOM_FIELD_MV_ON          (1<<EX_DMA_PV_BOTTOM_FIELD_MV)

#define EX_SET_DMA_SV_TOP_FIELD_Y_ON              (1<<EX_DMA_SV_TOP_FIELD_Y)
#define EX_SET_DMA_SV_BUTTOM_FIELD_Y_ON           (1<<EX_DMA_SV_BOTTOM_FIELD_Y)
#define EX_SET_DMA_SV_PRE_BUTTOM_FIELD_Y_ON       (1<<EX_DMA_SV_PRE_BOTTOM_FIELD_Y)
#define EX_SET_DMA_SV_FRAME_U_ON                  (1<<EX_DMA_SV_FRAME_U)
#define EX_SET_DMA_SV_FRAME_V_ON                  (1<<EX_DMA_SV_FRAME_V)
#define EX_SET_DMA_SV_PRE_BUTTOM_FIELD_MV_ON      (1<<EX_DMA_SV_PRE_BOTTOM_FIELD_MV)
#define EX_SET_DMA_SV_BUTTOM_FIELD_MV_ON          (1<<EX_DMA_SV_BOTTOM_FIELD_MV)

#define EX_SET_DMA_PG_FRAME_ON                    (1<<EX_DMA_PG_FRAME)
#define EX_SET_DMA_SG_FRAME_ON                    (1<<EX_DMA_SG_FRAME)
#define EX_SET_DMA_OSD_FRAME_ON                   (1<<EX_DMA_OSD_FRAME)
#define EX_SET_DMA_CURSOR_FRAME_ON                (1<<EX_DMA_CURSOR_FRAME)

#define EX_SET_DMA_DVE_FRAME_Y_ON                 (1<<EX_DMA_DVE_FRAME_Y)
#define EX_SET_DMA_DVE_FRAME_UV_ON                (1<<EX_DMA_DVE_FRAME_UV)
#define EX_SET_DMA_DVE_PRE_FRAME_Y_ON			  (1<<EX_DMA_DVE_PRE_FRAME_Y)
#define EX_SET_DMA_DVE_PRE_FRAME_UV_ON            (1<<EX_DMA_DVE_PRE_FRAME_UV)

#define EX_SET_DMA_ENDIAN_MODE(x)    ((x)<<24)
#define EX_CLR_DMA_ENDIAN_MODE (0x3<<24)

#define EX_SET_FCTL_DMA_AUTO_MODE (1<<31)

#define EX_DMA_CHANNEL_STATUS_MASK (0x3fffff)
#define EX_DMA_ONE_CHANNEL_BAS_MEM_SIZE 8


struct ex_fbas_cfg_table
{
    unsigned int frame0_addr;
    unsigned int frame1_addr;
};

#define EX_SET_XYZ_CFG_POS_X(x) ((x)<<0)
#define EX_GET_XYZ_CFG_POS_X(x) ((x)&0xffff)
#define EX_SET_XYZ_CFG_POS_Y(x) ((x)<<16)
#define EX_GET_XYZ_CFG_POS_Y(x) (((x)>>16)&0xfff)
#define EX_SET_XYZ_CFG_POS_Z(x) ((x)<<28)
#define EX_GET_XYZ_CFG_POS_Z(x) (((x)>>28)&0xf)

#define EX_SET_XYZ_PIC_SIZE_X(x) ((x)<<0)
#define EX_SET_XYZ_PIC_SIZE_Y(x) ((x)<<16)
#define EX_CLR_XYZ_TOTAL_FRAME ((0xf)<<28)
#define EX_SET_XYZ_TOTAL_FRAME(x) ((x)<<28)

#define EX_SET_XYZ_BLOCK_SIZE_X(x)  ((x)<<0)
#define EX_SET_XYZ_BLOCK_SIZE_Y(x)  ((x)<<16)
#define EX_SET_XYZ_INTERLEAVE(x) ((((x)>0)?1:0)<<28)
#define EX_SET_XYZ_PROGRESSIVE(x)  ((((x)>0)?1:0)<<29)
#define EX_SET_XYZ_RLDMODE (1<<30)

#define EX_SET_XYZ_INIT_BLOCK_X(x) ((x)<<0)
#define EX_SET_XYZ_INIT_BLOCK_Y(x) ((x)<<16)
#define EX_CLR_XYZ_BEGIN_FRAME_NUM ((0xf)<<28)
#define EX_SET_XYZ_BEGIN_FRAME_NUM(x) ((x)<<28)

#define EX_DMA_ONE_CHANNEL_XYZ_MEM_SIZE 16

struct ex_fxyz_cfg_table
{
    unsigned int fxyz_cur_pos_reg;
    unsigned int fxyz_pic_size_reg;
    unsigned int fxyz_block_size_reg;
    unsigned int fxyz_block_offset_reg;	
};

struct ex_fctl_cfg_table
{
    unsigned int fctl_dma_reg;
    unsigned int fctl_channel_dma_reg;
    unsigned int fctl_dma_int_timer;
    unsigned int fctl_dma_idle;
};

struct Ex_dit_cfg_info
{
	struct ex_dit_cfg dit_cfg_reg;
	DITCFG_REG *dit_cfg;

	struct ex_if_cfg  if_cfg_reg;
	IFCFG_REG  *if_cfg;
	
	struct ex_fxyz_cfg_table xyz_cfg_tbl[EX_DMA_CHANNEL_MAX];
	EX_DMAFXYZREG *dma_xyz;

	struct ex_fctl_cfg_table ctl_cfg_tbl;
	EX_DMAFCTLREG *dma_ctl;

	struct ex_fbas_cfg_table bas_cfg_tbl[EX_DMA_CHANNEL_MAX];
	EX_DMABASREG *dma_bas;

	int deint_1b_mode; // 0: 8bit motion info 1: 1bit motion info
	EX_ENDIAN_MODE ex_endian_mode;
    
    unsigned long map_addr;
    unsigned long y_base_addr[2];
    unsigned long mv_base_addr[2];
    unsigned long y_out_base_addr[2];
};

struct Ex_dit_config
{
    unsigned int width;
    unsigned int height;
};

struct Ex_dit_update_info
{
	unsigned int pre_y_addr;
	unsigned int cur_y_addr;
    unsigned int mv_used;
};

int ex_deint_clear_interrupt(void);
int ex_deint_enable(int enable);
int ex_deint_cfg_sync(void);
int	ex_deint_init(EXDITREG *reg); 
int ex_deint_denoise_config(EX_DENOISE_TOP denoise_type);
int ex_deint_motion_config(EX_MOTION_BIT motion_bit);

int ex_dma_cfg_init(EXDITREG* reg);
int ex_dma_cfg_channel_status(void);
int ex_dma_cfg_update(int value);
int ex_deint_io_cfg(struct Ex_dit_config *ex_dit_config);
int ex_dma_idle_check(void);
int ex_deint_cfg_baseaddr(struct Ex_dit_update_info *update_info);
void ex_deint_start(int odd_flag);
int ex_deint_is_done(void);
int ex_cfg_addr(unsigned long y_addr, unsigned long mv_addr, unsigned long y_out_addr, unsigned long map_addr);

#endif
