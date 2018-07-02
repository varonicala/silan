#include <linux/io.h>
#include <linux/kernel.h>
#include "ex_deinterlace.h"

struct Ex_dit_cfg_info ex_dit_cfg_info;

static void deint_writel(unsigned int val, unsigned int addr)
{
	(*(volatile unsigned int*)(addr)) = (val);
}

static unsigned int deint_readl(unsigned int addr)
{
	return (unsigned)(*(volatile unsigned int*)(addr));
}

int ex_deint_clear_interrupt(void)
{
	unsigned int reg_value;
	reg_value = deint_readl((unsigned int)&(ex_dit_cfg_info.dit_cfg->dit_cfg_02));	
	reg_value |= CLEAR_INT;	

	deint_writel(reg_value, (unsigned int)&ex_dit_cfg_info.dit_cfg->dit_cfg_02);

	return DEINT_TRUE;
}

int ex_deint_enable(int enable)
{
	if(enable)
		ex_dit_cfg_info.dit_cfg_reg.dit_cfg_01 |= EX_START_WORK|EX_AXI_ENABLE;
	else
		ex_dit_cfg_info.dit_cfg_reg.dit_cfg_01 &= ~(EX_START_WORK|EX_AXI_ENABLE); 

	deint_writel(ex_dit_cfg_info.dit_cfg_reg.dit_cfg_01, (unsigned int)&ex_dit_cfg_info.dit_cfg->dit_cfg_01);
	return DEINT_TRUE;
}

int ex_deint_cfg_sync(void)
{
	deint_writel(EX_SET_DI_SYNC, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_00);
	return DEINT_TRUE;
}

static int ex_deint_Y_block_size(int dx,int dy,int drop)
{
    unsigned int reg_val = EX_SET_DI_Y_DX(dx);
    reg_val |= EX_SET_DI_Y_DY(dy);
    reg_val |= EX_SET_DI_Y_DROP(drop);

	deint_writel(reg_val, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_05);
	return DEINT_TRUE;	
}


int ex_deint_denoise_config(EX_DENOISE_TOP denoise_type)
{
	switch(denoise_type)	
	{
		case EX_DI_DENOISE_TRI_REMOVE:
			ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_DENOISE_TRIANGLE;
			break;
		case EX_DI_DENOISE_ONE_PIXEL:
			ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_DENOISE_ONEPIXEL;
			break;
		case EX_DI_DENOISE_TWO_PIXEL:
			ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_DENOISE_TWOPIXEL;
			break;
		case EX_DI_DENOISE_THREE_PIXEL:
			ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_DENOISE_THREEPIXEL;
			break;
		default:
			ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_DENOISE_TRIANGLE;
			break;
	}
	deint_writel(ex_dit_cfg_info.if_cfg_reg.if_cfg_01, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_01);

	return DEINT_TRUE;	
}

int ex_deint_motion_config(EX_MOTION_BIT motion_bit)
{
	switch(motion_bit)	
	{
		case EX_DI_MOTION_1BIT:
			ex_dit_cfg_info.deint_1b_mode = 1;
			ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_SET_DI_1bit_MOTION;
			break;
		case EX_DI_MOTION_8BIT:
			ex_dit_cfg_info.deint_1b_mode = 0;
			ex_dit_cfg_info.if_cfg_reg.if_cfg_01 &= ~EX_SET_DI_1bit_MOTION;
			break;
		default:
			ex_dit_cfg_info.deint_1b_mode = 0;
			ex_dit_cfg_info.if_cfg_reg.if_cfg_01 &= ~EX_SET_DI_1bit_MOTION;
			break;
	}
	deint_writel(ex_dit_cfg_info.if_cfg_reg.if_cfg_01, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_01);

	return DEINT_TRUE;	
}

int	ex_deint_init(EXDITREG *reg) 
{
	ex_dit_cfg_info.dit_cfg = &reg->dit_cfg_reg;
	ex_dit_cfg_info.if_cfg = &reg->if_cfg_reg;

    memset(&ex_dit_cfg_info.dit_cfg_reg,0,sizeof(struct ex_dit_cfg));
    memset(&ex_dit_cfg_info.if_cfg_reg,0,sizeof(struct ex_if_cfg));

	ex_dit_cfg_info.if_cfg_reg.if_cfg_01 = 0;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_SET_DI_PULLDOWN_FIELD;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_SET_DI_WITH_PRE_FIELD;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_SET_DI_WITH_PRE_MOTION;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_01 |= EX_SET_DI_WITH_CUR_MOTION;
	deint_writel(ex_dit_cfg_info.if_cfg_reg.if_cfg_01, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_01);

	ex_dit_cfg_info.if_cfg_reg.if_cfg_02 = 0;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_02 |= (EX_EDGE_THRESH << 0);
	ex_dit_cfg_info.if_cfg_reg.if_cfg_02 |= (EX_MOTION_THRESH << 8);
	ex_dit_cfg_info.if_cfg_reg.if_cfg_02 |= (EX_DENOISE_FILTER_THESH << 16);
	deint_writel(ex_dit_cfg_info.if_cfg_reg.if_cfg_02, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_02);

	ex_dit_cfg_info.if_cfg_reg.if_cfg_03 = 0;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_03 |= (EX_PD_THRESHOLD_0 << 0);
	ex_dit_cfg_info.if_cfg_reg.if_cfg_03 |= (EX_PD_THRESHOLD_1 << 8);
	deint_writel(ex_dit_cfg_info.if_cfg_reg.if_cfg_03, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_03);

	ex_dit_cfg_info.if_cfg_reg.if_cfg_04 = 0;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_04 |= EX_DEFAULT_FRAME_ID;
	deint_writel(ex_dit_cfg_info.if_cfg_reg.if_cfg_04, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_04);

	ex_dit_cfg_info.if_cfg_reg.if_cfg_07 = 0;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_07 |= (EX_Y_STOP << 0);
	ex_dit_cfg_info.if_cfg_reg.if_cfg_07 |= (EX_Y_START << 8);
	ex_dit_cfg_info.if_cfg_reg.if_cfg_07 |= (EX_Y_CRIT << 16);
	deint_writel(ex_dit_cfg_info.if_cfg_reg.if_cfg_07, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_07);

	ex_dit_cfg_info.if_cfg_reg.if_cfg_08 = 0;
	ex_dit_cfg_info.if_cfg_reg.if_cfg_08 |= (EX_MV_STOP << 0);
	ex_dit_cfg_info.if_cfg_reg.if_cfg_08 |= (EX_MV_START << 8);
	ex_dit_cfg_info.if_cfg_reg.if_cfg_08 |= (EX_MV_CRIT << 16);
	deint_writel(ex_dit_cfg_info.if_cfg_reg.if_cfg_08, (unsigned int)&ex_dit_cfg_info.if_cfg->if_cfg_08);

    ex_deint_cfg_sync();

	return DEINT_TRUE;	
}

int ex_dma_cfg_init(EXDITREG* reg)
{
	ex_dit_cfg_info.dma_ctl = &reg->dma_ctl_reg;
	ex_dit_cfg_info.dma_xyz = &reg->dma_xyz_reg;
    ex_dit_cfg_info.dma_bas = &reg->dma_bas_reg;
		
    memset(&ex_dit_cfg_info.ctl_cfg_tbl,0,sizeof(struct ex_fctl_cfg_table));
    memset(&ex_dit_cfg_info.bas_cfg_tbl,0,sizeof(struct ex_fbas_cfg_table)*EX_DMA_CHANNEL_MAX);
    memset(&ex_dit_cfg_info.xyz_cfg_tbl,0,sizeof(struct ex_fxyz_cfg_table)*EX_DMA_CHANNEL_MAX);

	ex_dit_cfg_info.ctl_cfg_tbl.fctl_channel_dma_reg = 0;
	ex_dit_cfg_info.ctl_cfg_tbl.fctl_channel_dma_reg |= EX_SET_DMA_ENDIAN_MODE(ex_dit_cfg_info.ex_endian_mode);
	deint_writel(ex_dit_cfg_info.ctl_cfg_tbl.fctl_channel_dma_reg, (unsigned int)&ex_dit_cfg_info.dma_ctl->fctl_cfg_01);

    return DEINT_TRUE;
}

static int ex_dma_cfg_cur_xyz(EX_DMA_CHANNEL dma_id,int pos_x,int pos_y,int cur_frame_num)
{
	if(dma_id > EX_DMA_CHANNEL_MAX || dma_id < EX_DMA_CHANNEL_START)
        return DEINT_FALSE;

	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_cur_pos_reg = EX_SET_XYZ_CFG_POS_X(pos_x);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_cur_pos_reg |= EX_SET_XYZ_CFG_POS_Y(pos_y);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_cur_pos_reg |= EX_SET_XYZ_CFG_POS_Z(cur_frame_num);
	deint_writel(ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_cur_pos_reg,(unsigned int)&(ex_dit_cfg_info.dma_xyz->fxyz_cfg_00)+dma_id*EX_DMA_ONE_CHANNEL_XYZ_MEM_SIZE);

	return DEINT_TRUE;
}

static int ex_dma_cfg_picture_xyz(EX_DMA_CHANNEL dma_id,int dx,int dy,int total_frame)
{
	if(dma_id > EX_DMA_CHANNEL_MAX || dma_id < EX_DMA_CHANNEL_START)
        return DEINT_FALSE;

	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_pic_size_reg = EX_SET_XYZ_PIC_SIZE_X(dx);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_pic_size_reg |= EX_SET_XYZ_PIC_SIZE_Y(dy);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_pic_size_reg |= EX_SET_XYZ_TOTAL_FRAME(total_frame);
	deint_writel(ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_pic_size_reg,(unsigned int)&(ex_dit_cfg_info.dma_xyz->fxyz_cfg_01)+dma_id*EX_DMA_ONE_CHANNEL_XYZ_MEM_SIZE);

	return DEINT_TRUE;	
}
static int ex_dma_cfg_block_xyz(EX_DMA_CHANNEL dma_id,int dx,int dy,int is_interleave,int is_progressive)
{
    if(dma_id > EX_DMA_CHANNEL_MAX || dma_id < EX_DMA_CHANNEL_START)
        return DEINT_FALSE;

	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_size_reg = EX_SET_XYZ_BLOCK_SIZE_X(dx);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_size_reg |= EX_SET_XYZ_BLOCK_SIZE_Y(dy);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_size_reg |= EX_SET_XYZ_INTERLEAVE(is_interleave);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_size_reg |= EX_SET_XYZ_PROGRESSIVE(is_progressive);;

   deint_writel(ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_size_reg,(unsigned int)&(ex_dit_cfg_info.dma_xyz->fxyz_cfg_02)+dma_id*EX_DMA_ONE_CHANNEL_XYZ_MEM_SIZE);
	return DEINT_TRUE;	
}

static int ex_dma_cfg_block_offset(EX_DMA_CHANNEL dma_id,int offset_x,int offset_y,int begin_frame)
{
    if(dma_id > EX_DMA_CHANNEL_MAX || dma_id < EX_DMA_CHANNEL_START)
        return DEINT_FALSE;

	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_offset_reg = EX_SET_XYZ_INIT_BLOCK_X(offset_x);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_offset_reg |= EX_SET_XYZ_INIT_BLOCK_Y(offset_y);
	ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_offset_reg |= EX_SET_XYZ_BEGIN_FRAME_NUM(begin_frame);
   deint_writel(ex_dit_cfg_info.xyz_cfg_tbl[dma_id].fxyz_block_offset_reg,(unsigned int)&(ex_dit_cfg_info.dma_xyz->fxyz_cfg_03)+dma_id*EX_DMA_ONE_CHANNEL_XYZ_MEM_SIZE);

	return DEINT_TRUE;	
}

static int ex_dma_cfg_base_addr_ex(EX_DMA_CHANNEL dma_id,unsigned int frame_addr,int num)
{
	if(dma_id > EX_DMA_CHANNEL_MAX || dma_id < EX_DMA_CHANNEL_START)
		return DEINT_FALSE;
	num = num %2;
	switch(num)
	{
		case 0:
			ex_dit_cfg_info.bas_cfg_tbl[dma_id].frame0_addr = frame_addr;
			deint_writel(ex_dit_cfg_info.bas_cfg_tbl[dma_id].frame0_addr,(unsigned int)&(ex_dit_cfg_info.dma_bas->fbas_cfg_00)+dma_id*EX_DMA_ONE_CHANNEL_BAS_MEM_SIZE);
			break;
		case 1:
			ex_dit_cfg_info.bas_cfg_tbl[dma_id].frame1_addr = frame_addr;
			deint_writel(ex_dit_cfg_info.bas_cfg_tbl[dma_id].frame1_addr,(unsigned int)&(ex_dit_cfg_info.dma_bas->fbas_cfg_01)+dma_id*EX_DMA_ONE_CHANNEL_BAS_MEM_SIZE);
			break;
		default:
			break;
	}
	return DEINT_TRUE;	
}
/*
static int ex_dma_cfg_base_addr(EX_DMA_CHANNEL dma_id,unsigned int frame0_addr,unsigned int frame1_addr)
{
	if(dma_id > EX_DMA_CHANNEL_MAX || dma_id < EX_DMA_CHANNEL_START)
        return DEINT_FALSE;

	ex_dit_cfg_info.bas_cfg_tbl[dma_id].frame0_addr = frame0_addr;
	ex_dit_cfg_info.bas_cfg_tbl[dma_id].frame1_addr = frame1_addr;
	deint_writel(ex_dit_cfg_info.bas_cfg_tbl[dma_id].frame0_addr,(unsigned int)&(ex_dit_cfg_info.dma_bas->fbas_cfg_00)+dma_id*EX_DMA_ONE_CHANNEL_BAS_MEM_SIZE);
	deint_writel(ex_dit_cfg_info.bas_cfg_tbl[dma_id].frame1_addr,(unsigned int)&(ex_dit_cfg_info.dma_bas->fbas_cfg_01)+dma_id*EX_DMA_ONE_CHANNEL_BAS_MEM_SIZE);

	return DEINT_TRUE;	
}
*/
static int ex_dma_cfg_channel_enable(EX_DMA_CHANNEL dma_id,int enable)
{
	unsigned int dma_status;
	if(dma_id > EX_DMA_CHANNEL_MAX || dma_id < EX_DMA_CHANNEL_START)
		return DEINT_FALSE;

	dma_status = ex_dit_cfg_info.ctl_cfg_tbl.fctl_channel_dma_reg &(1<<dma_id);
	if((dma_status==0&&enable == 0) ||(dma_status>0&&enable == 1))
		return DEINT_TRUE;

	if(enable)		
		ex_dit_cfg_info.ctl_cfg_tbl.fctl_channel_dma_reg |= (1<<dma_id);
	else
		ex_dit_cfg_info.ctl_cfg_tbl.fctl_channel_dma_reg &= ~(1<<dma_id);	
	deint_writel(ex_dit_cfg_info.ctl_cfg_tbl.fctl_channel_dma_reg,(unsigned int)&(ex_dit_cfg_info.dma_ctl->fctl_cfg_01));
	return DEINT_TRUE;	
}

int ex_dma_cfg_channel_status(void)
{
	unsigned int status;
	unsigned int hardware_status;
	status = deint_readl((unsigned int)&(ex_dit_cfg_info.dma_ctl->fctl_cfg_04));

	hardware_status = status & 0x3fffff;
	if(hardware_status == 0)
		return 1;
	return 0;
}


int ex_dma_cfg_update(int value)
{	
	unsigned int reg_value;
	reg_value = deint_readl((unsigned int)&(ex_dit_cfg_info.dma_ctl->fctl_cfg_00));	
	reg_value |= value;	
	deint_writel(reg_value, (unsigned int)&(ex_dit_cfg_info.dma_ctl->fctl_cfg_00));

	return DEINT_TRUE;	
}

int ex_deint_io_cfg(struct Ex_dit_config *ex_dit_config)
{
    unsigned int y_drop;
    unsigned int in_offsetx, in_offsety, in_width, in_height;
    unsigned int out_offsetx, out_offsety, out_width, out_height;
    int y_interleave, is_progressive;
	unsigned int temp_width;

    in_offsetx = 0;
    in_offsety = 0;
    in_width = ex_dit_config->width;
    in_height = ex_dit_config->height;
    out_offsetx = 0;
    out_offsety = 0;
    out_width = ex_dit_config->width;
    out_height = ex_dit_config->height;  
    y_interleave = 1;
    is_progressive = 0;
	y_drop = 0;

    ex_dma_cfg_picture_xyz(EX_DMA_PV_TOP_FIELD_Y,in_width,in_height/2,4);
    ex_dma_cfg_picture_xyz(EX_DMA_PV_BOTTOM_FIELD_Y,in_width,in_height/2,4);	
    ex_dma_cfg_picture_xyz(EX_DMA_PV_PRE_BOTTOM_FIELD_Y,in_width,in_height/2,4);

	if(ex_dit_cfg_info.deint_1b_mode == 1)
	{
		temp_width = in_width/8;
		if(temp_width%8)
			temp_width = (temp_width&(~(0x7)))+8;
		ex_dma_cfg_picture_xyz(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,temp_width,in_height/2,4);
		ex_dma_cfg_picture_xyz(EX_DMA_PV_BOTTOM_FIELD_MV,temp_width,in_height/2,4);
	}
	else
	{
		ex_dma_cfg_picture_xyz(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,in_width,in_height/2,4);
		ex_dma_cfg_picture_xyz(EX_DMA_PV_BOTTOM_FIELD_MV,in_width,in_height/2,4);
	}
    ex_dma_cfg_picture_xyz(EX_DMA_PV_DEINT_Y,in_width,in_height,4);

    y_drop = in_offsetx- (in_offsetx&(~(EX_DMA_BOUNDARY_BYTES-1)));
    ex_deint_Y_block_size(in_width, in_height, y_drop*8);

    in_offsetx = in_offsetx - y_drop;
    ex_dma_cfg_block_xyz(EX_DMA_PV_TOP_FIELD_Y,in_width+y_drop,in_height/2,y_interleave,is_progressive);
    ex_dma_cfg_block_offset(EX_DMA_PV_TOP_FIELD_Y,in_offsetx,in_offsety,0);
    ex_dma_cfg_block_xyz(EX_DMA_PV_BOTTOM_FIELD_Y,in_width+y_drop,in_height/2,y_interleave,is_progressive);
    ex_dma_cfg_block_offset(EX_DMA_PV_BOTTOM_FIELD_Y,in_offsetx,in_offsety,0);
    ex_dma_cfg_block_xyz(EX_DMA_PV_PRE_BOTTOM_FIELD_Y,in_width+y_drop,in_height/2,y_interleave,is_progressive);
    ex_dma_cfg_block_offset(EX_DMA_PV_PRE_BOTTOM_FIELD_Y,in_offsetx,in_offsety,2);

	if(ex_dit_cfg_info.deint_1b_mode == 1)
	{
		ex_dma_cfg_block_xyz(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,(in_width+y_drop)/8,in_height/2,y_interleave,is_progressive);
		ex_dma_cfg_block_offset(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,in_offsetx,in_offsety,2);

		ex_dma_cfg_block_xyz(EX_DMA_PV_BOTTOM_FIELD_MV,(in_width+y_drop)/8,in_height/2,y_interleave,is_progressive);
		ex_dma_cfg_block_offset(EX_DMA_PV_BOTTOM_FIELD_MV,in_offsetx,in_offsety,0);
	}
	else
	{
		ex_dma_cfg_block_xyz(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,in_width+y_drop,in_height/2,y_interleave,is_progressive);
		ex_dma_cfg_block_offset(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,in_offsetx,in_offsety,2);

		ex_dma_cfg_block_xyz(EX_DMA_PV_BOTTOM_FIELD_MV,in_width+y_drop,in_height/2,y_interleave,is_progressive);
		ex_dma_cfg_block_offset(EX_DMA_PV_BOTTOM_FIELD_MV,in_offsetx,in_offsety,0);
	}
    ex_dma_cfg_block_offset(EX_DMA_PV_BOTTOM_FIELD_MV,in_offsetx,in_offsety,0);
    ex_dma_cfg_block_xyz(EX_DMA_PV_DEINT_Y,in_width+y_drop,in_height,0,1);
    ex_dma_cfg_block_offset(EX_DMA_PV_DEINT_Y,in_offsetx,in_offsety,0);

    return DEINT_TRUE;
}

int ex_dma_idle_check(void)
{
	unsigned int reg_value;
	reg_value = deint_readl((unsigned int)&(ex_dit_cfg_info.dma_ctl->fctl_cfg_03));
	if(reg_value & 0x1)
		return DEINT_TRUE;
	else
		return DEINT_FALSE;
}

int ex_deint_cfg_baseaddr(struct Ex_dit_update_info *update_info)
{

	if(update_info->mv_used == 0)
	{
		// base addr current
		ex_dma_cfg_base_addr_ex(EX_DMA_PV_TOP_FIELD_Y,ex_dit_cfg_info.y_base_addr[0], 0);
		ex_dma_cfg_cur_xyz(EX_DMA_PV_TOP_FIELD_Y,0,0,0);
		ex_dma_cfg_channel_enable(EX_DMA_PV_TOP_FIELD_Y,1);
    
		ex_dma_cfg_base_addr_ex(EX_DMA_PV_BOTTOM_FIELD_Y,ex_dit_cfg_info.y_base_addr[0], 0);
		ex_dma_cfg_cur_xyz(EX_DMA_PV_BOTTOM_FIELD_Y,0,0,0);
		ex_dma_cfg_channel_enable(EX_DMA_PV_BOTTOM_FIELD_Y,1);

		ex_dma_cfg_base_addr_ex(EX_DMA_PV_PRE_BOTTOM_FIELD_Y,ex_dit_cfg_info.y_base_addr[0], 0);
		ex_dma_cfg_cur_xyz(EX_DMA_PV_PRE_BOTTOM_FIELD_Y,0,0,0);
		ex_dma_cfg_channel_enable(EX_DMA_PV_PRE_BOTTOM_FIELD_Y,1);
    
		ex_dma_cfg_base_addr_ex(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,ex_dit_cfg_info.mv_base_addr[0], 0);
		ex_dma_cfg_cur_xyz(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,0,0,0);
		ex_dma_cfg_channel_enable(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,1);
    
		ex_dma_cfg_base_addr_ex(EX_DMA_PV_BOTTOM_FIELD_MV,ex_dit_cfg_info.mv_base_addr[0], 0);
		ex_dma_cfg_cur_xyz(EX_DMA_PV_BOTTOM_FIELD_MV,0,0,0);
		ex_dma_cfg_channel_enable(EX_DMA_PV_BOTTOM_FIELD_MV,1);
	}
	
	// base addr next
    ex_dma_cfg_base_addr_ex(EX_DMA_PV_TOP_FIELD_Y,ex_dit_cfg_info.y_base_addr[1], 1);
    ex_dma_cfg_channel_enable(EX_DMA_PV_TOP_FIELD_Y,1);
    
    ex_dma_cfg_base_addr_ex(EX_DMA_PV_BOTTOM_FIELD_Y,ex_dit_cfg_info.y_base_addr[1], 1);
    ex_dma_cfg_channel_enable(EX_DMA_PV_BOTTOM_FIELD_Y,1);

    ex_dma_cfg_base_addr_ex(EX_DMA_PV_PRE_BOTTOM_FIELD_Y,ex_dit_cfg_info.y_base_addr[0],1);
    ex_dma_cfg_channel_enable(EX_DMA_PV_PRE_BOTTOM_FIELD_Y,1);
    
    ex_dma_cfg_base_addr_ex(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,ex_dit_cfg_info.mv_base_addr[0], 1);
    ex_dma_cfg_channel_enable(EX_DMA_PV_PRE_BOTTOM_FIELD_MV,1);
    
    ex_dma_cfg_base_addr_ex(EX_DMA_PV_BOTTOM_FIELD_MV,ex_dit_cfg_info.mv_base_addr[0], 1);
    ex_dma_cfg_channel_enable(EX_DMA_PV_BOTTOM_FIELD_MV,1);

        
    ex_dma_cfg_update(EX_DEINT_CHANNEL_UPDATE);

    return DEINT_TRUE;
}

void ex_deint_start(int odd_flag)
{
    unsigned int reg_value = 0;

    if(odd_flag)
        odd_flag = 1;
    else
        odd_flag = 0;

    ex_dma_cfg_base_addr_ex(EX_DMA_PV_DEINT_Y,ex_dit_cfg_info.y_out_base_addr[odd_flag], 1);
    ex_dma_cfg_channel_enable(EX_DMA_PV_DEINT_Y,1);

    ex_dma_cfg_update(EX_Y_OUT_CHANNEL_UPDATE); 
	printk("Driver: odd_flag = %d\n", odd_flag);
    reg_value = EX_START_NEW_FRAME|(odd_flag<<2);
    deint_writel(reg_value,(unsigned int)&(ex_dit_cfg_info.dit_cfg->dit_cfg_00));
}

int ex_deint_is_done(void)
{
    unsigned int reg_value;
    reg_value = deint_readl((unsigned int)&(ex_dit_cfg_info.dit_cfg->dit_cfg_02));

    if(reg_value & 0x1)
        return DEINT_TRUE;
    else
        return DEINT_FALSE;
}

int ex_cfg_addr(unsigned long y_addr, unsigned long mv_addr, unsigned long y_out_addr, unsigned long map_addr)
{
    ex_dit_cfg_info.map_addr = map_addr;
    ex_dit_cfg_info.y_base_addr[0] = y_addr;
    ex_dit_cfg_info.y_base_addr[1] = y_addr+DEINT_Y_BUFFER;
    ex_dit_cfg_info.mv_base_addr[0] = mv_addr;
    ex_dit_cfg_info.mv_base_addr[1] = mv_addr+DEINT_MV_BUFFER;
    ex_dit_cfg_info.y_out_base_addr[0] = y_out_addr; 
    ex_dit_cfg_info.y_out_base_addr[1] = y_out_addr+DEINT_Y_OUT_BUFFER; 

    return DEINT_TRUE;
}


