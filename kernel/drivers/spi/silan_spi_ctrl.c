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
#include "silan_spi_ctrl.h"

#define SPI_REG
//#define SPI_FIFO

struct silan_spi
{
    struct spi_bitbang  bitbang;
    struct completion   done;

    unsigned char       *regs;
    unsigned int        irq;
    unsigned int        len;
    unsigned int        count;
    int                 tran_idx;
    int                 addr;
    unsigned short      cmd;
    unsigned long       clk_rate;

    const unsigned char	*tx;
    unsigned char       *rx;

    struct clk          *clk;
    struct resource     *ioarea;
    struct spi_master   *master;
    struct spi_device   *curdev;
    struct device       *dev;
};

static void init_spi(struct silan_spi *hw, int div)
{
    unsigned int tmp;

    writel(0xffffffff, hw->regs + SPI_TR_PIO);
    tmp = readl(hw->regs + SPI_CR);
    tmp |= SPICR_FIFO_RST;
    writel(tmp, hw->regs + SPI_CR);

#ifdef SPI_REG
    tmp = (SPICR_FIFO_TX_DISABLE | SPICR_FIFO_RX_DISABLE | SPICR_CS_SEL(0) | SPICR_SPIEN);
#else
    tmp = (SPICR_CS_SEL(0) | SPICR_SPIEN);
#endif
    tmp |= SPICR_WR_EN | SPICR_RD_EN;
    tmp |= SPICR_CLK_EN;
    tmp |= SPICR_DIV(div);
    writel(tmp, hw->regs + SPI_CR);
}

static void spi_start_stop(struct silan_spi *hw, int flag, int bit_len)
{
    unsigned int mystatus;

    if (flag == 1)
    {
        mystatus = readl(hw->regs + SPI_CR);
        mystatus |= SPICR_START;		//SPI Transfer Start
        mystatus &= (~(0x3f<<15));
        mystatus |= (bit_len<<15);
        writel(mystatus, hw->regs + SPI_CR);
    }
    else
    {
        mystatus = readl(hw->regs + SPI_CR);
        mystatus &= (~SPICR_START);
        writel(mystatus, hw->regs + SPI_CR);
    }
}

static void set_tr_mode(struct silan_spi *hw, unsigned int mode)
{
    unsigned int cr;

    cr = readl(hw->regs + SPI_CR);
    cr &= (~(0x3<<13));
    cr |= (mode<<13);
    writel(cr, hw->regs + SPI_CR);
}

#ifdef SPI_REG
static void spi_send_byte(struct silan_spi *hw, unsigned int data)
{
    writel(data<<24, hw->regs + SPI_TR_PIO);
    set_tr_mode(hw,SPI_TRMODE_WONLY);
    spi_start_stop(hw,1,8);
    while((readl(hw->regs + SPI_SR) & SPISR_XMIT_EMPTY)==0);
    while((readl(hw->regs + SPI_SR) & SPISR_DONE)==0);
    spi_start_stop(hw,0,8);
}

static void spi_send_bits(struct silan_spi *hw, unsigned int data, unsigned char bits)
{
    writel(data, hw->regs + SPI_TR_PIO);
    set_tr_mode(hw,SPI_TRMODE_WONLY);
    spi_start_stop(hw,1,bits);
    while((readl(hw->regs + SPI_SR) & SPISR_XMIT_EMPTY)==0);
    while((readl(hw->regs + SPI_SR) & SPISR_DONE)==0);
    spi_start_stop(hw,0,bits);
}
#endif

#ifdef SPI_FIFO
static void spi_send_fifo(struct silan_spi *hw, struct spi_transfer *t)
{
    int i, k;
    unsigned char *p, *buf;

    buf = (unsigned char *)t->tx_buf;
    writel(readl(hw->regs + SPI_CR) | SPICR_FIFO_RST, hw->regs + SPI_CR);
    writel(readl(hw->regs + SPI_CR) & ~(SPICR_FIFO_RST), hw->regs + SPI_CR);

	set_tr_mode(hw,SPI_TRMODE_WONLY);
    if (hw->len == 1)
    {
        k = *buf << 24;
        while ((readl(hw->regs + SPI_SR) & SPI_TFIFO_FULL));

        writel(k, hw->regs + SPI_TR_DMA);
        spi_start_stop(hw,1,8);
	    while((readl(hw->regs + SPI_SR) & SPI_TFIFO_EMPTY)==0);
	    while((readl(hw->regs + SPI_SR) & SPISR_DONE)==0);
	    spi_start_stop(hw,0,8);
    }
    else
    {
        for (i = 0; i < hw->len/4; i ++)
        {
            p = buf + i*4;
            k = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
            while ((readl(hw->regs + SPI_SR) & SPI_TFIFO_FULL));

            writel(k, hw->regs + SPI_TR_DMA);
            spi_start_stop(hw,1,32);
        }
	    while((readl(hw->regs + SPI_SR) & SPI_TFIFO_EMPTY)==0);
	    while((readl(hw->regs + SPI_SR) & SPISR_DONE)==0);
	    spi_start_stop(hw,0,32);
    }
}
#endif

unsigned char spi_read_status(struct silan_spi *hw)
{
    set_tr_mode(hw, SPI_TRMODE_WR);
    //writel(OPCODE_RDSR<<24, hw->regs + SPI_TR_PIO);
    writel(0x5<<24, hw->regs + SPI_TR_PIO);
    spi_start_stop(hw, 1, 16);
    while((readl(hw->regs + SPI_SR) & SPISR_RCV_FULL)==0);
    spi_start_stop(hw, 0, 16);
    return (readl(hw->regs + SPI_RR_PIO)&0xff);
}

int spi_ctrl_write_data(struct silan_spi *hw, struct spi_transfer *t)
{
    unsigned char *p;
    unsigned int i, data, num;

    data = ((hw->cmd)<<24)|(hw->addr);
    writel(data, hw->regs + SPI_TR_PIO);
    set_tr_mode(hw, SPI_TRMODE_WONLY);
    spi_start_stop(hw, 1, 32);
    num = t->len/4*4;
    while (hw->count < num)
    {
        while ((readl(hw->regs + SPI_SR) & SPISR_XMIT_EMPTY) == 0);
        while ((readl(hw->regs + SPI_SR) & SPISR_DONE) == 0);
        p = (unsigned char *)t->tx_buf;
        data = (*p<<24)|(*(p+1)<<16)|(*(p+2)<<8)|(*(p+3));
        //printk("### data %08x\n", data);
        writel(data, hw->regs + SPI_TR_PIO);
        hw->count += 4;
        t->tx_buf += 4;
    }

    while ((readl(hw->regs + SPI_SR) & SPISR_XMIT_EMPTY) == 0);
    while ((readl(hw->regs + SPI_SR) & SPISR_DONE) == 0);
    spi_start_stop(hw, 1, 8);
    num = t->len%4;
    for (i = 0; i < num; i++)
    {
        while((readl(hw->regs + SPI_SR) & SPISR_XMIT_EMPTY)==0);
        while((readl(hw->regs + SPI_SR) & SPISR_DONE)==0);
        p = (unsigned char *)t->tx_buf;
        data = *p;
        //printk("### data %x\n", data);
        writel(data<<24, hw->regs + SPI_TR_PIO);
        hw->count++;
        t->tx_buf++;
    }

    while ((readl(hw->regs + SPI_SR) & SPISR_XMIT_EMPTY) == 0);
    while((readl(hw->regs + SPI_SR) & SPISR_DONE) == 0);
    spi_start_stop(hw, 0, 32);
    while((spi_read_status(hw)&0x1) == 1);
#if 0
#ifdef SPI_REG
    int i;
    unsigned int k;
    unsigned char *p;

    if (hw->len == 1)
        spi_send_byte(hw, *(unsigned char*)t->tx_buf);
    else
    {
        for (i = 0; i < hw->len/4; i ++)
        {
            p = (unsigned char *)(t->tx_buf + i*4);
            k = *p << 24 | *(p + 1) << 16 | *(p + 2) << 8 | *(p + 3);
            spi_send_bits(hw, k, 32);
        }
    }
#else
    spi_send_fifo(hw, t);
#endif
#endif

    return hw->len;
}

int spi_ctrl_read_data(struct silan_spi *hw, struct spi_transfer *t)
{
    unsigned char *p;
    int i, data, val;
    int spi_base = hw->regs;

    data = ((hw->cmd)<<24)|(hw->addr);
    p = (unsigned char *)t->rx_buf;
    writel(data, spi_base + SPI_TR_PIO);
    set_tr_mode(hw, SPI_TRMODE_RONLY);
    spi_start_stop(hw, 1, 32);
    while ((readl(spi_base + SPI_SR) & SPISR_RCV_FULL) == 0);
    writel(0xffffffff, spi_base + SPI_TR_PIO);
    val = readl(spi_base + SPI_RR_PIO);
    while (hw->count < t->len)
    {
        while ((readl(spi_base + SPI_SR) & SPISR_RCV_FULL) == 0);
        val = readl(spi_base + SPI_RR_PIO);
        //printk("spi_ctrl_read_data val %x\n", val);
        for (i = 3; i >= 0; i--)
        {
            *p++ = (val&(0xff<<(i*8)))>>(i*8);
            hw->count++;
            if (hw->count >= t->len)
                goto exit;
        }
    }

exit:
    spi_start_stop(hw, 0, 32);
    return 0;
}

static int silan_spi_ctrl_txrx(struct spi_device *spi, struct spi_transfer *t)
{
    unsigned char *p;
    unsigned int tmp, count = 0;
    struct list_head *head;
    struct silan_spi *hw = spi_master_get_devdata(spi->master);

    hw->len = t->len;
    hw->count = 0;
    head = t->transfer_list.prev;
    if (t->tx_buf == NULL) return 0;
    hw->cmd = *(unsigned char *)t->tx_buf;

    t->tx_buf += 2;
    hw->count += 2;
    if (hw->count >= t->len)
    {
        spi_send_byte(hw, hw->cmd);
        *((unsigned char *)t->tx_buf) = 0;
        return hw->count;
    }

    p = (unsigned char *)t->tx_buf;
    hw->addr = (*p<<16) | (*(p + 1)<<8) | (*(p + 2));
    if (t->transfer_list.next == head)
    {
        tmp = ((hw->cmd)<<24)|(hw->addr);
        spi_send_bits(hw, tmp, 32);
        while ((spi_read_status(hw)&0x1) == 1);
        hw->count += 3;
    }

    while (t->transfer_list.next != head)
    {
        t = list_entry(t->transfer_list.next,struct spi_transfer,transfer_list);
        hw->count = 0;
        if (t->rx_buf)
        {
            count = spi_ctrl_read_data(hw, t);
        }

        if (t->tx_buf)
        {
            count = spi_ctrl_write_data(hw, t);
        }
    }

    return hw->count;
}

static int silan_spi_ctrl_setupxfer(struct spi_device *spi,struct spi_transfer *t)
{
    return 0;
}

static void silan_spi_ctrl_chipsel(struct spi_device *spi,int value)
{
    return;
}

static int silan_spi_ctrl_setup(struct spi_device *spi)
{
    return 0;
}

static int __init silan_spi_ctrl_probe(struct platform_device *pdev)
{
    int err;
    struct silan_spi *hw;
    struct spi_master *master;
    struct resource *res;

    printk("silan_spi_ctrl_probe start\n");
    silan_padmux_ctrl(SILAN_PADMUX_I2C1, PAD_OFF);
    silan_padmux_ctrl(SILAN_PADMUX_SPI, PAD_ON);
    silan_padmux_ctrl(SILAN_PADMUX_SPI_EX, PAD_ON);

    master = spi_alloc_master(&pdev->dev, sizeof(struct silan_spi));
    if (master == NULL)
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

    master->num_chipselect = 1;
    master->bus_num = 2;

    hw->bitbang.master         = hw->master;
    hw->bitbang.setup_transfer = silan_spi_ctrl_setupxfer;
    hw->bitbang.chipselect     = silan_spi_ctrl_chipsel;
    hw->bitbang.txrx_bufs      = silan_spi_ctrl_txrx;
    hw->bitbang.master->setup  = silan_spi_ctrl_setup;

    hw->clk = clk_get(&pdev->dev,"spi");
    if (!hw->clk)
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

    hw->ioarea = request_mem_region(res->start, (res->end - res->start) + 1, pdev->name);

    if (hw->ioarea == NULL)
    {
        printk(KERN_ERR"Cannot reserve region\n");
        err = -ENXIO;
        goto err_no_iores;
    }

    hw->regs = ioremap(res->start, (res->end - res->start)+1);
    if (hw->regs == NULL)
    {
        printk(KERN_ERR"Cannot map IO\n");
        err = -ENXIO;
        goto err_no_iomap;
    }

    hw->irq = platform_get_irq(pdev, 0);
    if (hw->irq < 0)
    {
        printk(KERN_ERR"No IRQ specified\n");
        err = -ENOENT;
        goto err_no_irq;
    }

    init_spi(hw, 8);

    err = spi_bitbang_start(&hw->bitbang);
    if (err)
    {
        printk(KERN_ERR"Failed to register SPI master\n");
        goto err_no_irq;
    }
    printk("silan_spi_ctrl_probe done\n");

err_no_irq:
    iounmap(hw->regs);

err_no_iomap:
    release_resource(hw->ioarea);
    kfree(hw->ioarea);

err_no_iores:
    spi_master_put(hw->master);

    return 0;
}

static int silan_spi_ctrl_remove(struct platform_device *dev)
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


static struct platform_driver silan_spi_ctrl_driver = {
    .remove		= silan_spi_ctrl_remove,
    .driver		= {
        .name	= "silan-spictrl",
        .owner	= THIS_MODULE,
    },
};
static int __init silan_spi_ctrl_init(void)
{
    return platform_driver_probe(&silan_spi_ctrl_driver, silan_spi_ctrl_probe);
}

static void __exit silan_spi_ctrl_exit(void)
{
    platform_driver_unregister(&silan_spi_ctrl_driver);
}

module_init(silan_spi_ctrl_init);
module_exit(silan_spi_ctrl_exit);

MODULE_DESCRIPTION("SILAN SPI Driver");
MODULE_LICENSE("GPL");

