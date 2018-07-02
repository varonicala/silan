#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/amba/pl08x.h>
#include "i2c-silan_new.h"

#ifdef CONFIG_I2C_DMA_MODE

static bool filter_tx(struct dma_chan *chan, void *param)
{
	struct silan_i2c_dev *dev = param;

	if((strcmp(dev_name(chan->device->dev), DMAC0_NAME) != 0) || (chan->chan_id != dev->dmatx_ch))
		return false;

	return true;
}

static bool filter_rx(struct dma_chan *chan, void *param)
{
	struct silan_i2c_dev *dev = param;

	if((strcmp(dev_name(chan->device->dev), DMAC0_NAME) != 0) || (chan->chan_id != dev->dmarx_ch))
		return false;

	return true;
}

static int mid_i2c_dma_init(struct silan_i2c_dev *dev)
{
	struct dma_chan *chan;
	dma_cap_mask_t mask;

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	/* 1. Init rx channel */
	chan = dma_request_channel(mask, filter_tx, dev);
	if (!chan)
		goto err_exit;
	dev->dmarx.chan = chan;

	/* 2. Init tx channel */
	chan = dma_request_channel(mask, filter_rx, dev);
	if (!chan)
		goto free_rxchan;
	dev->dmatx.chan = chan;
	
	dev->dma_inited = 1;
	return 0;

free_rxchan:
	dma_release_channel(dev->dmarx.chan);
err_exit:
	return -1;
}

static int mid_i2c_dma_start(struct silan_i2c_dev *dev)
{
	if (!dev->dmatx.chan)
		return -1;

	dev->dmatx.buf = kmalloc(I2C_DMA_BUFFER_SIZE, GFP_KERNEL);
	if (!dev->dmatx.buf) {
		printk("no memory for DMA TX buffer\n");
		return -1;
	}

	sg_init_one(&dev->dmatx.sg, dev->dmatx.buf, I2C_DMA_BUFFER_SIZE);

	if (!dev->dmarx.chan)
		return -1;

	/* Allocate and map DMA RX buffers */
	dev->dmarx.buf = kmalloc(I2C_DMA_BUFFER_SIZE, GFP_KERNEL);
	if (!dev->dmarx.buf) {
		printk("no memory for DMA TX buffer\n");
		return -1;
	}
	
	sg_init_one(&dev->dmarx.sg, dev->dmarx.buf, I2C_DMA_BUFFER_SIZE);
	
	return 0;
}

static void mid_i2c_dma_exit(struct silan_i2c_dev *dev)
{
	dma_release_channel(dev->dmatx.chan);
	dma_release_channel(dev->dmarx.chan);
}

/*
 * dev->dma_chan_done is cleared before the dma transfer starts,
 * callback for rx/tx channel will each increment it by 1.
 * Reaching 2 means the whole i2c transaction is done.
 */
static void silan_i2c_dmatx_done(void *arg)
{
	struct silan_i2c_dev *dev = arg;

	dev->dmatx_done = 1;

	return;
}

static void silan_i2c_dmarx_done(void *arg)
{
	struct silan_i2c_dev *dev = arg;
	
	dev->dmatx_done = 0;
	
	return;
}

static int mid_i2c_dma_write(struct silan_i2c_dev *dev, int dma_enable)
{
	struct dma_async_tx_descriptor *txdesc = NULL;
	struct dma_chan *txchan;
	struct dma_slave_config txconf;
	u32 tmp = 0;
	
	/* 1. setup DMA related registers */
	if (dma_enable) {
		tmp = readl(dev->base + DW_IC_DMA_CR);
		tmp |= TDMAE;
		writel(tmp, dev->base + DW_IC_DMA_CR);
	}

	dev->dma_chan_done = 0;
	txchan = dev->dmatx.chan;

	/* 2. Prepare the TX dma transfer */
	txconf.direction      = DMA_TO_DEVICE;
	txconf.dst_addr       = dev->dma_addr;
	txconf.dst_maxburst   = 0x1;
//	txconf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	txconf.dst_addr_width =  DMA_SLAVE_BUSWIDTH_4_BYTES;
	
	txchan->device->device_control(txchan, DMA_SLAVE_CONFIG,
				       (unsigned long) &txconf);

	dev->dmatx.sg.length = dev->tx_buf_len;
	
	if (dma_map_sg(txchan->device->dev, &dev->dmatx.sg, 1, DMA_TO_DEVICE) != 1) {
		printk("unable to map TX DMA\n");
		return -EBUSY;
	}

	txdesc = txchan->device->device_prep_slave_sg(txchan,
				&dev->dmatx.sg,
				1,
				DMA_TO_DEVICE,
				DMA_PREP_INTERRUPT | DMA_COMPL_SKIP_DEST_UNMAP);
	txdesc->callback = silan_i2c_dmatx_done;
	txdesc->callback_param = dev;

	txdesc->tx_submit(txdesc);

	/* Fire the DMA transaction */ 
	txchan->device->device_issue_pending(txchan);

	return 0;
}

static int mid_i2c_dma_read(struct silan_i2c_dev *dev, int dma_enable)
{
	struct dma_async_tx_descriptor *rxdesc = NULL;
	struct dma_chan *rxchan;
	struct dma_slave_config rxconf;
	u32 tmp = 0;
	
	/* 1. setup DMA related registers */
	if (dma_enable) {
		tmp = readl(dev->base + DW_IC_DMA_CR);
		tmp |= RDMAE;
		writel(tmp, dev->base + DW_IC_DMA_CR);
	}

	dev->dma_chan_done = 0;
	rxchan = dev->dmarx.chan;

	/* 2. Prepare the RX dma transfer */
	rxconf.direction      = DMA_FROM_DEVICE;
	rxconf.src_addr       = dev->dma_addr;
	rxconf.src_maxburst   = 0x1;
	rxconf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	//rxconf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	
	rxchan->device->device_control(rxchan, DMA_SLAVE_CONFIG,
				       (unsigned long) &rxconf);

	dev->dmarx.sg.length = dev->rx_buf_len;
	
	if(dma_map_sg(rxchan->device->dev, &dev->dmarx.sg, 1, DMA_FROM_DEVICE) != 1) {
		printk("unable to map RX DMA\n");
		return -EBUSY;
	}

	rxdesc = rxchan->device->device_prep_slave_sg(rxchan,
				&dev->dmarx.sg,
				1,
				DMA_FROM_DEVICE,
				DMA_PREP_INTERRUPT | DMA_COMPL_SKIP_DEST_UNMAP);
	rxdesc->callback       = silan_i2c_dmarx_done;
	rxdesc->callback_param = dev;

	rxdesc->tx_submit(rxdesc);

	/* Fire the DMA transaction */                                                                                                 
	rxchan->device->device_issue_pending(rxchan);
	
	return 0;
}

static struct silan_i2c_dma_ops mid_dma_ops = {
	.dma_init	= mid_i2c_dma_init,
	.dma_exit	= mid_i2c_dma_exit,
	.dma_start  = mid_i2c_dma_start,
	.dma_write  = mid_i2c_dma_write,
	.dma_read	= mid_i2c_dma_read,
};
#endif

int silan_i2c_mid_init(struct silan_i2c_dev *dev)
{
#ifdef CONFIG_I2C_DMA_MODE
	dev->dma_addr = (dma_addr_t)(dev->phy_base + DW_IC_DATA_CMD); 
	dev->dma_ops = &mid_dma_ops;
#endif
	return 0;
}
