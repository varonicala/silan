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

static u32 mbox_base;

#ifdef CONFIG_MIPS_SILAN_DLNA
extern struct silan_mbox_priv mbox_priv_backup;
#endif

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
	unsigned int regval = mbox_read_reg(SILAN_CXC_FIFO_RESET);
	regval |= SET_DSP0_FIFO_RESET;
	regval |= SET_DSP1_FIFO_RESET;
	mbox_write_reg(regval,SILAN_CXC_FIFO_RESET);
	udelay(1);
	regval &= ~SET_DSP0_FIFO_RESET;
	regval &= ~SET_DSP1_FIFO_RESET;
	mbox_write_reg(regval,SILAN_CXC_FIFO_RESET);
	return 0;
}

static int silan_mbox_shutdown(struct silan_mbox *mbox)
{
	u32 regval = mbox_read_reg(SILAN_CXC_RUNMASK_OFFSET);
	regval |= SET_DSP0_RUNST_MASK;
	regval |= SET_DSP1_RUNST_MASK;
	mbox_write_reg(regval,SILAN_CXC_RUNMASK_OFFSET);
	return 0;
}

static int silan_mbox_create_fifo_read(struct silan_mbox *mbox,struct dspdev_filter *dspfilter,SILAN_DSPNAME dspname)
{
	struct mbcmd mbcmd;
	struct silan_mbox_priv *priv = (struct silan_mbox_priv*)mbox->priv;
	int ret = 0;
	if(mutex_lock_interruptible(&create_fifo_read_lock))
		return -EINTR;
	mbcmd.sync = DSP_SYNC_CODE;
	mbcmd.seq = MIPS_TO_DSP;
	mbcmd.cmd = MBOX_CMD_SYSTEM_ATTR;
	priv->dsp2mipsAddr = dspfilter->sysex;
	priv->size = MIPS_DSP_SYSCHANGE_SIZE;
    
	mbcmd.u.read_fifo.addr = DSP_MMAP_AREA1_OFFSET+dspfilter->sysex-dspfilter->bufstart;
	mbcmd.u.read_fifo.size = MIPS_DSP_SYSCHANGE_SIZE;
	ret = dsp_set_system_addr(&mbcmd);
	mutex_unlock(&create_fifo_read_lock);
	return ret;
}

/* Mailbox FIFO handle functions */
static int silan_mbox_fifo_read(struct silan_mbox *mbox,SILAN_DSPNAME dspname,mbox_msg_t *msg,int task_num)
{
	struct silan_mbox_priv *priv = (struct silan_mbox_priv*)mbox->priv;
	struct mbcmd_st *cmdst = (struct mbcmd_st*)(priv->dsp2mipsAddr+(MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*task_num);
	spin_lock(&read_fifo_lock);
	if(cmdst->flags&DSP2MIPS_FIFO_FULL)
	{
		memcpy((u8*)&msg->cmdst,priv->dsp2mipsAddr+(MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*task_num,sizeof(struct mbcmd_st));		
		cmdst->flags &= ~DSP2MIPS_FIFO_FULL;
		spin_unlock(&read_fifo_lock);
		return 0;
	}
	spin_unlock(&read_fifo_lock);
	return -1;
}

static int silan_mbox_fifo_read_empty(struct silan_mbox *mbox,SILAN_DSPNAME dspname,int task_num)
{
	struct silan_mbox_priv *priv = (struct silan_mbox_priv*)mbox->priv;
	struct mbcmd_st *cmdst = (struct mbcmd_st*)(priv->dsp2mipsAddr+(MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*task_num);
	//struct mbcmd_st *cmdst = (struct mbcmd_st*)priv->dsp2mipsAddr;

	spin_lock(&read_fifo_lock);
	if(cmdst->flags&DSP2MIPS_FIFO_FULL)
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

static int cxc_send_fifo_data(SILAN_DSPNAME dspname,u32 data)
{
	u32 status = mbox_read_reg(SILAN_CXC_FIFO_STATUS);
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	switch(dspname)
	{
		case SILAN_DSP0:
			if(status&DSP0_FIFO_FULL)
				return -2;
			mbox_write_reg(data,SILAN_CXC_DSP0_FIFO_WDAT);
			break;
		case SILAN_DSP1:
			if(status&DSP1_FIFO_FULL)
				return -2;
			mbox_write_reg(data,SILAN_CXC_DSP1_FIFO_WDAT);
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

	for(i = 0; i < size/4; i++)
	{	
		ret =cxc_send_fifo_data(dspname,*(pdata+i));
		if(ret<0)
		{
			printk("send cmd error \n");
			return -1;
		}
	}

	return 0;
}

static int silan_mbox_fifo_write_empty(struct silan_mbox *mbox,SILAN_DSPNAME dspname)
{
	u32 status = mbox_read_reg(SILAN_CXC_FIFO_STATUS);
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	if(dspname == SILAN_DSP0)
	{
		if(status&DSP0_FIFO_EMPTY)
			return 1;
	}
	else
	{
		if(status&DSP1_FIFO_EMPTY)
			return 1;
	}
	return 0;
}

static int silan_mbox_fifo_write_full(struct silan_mbox *mbox,SILAN_DSPNAME dspname)
{
	u32 status = mbox_read_reg(SILAN_CXC_FIFO_STATUS);
	if(dspname >= DSPNAME_END || dspname < DSPNAME_START)
		return -1;

	if(dspname == SILAN_DSP0)
	{
		if(status&DSP0_FIFO_EMPTY)
			return 0;
	}
	else
	{
		if(status&DSP1_FIFO_EMPTY)
			return 0;
	}
	return 1;
}

/* Mailbox IRQ handle functions */
static SILAN_DSPNAME silan_mbox_irq_status(struct silan_mbox *mbox)
{
	SILAN_DSPNAME ret = DSPNAME_END;
	u32 status = mbox_read_reg(SILAN_CXC_IRQ_STATUS);

	if(status&SET_DSP0_IRQ_CLR)
		ret = SILAN_DSP0;

    if(status&SET_DSP1_IRQ_CLR)
		ret = SILAN_DSP1;

	return ret;
}

static void silan_mbox_irq_clr(struct silan_mbox *mbox)
{
	u32 status = mbox_read_reg(SILAN_CXC_IRQ_STATUS);
	mbox_write_reg(status,SILAN_CXC_IRQ_STATUS);
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

	res = platform_get_resource(pdev,IORESOURCE_MEM,2);
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

