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
#include <silan_memory.h>
#include <silan_gpio.h>
#include <linux/dma-mapping.h>

#include <linux/sched.h>
#include <linux/version.h>
#include <linux/miscdevice.h>

#include "silan_deint.h"

#define DEINT_CHAR_MAJOR    0x4F 
#define DEV_DEINT_NAME      "silan-deint"

static struct class *silan_deint_class = NULL;
static struct deint_info *info;

static int silan_deint_open(struct inode *inode, struct file *fp)
{

	return 0;
}

static int silan_deint_release(struct inode *inode, struct file *fp)
{
	fp->private_data = NULL;
	return 0;
}

static long silan_deint_ioctl(struct file *fp,unsigned int cmd,unsigned long argp)
{
	int ret = 0;
    unsigned int done_flag;
    unsigned int odd_flag = 0;
    unsigned int y_output_addr = 0;
    struct Ex_dit_update_info   update_info;
    struct Ex_dit_config        ex_dit_config;

	switch(cmd)
	{
		case DEINT_CFG:
            if(copy_from_user(&ex_dit_config,(void *)argp,sizeof(struct Ex_dit_config)))
				ret = -EFAULT;

            ex_deint_io_cfg(&ex_dit_config);
           	break;
		case DEINT_Y_UPDATE:
            if(copy_from_user(&update_info,(void *)argp,sizeof(struct Ex_dit_update_info)))
				ret = -EFAULT;

            ex_deint_cfg_baseaddr(&update_info);
            if(update_info.mv_used == 0)
			{
				memset((void *)(info->map_addr+DEINT_Y_BUFFER_SIZE), 0x0, DEINT_MV_BUFFER_SIZE);
				info->mv_used = 1;
			}
           	break;
		case DEINT_START:
            info->is_done = 0;
            if(copy_from_user(&odd_flag,(void *)argp,sizeof(unsigned int)))
				ret = -EFAULT;

            info->odd_flag = odd_flag ? 1 : 0;
            ex_deint_start(odd_flag);
           	break;
		case DEINT_CHECK_DONE:
            done_flag = info->is_done;
            if(copy_to_user((void *)argp, &done_flag, sizeof(unsigned int)))
                ret = -EFAULT;
           	break;
        case DEINT_Y_OUTPUT:
            y_output_addr = info->y_out_base_addr+info->odd_flag*DEINT_Y_OUT_BUFFER;
            if(copy_to_user((void *)argp, &y_output_addr, sizeof(unsigned int)))
                ret = -EFAULT;
            break;
		default:
			break;
	}

	return ret;
}

static int silan_deint_mmap(struct file *fp, struct vm_area_struct *vma)
{
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (remap_pfn_range(vma, vma->vm_start, (info->dma_addr >> PAGE_SHIFT), vma->vm_end-vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static const struct file_operations silan_deint_fops = {
	.owner	= THIS_MODULE,
	.open = silan_deint_open,
	.release = silan_deint_release,
	.unlocked_ioctl	= silan_deint_ioctl,
    .mmap = silan_deint_mmap
};

static int deint_mmap_video_memory(struct deint_info *deint_ptr)
{
    unsigned int size;
    dma_addr_t deint_dma_addr;
#ifdef CONFIG_SILAN_IVS1
	int ret;
	struct prom_phyaddr pa;
#endif

    size = DEINT_MV_BUFFER_SIZE+DEINT_Y_BUFFER_SIZE+DEINT_Y_OUT_BUFFER_SIZE;
#ifdef CONFIG_SILAN_IVS1
    ret = prom_phy_mem_malloc(size, SILAN_DEV_DIT, &pa);
	if (ret)
		return ret;
	if (pa.cpa != pa.dpa)
		printk("WARN: DIT: cpa is different with dpa\n");
    deint_dma_addr = pa.cpa;
    deint_ptr->dma_addr = pa.dpa;
#else
    deint_dma_addr = prom_phy_mem_malloc(size, SILAN_DEV_DIT);
    
    deint_ptr->dma_addr = deint_dma_addr;
#endif

    deint_ptr->map_addr = (dma_addr_t)ioremap_nocache(deint_dma_addr,size);
    deint_ptr->y_base_addr = deint_dma_addr;
    deint_ptr->mv_base_addr = deint_dma_addr+DEINT_Y_BUFFER_SIZE;
    deint_ptr->y_out_base_addr = deint_dma_addr+DEINT_Y_BUFFER_SIZE+DEINT_MV_BUFFER_SIZE;

    ex_cfg_addr(deint_ptr->y_base_addr, deint_ptr->mv_base_addr, deint_ptr->y_out_base_addr, deint_ptr->map_addr);
    return 0;
}

static irqreturn_t deint_handle_irq(int irq, void *ptr)
{
	int ret = 0;

    /* clear interrupt */
	printk("int deint irq\n");
    ex_deint_clear_interrupt();

	printk("int deint clear\n");
    info->is_done = 1;

    return IRQ_RETVAL(ret);
}

static int silan_deint_probe(struct platform_device *dev)
{
    int ret;
    struct resource *res;
    int irq;

    info = kmalloc(sizeof(struct deint_info), GFP_KERNEL);
    memset(info, 0, sizeof(struct deint_info));

    res = platform_get_resource(dev, IORESOURCE_MEM, 0);
    if(res == NULL)
	{
		ret = -ENOMEM;
        goto failed;
	}
    info->deint_reg = (EXDITREG*)ioremap_nocache(res->start, res->end-res->start);
    
    irq = platform_get_irq(dev, 0);
    if(irq == -ENXIO)
	{
		ret = -ENXIO;
        goto failed; 
	}
    
    ret = request_irq(irq, deint_handle_irq, 0, "deint", NULL);
	if (ret) 
	{
		ret = -EBUSY;
        goto failed;
	}

    /* deinterlace malloc */
    deint_mmap_video_memory(info);
    /* deinterlace init */ 
    ex_deint_init(info->deint_reg);  
    ex_dma_cfg_init(info->deint_reg);  
    ex_deint_enable(1);

	if(register_chrdev(DEINT_CHAR_MAJOR,DEV_DEINT_NAME,&silan_deint_fops))
    {
		printk("can't allocate major number\n");
		return -1;
	}

	silan_deint_class = class_create(THIS_MODULE,"deint_class");
	if(IS_ERR(silan_deint_class))
	{
		printk("can't create deint class\n");
		return -1;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27) 
	device_create(silan_deint_class,NULL,MKDEV(DEINT_CHAR_MAJOR,0),NULL,DEV_DEINT_NAME);
#else	
	device_create(silan_deint_class,NULL,MKDEV(DEINT_CHAR_MAJOR,0),DEV_DEINT_NAME);
#endif	
	return 0;

failed:
    platform_set_drvdata(dev, NULL);
    iounmap(info->deint_reg);
    kfree(info);
    printk("deint probe failed\n");
    return ret;
}

int silan_deint_remove(struct platform_device *dev)
{
	return 0;
}

struct platform_driver silan_deint_driver={
	.probe		= silan_deint_probe,
	.remove		= silan_deint_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= DEV_DEINT_NAME,
	},
};

static int __init silan_deint_init(void)
{
	if(platform_driver_register(&silan_deint_driver))
	{
		printk("register deint driver error\n");
		platform_driver_unregister(&silan_deint_driver);
	}
	return 0;
}

static void __exit silan_deint_exit(void)
{
	platform_driver_unregister(&silan_deint_driver);
	printk("silan deinterlace module removed\n");
}

module_init(silan_deint_init);
module_exit(silan_deint_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");
MODULE_DESCRIPTION("Driver for silan deinterlace");
