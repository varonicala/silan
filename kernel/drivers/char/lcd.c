
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

#include <linux/string.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/time.h>
#include <linux/kgdb.h>
#include <linux/gpio.h>

#include <silan_padmux.h>
#include "lcd.h"

#define LCD_DEV_NAME ("silan-lcd")

unsigned char *lcd_buf = NULL;

//#define LCD_PARALLEL
#define LCD_SPI

#ifdef LCD_PARALLEL

#define GPIO_START_NUM 3*32
#define INIT_GPIO() { \
    int i; \
    for (i = 0; i < 5; i ++) \
    { \
        gpio_request(GPIO_START_NUM + 15 + i, "silan-gpio"); \
        gpio_direction_output(GPIO_START_NUM + 15 + i, 0); \
    } \
    for (i = 0; i < 3; i ++) \
    { \
        gpio_request(GPIO_START_NUM + 12 + i, "silan-gpio"); \
        gpio_direction_output(GPIO_START_NUM + 12 + i, 0); \
    } \
    for (i = 0; i < 5; i ++) \
    { \
        gpio_request(GPIO_START_NUM + 20 + i, "silan-gpio"); \
        gpio_direction_output(GPIO_START_NUM + 20 + i, 0); \
    } \
}

#define CS(i) gpio_set_value(GPIO_START_NUM + 15, i)
#define RES(i) gpio_set_value(GPIO_START_NUM + 16, i)
#define A0(i) gpio_set_value(GPIO_START_NUM + 17, i)
#define WR(i) gpio_set_value(GPIO_START_NUM + 18, i)
#define RD(i) gpio_set_value(GPIO_START_NUM + 19, i)

#define WR_DATA(data) { \
    int k; \
    for (k = 2; k >= 0; k --) \
    { \
        gpio_set_value(GPIO_START_NUM + 12 + k, (data & 0x80) >> 7); \
        data <<= 1; \
    } \
    for (k = 4; k >= 0; k --) \
    { \
        gpio_set_value(GPIO_START_NUM + 20 + k, (data & 0x80) >> 7); \
        data <<= 1; \
    } \
}

void lcd_wr_cmd(unsigned char cmdx)
{
    A0(0);
    CS(0);
    WR_DATA(cmdx);
    RD(1);
    ndelay(100);
    WR(0);
    ndelay(100);
    WR(1);
    CS(1);
}

void lcd_wr_data(unsigned char cmdx)
{
    A0(1);
    CS(0);
    WR_DATA(cmdx);
    RD(1);
    ndelay(100);
    WR(0);
    ndelay(100);
    WR(1);
    CS(1);
}

#else

#define GPIO_START_NUM 0//1*32

#define INIT_GPIO() { \
    int i; \
    for (i = 0; i < 4; i ++) \
    { \
        gpio_request(GPIO_START_NUM + 15 + i, "silan-gpio"); \
        gpio_direction_output(GPIO_START_NUM + 15 + i, 0); \
    } \
    gpio_request(GPIO_START_NUM + 7, "silan-gpio"); \
    gpio_direction_output(GPIO_START_NUM + 7, 1); \
}

#define CS(i) gpio_set_value(GPIO_START_NUM + 18, i)
//#define RES(i) //gpio_set_value(GPIO_START_NUM + 14, i)
#define RES(i) gpio_set_value(GPIO_START_NUM + 7, i)
#define A0(i) gpio_set_value(GPIO_START_NUM + 16, i)
#define SI(i) gpio_set_value(GPIO_START_NUM + 15, i)
#define SCL(i) gpio_set_value(GPIO_START_NUM + 17, i)

void send_byte(unsigned char dat)
{
    unsigned char i;

    for (i = 0; i < 8; i ++)
    {
        SCL(0);
        if ((dat & 0x80) != 0)
            SI(1);
        else
            SI(0);

        dat <<= 1;
        ndelay(30);
        SCL(1);
        ndelay(30);
    }
}

void lcd_wr_cmd(unsigned char cmdx)
{
    A0(0);
    CS(0);
    send_byte(cmdx);
    CS(1);
}

void lcd_wr_data(unsigned char cmdx)
{
    A0(1);
    CS(0);
    send_byte(cmdx);
    CS(1);
}

#endif

void lcd_setx(unsigned char x)
{
    lcd_wr_cmd(0x10 | (x >> 4));
    lcd_wr_cmd(x & 0x0f);
}

void lcd_sety(unsigned char page)
{
    lcd_wr_cmd(page | 0xb0);
}

static void lcd_refresh_line(unsigned char* buf, int line)
{
    int j;

    lcd_setx(0);
    lcd_sety(line);
    for (j = 0; j < LCD_WIDTH; j ++)
        lcd_wr_data(*(buf + line*LCD_WIDTH + j));
}

static void lcd_refresh(unsigned char* buf)
{
    int j;

    for (j = 0; j < LCD_HEIGHT/8; j ++)
        lcd_refresh_line(buf, j);
}

void lcd_hardware_init(void)
{
    INIT_GPIO();

    RES(0); //reset
    mdelay(100);
    RES(1);
    lcd_wr_cmd(0xe2);
    mdelay(20);

#if 0
	lcd_wr_cmd(0xa2); //set 1/9bias

    lcd_wr_cmd(0xa0); //seg normal
    lcd_wr_cmd(0xc8); //com normal
    
    lcd_wr_cmd(0xa6); //display normal   
    lcd_wr_cmd(0x24); //v0 rb/ra
    lcd_wr_cmd(0x81); //vo mode
    lcd_wr_cmd(0x27); //vo value
    mdelay(20);

    lcd_wr_cmd(0x2c); //power set
    mdelay(100);
    lcd_wr_cmd(0x2e);
    mdelay(100);
    lcd_wr_cmd(0x2f);

    lcd_wr_cmd(0xf8); //booster select  
    lcd_wr_cmd(0x00); //4X

    lcd_wr_cmd(0xa4); //nomal display points
    lcd_wr_cmd(0xaf); //display on
    lcd_wr_cmd(0x40); //com0 ~ com64  
#else
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
#endif

    //lcd_refresh(lcd_buf); //clear lcd
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
            lcd_refresh(lcd_buf);
            break;
        
        case LCD_IOCTL_REFRESH_LINE:
            lcd_refresh_line(lcd_buf, arg);
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

    page = virt_to_phys(lcd_buf);
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

static int lcd_probe(struct platform_device* device)
{
    int err;

    //printk("####### silan lcd_probe start \n");
    
    silan_padmux_ctrl(SILAN_PADMUX_I2C1, PAD_OFF);
    if (!lcd_buf)
    {
        lcd_buf = (unsigned char*)kmalloc(PAGE_SIZE, GFP_KERNEL);
        if (!lcd_buf)
        {
            printk("lcd_probe err : lcd_buf kmalloc failed \n");
            err = -ENOMEM;
            goto failed;
        }
        memset(lcd_buf, 0, PAGE_SIZE);
    }
    
    if (misc_register(&lcd_misc) < 0)
    {
        printk("lcd_probe err : misc_register failed \n");
        err = -EBUSY;
        goto failed;
	}

    lcd_hardware_init();
    
    SetPageReserved(virt_to_page(lcd_buf));
    //printk("####### silan lcd_probe done \n");
    return 0;

failed:
    if (lcd_buf)
        kfree(lcd_buf);

    printk("####### silan lcd_probe failed \n");
    return err;
}

static int lcd_remove(struct platform_device* device)
{
    printk("lcd_remove \n");
    
    if (lcd_buf)
    {
        ClearPageReserved(virt_to_page(lcd_buf));
        kfree(lcd_buf);
    }
    
    return 0;
}

static int lcd_suspend(struct platform_device* device, pm_message_t msg)
{
    printk("lcd_suspend \n");

    return 0;
}

static int lcd_resume(struct platform_device* device)
{
    printk("lcd_resume \n");
	
    return 0;
}

static struct platform_driver lcd_driver = 
{
    .probe	= lcd_probe,
    .remove = lcd_remove,
    .suspend = lcd_suspend,
    .resume = lcd_resume,
    .driver = {
        .name = LCD_DEV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init lcd_init(void)
{
    int ret;

    printk("lcd_init \n");
    ret = platform_driver_register(&lcd_driver);
    if (ret)
        printk("lcd_init failed \n");
    else
        printk("lcd_init successful \n");

    return ret;
}

static void __exit lcd_exit(void)
{
    printk("lcd_exit \n");
    platform_driver_unregister(&lcd_driver);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");





