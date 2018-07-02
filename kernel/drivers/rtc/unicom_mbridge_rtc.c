#include <linux/types.h>
#include <linux/wait.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/rtc.h>
#include <linux/string.h>
#include <linux/log2.h>
#include <linux/init.h>
#include <silan_def.h>
#include "asm/mach-silan/dlna/silan_unicom.h"

/* The main structure of the device */
struct rtc_dev {
    /* General info */
    u32 major;
    u32 minor;
    u32 count;
    int irq;
    char *name;
    void __iomem *ioaddr;
    struct clk *clk;
    struct class *dev_class;
    struct device *device;
 //   struct cdev cdev;
	u32 *framebuf_dma;
	u32 *vir_mbridge_addr;
    struct mutex mutex;

	struct  silan_unicom_cxc *pUnicom;
}rtc_dev;


#define RTC_CHAR_MAJOR      0
#define RTC_DEV_NAME        "unicom_cxc"
/* number of bare RTC devices */
#define RTC_DEV_NUM         1
#define MBRIDGE_ADDR_SIZE  	16

static wait_queue_head_t wait;
int flag = 0;
int time_get_enable = 0;

static int unicom_mbridge_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	
	if(time_get_enable)
	{
		printk("wait interrupt from mcu.......\n");
		if(!wait_event_interruptible_timeout(wait, flag != 0, msecs_to_jiffies(30000)))
		{
			printk(KERN_ERR "FDIF read timeout\n");
			return -1;
		}
		printk("start read time from mcu\n");
		
//		*(struct rtc_time *)tm =*(struct rtc_time *)(void*)rtc_dev.vir_mbridge_addr;
		
		printk("%#x%#x-%#x-%#x %#x %#x %#x\n", *(unsigned char *)rtc_dev.vir_mbridge_addr,*((unsigned char *)rtc_dev.vir_mbridge_addr+6),
				*((unsigned char *)rtc_dev.vir_mbridge_addr+5),*((unsigned char *)rtc_dev.vir_mbridge_addr+4),
				*((unsigned char *)rtc_dev.vir_mbridge_addr+3),*((unsigned char *)rtc_dev.vir_mbridge_addr+2),
														*((unsigned char *)rtc_dev.vir_mbridge_addr+1));
		printk("###################################\n");
//		printk("read time %04d.%02d.%02d %02d:%02d:%02d\n",
//			1900+tm->tm_year,1+tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	}
	return 0;
}

static int unicom_mbridge_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	
	UNICOM_CXC_CMD_ST rtc = {0};
	rtc.u.framebuf_dma = rtc_dev.framebuf_dma;
	printk("framebuf_dma1=:%#x\n",rtc_dev.framebuf_dma);
	printk("framebuf_dma2=:%#x\n",rtc.u.framebuf_dma);
	printk("mbridge_rtc open......\n");
	rtc_dev.pUnicom->ops->fifo_write(rtc_dev.pUnicom, &rtc, UNICOM_CXC_MODULE_RTC);
//	cxc_set_mips2unicom_int();
//	*(struct rtc_time *)(void*)rtc_dev.vir_mbridge_addr = *(struct rtc_time *)tm;
	cxc_set_mips2unicom_int();

	return 0;
}

static irqreturn_t rtc_interrupt(int irq, void *dev_id)
{
	flag = 1;

//	printk("rtc_interrupt come\n");
	cxc_set_mcu2unicom_clr();	
	wake_up(&wait);
//	printk("rtc_interrupt over\n");
	return 0;
}

static int unicom_mbridge_rtc_open(struct device *dev)
{
	int ret;
/*
	UNICOM_CXC_CMD_ST rtc = {0};
	rtc.u.framebuf_dma = rtc_dev.framebuf_dma;
	printk("framebuf_dma1=:%#x\n",rtc_dev.framebuf_dma);
	printk("framebuf_dma2=:%#x\n",rtc.u.framebuf_dma);
	printk("mbridge_rtc open......\n");
	rtc_dev.pUnicom->ops->fifo_write(rtc_dev.pUnicom, &rtc, UNICOM_CXC_MODULE_RTC);
	
	cxc_set_mips2unicom_int();
*/
	printk("numirq = :%d\n",rtc_dev.pUnicom->irq);
    //printk("numirq = :%d\n",silan_unicom_cxc_get("silan_unicom_cxc")->irq);
	ret = request_irq(rtc_dev.pUnicom->irq, rtc_interrupt, 0, "rtc-dev", 0);
	if (unlikely(ret < 0)) {
		printk("Allocating the IRQ %d (error: %d)\n",
		       rtc_dev.pUnicom->irq, ret);
	}
	else
	{
		printk("request_irq successful\n");
	}
	time_get_enable = 1;
	return 0;
}

static struct rtc_class_ops unicom_mbridge_rtc_fops = {
	.open = unicom_mbridge_rtc_open,
	.read_time = unicom_mbridge_rtc_read_time,
	.set_time = unicom_mbridge_rtc_set_time
};

/*
 * rtc_drv_probe
 * @pdev: platform device pointer
 * Description: the driver is initialized through platform_device.
 */
static int unicom_mbridge_rtc_probe(struct platform_device* pdev)
{

	struct rtc_device *rtc ; 
//	struct resource *res;
	int ret;
	int size;

	pr_debug("%s: probe=%p\n", __func__, pdev);
	
	silan_unicom_cxc_probe(pdev);

    printk("----unicom_cxc probe successful---- \n");
	rtc_dev.pUnicom = silan_unicom_cxc_get("silan_unicom_cxc");	
    printk("----unicom_cxc_probe ok and get_unicom_cxc---- \n");
	
	//silan_unicom_mbridge_probe(pdev);

	size =  MBRIDGE_ADDR_SIZE;
	rtc_dev.vir_mbridge_addr = kmalloc(size, GFP_KERNEL);
	if(!rtc_dev.vir_mbridge_addr)
	{
		printk("unicom_mbridge dma buffer malloc failed\n");
        ret = -ENOMEM;
	}
	else
	{
		printk("vir_mbridge_addr=:%#x\n",rtc_dev.vir_mbridge_addr);
	}
	rtc_dev.framebuf_dma = virt_to_phys(rtc_dev.vir_mbridge_addr);

	init_waitqueue_head(&wait);
	/* register RTC and exit */
#if 1
	rtc = rtc_device_register("unicom_cxc_rtc", &pdev->dev, &unicom_mbridge_rtc_fops,
				  THIS_MODULE);

	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc);
	}
	else
	{
		printk(" rtc_device_register ok\n");
	}	
#endif

	platform_set_drvdata(pdev, rtc);
	return ret;

}

static int unicom_mbridge_rtc_remove(struct platform_device* device)
{
    struct rtc_dev *rtc;

    /* Release resource for the device */
  //  free_irq(rtc_dev.pUnicom->irq, rtc);
	rtc_device_unregister(rtc);
//	platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct platform_driver unicom_mbridge_rtc_driver = 
{
    .probe	= unicom_mbridge_rtc_probe,
    .remove = unicom_mbridge_rtc_remove,
    .driver = {
		.name = RTC_DEV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init unicom_mbridge_rtc_init(void)
{
    int ret;
    printk("/**************unicom_mbridge_rtc_init \n");
    ret = platform_driver_register(&unicom_mbridge_rtc_driver);
    if (ret)
        printk("unicom_mbridge_rtc_init failed \n");
    else
        printk("/****************unicom_mbridge_rtc_init successful \n");

    return ret;
}

static void __exit unicom_mbridge_rtc_exit(void)
{
    printk("unicom_mbridge_rtc_exit \n");
    platform_driver_unregister(&unicom_mbridge_rtc_driver);
}

module_init(unicom_mbridge_rtc_init);
module_exit(unicom_mbridge_rtc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chenwangxin");
MODULE_DESCRIPTION("UNICOM_MBRIDGE_RTC driver");




