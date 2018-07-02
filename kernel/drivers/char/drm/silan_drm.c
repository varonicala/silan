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
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#include "silan_drm.h"
#include "silan_drm_core.h"
#include "silan_dmac.h"

//#define DRM_DBG

struct silan_drm drm_info;
wait_queue_head_t inter_wait;
static int inter_flag;
//static spinlock_t s_drm_lock = __SPIN_LOCK_UNLOCKED(s_drm_lock);

int set_buf_data(char *buf, int val)
{
    int i;
    for(i = 0; i < 4; i++){
        buf[i] = (val >> (8*i)) & 0xff;
    }
    return 0;
}

int get_buf_data(char *buf)
{
    int val = 0, i;
    for(i = 0; i < 4; i++){
        val |= (buf[i] & 0xff) << (8*i);
    }
    return val;
}

static void _cipher(struct silan_drm *drm, enum patten pa)
{
    drm->cipher_ops->init(drm, pa);    

    drm->dma_ops->init(drm);

    drm->dma_ops->start(drm, DMA_TO_BUF);
    
    //drm->dma_ops->start(drm, DMA_FROM_BUF);
    
    drm->cipher_ops->start(drm);    
#ifndef DRM_ENGINE_CPU_READ
    drm->dma_ops->start(drm, DMA_FROM_BUF);
#endif
}

static void decipher(struct silan_drm *drm)
{
    _cipher(drm, DECIPHER);
}

static void cipher(struct silan_drm *drm)
{
    _cipher(drm, CIPHER);
}

static void set_rsa_buf(struct silan_drm *drm)
{
    u32 inpmode, outmode;
    void __iomem *base = (void __iomem *)drm->buf_ctrl_base;
    
    inpmode = (31 << 16) | SECDMX_BUF_FIFO_MODE  | SECDMX_BUF_CPU_CHANNEL;
    outmode = (31 << 16) | SECDMX_BUF_FIFO_MODE | SECDMX_BUF_DRM_HASH_CHANNEL;

    writel(SECDMX_BUF_RESET, base + BUF0_CTRL);
    writel(SECDMX_BUF_UNRESET, base + BUF0_CTRL);
        
    writel(inpmode, base + BUF0_INP_CFG);
    writel(outmode, base + BUF0_OUT_CFG);

    //writel(0x0, base + BUF_CFG);
}

static void set_rsa_c(struct silan_drm *drm, u32 *data, int len)
{
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    
    int i = 0;
    len /= 4;
    for(i = 0; i < len; i++){
        writel(data[len-i-1], base+DRM_RSA_C+i*4);    
    }
} 

static void set_rsa_n(struct silan_drm *drm, u32 *data, int len)
{
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    int i = 0;
    
    len /= 4;
    for(i = 0; i < len; i++){
        writel(data[len-i-1], base+DRM_RSA_N+i*4);    
    }
}

static void set_rsa_m(struct silan_drm *drm, u32 *data, int len)
{
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    int i = 0;

    len /= 4;
    for(i = 0; i < len; i++){
        writel(data[len-i-1], base+DRM_RSA_M+i*4);    
    }
}

static void set_rsa_d(struct silan_drm *drm, u32 *data, int len)
{
    void __iomem *base = (void __iomem *)drm->buf_ctrl_base;
    int i = 0;
    
    len /= 4;
    for(i = 0; i < len; i++){
        writel(data[i], base+BUF0+i*4);        
    }
}

static void set_rsa_e(struct silan_drm *drm, u32 *data, int len)
{
    void __iomem *base = (void __iomem *)drm->buf_ctrl_base;
    int i = 0;
    
    len /= 4;
    for(i = 0; i < len; i++){
        writel(data[i], base+BUF0+i*4);        
    }
}

static void rsa_start(struct silan_drm *drm)
{
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    u32 tmp;
    
    tmp = readl(base+DRM_RSA_CNTL);
    tmp |= 0x1;

    writel(tmp, base+DRM_RSA_CNTL);
}

#if 0
static int rsa_e_d_width(struct silan_drm *drm, int width)
{
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    u32 tmp;
    int map[7] = {2048, 1024, 512, 256, 128, 64};
    u8 i;

    for(i = 0; i < 6; i++){
        if(width == map[i]){
            break;
        }
    }
    if(i == 6){
        printk("Invalid width!\n");
        return -1;
    }

    tmp = readl(base+DRM_RSA_CNTL);
    tmp &= 0xffffff1f;
    tmp = tmp | (i << 5);

    writel(tmp, base+DRM_RSA_CNTL);

    return 0;
}

static int rsa_n_width(struct silan_drm *drm, int width)
{
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    u32 tmp;
    int map[7] = {2048, 1024, 512, 256, 128, 64};
    u8 i;

    for(i = 0; i < 6; i++){
        if(width == map[i]){
            break;
        }
    }
    if(i == 6){
        printk("Invalid width: %d!\n", width);
        return -1;
    }

    tmp = readl(base+DRM_RSA_CNTL);

    tmp &= 0xfffff8ff;
    tmp = tmp | (i << 8);

    writel(tmp, base+DRM_RSA_CNTL);

    return 0;
}
#endif

static int rsa_width(struct silan_drm *drm, int width, int n)
{
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    u32 tmp;
    int map[7] = {2048, 1024, 512, 256, 128, 64};
    u8 i;

    for(i = 0; i < 6; i++){
        if(width == map[i]){
            break;
        }
    }
    if(i == 6){
        printk("Invalid width: %d!\n", width);
        return -1;
    }

    tmp = readl(base+DRM_RSA_CNTL);

    if(n == 5) 
		tmp &= 0xffffff1f;
	else
		tmp &= 0xfffff8ff;

    tmp = tmp | (i << n);

    writel(tmp, base+DRM_RSA_CNTL);

    return 0;
}
#if 1
static int rsa_e_d_width(struct silan_drm *drm, int width)
{
    return rsa_width(drm, width, 5);
}

static int rsa_n_width(struct silan_drm *drm, int width)
{
    return rsa_width(drm, width, 8);
}
#endif
static int rsa_endian(struct silan_drm *drm, int endian)
{
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    
    u32 tmp;

    if(endian < 0 || endian > 1){
        printk("Invalid endian: %d!\n", endian);
        return -1;
    }

    tmp = readl(base+DRM_RSA_CNTL);

    tmp &= 0xffffffef;
    tmp = tmp | (endian << 4);

    writel(tmp, base+DRM_RSA_CNTL);

    return 0;
}

static void get_rsa_data(struct silan_drm *drm, u32 *data, int len)
{
    int i = 0;
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    len /= 4;
    for(i = 0; i < len; i++){
        data[i] = readl(base+DRM_RSA_C+i*4);
    }
}

static int drm_buffer_read(u8 *src, char __user *buf, size_t count, loff_t *ppos)
{
    if(copy_to_user(buf, src, count))
        return -EFAULT;
    
    return count;
}

static int drm_buffer_write(u8 *dst, char __user *buf, size_t count)
{
    memcpy(dst, buf, count);
    
    return count;    
}

static int silan_drm_open(struct inode *inode, struct file *fp)
{
#if 0
    struct silan_drm *drm = &drm_info;

    atomic_set(&drm->irqc, 0);
    init_waitqueue_head(&drm->irq_wait);
#endif

    return 0;
}

static ssize_t silan_drm_read(struct file *file, char __user *buf, size_t count,loff_t *ppos)
{
    struct silan_drm *drm = &drm_info;    
	void __iomem *base = (void __iomem *)drm->buf_ctrl_base;
    int i;
    int ret = 0;
    
    if (mutex_lock_interruptible(&drm->mutex))
        return -ERESTARTSYS;
#ifdef DRM_ENGINE_CPU_READ
    ret = drm_buffer_read(base+BUF1, buf, count, ppos);
#else
    ret = drm_buffer_read(drm->mmap_cpu+DRM_MMAP_SIZE/4/2, buf, count, ppos);
#endif

    if(ret < 0){
        printk("DRM read error\n");
    }

    mutex_unlock(&drm->mutex);

    return ret;
}

static ssize_t silan_drm_write(struct file *file, const char __user *buf, size_t count,loff_t *ppos)
{
    struct silan_drm *drm = &drm_info;    
    int ret = 0;
    
    if (mutex_lock_interruptible(&drm->mutex))
        return -ERESTARTSYS;
    
    ret = drm_buffer_write(((u32)drm->mmap_cpu), buf, count);
    //ret = drm_buffer_write(((u32)drm->mmap_cpu & 0x0FFFFFFF | 0xA0000000), buf, count);
    if(ret < 0){
        printk("DRM write error\n");
    }

    mutex_unlock(&drm->mutex);

    return ret;
}

static int silan_drm_release(struct inode *inode, struct file *fp)
{
    //memset(drm_info.mmap_cpu, 0, DRM_MMAP_SIZE);
    return 0;
}

static long silan_drm_ioctl(struct file *filp, unsigned int cmd, unsigned long argp)
{
    int ret = 0;
    struct drm_data data1;
    struct silan_drm *drm = &drm_info;
    u32 timeout = 500;    
    u32 data[64];
	
	//spin_lock(&s_drm_lock);
    if (mutex_lock_interruptible(&drm->mutex))
        return -ERESTARTSYS;
    
	switch(cmd){
        case DRM_IOCTL_DECIPHER:
            
            decipher(drm);    
        
            if(!wait_event_interruptible_timeout(drm->irq_wait, inter_flag != 0, 
                        msecs_to_jiffies(timeout))){
                ret = -ETIME;
                inter_flag = 0;
                break;
            } 
            inter_flag = 0;

            break;
        case DRM_IOCTL_CIPHER:
            
            cipher(drm);
            
            if(!wait_event_interruptible_timeout(drm->irq_wait, inter_flag != 0, 
                        msecs_to_jiffies(timeout))){
                ret = -ETIME;
                //printk("##### %s ret=%d [%08x %08x %08x %08x]#####\n", __func__, ETIME,
                //        readl(), );
                inter_flag = 0;
                break;
            } 
            inter_flag = 0;

            break;
        case DRM_IOCTL_READ:
            if(copy_from_user(&data1, (struct drm_data *)argp, sizeof(data1))){
                ret = -EFAULT;
				goto end;
			}
            if(copy_to_user((struct drm_data*)argp, &data1, sizeof(data1))){
                ret = -EFAULT;
				goto end;
			}
            break;
        case DRM_IOCTL_SET_KEY:
            if(copy_from_user(&drm->param.key, (u32 *)argp, sizeof(drm->param.key))){
                ret = -EFAULT;
				goto end;
			}
            if(drm->param.key == NULL){
                printk("Error key\n");
                ret = -EFAULT;
				goto end;
            }
#ifdef DRM_DBG
            for(i = 0; i < 4; i++){
                printk("key[%d]: %x\n", i, drm->param.key[i]);
            }
#endif
            break;
        case DRM_IOCTL_SET_VECTOR:
            if(copy_from_user(&drm->param.vec, (u32 *)argp, sizeof(drm->param.vec))){
                ret = -EFAULT;
				goto end;
			}
            if(drm->param.vec == NULL){
                printk("Error vector\n");
                ret = -EFAULT;
				goto end;
            }
#ifdef DRM_DBG
            for(i = 0; i < 4; i++){
                printk("vec[%d]: %x\n", i, drm->param.vec[i]);
            }
#endif
            break;
        case DRM_IOCTL_SET_COUNT:
            if(copy_from_user(&drm->param.cnt, (u32 *)argp, sizeof(drm->param.cnt))){
                ret = -EFAULT;
				goto end;
			}
            if(drm->param.cnt == NULL){
                printk("Error count\n");
                ret = -EFAULT;
				goto end;
            }
#ifdef DRM_DBG        
            for(i = 0; i < 4; i++){
                printk("cnt[%d]: %x\n", i, drm->param.cnt[i]);
            }
#endif
            break;
        case DRM_IOCTL_SET_SIZE:
            drm->param.size = argp;
            if(drm->param.size < 0){
                printk("Error size\n");
                ret = -EFAULT;
				goto end;
            }
#ifdef DRM_DBG
            printk("size: %d\n", drm->param.size);
#endif
            break;
        case DRM_IOCTL_SET_TYPE:
            drm->config.tp = argp;
            if(drm->config.tp < 0 || drm->config.tp > 5){
                printk("Error type\n");
                ret = -EFAULT;
				goto end;
            }
#ifdef DRM_DBG
            printk("type: %d\n", drm->config.tp);
#endif
            break;
        case DRM_IOCTL_SET_MODE:
            drm->config.mo = argp;
            if(drm->config.mo < 0 || drm->config.mo > 4){
                printk("Error mode\n");
                ret = -EFAULT;
				goto end;
            }
#ifdef DRM_DBG
            printk("mode: %d\n", drm->config.mo);
#endif
            break;
        case DRM_IOCTL_RSA_C:
            if(copy_from_user(data, (u32 *)argp, sizeof(data))){
                ret = -EFAULT;
				goto end;
			}
            if(data == NULL){
                printk("Error rsa c\n");
                ret = -EFAULT;
				goto end;
            }
            set_rsa_c(drm, data, sizeof(data));

            break;
        case DRM_IOCTL_RSA_M:
            if(copy_from_user(data, (u32 *)argp, sizeof(data))){
                ret = -EFAULT;
				goto end;
			}
            if(data == NULL){
                printk("Error rsa m\n");
                ret = -EFAULT;
				goto end;
            }
            set_rsa_m(drm, data, sizeof(data));
        
            break;
        case DRM_IOCTL_RSA_N:
            if(copy_from_user(data, (u32 *)argp, sizeof(data))){
                ret = -EFAULT;
				goto end;
			}
            if(data == NULL){
                printk("Error rsa n\n");
                ret = -EFAULT;
				goto end;
            }
            set_rsa_n(drm, data, sizeof(data));

            break;
        case DRM_IOCTL_RSA_D:
            if(copy_from_user(data, (u32 *)argp, sizeof(data))){
                ret = -EFAULT;
				goto end;
			}
            if(data == NULL){
                printk("Error rsa d\n");
                ret = -EFAULT;
				goto end;
            }
            set_rsa_buf(drm);
            set_rsa_d(drm, data, sizeof(data));
            rsa_start(drm);
            
            if(!wait_event_interruptible_timeout(drm->irq_wait, inter_flag != 0, 
                        msecs_to_jiffies(timeout))){
                ret = -ETIME;
                inter_flag = 0;
                break;
            } 
            inter_flag = 0;

            get_rsa_data(drm, data, sizeof(data));

            if(copy_to_user((u32 *)argp, &data, sizeof(data))){
                ret = -EFAULT;
				goto end;
			}

            break;
        case DRM_IOCTL_RSA_E:
            set_rsa_buf(drm);
            if(copy_from_user(data, (u32 *)argp, sizeof(data))){
                ret = -EFAULT;
				goto end;
			}
            if(data == NULL){
                printk("Error rsa e\n");
                ret = -EFAULT;
				goto end;
            }
            set_rsa_e(drm, data, sizeof(data));
            rsa_start(drm);

            if(!wait_event_interruptible_timeout(drm->irq_wait, inter_flag != 0, 
                        msecs_to_jiffies(timeout))){
                ret = -ETIME;
                inter_flag = 0;
                break;
            } 
            inter_flag = 0;

            get_rsa_data(drm, data, sizeof(data));
            
            if(copy_to_user((u32 *)argp, &data, sizeof(data))){
                ret = -EFAULT;
				return 0;
			}

            break;
        case DRM_IOCTL_RSA_E_D_WIDTH:
            if(rsa_e_d_width(drm, argp)){
                ret = -EFAULT;
				goto end;
			}
            break;
        case DRM_IOCTL_RSA_N_WIDTH:
            if(rsa_n_width(drm, argp)){
                ret = -EFAULT;
				goto end;
			}
            break;
        case DRM_IOCTL_RSA_ENDIAN:
            if(rsa_endian(drm, argp)){
				ret = -EFAULT;
				goto end;
			}
            break;
        default:
            break;
    }

end:
    mutex_unlock(&drm->mutex);
	//spin_unlock(&s_drm_lock);
    return ret;
}

static int silan_drm_mmap(struct file *fp, struct vm_area_struct *vma)
{
    vma->vm_flags |= VM_IO | VM_RESERVED;
    vma->vm_page_prot = PAGE_SHARED;
    if (remap_pfn_range(vma, vma->vm_start, virt_to_phys(drm_info.mmap_cpu)>>PAGE_SHIFT, 
                        vma->vm_end-vma->vm_start, vma->vm_page_prot))
        return -EAGAIN;
    
    return 0;
}

static unsigned int silan_drm_poll(struct file * file, poll_table * wait)
{
    unsigned int mask = 0;
#if 0
    struct silan_drm *drm = &drm_info;

    poll_wait(file, &drm->irq_wait, wait);
    if (atomic_read(&drm->irqc))
        mask |= POLLIN | POLLRDNORM;
#endif
    return mask;
}

static irqreturn_t drm_irq_isr(int irq, void *dev_id)
{
    struct silan_drm *drm = &drm_info;
    void __iomem *base = (void __iomem *)(&drm->drm_core_base->key_reg0);
    u32 val;
    
    val = readl(drm->dmac_base+PL080_INT_STATUS);
    if(val & (1 << drm->tx_chan.chan_id)){
        drm->dma_ops->stop(drm, DMA_TO_BUF);
        drm->dma_ops->complete(drm, DMA_TO_BUF);
        
        //drm->cipher_ops->start(drm);    
        
        goto end;
    }
    else if(val & (1 << drm->rx_chan.chan_id)){
        drm->dma_ops->stop(drm, DMA_FROM_BUF);
        drm->dma_ops->complete(drm, DMA_FROM_BUF);
#ifndef DRM_ENGINE_CPU_READ
        inter_flag = 1;
        wake_up_interruptible(&drm->irq_wait);
#endif
        goto end;
    }

    val = readl(&drm->drm_core_base->int_status);
    if(val & DRM_INT_REG_SECTOR_IDLE){
        writel(DRM_INT_REG_SECTOR_IDLE, &drm->drm_core_base->int_status);
#ifdef DRM_ENGINE_CPU_READ
        inter_flag = 1;
        wake_up_interruptible(&drm->irq_wait);
#endif
    }
    else if(val & DRM_INT_REG_BLOCK_READY){
        writel(DRM_INT_REG_BLOCK_READY, &drm->drm_core_base->int_status);
    }

    val = readl(base+DRM_RSA_INTR) & 0x1;
    if(val){
        writel(0x1, base+DRM_RSA_INTR);
        inter_flag = 1;
        wake_up_interruptible(&drm->irq_wait);
    }

end:
    return IRQ_HANDLED;
}

static const struct file_operations silan_drm_ops = {
    .owner   = THIS_MODULE,
    .open    = silan_drm_open,
    .read    = silan_drm_read,
    .write   = silan_drm_write,
    .release = silan_drm_release,
    .read    = silan_drm_read,
    .unlocked_ioctl = silan_drm_ioctl,
    .mmap    = silan_drm_mmap,
    .poll     = silan_drm_poll,
};

static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = SILAN_DRM_NAME,
    .fops  = &silan_drm_ops,
};

static void silan_drm_init_dma(struct silan_drm *drm)
{

    drm->mmap_cpu = (u32*)dma_alloc_coherent(drm->dev, DRM_MMAP_SIZE, (dma_addr_t*)&drm->map_dma, GFP_KERNEL);
    //drm->mmap_cpu = kzalloc(DRM_MMAP_SIZE, GFP_KERNEL);
    if(!drm->mmap_cpu){
        printk("%s: Unalbe to alloc DMA memory\n", __func__);
        return ;
    }
    printk("drm->mmap_cpu: %p\n", drm->mmap_cpu);
    SetPageReserved(virt_to_page(drm->mmap_cpu));
    
    silan_dmac_ops_init(drm);
}

static void silan_cipher_init(struct silan_drm *drm)
{
    drm->config.tp = AES;
    drm->config.mo = EBC;//CBC;

    silan_cipher_ops_init(drm);
}

static int drm_probe(struct platform_device *pdev)
{
    struct resource *res;
    int ret = 0;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(res == NULL){
        printk("Can't find dmac resource\n");
        return -ENOENT;
    }

    drm_info.dmac_base = ioremap(res->start, (res->end - res->start));
    if(drm_info.dmac_base == NULL){
        printk("Can not map dmac io\n");
        return -ENXIO;
    }
    
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if(res == NULL){
        printk("Can't find buf_ctrl resource\n");
        return -ENOENT;
    }

    drm_info.buf_ctrl_base = (u32)ioremap(res->start, (res->end - res->start));
    if(drm_info.buf_ctrl_base == (u32)NULL){
        printk("Can not map buf_ctrl io\n");
        return -ENXIO;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    if(res == NULL){
        printk("Can't find drm_core resource\n");
        return -ENOENT;
    }

    drm_info.drm_core_base = (DRM_CORE *)ioremap(res->start, (res->end - res->start));
    if(drm_info.drm_core_base == (DRM_CORE *)NULL){
        printk("Can not map dmr_core io\n");
        return -ENXIO;
    }

    drm_info.irq = platform_get_irq(pdev, 0);
    if(drm_info.irq <= 0){
        printk("Can not find IRQ\n");
        return -ENXIO;
    }

    ret = request_irq(drm_info.irq, drm_irq_isr, IRQF_DISABLED, "silan-drm", NULL);
    if(ret != 0){
        printk("silan_drm: Cannot allocate irq %d, ret: %d\n", drm_info.irq, ret);
        return -EIO;
    }

    silan_drm_init_dma(&drm_info);

    silan_cipher_init(&drm_info);
    
    init_waitqueue_head(&drm_info.irq_wait);
    
    mutex_init(&drm_info.mutex);
    
    if (misc_register(&misc) < 0){
        printk("Can not register the misc dev\n");
        return -EBUSY;
    }

    return 0;    
}

static int drm_remove(struct platform_device *pdev)
{
    if(misc_deregister(&misc)){
        printk("Can not deregister the misc dev\n");
        return -EBUSY;
    }
    
    return 0;
}
#if 0
int drm_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

int drm_resume(struct platform_device *pdev)
{
    return 0;
}
#endif
static struct platform_driver drm_driver = {
    .probe = drm_probe,
    .remove = drm_remove,
    .driver = {
        .name = "silan-drm",
        .owner  = THIS_MODULE,
    },

};

static int __init silan_drm_init(void)
{
    if(platform_driver_register(&drm_driver)){
        printk("Register driver error\n");
        platform_driver_unregister(&drm_driver);
    }

    return 0; 
}

static void __exit silan_drm_exit(void)
{
    platform_driver_unregister(&drm_driver);
}

module_init(silan_drm_init);
module_exit(silan_drm_exit);

MODULE_AUTHOR("chenjianneng@silan.com");
MODULE_DESCRIPTION("SILAN DRM driver");
MODULE_LICENSE("GPL");
