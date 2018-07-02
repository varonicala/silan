#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
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
#include <linux/gpio.h>
#include <asm/io.h>

#include <linux/sched.h>
#include <linux/version.h>
#include <linux/miscdevice.h>

#define DLNA_GPIO_NAME  "dlna_gpio"

#define GPIO_TOTAL_NUM   64

#define GPIO_MAGIC 'd'

#define GPIO_IOCTL_READ      _IO(GPIO_MAGIC, 1)
#define GPIO_IOCTL_WRITE     _IO(GPIO_MAGIC, 2)
#define GPIO_IOCTL_SET_OUT   _IO(GPIO_MAGIC, 3)
#define GPIO_IOCTL_SET_IN    _IO(GPIO_MAGIC, 4)
#define GPIO_IOCTL_SET_PULL  _IO(GPIO_MAGIC, 5)

#define GPIO1REN  0x54
#define GPIO2REN  0x58
#define SL_GPIO_NR 32
#define SYS_CTRL_BASE SILAN_CR_BASE

enum pull_mode
{
    PULLING_HIGH,
    PULLING_NONE
};

struct gpio_data_t
{
    int num;
    int value;
};

static int dlna_gpio_open(struct inode *inode, struct file *fp)
{
    return 0;
}

static int dlna_gpio_release(struct inode *inode, struct file *fp)
{
    return 0;
}

static ssize_t dlna_gpio_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    return 0;
}

static int value_read(int num)
{
    if(num < 0 || num > GPIO_TOTAL_NUM){
        printk("The gpio num is out of range!\n");
        return -1;
    }
#if 0
	printk("GPIO1 data: %x\n", readl(0xba010000));
	printk("GPIO1 dir: %x\n", readl(0xba010400));
	printk("GPIO2 data: %x\n", readl(0xba020000));
	printk("GPIO2 dir: %x\n", readl(0xba020400));
	printk("#### %s: num: %d \n", __func__, num); 
#endif
    return gpio_get_value(num);    
}

static int value_write(struct gpio_data_t gpio_data)
{
    if(gpio_data.num < 0 || gpio_data.num > GPIO_TOTAL_NUM){
        printk("The gpio num is out of range!\n");
        return -1;
    }

    if(gpio_data.value < 0){
        printk("Invalid value!\n");
        return -1;
    }

    if(gpio_data.value >= 1)
        gpio_data.value = 1;

    gpio_set_value(gpio_data.num, gpio_data.value);
#if 0
	printk("GPIO1 data: %x\n", readl(0xba010000));
	printk("GPIO1 dir: %x\n", readl(0xba010400));
	printk("GPIO2 data: %x\n", readl(0xba020000));
	printk("GPIO2 dir: %x\n", readl(0xba020400));
#endif
	return 0;    
}

static int gpio_set_direction(struct gpio_data_t gpio_data, int flag)
{
	//printk("## %s: value: %d, num: %d ###\n", __func__, gpio_data.value, gpio_data.num);
    if(gpio_data.num < 0 || gpio_data.num > GPIO_TOTAL_NUM){
        printk("The gpio num is out of range!\n");
        return -1;
    }

    if(gpio_data.value < 0){
        printk("Invalid gpio value!\n");
        return -1;
    }

    if(gpio_data.value >= 1)
        gpio_data.value = 1;

    if(flag == 1){
		gpio_request(gpio_data.num, "silan_gpio");
        gpio_direction_output(gpio_data.num, gpio_data.value);
    }
    else{
		gpio_request(gpio_data.num, "silan_gpio");
        gpio_direction_input(gpio_data.num);
    }

    return 0;
}

static int gpio_pull_mode(unsigned int pin, enum pull_mode _pullmode)
{
    int val = 0, offset;
    if((pin < 0 || pin > 63) && ((_pullmode != PULLING_HIGH) || (_pullmode != PULLING_NONE)))
        return -1;
    offset = (pin % SL_GPIO_NR);

	printk("kn gpio_pull_mode %d %d\n", pin, _pullmode);
    if(pin >= 0 && pin <= 31)
    {
        if(_pullmode == PULLING_HIGH)
        {
            val = readl(SYS_CTRL_BASE+GPIO1REN);
            val &= ~(1 << offset);  
            writel(val, SYS_CTRL_BASE+GPIO1REN);
        }
        else
        {
            val = readl(SYS_CTRL_BASE+GPIO1REN);
            val |= (1 << offset);  
            writel(val, SYS_CTRL_BASE+GPIO1REN);
        }
    }
    else if(pin >= 32 && pin <= 63)
    {
        if(_pullmode == PULLING_HIGH)
        {
            val = readl(SYS_CTRL_BASE+GPIO2REN);
            val &= ~(1 << offset);  
            writel(val, SYS_CTRL_BASE+GPIO2REN);
        }
        else
        {
            val = readl(SYS_CTRL_BASE+GPIO2REN);
            val |= (1 << offset);  
            writel(val, SYS_CTRL_BASE+GPIO2REN);
        }
    }
    
    return 0;
}

static long  dlna_gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long argp)
{
    int ret = 0;
    struct gpio_data_t gpio_data;
    
    switch(cmd){
        case GPIO_IOCTL_WRITE:
            if(copy_from_user(&gpio_data, (struct gpio_data_t *)argp, sizeof(gpio_data)))
                 return -EFAULT;
            value_write(gpio_data);
            break;
        case GPIO_IOCTL_SET_IN:
            if(copy_from_user(&gpio_data, (struct gpio_data_t *)argp, sizeof(gpio_data)))
                 return -EFAULT;
            gpio_set_direction(gpio_data, 0);
            break;
        case GPIO_IOCTL_SET_OUT:
            if(copy_from_user(&gpio_data, (struct gpio_data_t *)argp, sizeof(gpio_data)))
                 return -EFAULT;
            gpio_set_direction(gpio_data, 1);
            break;
        case GPIO_IOCTL_READ:
             if(copy_from_user(&gpio_data, (struct gpio_data_t *)argp, sizeof(gpio_data)))
                 return -EFAULT;
            gpio_data.value = value_read(gpio_data.num);
            if(copy_to_user((struct gpio_data_t*)argp, &gpio_data, sizeof(gpio_data)))
                 return -EFAULT;
            break;
        case GPIO_IOCTL_SET_PULL:
            gpio_pull_mode(argp&0xffff, (argp>>16)&0xffff);
            break;
        default:
             break;
    }

    return ret;
}

static const struct file_operations dlna_gpio_ops = {
    .owner   = THIS_MODULE,
    .open    = dlna_gpio_open,
    .release = dlna_gpio_release,
    .read    = dlna_gpio_read,
    .unlocked_ioctl = dlna_gpio_ioctl,
};

static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DLNA_GPIO_NAME,
    .fops  = &dlna_gpio_ops,
};

static __init int dlna_gpio_init(void)
{
    int err;
    if (misc_register(&misc) < 0){
        err = -EBUSY;
        return err;
    }
    
    return 0;
}

static __exit void dlna_gpio_exit(void)
{
    misc_deregister(&misc);
}

module_init(dlna_gpio_init);
module_exit(dlna_gpio_exit);

MODULE_AUTHOR("chenjianneng@silan.com");
MODULE_DESCRIPTION("DLNA GPIO SET driver");
MODULE_LICENSE("GPL");
