#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <silan_resources.h>
#include <silan_regs.h>
#include <silan_irq.h>
#include <silan_def.h>
#include <linux/dma-mapping.h>
#include <silan_memory.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/firmware.h>
#include <asm/uaccess.h>

#include "common.h"
#include "cmd.h"
#include "core.h"
#include "ctrl.h"
#include "cxc.h"
#include "netlink.h"

#define DSP_CHAR_MAJOR      0x4b
#define DSP_DEV_NAME        "silan-dsp"

static struct class *silan_dsp_class = NULL;
static struct dsp_dev_info *dsp_info, silan_dsp_info;

static int dsp_load_firmware(char *name)
{
    const struct firmware *silan_dsp_fw;
    struct platform_device *pdev;

    LOGD("dsp_load_firmware %s\n", name);
    pdev = platform_device_register_simple("dspfirmware", 0, NULL, 0);
    if (IS_ERR(pdev))
    {
        LOGE("silan dsp failed to register firmware\n");
        return PTR_ERR(pdev);
    }

    if (request_firmware(&silan_dsp_fw, name, &pdev->dev) < 0)
    {
        LOGE("dsp_load_firmware timeout..\n");
        platform_device_unregister(pdev);
        return -ENODEV;
    }

    platform_device_unregister(pdev);

    memcpy((unsigned char *)dsp_info->area0_cpu, silan_dsp_fw->data, silan_dsp_fw->size);

    release_firmware(silan_dsp_fw);

    return 0;
}

static int silan_dsp_reload(void)
{
    unsigned int data;

    data = readl(DSP_SW_REG);
    data &= ~DSP_SW_RESETn;
    writel(data, DSP_SW_REG);
    dsp_set_stall(0);

    if (dsp_load_firmware(dsp_info->firmware_name))
        return -1;

    cxc_clr_dsp2mips_int();
    cxc_clr_mips2dsp_int();
    
    data |= DSP_SW_RESETn;
    writel(data, DSP_SW_REG);
    
	dsp_info->init = 1;

    return 0;
}

static ssize_t silan_dsp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return count;
}

static ssize_t silan_dsp_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char name[FIRMWARE_NAME_LEN];

    if (count >= FIRMWARE_NAME_LEN)
        return 0;

    memset(name, 0, FIRMWARE_NAME_LEN);
    if (copy_from_user(name, buf, count))
        return 0;

    //if (strcmp(dsp_info->firmware_name, name) != 0)
    {
        strcpy(dsp_info->firmware_name, name);
        if (silan_dsp_reload())
            return 0;
    }
	if(dsp_info->init == 0){
        if (silan_dsp_reload())
            return 0;
	}

    return count;
}

static int silan_dsp_open(struct inode *inode,struct file *fp)
{
    //unsigned int data;
#if 0
    if (dsp_info->init == 0)
    {
        dsp_load_firmware();
#if 0
        data = readl(SILAN_SYS_REG6);
        data &= ~(0x8000000);
        writel(data, SILAN_SYS_REG6);
        mdelay(10);
        data |= 0x8000000;
        writel(data, SILAN_SYS_REG6);
        mdelay(10);
#endif
        dsp_set_stall(1);
        dsp_set_stall(0);
        dsp_info->init = 1;
    }
#endif
    if (dsp_info->open)
        return DSP_ERROR_DEVICE;

    dsp_info->open = 1;
    if(dsp_info->init == 0){//if you want load dsp only once
        strcpy(dsp_info->firmware_name, "dsp_firmware.bin");
        silan_dsp_reload();
    }

    return 0;
}

static int silan_dsp_close(struct inode *inode,struct file *fp)
{
    LOGD("silan_dsp_close \n");
    struct mbcmd mcmd;
    mcmd.cmd = DSP_CMD_STOP;
    mcmd.param = 0;
    dsp_send_cmd(&mcmd);
    dsp_info->open = 0;
#if 0
    dsp_info->init = 0;
#endif

    return 0;
}

static long silan_dsp_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    int ret = DSP_ERROR_CMD;
    struct mbcmd mcmd;

    LOGD("silan_dsp_ioctl cmd %d,   %d====\n", cmd, arg);
#if 0
    if (cmd == DSP_CMD_MEM)
    {
        int i, *p;
        p = dsp_info->area1_cpu;
        LOGD("111 \n");
        for (i = 0; i < 9; i++, p++)
            LOGD(" %x", *p);
        LOGD("\n222 \n");
    }
#endif
    switch (cmd)
    {
        default:
            mcmd.cmd = _IOC_NR(cmd);
            mcmd.param = arg;
            ret = dsp_send_cmd(&mcmd);
            if (ret)
                memset(dsp_info->firmware_name, 0, FIRMWARE_NAME_LEN);
            break;
    }

    return ret;
}

static int silan_dsp_mmap(struct file *fp, struct vm_area_struct *vma)
{
    vma->vm_flags |= VM_IO | VM_RESERVED;
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

    if (remap_pfn_range(vma, vma->vm_start, (dsp_info->area1_dma) >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

static struct file_operations silan_dsp_fops =
{
    .owner  = THIS_MODULE,
    .read = silan_dsp_read,
    .write = silan_dsp_write,
    .open = silan_dsp_open,
    .release = silan_dsp_close,
    .unlocked_ioctl = silan_dsp_ioctl,
    .mmap = silan_dsp_mmap
};

static int silan_dsp_probe(struct platform_device *pdev)
{
    int ret = DSP_ERROR_NONE;
    struct resource *res;
    struct clk *dsp_clk;

    dsp_info = &silan_dsp_info;

    /* DSP CFG */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res == NULL)
    {
        LOGE("dsp_ctl_probe failed, can't find dsp0 resource\n");
        goto failed;
    }

    dsp_info->dsp_base = (unsigned int)ioremap(res->start, (res->end - res->start));
    if (dsp_info->dsp_base == (unsigned int)NULL)
    {
        LOGE("dsp_ctl_probe failed, can't map dsp0 io\n");
        goto failed;
    }

    dsp_ctrl_init(dsp_info);

    /* DSP CXC */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (res == NULL)
    {
        LOGE("dsp_ctl_probe failed, can't find cxc resource\n");
        goto failed;
    }

    dsp_info->cxc_base = (unsigned int)ioremap(res->start, (res->end - res->start));
    if (dsp_info->cxc_base == (unsigned int)NULL)
    {
        LOGE("dsp_ctl_probe failed, can't map cxc io\n");
        goto failed;
    }

    dsp_cxc_init(dsp_info->cxc_base);

    dsp_mbox_init();

    /* DSP IRQ */
    dsp_info->irq = platform_get_irq(pdev, 0);
    if (request_irq(dsp_info->irq, silan_dsp_interrupt, IRQF_DISABLED, DSP_DEV_NAME, dsp_info))
    {
        LOGE("dsp_ctl_probe request irq failed\n");
        goto failed;
    }

    /* DSP CLK */
    dsp_clk = clk_get(NULL, "dsp0");
    if (IS_ERR(dsp_clk))
    {
        LOGE("dsp_ctl_probe failed, can't get dsp0 clk\n");
        goto failed;
    }
    clk_enable(dsp_clk);

    dsp_set_stall(1);

    if(register_chrdev(DSP_CHAR_MAJOR, DSP_DEV_NAME, &silan_dsp_fops))
    {
        LOGE("can't allocate major number\n");
        goto failed;
    }

    silan_dsp_class = class_create(THIS_MODULE, "dsp_class");
    if (IS_ERR(silan_dsp_class))
    {
        LOGE("can't create dsp class\n");
        goto failed;
    }

    device_create(silan_dsp_class, NULL, MKDEV(DSP_CHAR_MAJOR,0), NULL, DSP_DEV_NAME);
    LOGD("silan_dsp_probe done\n");

    return ret;

failed:
    LOGE("dsp_ctl_probe err \n");
    return DSP_ERROR_DEVICE;
}

static int silan_dsp_remove(struct platform_device *dev)
{
    device_destroy(silan_dsp_class, MKDEV(DSP_CHAR_MAJOR, 0));
    class_destroy(silan_dsp_class);
    unregister_chrdev(DSP_CHAR_MAJOR, DSP_DEV_NAME);
    platform_set_drvdata(dev, NULL);

    return 0;
}

static int silan_dsp_suspend(struct platform_device *dev, pm_message_t mesg)
{
    return 0;
}

static int silan_dsp_resume(struct platform_device *dev)
{
    return 0;
}

struct platform_driver silan_dsp_driver =
{
    .probe      = silan_dsp_probe,
    .remove     = silan_dsp_remove,
    .suspend    = silan_dsp_suspend,
    .resume     = silan_dsp_resume,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = DSP_DEV_NAME,
    },
};

static int __init silan_dsp_init(void)
{
    if (platform_driver_register(&silan_dsp_driver))
    {
        LOGE("silan_dsp_init failed, register driver err \n");
        platform_driver_unregister(&silan_dsp_driver);
    }
	netlink_init();

    return 0;
}

static void __exit silan_dsp_exit(void)
{
    platform_driver_unregister(&silan_dsp_driver);
    netlink_exit();
	LOGD("silan_dsp_exit \n");
}

module_init(silan_dsp_init);
module_exit(silan_dsp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("panjianguang");
MODULE_DESCRIPTION("Driver for silan dsp");
