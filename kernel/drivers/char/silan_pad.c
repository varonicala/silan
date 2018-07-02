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
#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_def.h>
#include <silan_gpio.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>

#include <linux/sched.h>
#include <linux/version.h>
#include <linux/miscdevice.h>

#include <silan_padmux.h>

#define SILAN_PADMUX_NAME   "silan-padmux"

#define PADMUX_MAGIC 'p'
#define PADMUX_IOCTL_ENABLE           _IO(PADMUX_MAGIC, 1)
#define PADMUX_IOCTL_DISABLE          _IO(PADMUX_MAGIC, 2)
#define PADMUX_IOCTL_CHECK            _IO(PADMUX_MAGIC, 3)

struct padmux_data{
    char name[16];
    int error;
};

static int silan_padmux_open(struct inode *inode,struct file *fp)
{
	return 0;
}

static int silan_padmux_release(struct inode *inode, struct file *fp)
{
	return 0;
}

static long silan_padmux_ioctl(struct file *fp, unsigned int cmd, unsigned long argp)
{
	int res = -1;
    int ret = 0;
    struct padmux_data param;

	switch(cmd) {
		case PADMUX_IOCTL_CHECK:
		    if(copy_from_user(&param,(struct padmux_data *)argp, sizeof(param)))
			    ret = -EFAULT;
            param.error = 0;
            res = silan_pad_check(param.name);
            if(res == -1){
                param.error = -1;
            }
			break;
        case PADMUX_IOCTL_ENABLE:
            if(copy_from_user(&param,(struct padmux_data *)argp, sizeof(param)))
			    ret = -EFAULT;
            param.error = 0;
            //res = silan_pad_check(param.name);
            //if(res == -1){
            //    param.error = -1;
            //    break;
           // }
            res = silan_pad_enable(param.name);
            if(res == -1){
                param.error = -1;
            }
			break;
        case PADMUX_IOCTL_DISABLE:
            if(copy_from_user(&param,(struct padmux_data *)argp, sizeof(param)))
			    ret = -EFAULT;
            param.error = 0;
            res = silan_pad_disable(param.name);
            if(res == -1){
                param.error = -1;
            }
			break;
		default:
			break;
	}
end:
    if(copy_to_user((struct padmux_data*)argp,&param,sizeof(param)))
				return -EFAULT;
	return ret;
}

static const struct file_operations silan_padmux_fops = {
	.owner	= THIS_MODULE,
	.open = silan_padmux_open,
	.release = silan_padmux_release,
	.unlocked_ioctl	= silan_padmux_ioctl,
};

static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = SILAN_PADMUX_NAME,
    .fops  = &silan_padmux_fops,
};

static int silan_padmux_probe(struct platform_device *pdev)
{
    if(misc_register(&misc) < 0){
        printk("Can not register the misc dev\n");
        return -EBUSY;
    }

	return 0;
}

int silan_padmux_remove(struct platform_device *dev)
{
	return 0;
}

struct platform_driver silan_padmux_driver={
	.probe		= silan_padmux_probe,
	.remove		= silan_padmux_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= SILAN_PADMUX_NAME,
	},
};

static int __init silan_padmux_init(void)
{
	if(platform_driver_register(&silan_padmux_driver)){
		printk("register padmux driver error\n");
		platform_driver_unregister(&silan_padmux_driver);
	}
	return 0;
}

static void __exit silan_padmux_exit(void)
{
	platform_driver_unregister(&silan_padmux_driver);
	printk("silan padmux module removed\n");
}

module_init(silan_padmux_init);
module_exit(silan_padmux_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Jianneng");
MODULE_DESCRIPTION("Driver for silan padmux");
