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
#include <linux/bcd.h>
#include <asm/io.h>

#define RTC_REG_OFFSET (0x40)

#define RTC_TIMER      ((0x0  + RTC_REG_OFFSET) << 2)
#define RTC_TMCON      ((0x1  + RTC_REG_OFFSET) << 2)
#define RTC_CLKOUT     ((0x2  + RTC_REG_OFFSET) << 2)
#define RTC_WEEK_ALARM ((0x3  + RTC_REG_OFFSET) << 2)
#define RTC_DAY_ALARM  ((0x4  + RTC_REG_OFFSET) << 2)
#define RTC_HOUR_ALARM ((0x5  + RTC_REG_OFFSET) << 2)
#define RTC_MIN_ALARM  ((0x6  + RTC_REG_OFFSET) << 2)
#define RTC_YEARL      ((0x7  + RTC_REG_OFFSET) << 2)
#define RTC_MON        ((0x8  + RTC_REG_OFFSET) << 2)
#define RTC_WEEK       ((0X9  + RTC_REG_OFFSET) << 2)
#define RTC_DAY        ((0xA  + RTC_REG_OFFSET) << 2)
#define RTC_HOUR       ((0xB  + RTC_REG_OFFSET) << 2)
#define RTC_MIN        ((0xC  + RTC_REG_OFFSET) << 2)
#define RTC_SEC        ((0xD  + RTC_REG_OFFSET) << 2)
#define RTC_YEARH      ((0xE  + RTC_REG_OFFSET) << 2)
#define RTC_CS         ((0xF  + RTC_REG_OFFSET) << 2)
#define RTC_CTRL       ((0x10 + RTC_REG_OFFSET) << 2)
#define RTC_SEC_COUNTH ((0x11 + RTC_REG_OFFSET) << 2)
#define RTC_SEC_COUNTL ((0x12 + RTC_REG_OFFSET) << 2)

#define RTC_SRAM       0X400

#define IRQ_TIMER      0x04
#define IRQ_TIMER_ENB  0x11
#define IRQ_ALARM      0x08
#define IRQ_ALARM_ENB  0x02

#define RTC_WR_DATA 50
#define RTC_SET_PERIODIC_TIME 51

#define RTC_DEV_NAME "silan-rtc"

struct silan_rtc
{
    int irq_num;
    char *name;
    unsigned char *rtc_reg;
    struct clk *clk;
    struct rtc_device *rtcdev;
    struct resource *ioarea;
};

static int silan_rtc_write_data(struct device *dev, unsigned char *buf);

static irqreturn_t silan_rtc_interrupt(int irq, void *_rtc)
{
    struct silan_rtc *rtc = _rtc;
    unsigned long events = 0, rtc_irq;

    printk("silan_rtc_interrupt \n");
    rtc_irq = readl(rtc->rtc_reg + RTC_CS);

    if (rtc_irq & IRQ_ALARM)
    {
        printk("silan_rtc_interrupt IRQ_ALARM\n");
        rtc_irq &= ~IRQ_ALARM;
        writel(rtc_irq, rtc->rtc_reg + RTC_CS);
        events |= RTC_AF | RTC_IRQF;
    }

    if (rtc_irq & IRQ_TIMER)
    {
        printk("silan_rtc_interrupt IRQ_TIMER\n");
        rtc_irq &= ~IRQ_TIMER;
        writel(rtc_irq, rtc->rtc_reg + RTC_CS);
        events |= RTC_UF | RTC_IRQF;
    }

    rtc_update_irq(rtc->rtcdev, 1, events);

    return IRQ_HANDLED;
}

static int silan_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
    struct silan_rtc *rtc = dev_get_drvdata(dev);
    unsigned int dat;

    dat = readl(rtc->rtc_reg + RTC_CS);
    if (enabled)
        dat |= IRQ_ALARM_ENB;
    else
        dat &= ~IRQ_ALARM_ENB;

    writel(dat, rtc->rtc_reg + RTC_CS);

    return 0;
}

static int silan_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
    struct silan_rtc *rtc = dev_get_drvdata(dev);

    alarm->time.tm_wday = bcd2bin(readl(rtc->rtc_reg + RTC_WEEK_ALARM));
    alarm->time.tm_mday = bcd2bin(readl(rtc->rtc_reg + RTC_DAY_ALARM));
    alarm->time.tm_hour = bcd2bin(readl(rtc->rtc_reg + RTC_HOUR_ALARM));
    alarm->time.tm_min = bcd2bin(readl(rtc->rtc_reg + RTC_MIN_ALARM));
    printk("silan_rtc_read_alarm wk %d, day %d, hour %d, min %d\n", alarm->time.tm_wday, alarm->time.tm_mday, alarm->time.tm_hour, alarm->time.tm_min);

    return 0;
}

static int silan_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
    struct silan_rtc *rtc = dev_get_drvdata(dev);

    printk("silan_rtc_set_alarm enable %d\n", alarm->enabled);
    if (!alarm->enabled)
    {
        silan_alarm_irq_enable(dev, 0);
        return 0;
    }

    writel(bin2bcd(alarm->time.tm_wday), rtc->rtc_reg + RTC_WEEK_ALARM);
    writel(bin2bcd(alarm->time.tm_mday), rtc->rtc_reg + RTC_DAY_ALARM);
    writel(bin2bcd(alarm->time.tm_hour), rtc->rtc_reg + RTC_HOUR_ALARM);
    writel(bin2bcd(alarm->time.tm_min), rtc->rtc_reg + RTC_MIN_ALARM);
    printk("silan_rtc_set_alarm wk %d, day %d, hour %d, min %d\n", alarm->time.tm_wday, alarm->time.tm_mday, alarm->time.tm_hour, alarm->time.tm_min);

    silan_alarm_irq_enable(dev, 1);

    return 0;
}

static int silan_periodic_irq_enable(struct device *dev, unsigned int enabled)
{
    struct silan_rtc *rtc = dev_get_drvdata(dev);
    unsigned int dat;

    dat = readl(rtc->rtc_reg + RTC_CS);
    if (enabled)
        dat |= IRQ_TIMER_ENB;
    else
        dat &= ~IRQ_TIMER_ENB;

    writel(dat, rtc->rtc_reg + RTC_CS);

    return 0;
}

static int silan_rtc_set_periodic_time(struct device *dev, int time)
{
    struct silan_rtc *rtc = dev_get_drvdata(dev);

    printk("silan_rtc_set_periodic_time sec %d\n", time);
    if (time > 0 && time <= 255)
    {
        writel(time - 1, rtc->rtc_reg + RTC_TIMER);
        writel(0x06, rtc->rtc_reg + RTC_TMCON);
        silan_periodic_irq_enable(dev, 1);
    }
    else if (time > 255 && time < 255*60)
    {
        writel(time/60 - 1, rtc->rtc_reg + RTC_TIMER);
        writel(0x07, rtc->rtc_reg + RTC_TMCON);
        silan_periodic_irq_enable(dev, 1);
    }
    else
        printk("silan_rtc_set_periodic_time failed \n");

    return 0;
}

static int silan_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
    struct silan_rtc *rtc = dev_get_drvdata(dev);

    tm->tm_sec = bcd2bin(readl(rtc->rtc_reg + RTC_SEC));
    tm->tm_min = bcd2bin(readl(rtc->rtc_reg + RTC_MIN));
    tm->tm_hour = bcd2bin(readl(rtc->rtc_reg + RTC_HOUR));
    tm->tm_mday = bcd2bin(readl(rtc->rtc_reg + RTC_DAY));
    tm->tm_mon = bcd2bin(readl(rtc->rtc_reg + RTC_MON));
    tm->tm_year = (bcd2bin(readl(rtc->rtc_reg + RTC_YEARH)))*100 + bcd2bin(readl(rtc->rtc_reg + RTC_YEARL));
    tm->tm_wday = bcd2bin(readl(rtc->rtc_reg + RTC_WEEK));

    printk("silan_rtc_read_time %04d.%02d.%02d %02d:%02d:%02d %d\n", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_wday);

    return 0;
}

static int silan_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
    struct silan_rtc *rtc = dev_get_drvdata(dev);

    printk("silan_rtc_set_time %04d.%02d.%02d %02d:%02d:%02d\n", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    writel(bin2bcd(tm->tm_sec), rtc->rtc_reg + RTC_SEC);
    writel(bin2bcd(tm->tm_min), rtc->rtc_reg + RTC_MIN);
    writel(bin2bcd(tm->tm_hour), rtc->rtc_reg + RTC_HOUR);
    writel(bin2bcd(tm->tm_mday), rtc->rtc_reg + RTC_DAY);
    writel(bin2bcd(tm->tm_wday), rtc->rtc_reg + RTC_WEEK);
    writel(bin2bcd(tm->tm_mon), rtc->rtc_reg + RTC_MON);
    writel(bin2bcd(tm->tm_year/100), rtc->rtc_reg + RTC_YEARH);
    writel(bin2bcd(tm->tm_year%100), rtc->rtc_reg + RTC_YEARL);

#if 0
    struct rtc_wkalrm alarm;

    alarm.enabled = 1;
    alarm.time.tm_wday = tm->tm_wday;
    alarm.time.tm_mday = tm->tm_mday;
    alarm.time.tm_hour = tm->tm_hour;
    alarm.time.tm_min = tm->tm_min + 1;
    silan_rtc_set_alarm(dev, &alarm);

    silan_rtc_set_periodic_time(dev, 10);

    //unsigned char *buf = NULL;
    //silan_rtc_write_data(dev, buf);
#endif

    return 0;
}

static int silan_rtc_open(struct device *dev)
{
    return 0;
}

static int silan_rtc_write_data(struct device *dev, unsigned char *buf)
{
    int i;
    unsigned char data[256];
    struct silan_rtc *rtc = dev_get_drvdata(dev);

    for (i = 0; i < 256; i ++)
        data[i] = i;

    for (i = 0; i < 256; i ++)
    {
        writel(data[i], rtc->rtc_reg + RTC_SRAM + i*4);
        printk("@@@@@ i %d %x\n", i, data[i]);
    }

    for (i = 0; i < 256; i ++)
    {
        data[i] = readl(rtc->rtc_reg + RTC_SRAM + i*4);
        printk("##### i %d %x\n", i, data[i]);
    }

    return 0;
}

static int silan_rtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
        case RTC_WR_DATA:
            silan_rtc_write_data(dev, (unsigned char *)arg);
            break;

        case RTC_SET_PERIODIC_TIME:
            silan_rtc_set_periodic_time(dev, arg);
            break;

        default:
            break;
    }

    return 0;
}

static struct rtc_class_ops silan_rtc_ops = {
    .open = silan_rtc_open,
    .ioctl = silan_rtc_ioctl,
    .read_time = silan_rtc_read_time,
    .set_time = silan_rtc_set_time,
    .read_alarm = silan_rtc_read_alarm,
    .set_alarm = silan_rtc_set_alarm,
    .alarm_irq_enable = silan_alarm_irq_enable,
};

static int silan_rtc_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct silan_rtc *rtc;
    int err = 0;

    printk("silan_rtc_probe start \n");
    rtc = kzalloc(sizeof(struct silan_rtc), GFP_KERNEL);
    if (!rtc)
    {
        printk("silan_rtc_probe kzalloc failed\n");
        return -ENOMEM;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
    {
        printk("silan_rtc_probe platform_get_resource failed\n");
        err = -ENXIO;
        goto fail1;
    }

    if (!request_mem_region(res->start, resource_size(res), pdev->name))
    {
        printk("silan_rtc_probe request_mem_region failed\n");
        err = -EBUSY;
        goto fail1;
    }

    rtc->rtc_reg = ioremap(res->start, resource_size(res));
    if (!rtc->rtc_reg)
    {
        printk("silan_rtc_probe ioremap rtc_reg failed\n");
        err = -ENOMEM;
        goto fail2;
    }

    platform_set_drvdata(pdev, rtc);

    rtc->rtcdev = rtc_device_register(pdev->name, &pdev->dev, &silan_rtc_ops, THIS_MODULE);
    if (IS_ERR(rtc->rtcdev))
    {
        printk("silan_rtc_probe device register failed\n");
        err = -EBUSY;
        goto fail3;
    }

    rtc->irq_num = platform_get_irq(pdev, 0);
    if (request_irq(rtc->irq_num, silan_rtc_interrupt, IRQF_DISABLED, RTC_DEV_NAME, rtc))
    {
        printk("silan_rtc_probe request irq failed\n");
        err = -EBUSY;
        goto fail4;
    }

    printk("silan_rtc_probe end \n");
    return 0;

fail4: rtc_device_unregister(rtc->rtcdev);
fail3: iounmap(rtc->rtc_reg);
fail2: release_mem_region(res->start, resource_size(res));
fail1: kfree(rtc);

    return err;
}

static int silan_rtc_remove(struct platform_device *pdev)
{
    struct silan_rtc *rtc = platform_get_drvdata(pdev);
    struct resource *res;

    free_irq(rtc->irq_num, rtc);
    rtc_device_unregister(rtc->rtcdev);
    iounmap(rtc->rtc_reg);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    release_mem_region(res->start, resource_size(res));

    kfree(rtc);

    platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct platform_driver rtc_driver = 
{
    .probe	= silan_rtc_probe,
    .remove = silan_rtc_remove,
    .driver = {
        .name = RTC_DEV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init silan_rtc_init(void)
{
    int ret;
    printk("silan_rtc_init! \n");
    ret = platform_driver_register(&rtc_driver);
    if (ret)
        printk("silan_rtc_init failed \n");
    else
        printk("silan_rtc_init successful! \n");

    return ret;
}

static void __exit silan_rtc_exit(void)
{
    printk("silan rtc_exit \n");
    platform_driver_unregister(&rtc_driver);
}

module_init(silan_rtc_init);
module_exit(silan_rtc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SILAN_RTC driver");




