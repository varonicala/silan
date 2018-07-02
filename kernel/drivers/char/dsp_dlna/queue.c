/*
 * Copyright (C) 2012 Silan Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/blkdev.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include "mailbox.h"
#include "dsp_ctl.h"
#include "dsp_core.h"
#include "task.h"

static mbox_msg_t msg_tmp;
extern dspdev_info_t dspdev_info;

int __mbox_msg_send(struct silan_mbox *mbox, mbox_msg_t *msg)
{
	int ret = 0, i = 1000;
	while (mbox_fifo_write_full(mbox,SILAN_DSP0)) 
	{
		if (--i == 0)
			return -1;
		udelay(1);
	}

	if(mbox->txq->callback)
		ret = mbox->txq->callback((void*)msg);
	mbox_fifo_write(mbox,SILAN_DSP0,msg);
#ifndef CONFIG_MIPS_SILAN_SUVII
	cxc_set_mips2dsp_int();	
#else
	dsp_set_mips2dsp_int(SILAN_DSP0,1);
#endif
	return ret;
}

int silan_mbox_msg_send(struct silan_mbox *mbox, mbox_msg_t *msg)
{
	unsigned long flags;

	/*return __mbox_msg_send(mbox,msg);*/

	spin_lock_irqsave(&mbox->txq->lock,flags);
	list_add_tail(&msg->list,&mbox->txq->queue);
	spin_unlock_irqrestore(&mbox->txq->lock,flags);

	schedule_work(&mbox->txq->work);
	return 0;
}

static void mbox_tx_work(struct work_struct *t)
{
	struct silan_mbox_queue *mq = container_of(t,struct silan_mbox_queue, work);
	mbox_msg_t *msg;
	unsigned long flags;
	int ret;

	while(!list_empty(&mq->queue)) 
	{
		spin_lock_irqsave(&mq->lock,flags);
		msg = list_first_entry(&mq->queue,mbox_msg_t,list);
		list_del(&msg->list);
		spin_unlock_irqrestore(&mq->lock,flags);
		ret = __mbox_msg_send(mq->mbox,msg);
		if (ret) 
			return;
	}

}
/*
 * Message receiver(workqueue)
 */
static void mbox_rx_work(struct work_struct *t)
{
	struct silan_mbox_queue *mq = container_of(t, struct silan_mbox_queue, work);
	unsigned long flags =0,i;
	struct silan_mbox *mbox = mq->mbox;
	mbox_msg_t *msg= &msg_tmp;
	SILAN_DSPNAME dspname;
	int ret = -1;

	dspname = mbox_irq_status(mbox);

	while(1)
	{
		for(i=0;i<DSP_FILTER_NUM;i++)
		{
			if(!mbox_fifo_read_empty(mbox,dspname,i))
			{
				flags  = 1;
				break;
			}
		}
		
		if (flags)
			break;
	}

	ret = mbox_fifo_read(mbox,dspname,msg,i);
	if(!ret)
	{
		spin_lock_irqsave(&mbox->rxq->lock,flags);
		list_add_tail(&msg->list,&mbox->rxq->queue);
		spin_unlock_irqrestore(&mbox->rxq->lock,flags);
	}
	else
	{
		printk("fatal err in fifo read \n");
		return;
	}
	if (mq->callback == NULL) 
	{
		sysfs_notify(&mq->mbox->dev.kobj, NULL, "silan-mbox");
		return;
	}
	
	while(!list_empty(&mq->queue)) 
	{
		spin_lock_irqsave(&mq->lock, flags);
		msg = list_first_entry(&mq->queue,mbox_msg_t,list);
		list_del(&msg->list);
		spin_unlock_irqrestore(&mq->lock, flags);
		if (!msg)
			break;

		mq->callback((void *)msg);
	}
}

#if 0
static void __mbox_rx_interrupt(struct silan_mbox *mbox)
{
	unsigned long flags =0 ;
	int ret = -1,i;
	SILAN_DSPNAME dspname = mbox_irq_status(mbox);
	mbox_msg_t *msg= &msg_tmp;

	schedule_delayed_work(&mbox->rxq->work,0);
}
#endif

static irqreturn_t mbox_interrupt(int irq, void *p)
{
	struct silan_mbox *mbox = p;
	schedule_work(&mbox->rxq->work);
	mbox_irq_clr(mbox);
	//__mbox_rx_interrupt(mbox);
	return IRQ_HANDLED;
}

static struct silan_mbox_queue *mbox_queue_alloc(struct silan_mbox *mbox,void (*work) (struct work_struct *))
{
	struct silan_mbox_queue *mq;

	mq = kzalloc(sizeof(struct silan_mbox_queue), GFP_KERNEL);
	if (!mq)
		return NULL;

	spin_lock_init(&mq->lock);
	INIT_LIST_HEAD(&mq->queue);
	INIT_WORK(&mq->work, work);
	mq->mbox = mbox;
	return mq;
}

static void mbox_queue_free(struct silan_mbox_queue *q)
{
	kfree(q);
}

int silan_mbox_init(struct silan_mbox *mbox)
{
	int ret;
	struct silan_mbox_queue *mq;

	if (likely(mbox->ops->startup)) 
	{
		ret = mbox->ops->startup(mbox);
		if (unlikely(ret))
			return ret;
	}
	ret = request_irq(mbox->irq, mbox_interrupt, 0,mbox->name, mbox);
	if (unlikely(ret))
   	{
		printk(KERN_ERR"failed to register mailbox interrupt:%d\n", ret);
		goto fail_request_irq;
	}

	mq = mbox_queue_alloc(mbox,mbox_tx_work);
	if (!mq) 
	{
		ret = -ENOMEM;
		goto fail_alloc_txq;
	}
	mbox->txq = mq;

	mq = mbox_queue_alloc(mbox,mbox_rx_work);
	if (!mq) 
	{
		ret = -ENOMEM;
		goto fail_alloc_rxq;
	}
	mbox->rxq = mq;

	return 0;

 fail_alloc_rxq:
	mbox_queue_free(mbox->txq);
 fail_alloc_txq:
	free_irq(mbox->irq, mbox);
 fail_request_irq:
	if (unlikely(mbox->ops->shutdown))
		mbox->ops->shutdown(mbox);

	return ret;
}

void silan_mbox_fini(struct silan_mbox *mbox)
{
	mbox_queue_free(mbox->txq);
	mbox_queue_free(mbox->rxq);

	free_irq(mbox->irq, mbox);

	if (unlikely(mbox->ops->shutdown))
		mbox->ops->shutdown(mbox);
}
