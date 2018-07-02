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
#include <linux/string.h>
#include <linux/log2.h>
#include <linux/init.h>
#include <silan_def.h>
#include <linux/miscdevice.h>
#include "asm/mach-silan/dlna/silan_unicom.h"


#define SPIFLASH_DEV_NAME        "unicom_spiflash"
#define MBRIDGE_ADDR_SIZE		(100*1024)
static DEFINE_SPINLOCK(spiflash_lock);
u32 *flashbuf_dma;
u32 *vir_flash_addr;

//static wait_queue_head_t wait;
//int flag = 0;
//int time_get_enable = 0;
struct silan_unicom_cxc *spiflash;


static int unicom_cxc_spiflash_open(struct inode *inode, struct file *file)
{
	spiflash->ops->fifo_write(spiflash, &flashbuf_dma, UNICOM_CXC_MODULE_SPIFLASH, 3, 4);
	return 0;
}

static int unicom_cxc_spiflash_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int unicom_cxc_spiflash_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
/*	
	if(time_get_enable)
	{
		printk("wait interrupt from mcu.......\n");
		if(!wait_event_interruptible_timeout(wait, flag != 0, msecs_to_jiffies(30000)))
		{
			printk(KERN_ERR "FDIF read timeout\n");
			return -1;
		}
		printk("start read time from mcu\n");
	*/
		memcpy(buf,vir_flash_addr, nbytes);
		return 0;
}
/*
static int unicom_cxc_spiflash_write(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	memcpy(virt_to_phys+1, buf, nbytes);
	cxc_set_mips2unicom_spiflash_int();

	return 0;
}*/
/*
static irqreturn_t spiflash_interrupt(int irq, void *dev_id)
{
	flag = 1;
	cxc_set_mcu2unicom_spiflash_clr();	
	wake_up(&wait);
	return 0;
}
*/

static long  unicom_cxc_spiflash_ioctl(struct file *file, unsigned int cmd, unsigned long argp)
{
	unsigned int spi_addr[1];
	spiflash_data tmp = {0};

	switch(cmd){
		case CXC_SPIFLASH_ADDR0:
			spin_lock(&spiflash_lock);
			spi_addr[0] = CXC_SPIFLASH_ADDR0;
			memcpy(vir_flash_addr, spi_addr, 4);
			printk("%#x\n",spi_addr[0]);
			printk("%#x\n",vir_flash_addr[0]);
			copy_from_user(&tmp, argp,sizeof(spiflash_data));
			copy_from_user(vir_flash_addr+1, tmp.spiflash_data_addr,tmp.spiflash_data_size);
			cxc_set_mips2unicom_spiflash_int();
			spin_unlock(&spiflash_lock);
			break;

		case CXC_SPIFLASH_ADDR1:
			spin_lock(&spiflash_lock);
			spi_addr[0] = CXC_SPIFLASH_ADDR1;
			memcpy(vir_flash_addr, spi_addr, 4);
			copy_from_user(&tmp, argp,sizeof(spiflash_data));
			copy_from_user(vir_flash_addr+1, tmp.spiflash_data_addr,tmp.spiflash_data_size);
			cxc_set_mips2unicom_spiflash_int();
			spin_unlock(&spiflash_lock);
			break;

		case CXC_SPIFLASH_ADDR2:
			spin_lock(&spiflash_lock);
			spi_addr[0] = CXC_SPIFLASH_ADDR2;
			memcpy(vir_flash_addr, spi_addr, 4);
			copy_from_user(&tmp, argp,sizeof(spiflash_data));
			copy_from_user(vir_flash_addr+1, tmp.spiflash_data_addr,tmp.spiflash_data_size);
			cxc_set_mips2unicom_spiflash_int();
			spin_unlock(&spiflash_lock);
			break;

		case CXC_SPIFLASH_ADDR3:
			spin_lock(&spiflash_lock);
			spi_addr[0] = CXC_SPIFLASH_ADDR3;
			memcpy(vir_flash_addr, spi_addr, 4);
			copy_from_user(&tmp, argp,sizeof(spiflash_data));
			copy_from_user(vir_flash_addr+1, tmp.spiflash_data_addr,tmp.spiflash_data_size);
			cxc_set_mips2unicom_spiflash_int();
			spin_unlock(&spiflash_lock);
			break;

		case CXC_SPIFLASH_ADDR4:
			spin_lock(&spiflash_lock);
			spi_addr[0] = CXC_SPIFLASH_ADDR4;
			memcpy(vir_flash_addr, spi_addr, 4);
			copy_from_user(&tmp, argp,sizeof(spiflash_data));
			copy_from_user(vir_flash_addr+1, tmp.spiflash_data_addr,tmp.spiflash_data_size);
			cxc_set_mips2unicom_spiflash_int();
			spin_unlock(&spiflash_lock);
			break;
		default:
			break;
	}	
	return 0;
}


static struct file_operations unicom_cxc_spiflash_fops = {
	.owner = THIS_MODULE,
	.open = unicom_cxc_spiflash_open,
    .release = unicom_cxc_spiflash_release,
	.read = unicom_cxc_spiflash_read,
//	.write = unicom_cxc_spiflash_write,
	.unlocked_ioctl = unicom_cxc_spiflash_ioctl,
};

static struct miscdevice misc_spiflash = {
	.minor = MISC_DYNAMIC_MINOR,
	.name =  SPIFLASH_DEV_NAME,
	.fops = &unicom_cxc_spiflash_fops,
};
/*
 * spiflash_drv_probe
 * @pdev: platform device pointer
 * Description: the driver is initialized through platform_device.
 */
static int unicom_cxc_spiflash_probe(struct platform_device* pdev)
{

	int ret;
	silan_unicom_cxc_probe(pdev);
	int size;

    printk("----unicom_cxc_spiflash probe successful---- \n");
	spiflash = silan_unicom_cxc_get("silan_unicom_cxc");	
    printk("----unicom_cxc_probe ok and get_unicom_cxc_spiflash---- \n");
	
	size =  MBRIDGE_ADDR_SIZE;
	vir_flash_addr = kmalloc(size, GFP_KERNEL);
	if(!vir_flash_addr)
	{
		printk("unicom_mbridge dma buffer malloc failed\n");
        ret = -ENOMEM;
	}
	else
	{
		printk("vir_flash_addr=:%#x\n",vir_flash_addr);
	}
	flashbuf_dma = virt_to_phys(vir_flash_addr);
	printk("flashbuf_dma = :%#x\n",flashbuf_dma);
	ret = misc_register(&misc_spiflash);

//	platform_set_drvdata(pdev, spiflash);
	return ret;

}

static int unicom_cxc_spiflash_remove(struct platform_device* device)
{
	kfree(vir_flash_addr);
	misc_deregister(&misc_spiflash);
    return 0;
}

static struct platform_driver unicom_cxc_spiflash_driver = 
{
    .probe	= unicom_cxc_spiflash_probe,
    .remove = unicom_cxc_spiflash_remove,
    .driver = {
		.name = SPIFLASH_DEV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init unicom_cxc_spiflash_init(void)
{
    int ret;
    printk("/**************unicom_cxc_spiflash_init \n");
    ret = platform_driver_register(&unicom_cxc_spiflash_driver);
    if (ret)
        printk("unicom_cxc_spiflash_init failed \n");
    else
        printk("/****************unicom_cxc_spiflash_init successful \n");

    return ret;
}

static void __exit unicom_cxc_spiflash_exit(void)
{
    printk("unicom_cxc_spiflash_exit \n");
    platform_driver_unregister(&unicom_cxc_spiflash_driver);
}

module_init(unicom_cxc_spiflash_init);
module_exit(unicom_cxc_spiflash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chenwangxin");
MODULE_DESCRIPTION("UNICOM_CXC_SPIFLASH driver");




