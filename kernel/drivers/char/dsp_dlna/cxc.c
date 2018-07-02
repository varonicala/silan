/*
 * Mailbox reservation modules for SILAN
 *
 * Copyright (C) 2012 Silan Corporation
 * Written by: panjianguang <panjianguang@silan.com.cn>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include "mailbox.h"
#include "queue.h"
#include "task.h"
#include "dsp_ctl.h"
#include "dsp_mem.h"

#define SILAN_CXC_RUNMASK_OFFSET		4
#define SILAN_CXC_FIFO_RESET			8
#define SILAN_CXC_DSP0_FIFO_WDAT		12
#define SILAN_CXC_DSP1_FIFO_WDAT		16
#define SILAN_CXC_FIFO_STATUS			20
#define SILAN_CXC_FIFO_RDAT				24
#define SILAN_CXC_IRQ_STATUS			28

static DEFINE_SPINLOCK(read_fifo_lock); 
static DEFINE_MUTEX(create_fifo_read_lock);

u32 mbox_base;

static inline u32 mbox_read_reg(unsigned int reg)
{
	return __raw_readl((void __iomem *)(mbox_base + reg));
}

static inline void mbox_write_reg(unsigned int val, unsigned int reg)
{
	__raw_writel(val, (void __iomem *)(mbox_base + reg));
}

/* Mailbox H/W preparations */
static int silan_mbox_startup(struct silan_mbox *mbox)
{
	return 0;
}

static int silan_mbox_shutdown(struct silan_mbox *mbox)
{
	return 0;
}

static int silan_mbox_create_fifo_read(struct silan_mbox *mbox,struct dspdev_filter *dspfilter,SILAN_DSPNAME dspname)
{
	return 0;
}

/* Mailbox FIFO handle functions */
static int silan_mbox_fifo_read(struct silan_mbox *mbox,SILAN_DSPNAME dspname,mbox_msg_t *msg,int task_num)
{
	struct mbcmd_st cmdst ;
	u32 *cmdst_ptr = (u32 *)&cmdst;
	u32 size = sizeof(struct mbcmd_st)/4;
	u32 i = 0;
	spin_lock(&read_fifo_lock);
	if(!mbox_read_reg(CXC_MUTEX(task_num)))
	{
		while(i < size)
		{
			*(cmdst_ptr+i) = mbox_read_reg(CXC_MAIL_BOX(MAILBOX_LEN_PER_TASK*task_num+i));
			i++;
		}
		msg->cmdst = cmdst;	
        
        mbox_write_reg(0,CXC_MUTEX(task_num));
		spin_unlock(&read_fifo_lock);
		return 0;
	}
	spin_unlock(&read_fifo_lock);
	return -1;
}

static int silan_mbox_fifo_read_empty(struct silan_mbox *mbox,SILAN_DSPNAME dspname,int task_num)
{
	int status = 0;

	spin_lock(&read_fifo_lock);
	if(!mbox_read_reg(CXC_MUTEX(task_num)))
	{
		status =mbox_read_reg(CXC_MAIL_BOX(MAILBOX_LEN_PER_TASK*task_num+6));
		mbox_write_reg(0,CXC_MUTEX(task_num));
	
	}
	if(status & DSP2MIPS_FIFO_FULL)
	{
		spin_unlock(&read_fifo_lock);
		return 0;
	}
	spin_unlock(&read_fifo_lock);

	return 1;
}

static int silan_mbox_fifo_read_full(struct silan_mbox *mbox,SILAN_DSPNAME dspname)
{
	struct silan_mbox_priv *priv = (struct silan_mbox_priv*)mbox->priv;
	struct mbcmd_st *cmdst = (struct mbcmd_st*)priv;
	if(cmdst->flags&DSP2MIPS_FIFO_FULL)
		return 1;
	return 0;
}

static int cxc_send_fifo_data(SILAN_DSPNAME dspname,u32 data,int id)
{
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	switch(dspname)
	{
		case SILAN_DSP0:
			mbox_write_reg(data,CXC_MAIL_BOX(id));
			break;
		case SILAN_DSP1:
			mbox_write_reg(data,CXC_MAIL_BOX(id));
			break;
		default:
			break;
	}

	return 0;
}

static int silan_mbox_fifo_write(struct silan_mbox *mbox,SILAN_DSPNAME dspname, mbox_msg_t *msg)
{
	int i,ret;
	u32 *pdata = (u32*)msg->cmd;
	u32 size = sizeof(struct mbcmd);
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;
	
	if(!mbox_read_reg(CXC_MUTEX(0)))
	{
		for(i = 0; i < size/4; i++)
		{	
			ret = cxc_send_fifo_data(dspname,*(pdata+i),i);
			if(ret<0)
			{
				printk("send cmd error \n");
				return -1;
			}
		}
	}
	else
	{
		printk("the fifo is locked \n");	
		return -1;
	}

	mbox_write_reg(0,CXC_MUTEX(0));
	return 0;
}


int silan_cxc_set_idx(int idx, int offset)
{
    if ((offset < CXC_DLNA_SET_END_FLAG) || (CXC_DLNA_OUT_WIDX < offset))
        return -1;

    mbox_write_reg(idx,CXC_MAIL_BOX(offset));

    return 0;
}

int silan_cxc_get_idx(int *pidx, int offset)
{
    if (NULL == pidx)
        return -1;

    if ((offset < CXC_DLNA_SET_END_FLAG) || (CXC_DLNA_OUT_WIDX < offset))
        return -1;

    *pidx = mbox_read_reg(CXC_MAIL_BOX(offset));

    return 0;
}

static int silan_mbox_fifo_write_empty(struct silan_mbox *mbox,SILAN_DSPNAME dspname)
{
	return 0;
}

static int silan_mbox_fifo_write_full(struct silan_mbox *mbox,SILAN_DSPNAME dspname)
{
	return 0;
}

/* Mailbox IRQ handle functions */
static SILAN_DSPNAME silan_mbox_irq_status(struct silan_mbox *mbox)
{
	return  SILAN_DSP0;
}

static void silan_mbox_irq_clr(struct silan_mbox *mbox)
{
	mbox_write_reg(0xffffffff,CXC_HOST_INT_CLR);
}

int cxc_set_mips2dsp_int(void)
{
	mbox_write_reg(1,CXC_DSP_INT_MASK);	
	mbox_write_reg(1,CXC_DSP_INT_SET);	

	return 0;
}

int silan_cxc_reg_init(void)
{
	mbox_write_reg(1,CXC_DSP_INT_MASK);	
	mbox_write_reg(0xffffffff,CXC_HOST_INT_CLR);
	mbox_write_reg(0xffffffff,CXC_DSP_INT_CLR);

	return 0;
}

static struct silan_mbox_ops silan_mbox_ops = 
{
	.startup	= silan_mbox_startup,
	.shutdown	= silan_mbox_shutdown,
	.create_fifo_read = silan_mbox_create_fifo_read,
	.fifo_read	= silan_mbox_fifo_read,
	.fifo_read_empty = silan_mbox_fifo_read_empty,
	.fifo_read_full	= silan_mbox_fifo_read_full,

	.fifo_write	= silan_mbox_fifo_write,
	.fifo_write_empty = silan_mbox_fifo_write_empty,
	.fifo_write_full = silan_mbox_fifo_write_full,

	.irq_status	= silan_mbox_irq_status,
	.irq_clr = silan_mbox_irq_clr,
};

/* DSP private */
static struct silan_mbox_priv silan_mbox_dsp_priv = 
{

};

static struct silan_mbox mbox_dsp_info = {
	.name	= "silan-mailbox",
	.ops	= &silan_mbox_ops,
	.priv	= &silan_mbox_dsp_priv,
};

struct silan_mbox* silan_mbox_get(const char *name)
{
	if(strcmp(mbox_dsp_info.name,name) != 0)
		return NULL;
	return &mbox_dsp_info;
}

int silan_mbox_put(const char *name)
{
	if(strcmp(mbox_dsp_info.name,name) != 0)
		return -ENODEV;
	silan_mbox_fini(&mbox_dsp_info);
	return 0;
}

int silan_mbox_probe(struct platform_device *pdev)
{
	struct resource *res;
	res = platform_get_resource(pdev,IORESOURCE_MEM,1);
	if(res == NULL)
	{
		printk("can't find CXC resource\n");
		return - ENOENT;
	}	

	mbox_base =(u32)ioremap(res->start,(res->end - res->start)+1);
	if(mbox_base == (u32)NULL)
	{
		printk("cannot map CXC io\n");
		return -ENXIO;
	}
	
	mbox_dsp_info.irq = platform_get_irq(pdev,0);							//get irq
	if(mbox_dsp_info.irq ==  -ENXIO)
	{
		printk("cannot find IRQ\n");
		return -ENXIO;
	}
    printk("[DSP] IRQ NUM: %d\n", mbox_dsp_info.irq);

	return 0;
}

