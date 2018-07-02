#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include "task.h"
#include "mailbox.h"
#include "dsp_mbcmd.h"
#include "dsp_ctl.h"
#include "dsp_mem.h"
#include "dsp_io.h"
#include "dsp_core.h"
#include "aud_pro.h"
#include "ringbuffer.h"
#include "silan_memory.h"

#define DSPTASK_MAX		256

static DECLARE_WAIT_QUEUE_HEAD(cfg_wait_q);
static u32 dlna_user_handle = 0;
static struct dsptask *dsptask[DSPTASK_MAX];
static u8 cfg_id = TASK_ID_ANON;

extern int dsp_mbcmd_send_and_wait(mbox_msg_t *msg,wait_queue_head_t *q);
extern DSP_CTL_INFO dsp_info;
extern dspdev_info_t dspdev_info;
extern int dsp_mbcmd_send(mbox_msg_t *msg);
extern int __mbox_msg_send(struct silan_mbox *mbox, mbox_msg_t *msg);

extern struct silan_mbox *g_mbox;
#ifdef CONFIG_MIPS_SILAN_DLNA
struct silan_mbox_priv mbox_priv_backup = {0};
extern dma_addr_t dsp_dlna_ring_buffer_dma;
#endif

void mbox_tadd(struct mbcmd_st *cmdst)
{
	struct dsptask *task = NULL;
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected TADD from DSP! \n");
		return;
	}
    task = kzalloc(sizeof(struct dsptask),GFP_KERNEL);
	if(task == NULL)
	{
		printk(KERN_WARNING "mbox: unexpected TADD mem zalloc! \n");
		return;
	}
	cfg_id = id;
	task->tid = id;
	task->state = DSP_ST_ALLOC;
	task->cmdst.errno = cmdst->errno;
	task->cmdst.taskid = id;
	dsptask[id] = task;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_tdel(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected TDEL from DSP! \n");
		return;
	}
	if(dsptask[id] == NULL)
	{
		printk(KERN_WARNING "mbox: unexpected TDEL task free! \n");
		return;
	}

	if(cmdst->errno)
		printk(KERN_WARNING "mbox: unexpected TDEL repond error! \n");

	kfree(dsptask[id]);
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_tcfg(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected TCFG from DSP! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	dsptask[id]->state = DSP_ST_CFG|DSP_ST_ALLOC;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_tstop(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected TSTOP from DSP! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	dsptask[id]->state |= DSP_ST_STOP|DSP_ST_ALLOC;
	wake_up_interruptible(&cfg_wait_q);

}

void mbox_send(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;

	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected send from DSP! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	dsptask[id]->cmdst.u.io_info.used_size = cmdst->u.io_info.used_size;
	dsptask[id]->cmdst.u.io_info.out_pcm_size = cmdst->u.io_info.out_pcm_size;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_request(struct mbcmd_st *cmdst)
{

}

void mbox_send_async(struct mbcmd_st *cmdst)
{
	struct dspdev_filter* dspfilter;
	mutex_lock(&dspdev_info.mutex);
	if(!list_empty(&dspdev_info.queue))
	{
		dspfilter = list_first_entry(&dspdev_info.queue,struct dspdev_filter,queue);
		if(cmdst->errno)
			dspfilter->wbuffer.pread = dspfilter->wbuffer.pwrite = 0;
		else
			dsp_increase_ringbuf_rptr(&dspfilter->wbuffer,cmdst->u.io_info.used_size);

		dsp_rbuffer_update(&dspfilter->rbuffer,cmdst);
		if(dspfilter->update)
		{
			dsp_send_cmd_async(dspfilter,&dspfilter->wbuffer);
			dspfilter->update = 0;
		}
		else
		{
			list_del(&dspfilter->queue);
			if(!list_empty(&dspdev_info.queue))
			{
				dspfilter = list_first_entry(&dspdev_info.queue,struct dspdev_filter,queue);
				dsp_send_cmd_async(dspfilter,&dspfilter->wbuffer);
				dspfilter->update = 0;
			}
		}
	}
	mutex_unlock(&dspdev_info.mutex);
}

void mbox_set_pp(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected TSTOP from DSP! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	dsptask[id]->state |= DSP_ST_STOP|DSP_ST_ALLOC;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_codec_type(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected codec type from DSP! \n");
		return;
	}
	if(dsptask[id] == NULL)
	{
		printk(KERN_WARNING "mbox: unexpected codec type free! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_codec_cfg(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected codec type from DSP! \n");
		return;
	}
	if(dsptask[id] == NULL)
	{
		printk(KERN_WARNING "mbox: unexpected codec type free! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_codec_start(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected start from DSP! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_codec_stop(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected start from DSP! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_codec_pause(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q))&&(id<DSPTASK_MAX))
	{
		printk(KERN_WARNING "mbox: unexpected start from DSP! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_codec_info(struct mbcmd_st *cmdst)
{
	u32 id = cmdst->taskid;
	if((!waitqueue_active(&cfg_wait_q)))
	{
		printk(KERN_WARNING "mbox: unexpected start from DSP! \n");
		return;
	}
	dsptask[id]->cmdst.errno = cmdst->errno;
	dsptask[id]->cmdst.u.au_info.channels = cmdst->u.au_info.channels;
	dsptask[id]->cmdst.u.au_info.rate = cmdst->u.au_info.rate;
	dsptask[id]->cmdst.u.au_info.bitpersample = cmdst->u.au_info.bitpersample;
	dsptask[id]->cmdst.framelen = cmdst->framelen;
	wake_up_interruptible(&cfg_wait_q);
}


void mbox_system_attr(struct mbcmd_st *cmdst)
{
	if((!waitqueue_active(&cfg_wait_q)))
	{
		printk(KERN_WARNING "mbox: unexpected start from DSP! \n");
		return;
	}
	wake_up_interruptible(&cfg_wait_q);
}

void mbox_runlevel(struct mbcmd_st *cmdst)
{

}

void mbox_debug(struct mbcmd_st *cmdst)
{

}

void mbox_poll(struct mbcmd_st *cmdst)
{

}

void mbox_suspend(struct mbcmd_st *cmdst)
{

}

void mbox_resume(struct mbcmd_st *cmdst)
{

}

void mbox_common_rcv(struct mbcmd_st *cmdst)
{
	if((!waitqueue_active(&cfg_wait_q)))
	{
		printk(KERN_WARNING "mbox: unexpected start from DSP! \n");
		return;
	}
	wake_up_interruptible(&cfg_wait_q);
}

static int dsp_task_add(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	int ret = 0;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	if(cfg_id != TASK_ID_ANON)
	{
		cmd->taskid= cfg_id;
		cfg_id = TASK_ID_ANON;
	}
	else
		ret = -EAGAIN;

	return 0;
}

static int dsp_task_del(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	msg.cmd = cmd;
	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	return 0;
}

static int dsp_task_bksend(struct mbcmd *cmd)
{
	int ret = 0;
	mbox_msg_t msg;
	msg.cmd = cmd;
	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	if(dsptask[cmd->taskid]->cmdst.errno)
		ret = -EAGAIN;
	cmd->u.io.used_size = dsptask[cmd->taskid]->cmdst.u.io_info.used_size;
	cmd->u.io.out_pcm_size = dsptask[cmd->taskid]->cmdst.u.io_info.out_pcm_size;
	return ret;
}

static int dsp_set_codec_type(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	int ret = 0;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	if(dsptask[cmd->taskid]->cmdst.errno)
		ret = -EAGAIN;
	return ret;
}

static int dsp_set_codec_cfg(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	int ret = 0;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	if(dsptask[cmd->taskid]->cmdst.errno)
		ret = -EAGAIN;
	return 0;
}

static int dsp_set_codec_start(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	int ret = 0;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	if(dsptask[cmd->taskid]->cmdst.errno)
		ret = -EAGAIN;
	return 0;
}

static int dsp_set_codec_stop(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	int ret = 0;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	if(dsptask[cmd->taskid]->cmdst.errno)
		ret = -EAGAIN;
	return 0;
}

static int dsp_set_codec_pause(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	int ret = 0;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	if(dsptask[cmd->taskid]->cmdst.errno)
		ret = -EAGAIN;
	return 0;
}

#ifdef CONFIG_MIPS_SILAN_DLNA
int dsp_dlna_ring_init(int buff_size)
{
    static char init_flag = 0;
    if (0 >= buff_size)
    {
        printk("Wrong size, %d.\n", buff_size);
        return -1;
    }

    dsp_info.dlna_ring_buff_size = buff_size;
#if 1
	if (0 == init_flag)
	{
		dsp_info.dsp_dlna_ring_buffer_dma = (dma_addr_t)prom_phy_mem_malloc(dsp_info.dlna_ring_buff_size, 0);
		if(unlikely(0 == dsp_info.dsp_dlna_ring_buffer_dma))
		{
			printk("dsp mem malloc failed\n");
			return -ENOMEM;
		}

		dsp_info.dsp_dlna_ring_buffer_cpu = (u8 *)ioremap_nocache(dsp_info.dsp_dlna_ring_buffer_dma,dsp_info.dlna_ring_buff_size);
		if(unlikely(NULL == dsp_info.dsp_dlna_ring_buffer_cpu))
		{
			printk("dsp mem ioremap failed\n");
			return -ENOMEM;
		}
		init_flag = 1;

	}
#else
	dsp_info.dsp_dlna_ring_buffer_cpu = (unsigned char*)dma_alloc_coherent(dsp_info.dev,dsp_info.dlna_ring_buff_size,(dma_addr_t*)&dsp_info.dsp_dlna_ring_buffer_dma,GFP_KERNEL);
#endif
	if(NULL == dsp_info.dsp_dlna_ring_buffer_cpu)
	{
		printk("dlna dma malloc failed!");
		return -ENOMEM;
	}
    memset((void *)dsp_info.dsp_dlna_ring_buffer_cpu, 0, dsp_info.dlna_ring_buff_size);

	return 0;
}

static int dsp_dlna_ring_deinit(void)
{
    return 0;

    if (dsp_info.dsp_dlna_ring_buffer_cpu)
        dma_free_coherent(dsp_info.dev,dsp_info.dlna_ring_buff_size, dsp_info.dsp_dlna_ring_buffer_cpu, dsp_info.dsp_dlna_ring_buffer_dma);
	return 0;
}

static int dsp_set_dlna_ring_buff_cfg(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	int ret = 0;
	msg.cmd = cmd;

    //first set share buff attr
    if (0 == cmd->u.dlna_ring_buff_cfg.phy_base_addr) // MMAP
    {
        if (0 != dsp_dlna_ring_init(cmd->u.dlna_ring_buff_cfg.size))
            return -EFAULT;

    	dsp_set_area1_attr(SILAN_DSP0, dsp_info.dsp_dlna_ring_buffer_dma, 
    					   cmd->u.dlna_ring_buff_cfg.size);
    }
    else // avmem
    	dsp_set_area1_attr(SILAN_DSP0, (u32)cmd->u.dlna_ring_buff_cfg.phy_base_addr, 
    					   cmd->u.dlna_ring_buff_cfg.size);

	{
		extern struct silan_mbox *g_mbox;
        if (0 == cmd->u.dlna_ring_buff_cfg.phy_base_addr)
		    ((struct silan_mbox_priv*)g_mbox->priv)->dsp2mipsAddr = dsp_info.dsp_dlna_ring_buffer_cpu;
        else
		    ((struct silan_mbox_priv*)g_mbox->priv)->dsp2mipsAddr = cmd->u.dlna_ring_buff_cfg.vir_base_addr;

        ((struct silan_mbox_priv*)g_mbox->priv)->size = cmd->u.dlna_ring_buff_cfg.sys_exchange_size;
	}

	cmd->u.dlna_ring_buff_cfg.vir_base_addr = (u8 *)DSP_MMAP_AREA1_OFFSET;
    printk("[MIPS] Ring buff info\n"
           "       phy_base_addr      : 0x%x\n"
           "       size               : %d\n"
           "       sys_exchange_size  : %d\n"
           "       index_size         : %d\n"
           "       in_buff_size       : %d\n"
           "       out_buff_frame_num : %d\n"
           "       out_buff_frame_size: %d\n\n",
           dsp_info.dsp_dlna_ring_buffer_dma,//cmd->u.dlna_ring_buff_cfg.phy_base_addr,
           cmd->u.dlna_ring_buff_cfg.size,
           cmd->u.dlna_ring_buff_cfg.sys_exchange_size,
           cmd->u.dlna_ring_buff_cfg.index_size,
           cmd->u.dlna_ring_buff_cfg.in_buff_size,
           cmd->u.dlna_ring_buff_cfg.out_buff_frame_num,
           cmd->u.dlna_ring_buff_cfg.out_buff_frame_size);

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	printk("[MIPS] cfg dsp ok./n");
	return ret;	
}

#endif

static int dsp_common_send(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	int ret = 0;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	return ret;
}

int dsp_set_suspend(void)
{
	struct mbcmd mbcmd;
	mbox_msg_t msg;

	mbcmd.sync = DSP_SYNC_CODE;
	mbcmd.seq = MIPS_TO_DSP;
	mbcmd.cmd = MBOX_DSP_SUSPEND;

	msg.cmd = &mbcmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);

	return 0;
}

int dsp_set_resume(void)
{
	struct mbcmd mbcmd;
	mbox_msg_t msg;

	mbcmd.sync = DSP_SYNC_CODE;
	mbcmd.seq = MIPS_TO_DSP;
	mbcmd.cmd = MBOX_DSP_RESUME;

	msg.cmd = &mbcmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);

	return 0;
}

int dsp_set_system_addr(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);

	return 0;	
}

int dsp_get_codec_info(struct mbcmd *cmd)
{
	mbox_msg_t msg;
	msg.cmd = cmd;

	dsp_mbcmd_send_and_wait(&msg,&cfg_wait_q);
	
	return 0;	
}

void dsp_send_cmd_async(struct dspdev_filter *dspfilter,struct dsp_ringbuffer *rbuf)
{
	struct mbcmd mbcmd;
	mbox_msg_t msg;
	int packet_size;

	packet_size = dsp_get_packet_len(rbuf,dspfilter->frame_sync);
	if(packet_size == 0)
		return ;

	mbcmd.sync = DSP_SYNC_CODE;
	mbcmd.seq = MIPS_TO_DSP;
	mbcmd.cmd = MBOX_CMD_BKSEND_ASYNC;
	mbcmd.taskid = dspfilter->taskid;

	mbcmd.u.io.in_buf.data = DSP_MMAP_AREA1_OFFSET+dspfilter->wbuffer.data-dspfilter->bufstart;
	mbcmd.u.io.in_buf.addr = mbcmd.u.io.in_buf.data +dspfilter->wbuffer.pread;
	mbcmd.u.io.in_buf.size = DSP_W_RINGBUFFER_SIZE;
	mbcmd.u.io.in_size = packet_size;

	mbcmd.u.io.out_buf.data = DSP_MMAP_AREA1_OFFSET+dspfilter->rbuffer.data-dspfilter->bufstart;
	mbcmd.u.io.out_buf.addr = mbcmd.u.io.out_buf.data+dspfilter->rbuffer.pwrite;
	mbcmd.u.io.out_buf.size = DSP_R_RINGBUFFER_SIZE;

	mbcmd.u.io.out_size = MIPS_DSP_OUT_SIZE;

	msg.cmd = &mbcmd;
	__mbox_msg_send(g_mbox,&msg);

}

void dsp_rbuffer_update(struct dsp_ringbuffer *wbuf,struct mbcmd_st *cmdst)
{
	if(cmdst->u.io_info.out_pcm_size != 0)
	{
		dsp_increase_ringbuf_wptr(wbuf,cmdst->u.io_info.out_pcm_size);
	}
	else
	{
		wbuf->error = 1;
	}
	wake_up(&wbuf->queue);
}

static int silan_dsp_close(struct file *fp)
{
	struct dspdev_filter *dspfilter = fp->private_data;


     //if (dspdev_info.users < 1)
      // return 0;

	dspfilter->state = DSPDEV_STATE_FREE;
	fp->private_data = NULL;
	//dspdev_info.users--;

	return 0;
}

long silan_task_ioctl(struct file *fp,unsigned int cmd,unsigned long argp)
{
	long ret = 0;
	dma_addr_t dsp_in_addr,dsp_out_addr;
	u8 *dsp_input_addr;
	u8 *dsp_output_addr;
	struct mbcmd mbcmd;
	TASKID taskID;
	BKIO_TYPE bkio;
	DEC_EXT	dec_ext;
	CODEC_PP PostProcess;
	CODEC_TYPE codecType;
	CODEC_START codecStart;
	CODEC_STOP codecStop;
	CODEC_PAUSE codecPause;
	CODEC_CFG	codecCfg;
	CODEC_INFO  codecInfo;
	int idx;
	struct dspdev_filter *dspfilter = fp->private_data;

	mbcmd.sync = DSP_SYNC_CODE;
	mbcmd.seq = MIPS_TO_DSP;
	//printk(" --- -- dsp ioctrl cmd :  %d ---- \n", cmd);
	switch(cmd)
	{
		case DSP_CMD_CLOSE:
			ret = silan_dsp_close(fp);

			break;
		case DSP_CMD_TADD:
			mbcmd.cmd = MBOX_CMD_TADD;
			ret = dsp_task_add(&mbcmd);
			if(!ret)
			{
				taskID.id = mbcmd.taskid;
				dspfilter->taskid = mbcmd.taskid;

				if (0 != mbcmd.taskid)
				{
					printk("Wrong task id, %d\n", mbcmd.taskid);
					ret = -EFAULT;
					goto ioctl_err;
				}

				if(copy_to_user((void __user*)argp,(void*)&taskID,sizeof(taskID)))
				{
					ret = -EFAULT;
					goto ioctl_err;
				}
			}
			break;
		case DSP_CMD_TDEL:
			mbcmd.cmd = MBOX_CMD_TDEL;
			if(copy_from_user((void*)&taskID,(void __user*)argp,sizeof(taskID)))
			{
					ret = -EFAULT;
					goto ioctl_err;
			}
			mbcmd.taskid = taskID.id;
			ret = dsp_task_del(&mbcmd);
			break;
		case DSP_CMD_TCFG:
			break;
		case DSP_CMD_TSTOP:
			break;
		case DSP_CMD_BKSEND:
			mbcmd.cmd = MBOX_CMD_BKSEND;
			if(copy_from_user((void*)&bkio,(void __user*)argp,sizeof(bkio)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			dsp_get_input_addr(SILAN_DSP0,&dsp_input_addr,&dsp_in_addr, bkio.taskid);
			dsp_get_output_addr(SILAN_DSP0,&dsp_output_addr,&dsp_out_addr, bkio.taskid);

			if(bkio.in_size >= MMAP_BLOCK_INPUT)
				bkio.in_size = MMAP_BLOCK_INPUT;

			mbcmd.taskid = bkio.taskid;

			mbcmd.u.io.in_buf.data = DSP_MMAP_AREA1_OFFSET+dspfilter->wbuffer.data-dspfilter->bufstart;
			mbcmd.u.io.in_buf.addr = mbcmd.u.io.in_buf.data +dspfilter->wbuffer.pread;
			mbcmd.u.io.in_buf.size = DSP_W_RINGBUFFER_SIZE;
			mbcmd.u.io.in_size = bkio.in_size;

			mbcmd.u.io.out_buf.data = DSP_MMAP_AREA1_OFFSET+dspfilter->rbuffer.data-dspfilter->bufstart;
			mbcmd.u.io.out_buf.addr = mbcmd.u.io.out_buf.data+dspfilter->rbuffer.pwrite;
			mbcmd.u.io.out_buf.size = DSP_R_RINGBUFFER_SIZE;
			mbcmd.u.io.out_size = bkio.out_size;


			ret = dsp_task_bksend(&mbcmd);
			if(!ret)
			{
				bkio.used_size =mbcmd.u.io.used_size;
				bkio.out_pcm_size =mbcmd.u.io.out_pcm_size;

				if(copy_to_user((void __user*)argp,(void*)&bkio,sizeof(bkio)))
				{
					ret = -EFAULT;
					goto ioctl_err;
				}
			}
			break;
		case DSP_CMD_BKREQ:
			break;
		case DSP_CMD_BKSEND_ASYNC:
			if(copy_from_user((void*)&dec_ext,(void __user*)argp,sizeof(dec_ext)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			dspfilter->frame_sync = dec_ext.flag;
			break;
		case DSP_CMD_PP:
			mbcmd.cmd = MBOX_CMD_PP;
			if(copy_from_user((void*)&PostProcess,(void __user*)argp,sizeof(PostProcess)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			mbcmd.taskid = PostProcess.taskid;
			mbcmd.u.pp = PostProcess.pp;
			ret = dsp_task_bksend(&mbcmd);
			break;
		case DSP_CMD_FLUSH_BUF:
			dspfilter->wbuffer.pread = dspfilter->wbuffer.pwrite = 0;
			dspfilter->rbuffer.pread = dspfilter->rbuffer.pwrite = 0;
			break;
		case DSP_CMD_CODEC_TYPE:
			mbcmd.cmd = MBOX_CMD_CODEC_TYPE;
			if(copy_from_user((void*)&codecType,(void __user*)argp,sizeof(codecType)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			mbcmd.taskid = codecType.id;
			mbcmd.u.codec = codecType.codec;
			dspfilter->type = codecType.codec;
			ret = dsp_set_codec_type(&mbcmd);
			break;
		case DSP_CMD_CODEC_CFG:
			mbcmd.cmd = MBOX_CMD_CODEC_CFG;
			//dsp_get_input_addr(SILAN_DSP0,&dsp_input_addr,&dsp_in_addr);
			if(copy_from_user((void*)&codecCfg,(void __user*)argp,sizeof(codecCfg)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			mbcmd.taskid = codecCfg.taskid;
			dsp_get_input_addr(SILAN_DSP0,&dsp_input_addr,&dsp_in_addr, codecCfg.taskid);
			if(codecCfg.codec_type == DSP_CODEC_PCM)
			{
				mbcmd.u.cfg.para.pcm_cfg = codecCfg.u.pcm_cfg;
			}
			if(codecCfg.codec_type == DSP_CODEC_WMA )
			{
				mbcmd.u.cfg.para.wma_cfg = codecCfg.u.wma_cfg;
			}
			if(codecCfg.codec_type == DSP_CODEC_FLAC)
			{
				mbcmd.u.cfg.para.flac_cfg = codecCfg.u.flac_cfg;
			}
			if(codecCfg.codec_type == DSP_CODEC_COOK)
			{
				mbcmd.u.cfg.para.cook_cfg = codecCfg.u.cook_cfg;
			}
			if(codecCfg.codec_type == DSP_CODEC_VTK)
			{
				mbcmd.u.cfg.para.vtk_cfg = codecCfg.u.vtk_cfg;
			}
			if(codecCfg.codec_type == DSP_CODEC_HOT_FDIP)
			{
				mbcmd.u.cfg.para.fdif_cfg = codecCfg.u.fdif_cfg;
			}
			mbcmd.u.cfg.codec_type = codecCfg.codec_type;
			ret = dsp_set_codec_cfg(&mbcmd);
			break;
		case DSP_CMD_CODEC_START:
			mbcmd.cmd = MBOX_CMD_CODEC_START;
			if(copy_from_user((void*)&codecStart,(void __user*)argp,sizeof(codecStart)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			mbcmd.taskid = codecStart.id;
			ret = dsp_set_codec_start(&mbcmd);
			break;
		case DSP_CMD_CODEC_STOP:
			mbcmd.cmd = MBOX_CMD_CODEC_STOP;
			if(copy_from_user((void*)&codecStop,(void __user*)argp,sizeof(codecStop)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			mbcmd.taskid = codecStop.id;
			ret = dsp_set_codec_stop(&mbcmd);
			break;
		case DSP_CMD_CODEC_PAUSE:
			mbcmd.cmd = MBOX_CMD_CODEC_PAUSE;
			if(copy_from_user((void*)&codecPause,(void __user*)argp,sizeof(codecPause)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			mbcmd.taskid = codecPause.id;
			ret = dsp_set_codec_pause(&mbcmd);
			break;
		case DSP_CMD_CODEC_INFO:
			mbcmd.cmd = MBOX_CMD_CODEC_INFO;
			if(copy_from_user((void*)&codecInfo,(void __user*)argp,sizeof(codecInfo)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			mbcmd.taskid = codecInfo.id;
			ret = dsp_get_codec_info(&mbcmd);
			if(!ret)
			{
				codecInfo.channels = dsptask[mbcmd.taskid]->cmdst.u.au_info.channels;
				codecInfo.rate = dsptask[mbcmd.taskid]->cmdst.u.au_info.rate;
				codecInfo.bitpersample = dsptask[mbcmd.taskid]->cmdst.u.au_info.bitpersample;
				codecInfo.framelen = dsptask[mbcmd.taskid]->cmdst.framelen;
				if(copy_to_user((void __user*)argp,(void*)&codecInfo,sizeof(codecInfo)))
				{
					ret = -EFAULT;
					goto ioctl_err;
				}
			}
			break;
		case DSP_CMD_SYSTEM_ATTR:
			break;
		case DSP_CMD_RUNLEVEL:
			break;
		case DSP_CMD_DEBUG:
			break;
		case DSP_CMD_POLL:
			break;
#ifdef CONFIG_MIPS_SILAN_DLNA
		case DSP_CMD_DLNA_RING_BUFF_CFG:
			mbcmd.cmd = MBOX_DSP_DLNA_RING_BUFF_CFG;
			if(copy_from_user((void*)&mbcmd.u.dlna_ring_buff_cfg,(void __user*)argp,sizeof(mbcmd.u.dlna_ring_buff_cfg)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			ret = dsp_set_dlna_ring_buff_cfg(&mbcmd);

			break;
        case DSP_CMD_DLNA_SET_SEEK_FLAG:
            mbcmd.cmd = MBOX_DSP_DLNA_SET_SEEK_FLAG;
			ret = dsp_common_send(&mbcmd);

            break;
        case DSP_CMD_DLNA_SET_FILE_SIZE:
			mbcmd.cmd = MBOX_DSP_DLNA_SET_FILE_SIZE;
			if(copy_from_user((void*)&mbcmd.u.dlna_file_size.file_size,(void __user*)argp,sizeof(mbcmd.u.dlna_file_size)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			ret = dsp_common_send(&mbcmd);

			break;
        case DSP_CMD_DLNA_RING_BUFF_RESET:
			mbcmd.cmd = MBOX_DSP_DLNA_RING_BUFF_RESET;
		    ret = dsp_common_send(&mbcmd);

            break;
        case DSP_CMD_DLNA_DECODE_SUSPEND:
			mbcmd.cmd = MBOX_DSP_DLNA_DECODE_SUSPEND;
		    ret = dsp_common_send(&mbcmd);

            break;
        case DSP_CMD_DLNA_DECODE_STOP:
			mbcmd.cmd = MBOX_DSP_DLNA_DECODE_STOP;
		    ret = dsp_common_send(&mbcmd);

            break;
        case DSP_CMD_DLNA_DECODE_START:
			mbcmd.cmd = MBOX_DSP_DLNA_DECODE_START;
		    ret = dsp_common_send(&mbcmd);

            break;
	    case DSP_CMD_DLNA_DECODE_RESUME:
			mbcmd.cmd = MBOX_DSP_DLNA_DECODE_RESUME;
		    ret = dsp_common_send(&mbcmd);

            break;
        case DSP_CMD_DLNA_RING_BUFF_FREE:
            ret = dsp_dlna_ring_deinit();

			break;
		case DSP_CMD_DLNA_SET_CUR_USER:
         	if(copy_from_user((void*)&dlna_user_handle,(void __user*)argp,sizeof(dlna_user_handle)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

            break;
         case DSP_CMD_DLNA_GET_CUR_USER:
            if(copy_to_user((void __user*)argp,(void*)&dlna_user_handle,sizeof(dlna_user_handle)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

            break;

		case DSP_CMD_DLNA_GET_IN_RIDX_INFO:
			if (0 != silan_cxc_get_idx(&idx, CXC_DLNA_IN_RIDX))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if(copy_to_user((void __user*)argp,(void*)&idx,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			break;
		case DSP_CMD_DLNA_GET_IN_WIDX_INFO:
			if (0 != silan_cxc_get_idx(&idx, CXC_DLNA_IN_WIDX))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if(copy_to_user((void __user*)argp,(void*)&idx,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			break;
		case DSP_CMD_DLNA_GET_OUT_RIDX_INFO:
			if (0 != silan_cxc_get_idx(&idx, CXC_DLNA_OUT_RIDX))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if(copy_to_user((void __user*)argp,(void*)&idx,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			break;
		case DSP_CMD_DLNA_GET_OUT_WIDX_INFO:
			if (0 != silan_cxc_get_idx(&idx, CXC_DLNA_OUT_WIDX))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if(copy_to_user((void __user*)argp,(void*)&idx,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			break;
		case DSP_CMD_DLNA_SET_IN_RIDX_INFO:
			if(copy_from_user((void*)&idx,(void __user*)argp,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if (0 != silan_cxc_set_idx(idx, CXC_DLNA_IN_RIDX))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			break;
		case DSP_CMD_DLNA_SET_IN_WIDX_INFO:
			if(copy_from_user((void*)&idx,(void __user*)argp,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if (0 != silan_cxc_set_idx(idx, CXC_DLNA_IN_WIDX))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			break;
		case DSP_CMD_DLNA_SET_OUT_RIDX_INFO:
			if(copy_from_user((void*)&idx,(void __user*)argp,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if (0 != silan_cxc_set_idx(idx, CXC_DLNA_OUT_RIDX))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			break;
		case DSP_CMD_DLNA_SET_OUT_WIDX_INFO:
			if(copy_from_user((void*)&idx,(void __user*)argp,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if (0 != silan_cxc_set_idx(idx, CXC_DLNA_OUT_WIDX))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			break;
        case DSP_CMD_DLNA_SET_END_FLAG:
			if (0 != silan_cxc_set_idx(1, CXC_DLNA_SET_END_FLAG))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
            break;
        case DSP_CMD_DLNA_GET_DECODE_STATE:
			if (0 != silan_cxc_get_idx(&idx, CXC_DLNA_GET_DECODE_STATE))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}

			if(copy_to_user((void __user*)argp,(void*)&idx,sizeof(idx)))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
			break;
        case DSP_CMD_DLNA_CLEAR_DECODE_STATE:
			if (0 != silan_cxc_set_idx(0, CXC_DLNA_GET_DECODE_STATE))
			{
				ret = -EFAULT;
				goto ioctl_err;
			}
            break;
#endif
        case DSP_CMD_PRINT_STAT_ENABLE:
			mbcmd.cmd = MBOX_DSP_PRINT_STAT_ENABLE;
		    ret = dsp_common_send(&mbcmd);
            break;
        case DSP_CMD_PRINT_STAT_DISABLE:
			mbcmd.cmd = MBOX_DSP_PRINT_STAT_DISABLE;
		    ret = dsp_common_send(&mbcmd);

            break;
		default:
		    printk("default case, %d \n", cmd);
			break;
	}
ioctl_err:
	return ret;
}

