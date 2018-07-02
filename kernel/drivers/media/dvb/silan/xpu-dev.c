/*
 * XPUDTV driver
 *
 * Copyright (C) 2012 panjianguang <panjianguang@silan.com.cn>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation; either version 2 of
 *    the License, or (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/clk.h>

#include <silan_irq.h>
#include <silan_generic.h>
#include "xpudtv.h"

inline void recv_taskq(unsigned long arg)
{
    xpu_avc_recv((struct xpudtv *)arg);
}

static irqreturn_t xpu_handler_irq(int irq, void *dev_id)
{
    struct xpudtv *xdtv = (struct xpudtv*)dev_id;

    /*clear irq*/
    writel(0xffffffff, xdtv->base_addr + CXC_XPU_INT_CLR);
    tasklet_schedule(&xdtv->recv_taskq);
    return IRQ_HANDLED; 
}

static int xpu_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = -1;
    struct xpudtv *xdtv;

    xdtv = kzalloc(sizeof(*xdtv), GFP_KERNEL);
    if (!xdtv)
        return -ENOMEM;

    dev_set_drvdata(&client->dev, xdtv);
    xdtv->device = &client->dev;

    mutex_init(&xdtv->avc_mutex);
    init_waitqueue_head(&xdtv->avc_wait);
    mutex_init(&xdtv->demux_mutex);

    xdtv->i2c = client->adapter;

    xdtv->clk = clk_get(xdtv->device, "xpu");
    if (xdtv->clk == NULL)
    {
        printk(KERN_ERR "xpu failed to get clk!\n");
        goto err1;
    }

    xdtv->base_addr = ioremap(SILAN_XPU_CXC_PHY_BASE, SILAN_XPU_CXC_SIZE);
    if(xdtv->base_addr == NULL) {
        printk(KERN_ERR "xpu failed to ioremap\n");
        goto err1;
    }

    xdtv->cxc_irq = PIC_IRQ_DRM; //MIPS_XPU_CXC_IRQ;// SILAN_XPU_IRQ;
    ret = request_irq(xdtv->cxc_irq, xpu_handler_irq, 0, "silan_xpu", xdtv);
    if (unlikely(ret))
    {
        printk(KERN_ERR "xpu failed to register interrupt\n");
        goto err2;
    }

    ret = xpu_dvb_register(xdtv);
    if (unlikely(ret))
    {
        printk(KERN_ERR "xpu failed to register dvb\n");
        goto err3;
    }

    tasklet_init(&xdtv->recv_taskq, recv_taskq, (unsigned long)xdtv);
    printk(KERN_ALERT "SILAN XPU Driver probe done.\n");
    return 0;

err3:
    free_irq(xdtv->cxc_irq,xdtv);
err2:
    iounmap(xdtv->base_addr);
err1:
    kfree(xdtv);
    return ret;    
}

static int xpu_remove(struct i2c_client *client)
{
    struct xpudtv *xdtv = dev_get_drvdata(&client->dev);    
    if (!xdtv)
        return -ENOMEM;
    xpu_dvb_unregister(xdtv);
    free_irq(xdtv->cxc_irq,xdtv);
    iounmap(xdtv->base_addr);
    return 0;
}

static int xpu_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret;
    struct xpudtv *xdtv = dev_get_drvdata(&client->dev);
#ifdef CONFIG_MIPS_SILAN_SUVIII
    volatile int tmp;
#endif
    if (!xdtv)
        return -ENOMEM;
    ret = xpu_stop_streaming(xdtv);
    if (ret)
    {
        printk(KERN_ERR "%s error! errorcode: %d\n", __func__, ret);
        return ret;
    }
#ifdef CONFIG_MIPS_SILAN_SUVIII
    do
    {
        tmp = sl_readl(SILAN_XPU_STATUS);
        tmp = tmp & 0x4;
    }while(tmp);

    clk_disable(xdtv->clk);
#endif
    return 0;
}

static int xpu_resume(struct i2c_client *client)
{
    int ret;
    struct xpudtv *xdtv = dev_get_drvdata(&client->dev);
    if (!xdtv)
        return -ENOMEM;
#ifdef CONFIG_MIPS_SILAN_SUVIII
    clk_enable(xdtv->clk);
#endif
    ret = xpu_start_streaming(xdtv);
    if (ret)
    {
        printk(KERN_ERR "%s error! errorcode: %d\n", __func__, ret);
        return ret;
    }
    return 0;
}

static const struct i2c_device_id xpu_id[] = 
{
    {"silan_xpu", 0},
};

static struct i2c_driver xpu_driver = 
{
    .probe    = xpu_probe,
    .remove   = xpu_remove,
    .suspend  = xpu_suspend,
    .resume   = xpu_resume,

    .id_table = xpu_id,
    .driver   = {
        .name = "silan_xpu",
        .owner= THIS_MODULE,
    },
};

static int __devinit xpu_init(void)
{
    int ret;
    printk(KERN_ALERT "SILAN XPU Driver init.\n");
    ret = i2c_add_driver(&xpu_driver);
    return ret;
}

static void __exit xpu_exit(void)
{
    printk(KERN_ALERT "SILAN XPU Driver exit.\n");
    i2c_del_driver(&xpu_driver);
}

late_initcall(xpu_init);
module_exit(xpu_exit);

MODULE_DESCRIPTION("SILAN XPU Driver");
MODULE_LICENSE("GPL");
