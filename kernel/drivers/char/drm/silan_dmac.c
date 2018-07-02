#include <linux/module.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_def.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>

#include "silan_dmac.h"
#include "silan_drm.h"

static const struct burst_table burst_sizes[] = {
	{
		.burstwords = 256,
		.reg = (PL080_BSIZE_256 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_256 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 128,
		.reg = (PL080_BSIZE_128 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_128 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 64,
		.reg = (PL080_BSIZE_64 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_64 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 32,
		.reg = (PL080_BSIZE_32 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_32 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 16,
		.reg = (PL080_BSIZE_16 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_16 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 8,
		.reg = (PL080_BSIZE_8 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_8 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 4,
		.reg = (PL080_BSIZE_4 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_4 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 1,
		.reg = (PL080_BSIZE_1 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_1 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
};

static void dma_request(struct dma_chan *chan, u32 chan_id, void __iomem *dmac_base)
{
	chan->chan_id = chan_id;
	chan->dma_base_addr = dmac_base;
	chan->dma_cx_base_addr = dmac_base + PL080_Cx_BASE(chan->chan_id);
}

static void dma_prepare(void __iomem *dmac_base_addr)
{
	writel(0x1,	dmac_base_addr + PL080_CONFIG);		/* Enable dmac */
	writel(0x000000FF, dmac_base_addr + PL080_ERR_CLEAR);
	writel(0x000000FF, dmac_base_addr + PL080_TC_CLEAR);
}

static void dma_config_addr(struct dma_chan *chan, u32 src_addr, u32 dst_addr)
{
	chan->src_addr = src_addr & 0x1FFFFFFF;
	chan->dst_addr = dst_addr & 0x1FFFFFFF;
}

static int dma_config_sg(struct dma_chan *chan, int peri_id, enum dma_data_direction direction, int len, enum dma_buswidth width, int burst_size)
{
	u32 cctl = 0;
	u32 ccfg = 0;
	int i;

	cctl |= PL080_CONTROL_TC_IRQ_EN;
	cctl |= (len / width & 0x0FFF);
	ccfg |= PL080_CONFIG_TC_IRQ_MASK;
	ccfg |= PL080_CONFIG_ERR_IRQ_MASK;

	if(direction == DMA_TO_DEVICE) {
		cctl |= PL080_CONTROL_SRC_INCR;
		cctl |= PL080_CONTROL_SRC_AHB2;
		ccfg |= PL080_FLOW_MEM2PER << PL080_CONFIG_FLOW_CONTROL_SHIFT;
		ccfg |= (peri_id & 0x0F) << PL080_CONFIG_DST_SEL_SHIFT;
	} else {
		cctl |= PL080_CONTROL_DST_AHB2;
		cctl |= PL080_CONTROL_DST_INCR;
		// cctl |= PL080_CONTROL_SRC_INCR; // BUF1 DMA READ
		// ccfg |= PL080_FLOW_MEM2MEM << PL080_CONFIG_FLOW_CONTROL_SHIFT;
		ccfg |= PL080_FLOW_PER2MEM << PL080_CONFIG_FLOW_CONTROL_SHIFT;
		ccfg |= (peri_id & 0x0F) << PL080_CONFIG_SRC_SEL_SHIFT;
	}

	switch(width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			cctl |= (PL080_WIDTH_8BIT << PL080_CONTROL_SWIDTH_SHIFT) |
				(PL080_WIDTH_8BIT << PL080_CONTROL_DWIDTH_SHIFT);
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			cctl |= (PL080_WIDTH_16BIT << PL080_CONTROL_SWIDTH_SHIFT) |
				(PL080_WIDTH_16BIT << PL080_CONTROL_DWIDTH_SHIFT);
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			cctl |= (PL080_WIDTH_32BIT << PL080_CONTROL_SWIDTH_SHIFT) |
				(PL080_WIDTH_32BIT << PL080_CONTROL_DWIDTH_SHIFT);
			break;
		default:
			return -EINVAL;
	}

	for(i = 0; i < ARRAY_SIZE(burst_sizes); i++)
		if(burst_sizes[i].burstwords <= burst_size)
			break;
	cctl |= burst_sizes[i].reg;

	chan->cctl = cctl;
	chan->ccfg = ccfg;

	return 0;
}

/* Silan_dmac_initart the dma channel */
static void dma_start(struct dma_chan *chan)
{
	u32 val;

#ifdef DRM_DBG
	printk(
		"WRITE channel %d: csrc=0x%08x, cdst=0x%08x, "
		"cctl=0x%08x, ccfg=0x%08x\n",
		chan->chan_id, chan->src_addr, chan->dst_addr, chan->cctl,
		chan->ccfg);
#endif

	writel(chan->src_addr, chan->dma_cx_base_addr + PL080_CH_SRC_ADDR);
	writel(chan->dst_addr, chan->dma_cx_base_addr + PL080_CH_DST_ADDR);
	writel(0, chan->dma_cx_base_addr + PL080_CH_LLI);
	writel(chan->cctl, chan->dma_cx_base_addr + PL080_CH_CONTROL);
	writel(chan->ccfg, chan->dma_cx_base_addr + PL080_CH_CONFIG);

	val = readl(chan->dma_cx_base_addr + PL080_CH_CONFIG);
	writel(val | PL080_CONFIG_ENABLE, chan->dma_cx_base_addr + PL080_CH_CONFIG);
}

/* Stop the dma channel */
static void dma_stop(struct dma_chan *chan)
{
	u32 channel = 0;
	u32 val = 0;
	
	channel = chan->chan_id;
	val = readl(chan->dma_cx_base_addr + PL080_CH_CONFIG);

	val &= ~(PL080_CONFIG_ENABLE | PL080_CONFIG_ERR_IRQ_MASK |
			PL080_CONFIG_TC_IRQ_MASK);

	writel(val, chan->dma_base_addr + PL080_CH_CONFIG);
	writel(1 << channel, chan->dma_base_addr + PL080_ERR_CLEAR);
	writel(1 << channel, chan->dma_base_addr + PL080_TC_CLEAR);
}

static void sl_drm_dma_cleanup(struct silan_drm *drm)
{

}

static void sl_drm_dmac_stop_dma(struct silan_drm *drm, enum drm_direction dir)
{
	if(dir == DMA_TO_BUF){
		dma_stop(&drm->tx_chan);
	}
	else{
		dma_stop(&drm->rx_chan);
	}
}

static void sl_drm_dmac_complete_dma(struct silan_drm *drm, enum drm_direction dir)
{
	u8 *val1, *val2;
	int i;
	
	if(dir == DMA_TO_BUF){
#ifdef DRM_DBG
		printk("===========aes in============\n");
		val1 = (drm->mmap_cpu);

		for(i = 0; i < drm->param.size; i++){
			if(i != 0 && i % 16 == 0)
				printk("\n");
			printk("%02x ", val1[i]);
		}
		printk("\n");
#endif
	}
	else{
#ifdef DRM_DBG
		printk("===========aes in============\n");
		printk("===========aes out============\n");
		val2 = (drm->mmap_cpu+DRM_MMAP_SIZE/2/4);
		for(i = 0; i < drm->param.size; i++){
			if(i != 0 && i % 16 == 0)
				printk("\n");
			printk("%02x ", val2[i]);
		}
		printk("\n");
#endif
	}
}

static void sl_drm_dmac_start_dma(struct silan_drm *drm, enum drm_direction dir)
{
	u32 burst_size = 0;
	u32 peri_id = 0;
	u32 len = drm->param.size;
	void __iomem *base = (void __iomem *)drm->buf_ctrl_base;
	u8 *val1, *val2;
	int i;
	
	if(dir == DMA_TO_BUF){
		burst_size = drm->tx_chan.burst_size;
		peri_id = drm->tx_chan.peri_id;
		dma_config_sg(&drm->tx_chan, peri_id, DMA_TO_DEVICE, len, DMA_SLAVE_BUSWIDTH_4_BYTES, burst_size);
#ifdef DRM_DBG
		printk("before dma start complete\n");
		printk("===========aes in============\n");
		//val1 = (drm->mmap_cpu);
		val1 = 0xa1d20000; 
		for(i = 0; i < drm->param.size; i++){
			if(i != 0 && i % 16 == 0)
				printk("\n");
			printk("%02x ", val1[i]);
		}
		printk("\n");
#endif
		dma_start(&drm->tx_chan);
	}
	else{
		burst_size = drm->rx_chan.burst_size;
		peri_id = drm->rx_chan.peri_id;
		dma_config_sg(&drm->rx_chan, peri_id, DMA_FROM_DEVICE, len, DMA_SLAVE_BUSWIDTH_4_BYTES, burst_size);

		writel(((drm->param.size / 4) << 16) | (1 << 15), base + BUF1_DMA_RD_CFG);

		dma_start(&drm->rx_chan);
	}
}

static int sl_drm_dmac_init(struct silan_drm *drm)
{
	dma_request(&drm->tx_chan, TX_CHAN, drm->dmac_base);
	dma_request(&drm->rx_chan, RX_CHAN, drm->dmac_base);

	dma_prepare(drm->dmac_base);
	
	dma_config_addr(&drm->tx_chan, (u32)(drm->mmap_cpu), 0);
	dma_config_addr(&drm->rx_chan, 0x00010000, (u32)(drm->mmap_cpu+DRM_MMAP_SIZE/2/4));
	
	drm->tx_chan.burst_size = 4;
	drm->rx_chan.burst_size = 4;

	drm->tx_chan.peri_id = ID_BUF0_WR;
	drm->rx_chan.peri_id = ID_BUF1_RD;
	
	return 0;
}

static struct silan_drm_dma_ops sl_drm_dmac_ops = {
	.init     = sl_drm_dmac_init,
	.start    = sl_drm_dmac_start_dma,
	.stop     = sl_drm_dmac_stop_dma,
	.complete = sl_drm_dmac_complete_dma,
	.cleanup  = sl_drm_dma_cleanup,
};

void silan_dmac_ops_init(struct silan_drm *drm)
{
	drm->dma_ops = &sl_drm_dmac_ops;
}

