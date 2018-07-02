#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/highmem.h>
#include <linux/firmware.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <linux/spi/spi.h>
#include <silan_resources.h>
#include <linux/gpio.h>
#include "lcd.h"


#define LCD_DEV_NAME ("lcd-spi")

typedef struct
{
    unsigned char *buf;
    unsigned char data[256];
    struct mutex lock;
    struct spi_device *spi;
}JMOLCD;

JMOLCD *lcd;

#ifdef CONFIG_SILAN_DLNA
#define INIT_GPIO() { \
    gpio_request(21, "silan-gpio"); \
    gpio_direction_output(21, 0); \
    gpio_request(16, "silan-gpio"); \
    gpio_direction_output(16, 0); \
}

#define A0(i) gpio_set_value(16, i)
#define RES(i) gpio_set_value(21, i)

#else
#define INIT_GPIO() { \
    gpio_request(46, "silan-gpio"); \
    gpio_direction_output(46, 0); \
    gpio_request(27, "silan-gpio"); \
    gpio_direction_output(27, 0); \
}

#define A0(i) gpio_set_value(27, i)
#define RES(i) gpio_set_value(46, i)

#endif

static void send_data(unsigned char *data, int len)
{
    struct spi_message m;
    struct spi_transfer t[2];
    
    spi_message_init(&m);
    memset(t, 0, (sizeof t));
    t[0].tx_buf = lcd->data;
    t[0].len = len;
    memcpy(lcd->data, data, len);
    spi_message_add_tail(&t[0], &m);
    spi_sync(lcd->spi, &m);
}

static void lcd_wr_cmd(unsigned char cmdx)
{
    A0(0);
    send_data(&cmdx, 1);
}

static void lcd_wr_data(unsigned char *data, int len)
{
    A0(1);
    send_data(data, len);
}

static void lcd_setx(unsigned char x)
{
    lcd_wr_cmd(0x10 | (x >> 4));
    lcd_wr_cmd(x & 0x0f);
}

static void lcd_sety(unsigned char page)
{
    lcd_wr_cmd(page | 0xb0);
}

static void lcd_refresh_line(unsigned char *buf, int line)
{
    lcd_setx(0);
    lcd_sety(line);
    lcd_wr_data(buf + line*LCD_WIDTH, LCD_WIDTH);
}

static void lcd_refresh(unsigned char *buf)
{
    int j;

    for (j = 0; j < LCD_HEIGHT/8; j ++)
        lcd_refresh_line(buf, j);
}

static void lcd_hardware_init(void)
{
    INIT_GPIO();

    RES(0); //reset
    mdelay(100);
    RES(1);
    lcd_wr_cmd(0xe2);
    mdelay(20);
	
    lcd_wr_cmd(0x2c);
    lcd_wr_cmd(0x2e);
    lcd_wr_cmd(0x2f);
    lcd_wr_cmd(0x23);
    lcd_wr_cmd(0x81);
    lcd_wr_cmd(0x28);
    lcd_wr_cmd(0xa2);
    lcd_wr_cmd(0xc8);
    lcd_wr_cmd(0xa0);
    lcd_wr_cmd(0xaf);

    lcd_refresh(lcd->buf); //clear lcd
}

static int lcd_open(struct inode* inode, struct file* filp)
{
    return 0;
}

static ssize_t lcd_read(struct file* filp, char __user* buf, size_t len, loff_t* pos)
{
    return 0;
}

static ssize_t lcd_write(struct file* filp, const char __user* buf, size_t len, loff_t* pos)
{
    return 0;
}

static long lcd_ioctl(struct file* filp, u_int cmd, u_long arg)
{
    int ret = 0;

    switch(cmd)
    {
        case LCD_IOCTL_REFRESH_SCR:
            lcd_refresh(lcd->buf);
            break;
        
        case LCD_IOCTL_REFRESH_LINE:
            lcd_refresh_line(lcd->buf, arg);
            break;

        default:
            break;
    }

    return ret;
}

static int lcd_release(struct inode* inode, struct file* filp)
{
    return 0;
}

static int lcd_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long page;
    unsigned long start = (unsigned long)vma->vm_start;
    unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

    page = virt_to_phys(lcd->buf);
    if (remap_pfn_range(vma, start, page >> PAGE_SHIFT, size, PAGE_SHARED))
        return -1;

    return 0;
}

struct file_operations lcd_fops = {
    .owner = THIS_MODULE,
    .open = lcd_open,
    .read = lcd_read,
    .write = lcd_write,
    .unlocked_ioctl = lcd_ioctl,
    .release = lcd_release,
    .mmap = lcd_mmap,
};

static struct miscdevice lcd_misc = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = LCD_DEV_NAME,
    .fops    = &lcd_fops,
};

static int __devinit lcd_spi_probe(struct spi_device *spi)
{
    int err;

    lcd = kzalloc(sizeof(JMOLCD), GFP_KERNEL);
    if (!lcd)
        return -ENOMEM;
    
    lcd->spi = spi;
    mutex_init(&lcd->lock);
    dev_set_drvdata(&spi->dev, lcd);

    lcd->buf = (unsigned char*)kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!lcd->buf)
    {
        printk("lcd_spi_probe err : lcd_buf kmalloc failed \n");
        err = -ENOMEM;
        goto failed;
    }
    memset(lcd->buf, 0, PAGE_SIZE);
    
    if (misc_register(&lcd_misc) < 0)
    {
        printk("lcd_spi_probe err : misc_register failed \n");
        err = -EBUSY;
        goto failed;
    }

    lcd_hardware_init();
    
    SetPageReserved(virt_to_page(lcd->buf));
    printk("lcd_spi_probe done \n");
    return 0;

failed:
    if (lcd->buf)
        kfree(lcd->buf);

    printk("lcd_spi_probe failed \n");
    return err;
}


static int __devexit lcd_spi_remove(struct spi_device *spi)
{
    if (lcd->buf)
    {
        ClearPageReserved(virt_to_page(lcd->buf));
        kfree(lcd->buf);
        lcd->buf = NULL;
    }
 
    return 0;
}

static struct spi_driver lcd_driver = {
    .driver = {
        .name	= LCD_DEV_NAME,
        .bus	= &spi_bus_type,
        .owner	= THIS_MODULE,
    },
    .probe	= lcd_spi_probe,
    .remove	= lcd_spi_remove,
};


static int __init lcd_spi_init(void)
{
    return spi_register_driver(&lcd_driver);
}


static void __exit lcd_spi_exit(void)
{
    spi_unregister_driver(&lcd_driver);
}


module_init(lcd_spi_init);
module_exit(lcd_spi_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LCD SPI driver for JMO12864");


