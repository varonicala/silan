#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <silan_padmux.h>
#include <asm/irq.h>
#include <silan_generic.h>
#include <linux/gpio.h>
#include "i2c-silan_new.h"
#include <silan_reset.h>

static char *abort_sources[] = {
	[ABRT_7B_ADDR_NOACK] =
		"slave address not acknowledged (7bit mode)",
	[ABRT_10ADDR1_NOACK] =
		"first address byte not acknowledged (10bit mode)",
	[ABRT_10ADDR2_NOACK] =
		"second address byte not acknowledged (10bit mode)",
	[ABRT_TXDATA_NOACK] =
		"data not acknowledged",
	[ABRT_GCALL_NOACK] =
		"no acknowledgement for a general call",
	[ABRT_GCALL_READ] =
		"read after general call",
	[ABRT_SBYTE_ACKDET] =
		"start byte acknowledged",
	[ABRT_SBYTE_NORSTRT] =
		"trying to send start byte when restart is disabled",
	[ABRT_10B_RD_NORSTRT] =
		"trying to read when restart is disabled (10bit mode)",
	[ABRT_MASTER_DIS] =
		"trying to use disabled adapter",
	[ARB_LOST] =
		"lost arbitration",
};

static u32 i2c_silan_scl_hcnt(u32 ic_clk, u32 tSYMBOL, 
							  u32 tf, int cond, int offset)
{
	/*
	 * DesignWare I2C core doesn't seem to have solid strategy to meet
	 * the tHD;STA timing spec.  Configuring _HCNT based on tHIGH spec
	 * will result in violation of the tHD;STA spec.
	 */
	if (cond)
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
		 *
		 * This is based on the DW manuals, and represents an ideal
		 * configuration.  The resulting I2C bus speed will be
		 * faster than any of the others.
		 *
		 * If your harsilanare is free from tHD;STA issue, try this one.
		 */
		return (ic_clk * tSYMBOL + 5000) / 10000 - 8 + offset;
	else
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
		 *
		 * This is just experimental rule; the tHD;STA period turned
		 * out to be proportinal to (_HCNT + 3).  With this setting,
		 * we could meet both tHIGH and tHD;STA timing specs.
		 *
		 * If unsure, you'd better to take this alternative.
		 *
		 * The reason why we need to take into account "tf" here,
		 * is the same as described in i2c_silan_scl_lcnt().
		 */
		return (ic_clk * (tSYMBOL + tf) + 5000) / 10000 - 3 + offset;
}

static u32 i2c_silan_scl_lcnt(u32 ic_clk, u32 tLOW, u32 tf, int offset)
{
	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (tLOW + tf)
	 *
	 * DW I2C core starts counting the SCL CNTs for the LOW period
	 * of the SCL clock (tLOW) as soon as it pulls the SCL line.
	 * In order to meet the tLOW timing spec, we need to take into
	 * account the fall time of SCL signal (tf).  Default tf value
	 * should be 0.3 us, for safety.
	 */
	return ((ic_clk * (tLOW + tf) + 5000) / 10000) - 1 + offset;
}

/**
 * i2c_silan_init() - initialize the designware i2c master harsilanare
 * @dev: device private data
 *
 * This functions configures and enables the I2C master.
 * This function is called during I2C init function, and in case of timeout at
 * run time.
 */
static void i2c_silan_init(struct silan_i2c_dev *dev)
{
	u32 input_clock_khz = clk_get_rate(dev->clk) / 1000 ;  //400k
	u32 ic_con, hcnt, lcnt;

	/* Disable the adapter */
	writel(0, dev->base + DW_IC_ENABLE);

	/* set standard and fast speed deviders for high/low periods */

	/* Standard-mode */
	hcnt = i2c_silan_scl_hcnt(input_clock_khz,
				40,	/* tHD;STA = tHIGH = 4.0 us */
				3,	/* tf = 0.3 us */
				0,	/* 0: DW default, 1: Ideal */
				0);	/* No offset */
	lcnt = i2c_silan_scl_lcnt(input_clock_khz,
				47,	/* tLOW = 4.7 us */
				3,	/* tf = 0.3 us */
				0);	/* No offset */
	writel(hcnt, dev->base + DW_IC_SS_SCL_HCNT);
	writel(lcnt, dev->base + DW_IC_SS_SCL_LCNT);
	dev_dbg(dev->dev, "Standard-mode HCNT:LCNT = %d:%d\n", hcnt, lcnt);

	/* Fast-mode */
	hcnt = i2c_silan_scl_hcnt(input_clock_khz,
				6,	/* tHD;STA = tHIGH = 0.6 us */
				3,	/* tf = 0.3 us */
				0,	/* 0: DW default, 1: Ideal */
				0);	/* No offset */
	lcnt = i2c_silan_scl_lcnt(input_clock_khz,
				13,	/* tLOW = 1.3 us */
				3,	/* tf = 0.3 us */
				0);	/* No offset */
	writel(hcnt, dev->base + DW_IC_FS_SCL_HCNT);
	writel(lcnt, dev->base + DW_IC_FS_SCL_LCNT);
	dev_dbg(dev->dev, "Fast-mode HCNT:LCNT = %d:%d\n", hcnt, lcnt);

	/* Configure Tx/Rx FIFO threshold levels */
	writel(dev->tx_fifo_depth - 1, dev->base + DW_IC_TX_TL);
	writel(0, dev->base + DW_IC_RX_TL);

	/* configure the i2c master */
	ic_con = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE |
		//DW_IC_CON_RESTART_EN | DW_IC_CON_SPEED_FAST;
		DW_IC_CON_RESTART_EN | DW_IC_CON_SPEED_STD;
	writel(ic_con, dev->base + DW_IC_CON);
}

/*
 * Waiting for bus not busy
 */
static int i2c_silan_wait_bus_not_busy(struct silan_i2c_dev *dev)
{
	int timeout = TIMEOUT;

	while (readl(dev->base + DW_IC_STATUS) & DW_IC_STATUS_ACTIVITY) {
		if (timeout <= 0) {
			dev_warn(dev->dev, "timeout waiting for bus ready\n");
			return -ETIMEDOUT;
		}
		timeout--;
		mdelay(1);
	}

	return 0;
}

static void i2c_silan_xfer_init(struct silan_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 ic_con;

	/* Disable the adapter */
	writel(0, dev->base + DW_IC_ENABLE);

	/* set the slave (target) address */
	writel(msgs[dev->msg_write_idx].addr, dev->base + DW_IC_TAR);

	/* if the slave address is ten bit address, enable 10BITADDR */
	ic_con = readl(dev->base + DW_IC_CON);
	if (msgs[dev->msg_write_idx].flags & I2C_M_TEN)
		ic_con |= DW_IC_CON_10BITADDR_MASTER;
	else
		ic_con &= ~DW_IC_CON_10BITADDR_MASTER;
	writel(ic_con, dev->base + DW_IC_CON);

	/* Enable the adapter */
	writel(1, dev->base + DW_IC_ENABLE);

	/* Enable interrupts */
	writel(DW_IC_INTR_DEFAULT_MASK, dev->base + DW_IC_INTR_MASK);
}

#ifdef CONFIG_I2C_DMA_MODE
static void i2c_silan_dma_xfer_msg(struct silan_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 intr_mask;
	u32 buf_len;
	u16 i;

	intr_mask = DW_IC_INTR_DEFAULT_MASK;

	for(; dev->msg_write_idx < dev->msgs_num; dev->msg_write_idx++) {
		buf_len = msgs[dev->msg_write_idx].len;

		if (msgs[dev->msg_write_idx].flags & I2C_M_RD) {
#if 0		
			dev->tx_buf_len = buf_len * 4;
			for(i = 0; i < buf_len; i++)
				dev->dmatx.buf[i] = 0x100;
			
			if(dev->dma_ops || dev->dma_ops->dma_write)
				dev->dma_ops->dma_write(dev, 1);
			
			while(!dev->dmatx_done);
			dev->dmatx_done = 0;
			
			dev->rx_buf_len = buf_len;
			
			if(dev->dma_ops || dev->dma_ops->dma_read)
				dev->dma_ops->dma_read(dev, 1);		
			memcpy(msgs[dev->msg_write_idx].buf, dev->dmarx.buf, dev->rx_buf_len);
#endif		

#if 1	
			for(i = 0; i < buf_len; i++)
				writel(0x100, dev->base + DW_IC_DATA_CMD);
			
			dev->rx_buf_len = buf_len;
			if(dev->dma_ops || dev->dma_ops->dma_read)
				dev->dma_ops->dma_read(dev, 1);		
		
			memcpy(msgs[dev->msg_write_idx].buf, dev->dmarx.buf, dev->rx_buf_len);
#endif
		} else {
			if(buf_len == 1)
				writel(msgs[dev->msg_write_idx].buf[0], dev->base + DW_IC_DATA_CMD);
			else {
				dev->tx_buf_len = buf_len * 4;
#if 0
				for(i = 0; i < buf_len; i++)
					writel(msgs[dev->msg_write_idx].buf[i], dev->base + DW_IC_DATA_CMD);
				memcpy(dev->dmatx.buf, msgs[dev->msg_write_idx].buf, buf_len);
#endif				
				for(i = 0; i < buf_len; i++)
					dev->dmatx.buf[i] = (u32)msgs[dev->msg_write_idx].buf[i];
#ifdef I2C_DUG			
				for(i = 0; i < buf_len; i++)
					printk("buf[%d]: %x\n",i,  dev->dmatx.buf[i]);
#endif
				if(dev->dma_ops || dev->dma_ops->dma_write)
					dev->dma_ops->dma_write(dev, 1);		
			}
		}
	}

	/*
	 * If i2c_msg index search is completed, we don't need TX_EMPTY
	 * and RX_FULL interrupt any more.
	 */
	intr_mask &= ~DW_IC_INTR_TX_EMPTY;
	intr_mask &= ~DW_IC_INTR_RX_FULL;

	writel(intr_mask, dev->base + DW_IC_INTR_MASK);

}
#endif
/*
 * Initiate (and continue) low level master read/write transaction.
 * This function is only called from i2c_silan_isr, and pumping i2c_msg
 * messages into the tx buffer.  Even if the size of i2c_msg data is
 * longer than the size of the tx buffer, it handles everything.
 */
static void i2c_silan_xfer_msg(struct silan_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 intr_mask;
	int tx_limit, rx_limit;
	u32 addr = msgs[dev->msg_write_idx].addr;
	u32 buf_len = dev->tx_buf_len;
	u8 *buf = dev->tx_buf;

	intr_mask = DW_IC_INTR_DEFAULT_MASK;

#ifdef CONFIG_I2C_DMA_MODE
	i2c_silan_dma_xfer_msg(dev);
	return ;
#endif

	for (; dev->msg_write_idx < dev->msgs_num; dev->msg_write_idx++) {
		/*
		 * if target address has changed, we need to
		 * reprogram the target address in the i2c
		 * adapter when we are done with this transfer
		 */
		if (msgs[dev->msg_write_idx].addr != addr) {
			dev_err(dev->dev,
				"%s: invalid target address\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (msgs[dev->msg_write_idx].len == 0) {
			dev_err(dev->dev,
				"%s: invalid message length\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (!(dev->status & STATUS_WRITE_IN_PROGRESS)) {
			/* new i2c_msg */
			buf = msgs[dev->msg_write_idx].buf;
			buf_len = msgs[dev->msg_write_idx].len;
			//for(i=0;i<buf_len;i++)
			//	printk("buf[%d]: %x\n", i, buf[i]);
		}

		tx_limit = dev->tx_fifo_depth - readl(dev->base + DW_IC_TXFLR);
		rx_limit = dev->rx_fifo_depth - readl(dev->base + DW_IC_RXFLR);

		while (buf_len > 0 && tx_limit > 0 && rx_limit > 0) {
			if (msgs[dev->msg_write_idx].flags & I2C_M_RD) {
				writel(0x100, dev->base + DW_IC_DATA_CMD);
				rx_limit--;
			} else{
				writel(*buf++, dev->base + DW_IC_DATA_CMD);
			}
			tx_limit--; buf_len--;
		}

		dev->tx_buf = buf;
		dev->tx_buf_len = buf_len;

		if (buf_len > 0) {
			/* more bytes to be written */
			dev->status |= STATUS_WRITE_IN_PROGRESS;
			break;
		} else
			dev->status &= ~STATUS_WRITE_IN_PROGRESS;
	}

	/*
	 * If i2c_msg index search is completed, we don't need TX_EMPTY
	 * interrupt any more.
	 */
	if (dev->msg_write_idx == dev->msgs_num)
		intr_mask &= ~DW_IC_INTR_TX_EMPTY;

	if (dev->msg_err)
		intr_mask = 0;
	
	writel(intr_mask, dev->base + DW_IC_INTR_MASK);
}

static void i2c_silan_read(struct silan_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	int rx_valid;
	
	for (; dev->msg_read_idx < dev->msgs_num; dev->msg_read_idx++) {
		u32 len;
		u8 *buf;

		if (!(msgs[dev->msg_read_idx].flags & I2C_M_RD))
			continue;

		if (!(dev->status & STATUS_READ_IN_PROGRESS)) {
			len = msgs[dev->msg_read_idx].len;
			buf = msgs[dev->msg_read_idx].buf;
		} else {
			len = dev->rx_buf_len;
			buf = dev->rx_buf;
		}

		rx_valid = readl(dev->base + DW_IC_RXFLR);

		for (; len > 0 && rx_valid > 0; len--, rx_valid--)
			*buf++ = readl(dev->base + DW_IC_DATA_CMD);

		if (len > 0) {
			dev->status |= STATUS_READ_IN_PROGRESS;
			dev->rx_buf_len = len;
			dev->rx_buf = buf;
			return;
		} else
			dev->status &= ~STATUS_READ_IN_PROGRESS;
	}
}

static int i2c_silan_handle_tx_abort(struct silan_i2c_dev *dev)
{
	unsigned long abort_source = dev->abort_source;
	int i;

	if (abort_source & DW_IC_TX_ABRT_NOACK) {
		for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
			//dev_dbg(dev->dev,
			dev_err(dev->dev,
				"%s: %s\n", __func__, abort_sources[i]);
		return -EREMOTEIO;
	}

	for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
		dev_err(dev->dev, "%s: %s\n", __func__, abort_sources[i]);

	if (abort_source & DW_IC_TX_ARB_LOST)
		return -EAGAIN;
	else if (abort_source & DW_IC_TX_ABRT_GCALL_READ)
		return -EINVAL; /* wrong msgs[] data */
	else
		return -EIO;
}

void reset_i2c_bus(struct silan_i2c_dev *dev)
{
	u32 gpio_clk = 20;
	u32 i;
	
	silan_padmux_ctrl(SILAN_PADMUX_I2C1, PAD_OFF);
	
	gpio_request(gpio_clk, "silan_gpio");
	gpio_direction_output(gpio_clk, 0);

	for(i = 0; i < 10; i++){
		gpio_set_value(gpio_clk, 0);
		udelay(20);
		gpio_set_value(gpio_clk, 1);
		udelay(20);
	}
	
	silan_padmux_ctrl(SILAN_PADMUX_I2C1, PAD_ON);	

	silan_module_rst(SILAN_SR_I2C);
}

/*
 * Prepare controller for a transaction and call i2c_silan_xfer_msg
 */
static int
i2c_silan_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct silan_i2c_dev *dev = i2c_get_adapdata(adap);
	int ret;

	dev_dbg(dev->dev, "%s: msgs: %d\n", __func__, num);

	mutex_lock(&dev->lock);

	INIT_COMPLETION(dev->cmd_complete);
	dev->msgs = msgs;
	dev->msgs_num = num;
	dev->cmd_err = 0;
	dev->msg_write_idx = 0;
	dev->msg_read_idx = 0;
	dev->msg_err = 0;
	dev->status = STATUS_IDLE;
	dev->abort_source = 0;

	ret = i2c_silan_wait_bus_not_busy(dev);
	if (ret < 0)
		goto done;

	/* start the transfers */
	i2c_silan_xfer_init(dev);

	/* wait for tx to complete */
	ret = wait_for_completion_interruptible_timeout(&dev->cmd_complete, HZ);
	if (ret == 0) {
		dev_err(dev->dev, "controller timed out\n");
		reset_i2c_bus(dev);

		i2c_silan_init(dev);
		ret = -ETIMEDOUT;
		goto done;
	} else if (ret < 0)
		goto done;

	if (dev->msg_err) {
		ret = dev->msg_err;
		goto done;
	}

	/* no error */
	if (likely(!dev->cmd_err)) {
		/* Disable the adapter */
		writel(0, dev->base + DW_IC_ENABLE);
		ret = num;
		goto done;
	}

	/* We have an error */
	if (dev->cmd_err == DW_IC_ERR_TX_ABRT) {
		ret = i2c_silan_handle_tx_abort(dev);
		goto done;
	}
	ret = -EIO;

done:
	mutex_unlock(&dev->lock);

	return ret;
}

static u32 i2c_silan_func(struct i2c_adapter *adap)
{
	return	I2C_FUNC_I2C |
		I2C_FUNC_10BIT_ADDR |
		I2C_FUNC_SMBUS_BYTE |
		I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA |
		I2C_FUNC_SMBUS_I2C_BLOCK;
}

static u32 i2c_silan_read_clear_intrbits(struct silan_i2c_dev *dev)
{
	u32 stat;

	/*
	 * The IC_INTR_STAT register just indicates "enabled" interrupts.
	 * Ths unmasked raw version of interrupt status bits are available
	 * in the IC_RAW_INTR_STAT register.
	 *
	 * That is,
	 *   stat = readl(IC_INTR_STAT);
	 * equals to,
	 *   stat = readl(IC_RAW_INTR_STAT) & readl(IC_INTR_MASK);
	 *
	 * The raw version might be useful for debugging purposes.
	 */
	stat = readl(dev->base + DW_IC_INTR_STAT);

	/*
	 * Do not use the IC_CLR_INTR register to clear interrupts, or
	 * you'll miss some interrupts, triggered during the period from
	 * readl(IC_INTR_STAT) to readl(IC_CLR_INTR).
	 *
	 * Instead, use the separately-prepared IC_CLR_* registers.
	 */
	if (stat & DW_IC_INTR_RX_UNDER)
		readl(dev->base + DW_IC_CLR_RX_UNDER);
	if (stat & DW_IC_INTR_RX_OVER)
		readl(dev->base + DW_IC_CLR_RX_OVER);
	if (stat & DW_IC_INTR_TX_OVER)
		readl(dev->base + DW_IC_CLR_TX_OVER);
	if (stat & DW_IC_INTR_RD_REQ)
		readl(dev->base + DW_IC_CLR_RD_REQ);
	if (stat & DW_IC_INTR_TX_ABRT) {
		/*
		 * The IC_TX_ABRT_SOURCE register is cleared whenever
		 * the IC_CLR_TX_ABRT is read.  Preserve it beforehand.
		 */
		dev->abort_source = readl(dev->base + DW_IC_TX_ABRT_SOURCE);
		readl(dev->base + DW_IC_CLR_TX_ABRT);
	}
	if (stat & DW_IC_INTR_RX_DONE)
		readl(dev->base + DW_IC_CLR_RX_DONE);
	if (stat & DW_IC_INTR_ACTIVITY)
		readl(dev->base + DW_IC_CLR_ACTIVITY);
	if (stat & DW_IC_INTR_STOP_DET)
		readl(dev->base + DW_IC_CLR_STOP_DET);
	if (stat & DW_IC_INTR_START_DET)
		readl(dev->base + DW_IC_CLR_START_DET);
	if (stat & DW_IC_INTR_GEN_CALL)
		readl(dev->base + DW_IC_CLR_GEN_CALL);

	return stat;
}

/*
 * Interrupt service routine. This gets called whenever an I2C interrupt
 * occurs.
 */
static irqreturn_t i2c_silan_isr(int this_irq, void *dev_id)
{
	struct silan_i2c_dev *dev = dev_id;
	u32 stat;

	stat = i2c_silan_read_clear_intrbits(dev);
	dev_dbg(dev->dev, "%s: stat=0x%x\n", __func__, stat);
	if (stat & DW_IC_INTR_TX_ABRT) {
		dev->cmd_err |= DW_IC_ERR_TX_ABRT;
		dev->status = STATUS_IDLE;
		//reset_i2c_bus();
		/*
		 * Anytime TX_ABRT is set, the contents of the tx/rx
		 * buffers are flushed.  Make sure to skip them.
		 */
		writel(0, dev->base + DW_IC_INTR_MASK);
		goto tx_aborted;
	}

	if (stat & DW_IC_INTR_RX_FULL)
		i2c_silan_read(dev);

	if (stat & DW_IC_INTR_TX_EMPTY)
		i2c_silan_xfer_msg(dev);

	/*
	 * No need to modify or disable the interrupt mask here.
	 * i2c_silan_xfer_msg() will take care of it according to
	 * the current transmit status.
	 */

tx_aborted:
	if ((stat & (DW_IC_INTR_TX_ABRT | DW_IC_INTR_STOP_DET)) || dev->msg_err)
		complete(&dev->cmd_complete);
	
	return IRQ_HANDLED;
}

static struct i2c_algorithm i2c_silan_algo = {
	.master_xfer	= i2c_silan_xfer,
	.functionality	= i2c_silan_func,
};

static int __devinit silan_i2c_probe(struct platform_device *pdev)
{
	struct silan_i2c_dev *dev;
	struct i2c_adapter *adap;
	struct resource *mem, *ioarea, *dma_res[2];
	int irq, r;
	/* NOTE: driver uses the static register mapping */
#ifndef CONFIG_MIPS_SILAN_DLNA
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 3);
#else
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
#endif
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -EINVAL;
	}

#ifndef CONFIG_MIPS_SILAN_DLNA
	irq = platform_get_irq(pdev, 3);
#else
	irq = platform_get_irq(pdev, 0);
#endif
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return irq; /* -ENXIO */
	}

	ioarea = request_mem_region(mem->start, resource_size(mem),
			pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "I2C region already claimed\n");
		return -EBUSY;
	}
	
	dma_res[0] = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	dma_res[1] = platform_get_resource(pdev, IORESOURCE_DMA, 1);
	if (!dma_res[0] || !dma_res[1]) {
		dev_err(&pdev->dev, "no DMA resource\n");
		return  -ENXIO;
		goto err_release_region;
	}

	dev = kzalloc(sizeof(struct silan_i2c_dev), GFP_KERNEL);
	if (!dev) {
		r = -ENOMEM;
		goto err_release_region;
	}

	init_completion(&dev->cmd_complete);
	mutex_init(&dev->lock);
	dev->dev = get_device(&pdev->dev);
	dev->irq = irq;
	platform_set_drvdata(pdev, dev);

#ifdef CONFIG_I2C_DMA_MODE	
	dev->dmatx_ch = dma_res[0]->start;
	dev->dmarx_ch = dma_res[1]->start;
	dev->phy_base = mem->start;
	dev->dma_inited = 0;
	dev->dmatx_done = 0;
#endif
	dev->clk = clk_get(&pdev->dev, "i2c");
	if (IS_ERR(dev->clk)) {
		r = -ENODEV;
		goto err_free_mem;
	}
	clk_enable(dev->clk);

#ifndef CONFIG_MIPS_SILAN_DLNA
	sl_writel(((1<<22) | (sl_readl(I2C4_CLK))), I2C4_CLK);
	sl_writel(((1<<25) | (sl_readl(I2C4_ENABLE))) , I2C4_ENABLE);
#else
	silan_padmux_ctrl(SILAN_PADMUX_I2C1, PAD_ON);	
#endif

	dev->base = ioremap(mem->start, resource_size(mem));
	if (dev->base == NULL) {
		dev_err(&pdev->dev, "failure mapping io resources\n");
		r = -EBUSY;
		goto err_unuse_clocks;
	}
	
	{
		dev->tx_fifo_depth = FIFO_DEPTH;//((param1 >> 16) & 0xff) + 1;
		dev->rx_fifo_depth = FIFO_DEPTH;//((param1 >> 8)  & 0xff) + 1;
	}
	i2c_silan_init(dev);

	writel(0, dev->base + DW_IC_INTR_MASK); /* disable IRQ */
	r = request_irq(dev->irq, i2c_silan_isr, IRQF_DISABLED, pdev->name, dev);
	if (r) {
		dev_err(&pdev->dev, "failure requesting irq %i\n", dev->irq);
		goto err_iounmap;
	}

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "silan-i2c", sizeof(adap->name));
	adap->algo = &i2c_silan_algo;
	adap->dev.parent = &pdev->dev;

#ifndef CONFIG_MIPS_SILAN_DLNA
	adap->nr = 3;
#else
	adap->nr = 0;
#endif
	r = i2c_add_numbered_adapter(adap);
	if (r) {
		dev_err(&pdev->dev, "failure adding adapter\n");
		goto err_free_irq;
	}

#ifdef CONFIG_I2C_DMA_MODE
	u32 ret = -1;
	silan_i2c_mid_init(dev);
	
	if(dev->dma_ops && dev->dma_ops->dma_init){
		ret = dev->dma_ops->dma_init(dev);
		if(ret){
			dev_err(&pdev->dev, "DMA init failed\n");
			dev->dma_inited = 0;
		}
	}
	if(dev->dma_ops || dev->dma_ops->dma_start)
		dev->dma_ops->dma_start(dev);
#endif

	return 0;

err_free_irq:
	free_irq(dev->irq, dev);
err_iounmap:
	iounmap(dev->base);
err_unuse_clocks:
	clk_disable(dev->clk);
	clk_put(dev->clk);
	dev->clk = NULL;
err_free_mem:
	platform_set_drvdata(pdev, NULL);
	put_device(&pdev->dev);
	kfree(dev);
err_release_region:
	release_mem_region(mem->start, resource_size(mem));

	return r;
}

static int __devexit silan_i2c_remove(struct platform_device *pdev)
{
	struct silan_i2c_dev *dev = platform_get_drvdata(pdev);
	struct resource *mem;

	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&dev->adapter);
	put_device(&pdev->dev);

	clk_disable(dev->clk);
	clk_put(dev->clk);
	dev->clk = NULL;

	writel(0, dev->base + DW_IC_ENABLE);
	free_irq(dev->irq, dev);
	kfree(dev);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	release_mem_region(mem->start, resource_size(mem));
#ifdef CONFIG_I2C_DMA_MODE
	if(dev->dma_ops && dev->dma_ops->dma_exit)
		dev->dma_ops->dma_exit(dev);
#endif
	return 0;
}

/* work with hotplug and coldplug */
MODULE_ALIAS("platform:i2c_designware");

static struct platform_driver silan_i2c_driver = {
	.remove		= __devexit_p(silan_i2c_remove),
	.driver		= {
		.name	= "silan-i2c",
		.owner	= THIS_MODULE,
	},
};

static int __init silan_i2c_init_driver(void)
{
	return platform_driver_probe(&silan_i2c_driver, silan_i2c_probe);
}
module_init(silan_i2c_init_driver);

static void __exit silan_i2c_exit_driver(void)
{
	platform_driver_unregister(&silan_i2c_driver);
}
module_exit(silan_i2c_exit_driver);

MODULE_AUTHOR("Baruch Siach <baruch@tkos.co.il>");
MODULE_DESCRIPTION("Synopsys DesignWare I2C bus adapter");
MODULE_LICENSE("GPL");
