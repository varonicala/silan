/*
 * spdif-dma.c -- ALSA Soc Audio Layer
 *
 * This program is free software; you can redistributeit and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or(at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/amba/pl08x.h>
#include <linux/interrupt.h>
#include <linux/time.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "suv_pcm.h"

struct sl_pcm_runtime_data
{
	u32 buffer_bytes;
	u32 period_bytes;
	u32 frag_bytes;
	int frags;
	int frag_count;
	int period_count;
	int endInt_flag;
	spinlock_t dma_lock;
	struct dma_async_tx_descriptor *desc;
	struct dma_chan *dma_chan;
	dma_addr_t dma_addr;
	unsigned long pos;
	int dmacount;
	struct tasklet_struct tasklet;
	int stream;
	struct snd_pcm_substream *substream;
	struct sl_dma_data dma_data;	
};

static const struct snd_pcm_hardware dma_hardware = 
{
	.info	= 	SNDRV_PCM_INFO_INTERLEAVED|
				SNDRV_PCM_INFO_BLOCK_TRANSFER| 
		  		SNDRV_PCM_INFO_BATCH|
				SNDRV_PCM_INFO_MMAP|
				SNDRV_PCM_INFO_MMAP_VALID|
		  		SNDRV_PCM_INFO_PAUSE|
				SNDRV_PCM_INFO_RESUME,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
		SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE,
	.buffer_bytes_max = 192*1024*2,
	.period_bytes_min = 1024,
	.period_bytes_max = 32*1024,
	.periods_min	= 2,
	.periods_max	= 4*1024,
	.channels_min	= 1,
	.channels_max	= 8,
	.fifo_size = 32,
};

static void dma_tasklet(unsigned long data);

static void audio_dma_complete(void *data)
{
	struct sl_pcm_runtime_data *prtd = (struct sl_pcm_runtime_data *)data;
	
#ifdef CONFIG_SILAN_DMA_LLI_INT
	prtd->endInt_flag = !prtd->endInt_flag;
	if (prtd->endInt_flag) {
		prtd->period_count++;
		prtd->period_count %= prtd->frags;
		return;
	}
#endif

	if(prtd->frag_count >=0)
	{
		prtd->dmacount--;
		dma_tasklet((unsigned long)prtd);
//		tasklet_schedule(&prtd->tasklet);
	}
}

static bool filter(struct dma_chan *chan, void *param)
{
	struct sl_pcm_runtime_data *prtd = param;
	
#ifdef CONFIG_MIPS_SILAN_DLNA
	if((strcmp(dev_name(chan->device->dev), DMAC0_NAME) != 0) || (chan->chan_id != prtd->dma_data.dma_request))
#else
	if((strcmp(dev_name(chan->device->dev), DMAC1_NAME) != 0) || (chan->chan_id != prtd->dma_data.dma_request))
#endif
		return false;

	chan->private = &prtd->dma_data;
	return true;
}

static int sl_dma_alloc(struct snd_pcm_substream *substream,struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sl_pcm_dma_params *dma_params;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sl_pcm_runtime_data *prtd = runtime->private_data;
	struct dma_slave_config slave_config;
	dma_cap_mask_t mask;
	enum dma_slave_buswidth buswidth;
	int ret;

	dma_params = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream); 

	prtd->dma_data.priority = DMA_PRIO_HIGH;
	prtd->dma_data.dma_request = dma_params->dma;
	prtd->dma_data.peripheral_type = dma_params->type;
	
	/* Try to grab a DMA channel */
	if(!prtd->dma_chan)
	{
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		prtd->dma_chan = dma_request_channel(mask, filter, prtd);
		if(!prtd->dma_chan)
			return -EINVAL;
	}

	switch(params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
#ifdef CONFIG_SILAN_DLNA
		if (prtd->dma_data.peripheral_type == SL_DMATYPE_SPDIF_IN)
			buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
		else
#endif
		buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;
	default:
		return -1;
	}

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		slave_config.direction = DMA_TO_DEVICE;
		slave_config.dst_addr = dma_params->dma_addr;
		slave_config.dst_addr_width = buswidth;
		slave_config.dst_maxburst = dma_params->burstsize;
	} else {
		slave_config.direction = DMA_FROM_DEVICE;
		slave_config.src_addr = dma_params->dma_addr;
		slave_config.src_addr_width = buswidth;
		slave_config.src_maxburst = dma_params->burstsize;
	}
	
	ret = dmaengine_slave_config(prtd->dma_chan, &slave_config);
	if(ret)
		return ret;
	return 0;
}

static int silan_pcm_hw_params(struct snd_pcm_substream *substream,struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sl_pcm_runtime_data *prtd = runtime->private_data;
	int ret;

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if(ret < 0) {
		printk("Faild to malloc pages for stream:%d\n", substream->stream);
		return ret;
	}

	ret = sl_dma_alloc(substream, params);
	if(ret)
	{
		printk("silan dma alloc failed \n");
		return ret;
	}
	
	prtd->stream = substream->stream;
	prtd->substream = substream;
	prtd->pos = 0;
	tasklet_init(&prtd->tasklet, dma_tasklet, (unsigned int)prtd);

	return 0;
}

static int silan_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sl_pcm_runtime_data *prtd = runtime->private_data;
	
	tasklet_kill(&prtd->tasklet);
	if(prtd->dma_chan) {
		dmaengine_terminate_all(prtd->dma_chan);
		dma_release_channel(prtd->dma_chan);
		prtd->dma_chan = NULL;
	}

	snd_pcm_lib_free_pages(substream);	
	return 0;
}

static int silan_pcm_prepare(struct snd_pcm_substream *substream)
{
    struct timeval tv;
    struct timespec tstamp;
  //  long long t_tm;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sl_pcm_runtime_data *prtd = runtime->private_data;
	
	prtd->dma_addr = runtime->dma_addr;
	prtd->buffer_bytes = snd_pcm_lib_buffer_bytes(substream);	
	prtd->period_bytes = snd_pcm_lib_period_bytes(substream);
	if(prtd->buffer_bytes == prtd->period_bytes)
	{
		prtd->frag_bytes = prtd->period_bytes>>1;
		prtd->frags = 2;
	}
	else
	{
		prtd->frag_bytes = prtd->period_bytes;
		prtd->frags = prtd->buffer_bytes/prtd->period_bytes;
	}
    //gettimeofday(&tv, NULL);
    do_gettimeofday(&tv);
    //t_tm = timeval_to_ns(&tv);
    getnstimeofday(&tstamp);
	printk("======buffer_bytes %d ,time:%d(ms) \n",prtd->buffer_bytes,tstamp.tv_sec*1000+(tstamp.tv_nsec/1000000));
//    printk("time:%d(ms)\n",tstamp.tv_sec*1000+(tstamp.tv_nsec/1000000));
	prtd->frag_count = 0;
	prtd->period_count = 0;
	prtd->endInt_flag = 1;
	prtd->pos = 0;
	return 0;
}

static struct dma_async_tx_descriptor *dma_submit(struct sl_pcm_runtime_data *prtd, dma_addr_t dma_addr)
{
	struct dma_chan *chan = prtd->dma_chan;
	struct dma_async_tx_descriptor *desc;
	struct scatterlist sg;

	sg_init_table(&sg, 1);

	sg_set_page(&sg, pfn_to_page(PFN_DOWN(dma_addr)),
		 prtd->frag_bytes, dma_addr & (PAGE_SIZE - 1));
	sg_dma_address(&sg) = dma_addr;
	
	desc = chan->device->device_prep_slave_sg(chan, &sg, 1, 
		prtd->stream == SNDRV_PCM_STREAM_PLAYBACK ?
		DMA_TO_DEVICE : DMA_FROM_DEVICE,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK );

	if(!desc) {
		printk("cannot prepare slave dma\n");
		return NULL;
	}

	desc->callback = audio_dma_complete;
	desc->callback_param = prtd;
	desc->tx_submit(desc);

	return desc;
}

#define NR_DMA_CHAIN 2
static void dma_tasklet(unsigned long data)
{
	struct sl_pcm_runtime_data *prtd = (struct sl_pcm_runtime_data *)data;
	struct dma_chan *chan = prtd->dma_chan;
	struct dma_async_tx_descriptor *desc;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&prtd->dma_lock,flags);
	if(prtd->frag_count < 0) 
	{
		spin_unlock_irqrestore(&prtd->dma_lock,flags);
		chan->device->device_control(chan,DMA_TERMINATE_ALL,0);
		for(i=0;i<1;i++)
		{
			desc = dma_submit(prtd, prtd->dma_addr+i*prtd->frag_bytes);
			if(!desc)
				return;
		}
		prtd->dmacount = NR_DMA_CHAIN;
		prtd->frag_count = 1;//NR_DMA_CHAIN%prtd->frags;
		chan->device->device_issue_pending(chan);
		return;
	}

	BUG_ON(prtd->dmacount>= NR_DMA_CHAIN);
	while(prtd->dmacount < NR_DMA_CHAIN) 
	{
		prtd->dmacount++;
		spin_unlock_irqrestore(&prtd->dma_lock,flags);
		desc = dma_submit(prtd, prtd->dma_addr + prtd->frag_count*prtd->frag_bytes);
		if(!desc)
			return;

		chan->device->device_issue_pending(chan);

		spin_lock_irqsave(&prtd->dma_lock,flags);

		prtd->frag_count++;
		prtd->frag_count %= prtd->frags;
		prtd->pos += prtd->frag_bytes;
		prtd->pos %= prtd->buffer_bytes;
		if((prtd->frag_count*prtd->frag_bytes)%prtd->period_bytes == 0)	
			snd_pcm_period_elapsed(prtd->substream);
	}
	spin_unlock_irqrestore(&prtd->dma_lock,flags);
}

static int silan_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sl_pcm_runtime_data *prtd = runtime->private_data;

	switch(cmd) {
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_START:
		prtd->frag_count = -1;
		tasklet_schedule(&prtd->tasklet);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		dmaengine_terminate_all(prtd->dma_chan);
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
		dmaengine_device_control(prtd->dma_chan, DMA_PAUSE, 0);
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		dmaengine_device_control(prtd->dma_chan, DMA_RESUME, 0);
		break;

	default:
		return -EINVAL;
	}	
	return 0;
}

static snd_pcm_uframes_t silan_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sl_pcm_runtime_data *prtd = runtime->private_data;
#ifdef CONFIG_SILAN_DMA_LLI_INT
	struct dma_chan *chan = prtd->dma_chan;
	dma_addr_t src, dst;
	unsigned long res;
	extern int sl_dma_getposition(struct dma_chan *, dma_addr_t *, dma_addr_t *);

	if (sl_dma_getposition(chan, &src, &dst) != 0)
		return 0;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		res = src - prtd->dma_addr;
		res %= prtd->frag_bytes;
	}
	else {
		res = dst - prtd->dma_addr;
		res %= prtd->frag_bytes;
	}

	res += prtd->period_count*prtd->frag_bytes;
	res %= snd_pcm_lib_buffer_bytes(substream);

#if 1
	if(readl(0xbc000108) == 0){
		prtd->frag_count = -1;
		printk("## %s %d frag_count: %d##\n", __func__, __LINE__, prtd->frag_count);	
		printk("#### src: %x, dst: %x, res: %d\n", src, dst, res);
		tasklet_schedule(&prtd->tasklet);
	}
#endif

	return bytes_to_frames(substream->runtime, res);
#else
	return bytes_to_frames(substream->runtime, prtd->pos);
#endif
}

static int silan_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sl_pcm_runtime_data *prtd;
	int ret;

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if(prtd == NULL)
		return -ENOMEM;
	
	runtime->private_data = prtd;
	spin_lock_init(&prtd->dma_lock);

	ret = snd_soc_set_runtime_hwparams(substream, &dma_hardware);
	if(ret)
		return ret;

	ret = snd_pcm_hw_constraint_integer(substream->runtime,SNDRV_PCM_HW_PARAM_PERIODS);
	if(ret < 0) {
		printk("hw_constraint_integer failed.\n");
		kfree(prtd);
		return ret;
	}
	
	return 0;
}

static int silan_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sl_pcm_runtime_data *prtd = runtime->private_data;

	if(!prtd)
		printk("dma_close called with prtd == NULL\n");

	kfree(prtd);
	return 0;
}

static int silan_pcm_mmap(struct snd_pcm_substream *substream,
		struct vm_area_struct *vma)
{
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	return remap_pfn_range(vma, vma->vm_start, 
				substream->dma_buffer.addr >> PAGE_SHIFT,
				vma->vm_end - vma->vm_start, vma->vm_page_prot);
}

static struct snd_pcm_ops silan_pcm_ops = {
	.open		= silan_pcm_open,
	.close		= silan_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params 	= silan_pcm_hw_params,
	.hw_free	= silan_pcm_hw_free,
	.prepare	= silan_pcm_prepare,
	.trigger	= silan_pcm_trigger,
	.pointer	= silan_pcm_pointer,
	.mmap		= silan_pcm_mmap,
};

static u64 dma_mask = DMA_BIT_MASK(32);

static int silan_pcm_new(struct snd_card *card, struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	if(!card->dev->dma_mask)
		card->dev->dma_mask = &dma_mask;
	if(!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	return snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV, 
			card->dev, 2*128*1024, 4*1024*1024);
}

static void silan_pcm_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	int stream;

	for(stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if(!substream)
			continue;
		snd_pcm_lib_preallocate_free(substream);
	}
}

static struct snd_soc_platform_driver silan_asoc_platform = {
	.pcm_new	= silan_pcm_new,
	.pcm_free	= silan_pcm_free,
	.ops		= &silan_pcm_ops,
};

static int __devinit silan_asoc_platform_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &silan_asoc_platform);
}

static int __devexit silan_asoc_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver silan_pcm_driver = {
	.driver = {
		.name	= "silan-pcm",
		.owner	= THIS_MODULE,
	},
	.probe	= silan_asoc_platform_probe,
	.remove	= __devexit_p(silan_asoc_platform_remove),
};

static int __init silan_asoc_init(void)
{
	return platform_driver_register(&silan_pcm_driver);
}
module_init(silan_asoc_init);

static void __exit silan_asoc_exit(void)
{
	platform_driver_unregister(&silan_pcm_driver);
}
module_exit(silan_asoc_exit);

MODULE_AUTHOR("Lu Xuegang, <luxuegang@silan.com>");
MODULE_DESCRIPTION("Silan ASoc DMA Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:silan-audio");
