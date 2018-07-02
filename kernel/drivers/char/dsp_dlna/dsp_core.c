#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_def.h>
#include <silan_gpio.h>
#include <silan_reset.h>
#include <linux/dma-mapping.h>
#include <silan_memory.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/miscdevice.h>

#include "dsp_core.h"
#include "dsp_ctl.h"
#include "mailbox.h"
#include "queue.h"
#include "task.h"
#include "dsp_mbcmd.h"
#include "dsp_mem.h"
#include "ringbuffer.h"
#include "aud_pro.h"

#define DSP_CHAR_MAJOR		0x4b
#define DEV_DSP_NAME		"silan-dsp"
#define MAX_WAIT_TIME		500
#define DSP_MSG_TIMEOUT		(10*HZ)
//#define TEST_LOAD_FIRMWARE

static struct class *silan_dsp_class = NULL ;
struct silan_mbox *g_mbox;
//static struct silan_mbox *g_mbox;
extern DSP_CTL_INFO dsp_info; 

dspdev_info_t dspdev_info;

struct cmdinfo
{
	char *name;
	void (*handle)(struct mbcmd_st* cmdst);
};

static const struct cmdinfo dspcmd[] =
{
	{"task add",		mbox_tadd		},
	{"task del",		mbox_tdel		},
	{"task cfg",		mbox_tcfg		},
	{"tast stop",		mbox_tstop		},


	{"bk send",			mbox_send		},
	{"bk request",		mbox_request	},
	{"send async",		mbox_send_async	},	
	{"set pp",			mbox_set_pp		},

	{"codec type",		mbox_codec_type	},
	{"codec cfg",		mbox_codec_cfg	},
	{"codec start",		mbox_codec_start},
	{"codec stop",		mbox_codec_stop	},
	{"codec pause",		mbox_codec_pause},
	{"codec info ",		mbox_codec_info },

	{"system attr",		mbox_system_attr},

	{"dsp runlevel",	mbox_runlevel	},

	{"dsp debug mode",	mbox_debug		},

	{"dsp poll",		mbox_poll		},
	{"dsp suspend",		mbox_suspend	},
	{"dsp resume",		mbox_resume		},
#ifdef CONFIG_MIPS_SILAN_DLNA
	{"dsp ring buff",	mbox_common_rcv},
	{"set file size",	mbox_common_rcv},
	{"ring buff reset", mbox_common_rcv},
	{"dlna decode suspend", mbox_common_rcv},
	{"dlna decode resume", mbox_common_rcv},
	{"dlna set seek flag", mbox_common_rcv},  
	{"dlna decode stop", mbox_common_rcv},     
	{"dlna decode start", mbox_common_rcv},  
	{"dlna set aif", mbox_common_rcv},  
#endif
	{"dsp print enable", mbox_common_rcv},    
	{"dsp print disable", mbox_common_rcv},    

};

int dsp_mbcmd_send(mbox_msg_t *msg)
{
	return silan_mbox_msg_send(g_mbox,msg);
}

int dsp_mbcmd_send_and_wait(mbox_msg_t *msg,wait_queue_head_t *q)
{
	int ret;

	DEFINE_WAIT(wait);
	prepare_to_wait(q,&wait,TASK_INTERRUPTIBLE);
	ret = silan_mbox_msg_send(g_mbox,msg);
	if(ret)
	{
		printk("silan mbox msg send error \n");
		goto out;
	}
	schedule_timeout(DSP_MSG_TIMEOUT);
out:
	finish_wait(q,&wait);	
	return ret;
}

static int dsp_sender_prepare(void *msg)
{
	return 0;
}

static int dsp_receiver(void *msg)
{
	mbox_msg_t *msg_r = (mbox_msg_t*)msg;
	struct mbcmd_st *cmdst = &msg_r->cmdst;
	if(cmdst->cmd >= MBOX_CMD_MAX || cmdst->cmd < 0)
		return -1;
    
	(dspcmd[cmdst->cmd].handle)(cmdst);	
	return 0;
}

static int dsp_mbox_late_init(struct silan_mbox *mbox)
{
	mbox->txq->callback = dsp_sender_prepare;
	mbox->rxq->callback = dsp_receiver;
	return 0;
}

static int dsp_buffer_write(struct dsp_ringbuffer *buf,const u8 *src, size_t len)
{
	ssize_t free;
	if (!len)
		return 0;
	if (!buf->data)
		return 0;

	free = dsp_ringbuffer_free(buf);
	if (len > free) {
		printk("dspdev: buffer overflow(%d, %d)\n",len,free);
		return -EOVERFLOW;
	}

	return dsp_ringbuffer_write(buf, src, len);
}

static ssize_t dsp_buffer_read(struct dsp_ringbuffer *src,int non_blocking, char __user *buf,
				      size_t count, loff_t *ppos)
{
	size_t todo;
	ssize_t avail;
	ssize_t ret = 0;
	long timeout;

	if (!src->data)
		return 0;

	if (src->error) {
		ret = src->error;
		dsp_ringbuffer_flush(src);
		return ret;
	}

	timeout = msecs_to_jiffies(MAX_WAIT_TIME);
	for (todo = count; todo > 0; todo -= ret) {
		if (non_blocking && dsp_ringbuffer_empty(src)) {
			ret = -EWOULDBLOCK;
			break;
		}
		ret = wait_event_interruptible_timeout(src->queue,
					       !dsp_ringbuffer_empty(src) ||
					       (src->error != 0),timeout);
		if (ret < 0)
		{
			printk("wait error\n");
			break;
		}
		if (src->error) {
			dsp_ringbuffer_flush(src);
			return 0;
		}

		avail = dsp_ringbuffer_avail(src);
		if (avail > todo)
			avail = todo;

		ret = dsp_ringbuffer_read_user(src, buf, avail);
		if (ret < 0)
			break;

		buf += ret;
	    break;
	}
	return ret;
}

static ssize_t silan_dsp_read(struct file *file, char __user *buf, size_t count,loff_t *ppos)
{
	struct dspdev_filter *dspfilter = file->private_data;
	int ret;
	if (mutex_lock_interruptible(&dspfilter->rmutex))
		return -ERESTARTSYS;

	ret = dsp_buffer_read(&dspfilter->rbuffer,file->f_flags & O_NONBLOCK,
					     buf, count, ppos);
	mutex_unlock(&dspfilter->rmutex);
	return ret;
}

static ssize_t silan_dsp_write(struct file *file, const char __user *buf, size_t count,loff_t *ppos)
{
	struct dspdev_filter *dspfilter = file->private_data;
	struct dspdev_filter *pos;
	dspdev_info_t *dspdev_info = dspfilter->pdspdev_info;
	int ret,flag;
	if (mutex_lock_interruptible(&dspdev_info->mutex))
		return -ERESTARTSYS;
	ret = dsp_buffer_write(&dspfilter->wbuffer,buf, count);

	if(dsp_aud_valid_frame(&dspfilter->wbuffer,dspfilter->type))
	{
		dspfilter->update = dspfilter->frame_sync;
		if(list_empty(&dspdev_info->queue))
		{
			dsp_send_cmd_async(dspfilter,&dspfilter->wbuffer);	
			dspfilter->update = 0;
			list_add_tail(&dspfilter->queue,&dspdev_info->queue);
		}
		else
		{
			flag = 0;
			list_for_each_entry(pos,&dspdev_info->queue,queue)
			{
				if(pos == dspfilter)
				{
					flag = 1;
					break;
				}
			}
			if(!flag){
				list_add_tail(&dspfilter->queue,&dspdev_info->queue);
			}
		}
	}

	mutex_unlock(&dspdev_info->mutex);
	return ret;
}

static int silan_dsp_open(struct inode *inode,struct file *fp)
{
	struct dspdev_filter *dspfilter;
	int ret = 0;
   
	if(mutex_lock_interruptible(&dspdev_info.mutex))
		return -EPERM;
	
	if (dspdev_info.users > 0){
		mutex_unlock(&dspdev_info.mutex);
		return -EINTR;
	}
	
	if(dspdev_info.initialized == 0)
	{
        printk("[MIPS] load firmware start.\n");

#ifndef  TEST_LOAD_FIRMWARE	
		ret = dsp_load_firmware();
		if(ret)
		{
			mutex_unlock(&dspdev_info.mutex);
			return ret;
		}
		dsp_set_stall(SILAN_DSP0,1);
#ifdef CONFIG_MIPS_SILAN_SUVII
		silan_module_rst(SILAN_SR_DSP0);
#endif
		dsp_set_stall(SILAN_DSP0,0);
#endif
		dspdev_info.initialized = 1;

        printk("[MIPS] load firmware end.\n");
	}
#if 0 
	for (i = 0; i < dspdev_info.filternum; i++)
		if (dspdev_info.filter[i].state == DSPDEV_STATE_FREE)
			break;
    
	if (i == dspdev_info.filternum) 
	{
		mutex_unlock(&dspdev_info.mutex);
		return -EMFILE;
	}
#endif
	dspfilter = &dspdev_info.filter[0];
	dspfilter->wbuffer.pread = dspfilter->wbuffer.pwrite = 0;
	dspfilter->rbuffer.pread = dspfilter->rbuffer.pwrite = 0;
	dspfilter->wbuffer.error = dspfilter->rbuffer.error =0;
	dspdev_info.filter[0].state = DSPDEV_STATE_BUSY;
	fp->private_data = dspfilter;
	
	mbox_fifo_create_read(g_mbox,dspfilter,SILAN_DSP0);

	dspdev_info.users++;
	mutex_unlock(&dspdev_info.mutex);
	return 0;
}

static int silan_dsp_release(struct inode *inode,struct file *fp)
{
	if(dspdev_info.users > 1)
		return 0;
	
	dspdev_info.users--;
    
	return 0;
#if 0
	struct dspdev_filter *dspfilter = fp->private_data;

    if (dspdev_info.users < 1)
        return 0;
    
	if(mutex_lock_interruptible(&dspdev_info.mutex))
		return -EINTR;

	dspfilter->state = DSPDEV_STATE_FREE;
	fp->private_data = NULL;
	dspdev_info.users--;
	mutex_unlock(&dspdev_info.mutex);
	return 0;
#endif
}

static long silan_dsp_ioctl(struct file *fp,unsigned int cmd,unsigned long arg)
{
	long ret;
	struct dspdev_filter *dspfilter = fp->private_data;
	dspdev_info_t *dspdev_info = dspfilter->pdspdev_info;

	mutex_lock(&dspdev_info->mutex);
#ifdef TEST_LOAD_FIRMWARE
	if(cmd == DSP_LOAD_FIRMWARE)
	{
		ret = dsp_load_firmware();
		if(ret)
		{
			mutex_unlock(&dspdev_info->mutex);
			return ret;
		}
		dsp_set_stall(SILAN_DSP0,1);
		silan_module_rst(SILAN_SR_DSP0);
		dsp_set_stall(SILAN_DSP0,0);
		mbox_fifo_create_read(g_mbox,SILAN_DSP0);
	
		mutex_unlock(&dspdev_info->mutex);
		return 0;
	}
#endif	
	ret = silan_task_ioctl(fp,cmd,arg);

	mutex_unlock(&dspdev_info->mutex);
	return ret;
}

static int silan_dsp_mmap(struct file *fp, struct vm_area_struct *vma)
{
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    #ifdef CONFIG_MIPS_SILAN_DLNA
    	if (remap_pfn_range(vma, vma->vm_start, (dsp_info.dsp_dlna_ring_buffer_dma) >> PAGE_SHIFT, vma->vm_end-vma->vm_start, vma->vm_page_prot))
    		return -EAGAIN;
    #else
    	if (remap_pfn_range(vma, vma->vm_start, virt_to_phys(dsp_info.mmap_cpu)>>PAGE_SHIFT, vma->vm_end-vma->vm_start, vma->vm_page_prot))
    		return -EAGAIN;
    #endif
	return 0;
}

static int silan_dsp_ringbuffer_init(void)
{
	int i,size;
	dma_addr_t ring_buffer_dma;
	u_char* ring_buffer,*rbuffer,*wbuffer;

	dspdev_info.users = 0;
	dspdev_info.filternum = DSP_FILTER_NUM;
	dspdev_info.filter = kmalloc(dspdev_info.filternum*sizeof(struct dspdev_filter), GFP_KERNEL);
	if (!dspdev_info.filter)
		return -ENOMEM;

	size = (MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*dspdev_info.filternum;
	ring_buffer_dma = prom_phy_mem_malloc(size, 0);
	if(!ring_buffer_dma)
		return -ENOMEM;
	dsp_set_area1_attr(SILAN_DSP0,ring_buffer_dma,size);
	
	ring_buffer = ioremap_nocache(ring_buffer_dma,size);
    
	for(i = 0; i < dspdev_info.filternum; i++)
	{
		dspdev_info.filter[i].state = DSPDEV_STATE_FREE;
		dspdev_info.filter[i].pdspdev_info = &dspdev_info;
		dspdev_info.filter[i].bufstart = ring_buffer;
		dspdev_info.filter[i].sysex = ring_buffer;

		mutex_init(&dspdev_info.filter[i].rmutex);
		mutex_init(&dspdev_info.filter[i].wmutex);

		rbuffer = ring_buffer+(MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*i+MIPS_DSP_SYSCHANGE_SIZE;
		wbuffer = ring_buffer+(MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*i+MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE;
		
		dsp_ringbuffer_init(&dspdev_info.filter[i].rbuffer, rbuffer,DSP_R_RINGBUFFER_SIZE);
		dsp_ringbuffer_init(&dspdev_info.filter[i].wbuffer, wbuffer,DSP_W_RINGBUFFER_SIZE);
	
		init_timer(&dspdev_info.filter[i].timer);
	
		dspdev_info.filter[i].rdma_addr = ring_buffer_dma+(DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*i;
		dspdev_info.filter[i].wdma_addr = ring_buffer_dma+(DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*i+DSP_R_RINGBUFFER_SIZE;
	}

	return 0;
}

static struct file_operations silan_dsp_fops = 
{
	.owner	= THIS_MODULE,
	.read = silan_dsp_read,
	.write = silan_dsp_write,
	.open = silan_dsp_open,
	.release = silan_dsp_release,
	.unlocked_ioctl	= silan_dsp_ioctl,
	.mmap = silan_dsp_mmap
};

static int silan_dsp_probe(struct platform_device *pdev)
{
	int ret = 0;
	mutex_init(&dspdev_info.mutex);
	INIT_LIST_HEAD(&dspdev_info.queue);
	
	ret = dsp_ctl_probe(pdev);
	if(ret)
		goto out;
    
	ret = silan_dsp_ringbuffer_init();
	if(ret)
		goto out;
    
	ret = silan_mbox_probe(pdev);
	if(ret)
		goto out;

	g_mbox = silan_mbox_get("silan-mailbox");
	if(!g_mbox)
		goto out;

	ret = silan_mbox_init(g_mbox);
	if(ret)
		goto out;

	ret = dsp_mbox_late_init(g_mbox);
	if(ret)
		goto out;

	if(register_chrdev(DSP_CHAR_MAJOR,DEV_DSP_NAME,&silan_dsp_fops))
	{
		printk("can't allocate major number\n");
		goto out;
	}
	
	silan_dsp_class = class_create(THIS_MODULE,"dsp_class");
	if(IS_ERR(silan_dsp_class))
	{
		printk("can't create dsp class\n");
		silan_dsp_class = NULL;
		goto out;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27) 
	device_create(silan_dsp_class,NULL,MKDEV(DSP_CHAR_MAJOR,0),NULL,DEV_DSP_NAME);
#else	
	device_create(silan_dsp_class,NULL,MKDEV(DSP_CHAR_MAJOR,0),DEV_DSP_NAME);
#endif	


	silan_cxc_set_idx(0, CXC_DLNA_SET_END_FLAG);
	silan_cxc_set_idx(0, CXC_DLNA_GET_DECODE_STATE);
	silan_cxc_set_idx(0, CXC_DLNA_IN_RIDX);
	silan_cxc_set_idx(0, CXC_DLNA_IN_WIDX);
	silan_cxc_set_idx(0, CXC_DLNA_OUT_WIDX);
	silan_cxc_set_idx(0, CXC_DLNA_OUT_RIDX);


    {
        extern int dsp_dlna_ring_init(int buff_size);
        if (0 != dsp_dlna_ring_init(1024*642))
            return -EFAULT;

        dsp_set_area1_attr(SILAN_DSP0, dsp_info.dsp_dlna_ring_buffer_dma, 
                1024*642);
    }



	return 0;
out:
	return ret;
}

int silan_dsp_remove(struct platform_device *dev)
{
	silan_mbox_put("silan-mailbox");
	device_destroy(silan_dsp_class,MKDEV(DSP_CHAR_MAJOR,0));
	class_destroy(silan_dsp_class);
	unregister_chrdev(DSP_CHAR_MAJOR,"silan_dsp");
	platform_set_drvdata(dev,NULL);
	return 0;
}


int silan_dsp_suspend(struct platform_device *dev, pm_message_t mesg)
{
	dsp_set_suspend();
	return 0;
}

int silan_dsp_resume(struct platform_device *dev)
{
	dsp_set_resume();
	return 0;
}

struct platform_driver silan_dsp_driver={
	.probe		= silan_dsp_probe,
	.remove		= silan_dsp_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= DEV_DSP_NAME,
	},
	.suspend	= silan_dsp_suspend,
	.resume		= silan_dsp_resume,
};

static int __init silan_dsp_init(void)
{
	if(platform_driver_register(&silan_dsp_driver))
	{
		printk("register driver error\n");
		platform_driver_unregister(&silan_dsp_driver);
	}
	return 0;
}

static void __exit silan_dsp_exit(void)
{
	platform_driver_unregister(&silan_dsp_driver);
	printk("silan dsp module removed\n");
}

module_init(silan_dsp_init);
module_exit(silan_dsp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("panjianguang");
MODULE_DESCRIPTION("Driver for silan dsp");
