#include <linux/types.h>
#include <linux/wait.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/init.h>
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
#include "asm/mach-silan/dlna/silan_unicom.h"


/* The main structure of the device */
struct rtc_dev {
    /* General info */
    u32 major;
    u32 minor;
    u32 count;
    int rtc_irq;
    char *name;
    void __iomem *ioaddr;
    struct clk *clk;
    struct class *dev_class;
    struct device *device;
 //   struct cdev cdev;

    struct mutex mutex;

	struct silan_unicom_cxc *pUnicom;
}rtc_dev;


#define RTC_CHAR_MAJOR      0
#define RTC_DEV_NAME        "unicom_cxc_rtc"
/* number of bare RTC devices */
#define RTC_DEV_NUM         1

#ifdef RTC_DEBUG
u32 log_levels = LOG_LEVELS_DEVELOP;
#endif

static wait_queue_head_t wait;
int flag = 0;
int time_get_enable = 0;

static int unicom_cxc_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned int read_data[2];
	printk("unicom_cxc_rtc_read_time from mcu\n");
	UNICOM_CXC_CMD_ST rtc = {0};
//	rtc.unicom_cxc_cmd =  UNICOM_CXC_GET_RTC;
	
	if(time_get_enable)
	{
	silan_unicom_cxc_get_time();
	
	printk("wait interrupt from mcu.......\n");
	if(!wait_event_interruptible_timeout(wait, flag != 0, msecs_to_jiffies(30000)))
	{
		printk(KERN_ERR "FDIF read timeout\n");
		return -1;
	}
	printk("start read time from mcu\n");
	rtc_dev.pUnicom->ops->fifo_read(rtc_dev.pUnicom , read_data,UNICOM_CXC_MODULE_RTC, 0, 2);
	
	rtc.u.rtc.tm_sec = read_data[0]&0x000000ff;
	rtc.u.rtc.tm_min = (read_data[0]>>8)&0x000000ff;
	rtc.u.rtc.tm_hour = (read_data[0]>>16)&0x000000ff;
	rtc.u.rtc.tm_mday = (read_data[0]>>24)&0x000000ff;
	rtc.u.rtc.tm_mon = read_data[1]&0x000000ff;
	rtc.u.rtc.tm_year = (((read_data[1]>>16)&0x000000ff))*100
							+((read_data[1]>>8)&0x000000ff);

	printk("read time %04d.%02d.%02d %02d:%02d:%02d\n",
			rtc.u.rtc.tm_year,rtc.u.rtc.tm_mon,rtc.u.rtc.tm_mday,
		rtc.u.rtc.tm_hour,rtc.u.rtc.tm_min,rtc.u.rtc.tm_sec);
	}
	return 0;
}

static irqreturn_t rtc_interrupt(int rtc_irq, void *dev_id)
{
	flag = 1;
	cxc_set_mcu2unicom_rtc_clr();	
	wake_up(&wait);
	return 0;
}

static int unicom_cxc_rtc_open(struct device *dev)
{
	int ret;
	ret = request_irq(rtc_dev.pUnicom->rtc_irq, rtc_interrupt, 0, "rtc-dev", 0);
	printk("rtc_numirq = :%d\n",rtc_dev.pUnicom->rtc_irq);
	if (unlikely(ret < 0)) {
		printk("Allocating the IRQ %d (error: %d)\n",
		       rtc_dev.pUnicom->rtc_irq, ret);
	}
	time_get_enable = 1;
	return 0;
}

static int unicom_cxc_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned int write_data[2];
	UNICOM_CXC_CMD_ST rtc = {0};
	rtc.u.rtc = *(struct rtc_time *)tm;

//	rtc.unicom_cxc_cmd =  UNICOM_CXC_SET_RTC;
//	rtc.unicom_module_id = UNICOM_CXC_MODULE_RTC;
	silan_unicom_cxc_set_time();

	printk("read time %04d.%02d.%02d %02d:%02d:%02d\n",
			1900+rtc.u.rtc.tm_year,1+rtc.u.rtc.tm_mon,rtc.u.rtc.tm_mday,
					rtc.u.rtc.tm_hour,rtc.u.rtc.tm_min,rtc.u.rtc.tm_sec);

	write_data[1] = ((19&0x000000ff)<<16)+((rtc.u.rtc.tm_year&0x0000ffff)<<8)+
					((1+rtc.u.rtc.tm_mon)&0x000000ff);
	write_data[0] = ((rtc.u.rtc.tm_mday&0x000000ff)<<24)+((rtc.u.rtc.tm_hour&0x000000ff)<<16)
					+((rtc.u.rtc.tm_min&0x000000ff)<<8)+(rtc.u.rtc.tm_sec&0x000000ff);
	rtc_dev.pUnicom->ops->fifo_write(rtc_dev.pUnicom, write_data, UNICOM_CXC_MODULE_RTC, 0, 2);

	cxc_set_mips2unicom_rtc_int();
	return 0;
}

static struct rtc_class_ops rtc_fops = {
	.open = unicom_cxc_rtc_open,
	.read_time = unicom_cxc_rtc_read_time,
	.set_time = unicom_cxc_rtc_set_time
};

/*
 * rtc_drv_probe
 * @pdev: platform device pointer
 * Description: the driver is initialized through platform_device.
 */
static int rtc_probe(struct platform_device* pdev)
{

	struct rtc_device *rtc ; 
//	struct resource *res;
	int ret = 0;
	pr_debug("%s: probe=%p\n", __func__, pdev);
	
	silan_unicom_cxc_probe(pdev);

    printk("unicom_cxc probe successful \n");
	rtc_dev.pUnicom = silan_unicom_cxc_get("silan_unicom_cxc");	
    printk("unicom_cxc_probe ok and get_unicom_cxc \n");

	init_waitqueue_head(&wait);

	/* register RTC and exit */
#if 1
	rtc = rtc_device_register("unicom_cxc", &pdev->dev, &rtc_fops,
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

static int rtc_remove(struct platform_device* device)
{
    struct rtc_dev *rtc;

	printk(" rtc_device_remove\n");
    /* Release resource for the device */
    free_irq(rtc_dev.pUnicom->rtc_irq, rtc);
	rtc_device_unregister(rtc);
//	platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct platform_driver rtc_driver = 
{
    .probe	= rtc_probe,
    .remove = rtc_remove,
    .driver = {
		.name = RTC_DEV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init unicom_cxc_rtc_init(void)
{
    int ret;
    printk("unicom_cxc_rtc_init! \n");
    ret = platform_driver_register(&rtc_driver);
    if (ret)
        printk("unicom_cxc_rtc_init failed \n");
    else
        printk("unicom_cxc_rtc_init successful! \n");

    return ret;
}

static void __exit unicom_cxc_rtc_exit(void)
{
    printk("rtc_exit \n");
    platform_driver_unregister(&rtc_driver);
}

module_init(unicom_cxc_rtc_init);
module_exit(unicom_cxc_rtc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chenwangxin");
MODULE_DESCRIPTION("UNICOM_CXC_RTC driver");




