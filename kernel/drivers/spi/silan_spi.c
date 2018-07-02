#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/list.h>

#include <linux/spi/flash.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#include <asm/io.h>
#include <asm/dma.h>
#include <silan_padmux.h>
#include "silan_spi.h"

#define SPI_TIMEOUT (2*HZ)
struct silan_spi {
	struct spi_bitbang	bitbang;
	struct completion	done;

	unsigned char		*regs;
	unsigned int		irq;
	unsigned int		len;
	unsigned int		count;
	int 				tran_idx;
	unsigned short 		cmd;
	unsigned long 		clk_rate;
	
	const unsigned char	*tx;
	unsigned char		*rx;
	
	struct clk			*clk;
	struct resource		*ioarea;
	struct spi_master	*master;
	struct spi_device	*curdev;
	struct device		*dev;
};

void spi_send_end_cmd(struct silan_spi *hw)
{
	unsigned int tmp;
	tmp = readl(hw->regs + SPI_CTRL);
	tmp |= PROGRAM_END;
	writel(tmp,hw->regs + SPI_CTRL);
}

int spi_send_cmd(struct silan_spi *hw,struct spi_transfer *t)
{
	unsigned char val;
	unsigned long delay;
	struct list_head *head;
	
	if(hw->count < hw->len)
	{
		val = *(unsigned char*)t->tx_buf;
		writel(val,hw->regs+SPI_DATA);
		t->tx_buf ++;	
		hw->count ++;
	}

	writel(hw->cmd,hw->regs+SPI_CMD);
	
	while(hw->count < hw->len)
	{
		delay = jiffies + SPI_TIMEOUT;
		//while(((readl(hw->regs+SPI_STATUS) & READY_FOR_NEXT) != READY_FOR_NEXT)&&time_before(jiffies,delay));
		while(((readl(hw->regs+SPI_STATUS) & READY_FOR_NEXT) != READY_FOR_NEXT));
				
		val = *(unsigned char*)t->tx_buf;
		writel(val,hw->regs+SPI_DATA);
		t->tx_buf ++;	
		hw->count ++;
	}
	if(hw->len != 0x2)
		while(((readl(hw->regs+SPI_STATUS) & READY_FOR_NEXT) != READY_FOR_NEXT));

	head = t->transfer_list.prev;
	if(t->transfer_list.next == head)
	{
		spi_send_end_cmd(hw);
	}
	return 0;
}

int spi_write_data(struct silan_spi *hw,struct spi_transfer *t)
{
	unsigned char val;
	unsigned long delay = 0;
	while (hw->count < t->len )
	{
		val = *(unsigned char*)t->tx_buf;
		writel(val,hw->regs+SPI_DATA);
		t->tx_buf ++;	
		hw->count ++;
		//while(((readl(hw->regs+SPI_STATUS) & READY_FOR_NEXT) != READY_FOR_NEXT)&&time_before(jiffies,delay));
		while(((readl(hw->regs+SPI_STATUS) & READY_FOR_NEXT) != READY_FOR_NEXT));
	}
	
	spi_send_end_cmd(hw);
	return 0;
}

int spi_read_data(struct silan_spi *hw,struct spi_transfer *t)
{
	unsigned long delay = 0;	
	unsigned int tmp;
	int need_read = 1;
	hw->cmd &= ~(1<<8);
	writel(hw->cmd,hw->regs+SPI_CMD);
	while(hw->count < t->len)
	{
		if(hw->len != 0x1 || hw->cmd == 0x603)
		{
#ifdef CONFIG_MIPS_SILAN_DLNA
			if (need_read)
			{
				readl(hw->regs+SPI_DATA);
				need_read = 0;
			}
#else
			writel(0,hw->regs+SPI_DATA);
#endif
		}
		//while(((readl(hw->regs+SPI_STATUS) & READY_FOR_NEXT)!= READY_FOR_NEXT)&&time_before(jiffies,delay));
		while(((readl(hw->regs+SPI_STATUS) & READY_FOR_NEXT)!= READY_FOR_NEXT));

		*(unsigned char*)t->rx_buf = (unsigned char)readl(hw->regs+SPI_DATA);
		t->rx_buf++; 
		hw->count ++;

		if(hw->count == t->len )
		{
			tmp = readl(hw->regs + SPI_CTRL);
			tmp |= PROGRAM_END;
			writel(tmp,hw->regs + SPI_CTRL);
		}

	}
	t->rx_buf--;	
	return 0;
}

int spi_fast_read(struct silan_spi *hw,struct spi_transfer *t,unsigned char read_flag)
{
	unsigned int reg_val;
	unsigned int addr;
	addr = *(unsigned int *)t->tx_buf;
	switch(read_flag)	
	{
		case 1:
			reg_val = readl(hw->regs + SPI_CTRL);
			reg_val &= 0x07ef;	
			writel(reg_val,hw->regs + SPI_CTRL);
			while(hw->count  < t->len)
			{
				*(unsigned int*)t->rx_buf = *(unsigned int *)(SPI_MAP_ADDR + hw->count +addr);
				t->rx_buf += 4;
				hw->count += 4;
			}
			break;
		case 2:
			reg_val = readl(hw->regs + SPI_CTRL);
			reg_val |= 	DOUBLE_READ;
			writel(reg_val,hw->regs + SPI_CTRL);
			while(hw->count  < t->len)
			{
				*(unsigned int*)t->rx_buf = *(unsigned int *)(SPI_MAP_ADDR + hw->count +addr);
				t->rx_buf += 4;
				hw->count += 4;
			}
			break;
		case 4:
			reg_val = readl(hw->regs + SPI_CTRL);
			reg_val |= 	QUAL_READ;
			writel(reg_val,hw->regs + SPI_CTRL);
			while(hw->count  < t->len)
			{
				*(unsigned int*)t->rx_buf = *(unsigned int *)(SPI_MAP_ADDR + hw->count +addr);
				t->rx_buf += 4;
				hw->count += 4;
			}
			break;
		default:
			break;
	}
	return hw->count;
}

static int silan_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{

	unsigned int tmp,count = 0;
	struct list_head *head;
	struct silan_spi *hw = spi_master_get_devdata(spi->master);		
	hw->len = t->len;
	hw->count =0;
	head = t->transfer_list.prev;
	if(t->tx_buf == NULL) return 0;
	hw->cmd = *(unsigned short*)t->tx_buf;

	t->tx_buf += 2;
	hw->count += 2;

	tmp = readl(hw->regs+SPI_CTRL);
	//tmp |= SEL_BANK2 + RISC_ACCESS;           
	tmp |=  RISC_ACCESS;
	tmp &= ~(1<<SEL_BANK2);
	tmp &= ~(1<<9);
	writel(tmp,hw->regs+SPI_CTRL);

	if(t->tx_buf && hw->count < hw->len)
		count = spi_send_cmd(hw,t);
//	if(t->rx_buf)
//		count = spi_fast_read(hw,t,1);

	if(t->transfer_list.next == head && hw->count == hw->len)
		count = spi_send_cmd(hw,t);

	while(t->transfer_list.next != head)
	{
		t = list_entry(t->transfer_list.next,struct spi_transfer,transfer_list);
		hw->count = 0;	
		if(t->rx_buf)
		{
			count = spi_read_data(hw,t);
		}
	
		if(t->tx_buf)
		{
			count = spi_write_data(hw,t);
		}
	}

#if 0
	tmp = readl(hw->regs+SPI_CTRL);
	tmp &= ~(RISC_ACCESS);
	writel(tmp,hw->regs+SPI_CTRL);
#endif
	return hw->count;
	return 0;
}
	
static int silan_spi_setupxfer(struct spi_device *spi,struct spi_transfer *t)
{
	return 0;
}

void silan_spi_chipsel(struct spi_device *spi,int value)
{

}

static int silan_spi_setup(struct spi_device *spi)
{
	return 0;
}

static int silan_spi_initialsetup(struct silan_spi *hw)
{
	if(clk_enable(hw->clk))
	{
		printk("enable spi clock error\n");
		return -1; 
	}	

	hw->clk_rate = clk_get_rate(hw->clk);	
	if(!hw->clk_rate)
	{
		printk("Get spi clock rate error\n");
		return -1;
	}

	return 0;
}
static int __init silan_spi_probe(struct platform_device *pdev)
{
	int err;
	struct silan_spi *hw;
	struct spi_master *master;
	struct resource *res;	

	master = spi_alloc_master(&pdev->dev, sizeof(struct silan_spi));
	if(master == NULL)
	{
		printk(KERN_ERR"No Memory\n");
		err = -ENOMEM;
		return err;
	}
	hw = spi_master_get_devdata(master);
	memset(hw, 0, sizeof(struct silan_spi));

	hw->master = spi_master_get(master);
	hw->dev = &pdev->dev;

	platform_set_drvdata(pdev, hw);
	init_completion(&hw->done);

	master->num_chipselect = 3;
	master->bus_num = 1;

	hw->bitbang.master         = hw->master;
	hw->bitbang.setup_transfer = silan_spi_setupxfer;
	hw->bitbang.chipselect     = silan_spi_chipsel;
	hw->bitbang.txrx_bufs      = silan_spi_txrx;
	hw->bitbang.master->setup  = silan_spi_setup;

	hw->clk = clk_get(&pdev->dev,"i2c");
	if(!hw->clk)
	{
		printk("can't get clk\n");
		return -1;
	}

	if(clk_enable(hw->clk))
	{
		printk("enable spi clock error\n");
		return -1; 
	}	

	hw->clk_rate = clk_get_rate(hw->clk);	
	if(!hw->clk_rate)
	{
		printk("Get spi clock rate error\n");
		return -1;
	}	
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ERR"Cannot get IORESOURCE_MEM\n");
		err = -ENOENT;
		goto err_no_iores;
	}

	hw->ioarea = request_mem_region(res->start, (res->end - res->start)+1,
					pdev->name);

	if (hw->ioarea == NULL) {
		printk(KERN_ERR"Cannot reserve region\n");
		err = -ENXIO;
		goto err_no_iores;
	}

	hw->regs = ioremap(res->start, (res->end - res->start)+1);
	if (hw->regs == NULL) {
		printk(KERN_ERR"Cannot map IO\n");
		err = -ENXIO;
		goto err_no_iomap;
	}

	hw->irq = platform_get_irq(pdev, 0);
	if (hw->irq < 0) {
		printk(KERN_ERR"No IRQ specified\n");
		err = -ENOENT;
		goto err_no_irq;
	}
	err = spi_bitbang_start(&hw->bitbang);
	if (err) {
		printk(KERN_ERR"Failed to register SPI master\n");
		//goto err_register;
	}

err_no_irq:
	iounmap(hw->regs);

err_no_iomap:
	release_resource(hw->ioarea);
	kfree(hw->ioarea);

err_no_iores:
	spi_master_put(hw->master);
	
	return 0;
}

static int __exit silan_spi_remove(struct platform_device *dev)
{
	struct silan_spi *hw = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	spi_unregister_master(hw->master);

	free_irq(hw->irq, hw);
	iounmap(hw->regs);

	release_resource(hw->ioarea);
	kfree(hw->ioarea);

	spi_master_put(hw->master);
	return 0;
}

#ifdef CONFIG_PM

static int silan_spi_suspend(struct device *dev)
{
	struct silan_spi *hw = platform_get_drvdata(to_platform_device(dev));
	//if(hw->pdata && hw->pdata->gpio_setup)
	//	hw->pdata->gpio_setup(hw->pdata, 0);
	
	clk_disable(hw->clk);

	return 0;
}

static int silan_spi_resume(struct device *dev)
{
	struct silan_spi *hw = platform_get_drvdata(to_platform_device(dev));

	silan_spi_initialsetup(hw);

	return 0;
}

static const struct dev_pm_ops silan_spi_pm_ops = {
	.suspend = silan_spi_suspend,
	.resume  = silan_spi_resume,
};

#define SILAN_SPI_PM_OPS &silan_spi_pm_ops
#else
#define SILAN_SPI_PM_OPS NULL
#endif

static struct platform_driver silan_spi_driver = {
	.remove		= __exit_p(silan_spi_remove),
	.driver		= {
		.name	= "silan-spi",
		.owner	= THIS_MODULE,
		.pm     = SILAN_SPI_PM_OPS,
	},
};
static int __init silan_spi_init(void)
{
    return platform_driver_probe(&silan_spi_driver, silan_spi_probe);
}

static void __exit silan_spi_exit(void)
{
        platform_driver_unregister(&silan_spi_driver);
}

module_init(silan_spi_init);
module_exit(silan_spi_exit);

MODULE_DESCRIPTION("SILAN SPI Driver");
MODULE_AUTHOR("HUAJIE_YANG");
MODULE_LICENSE("GPL");

