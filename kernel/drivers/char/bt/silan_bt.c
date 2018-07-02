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

#include "type_def.h"
#include "silan_bt.h"

#define WD_LEN 4096

void init_dmac(void);
void init_uart(UINT32 baudrate, UINT32 parity);
void init_bt_uart(UINT32 baudrate, UINT32 parity);
void deinit_bt_uart(void);
size_t get_uart_rcv_data_len_in_buf();
size_t get_uart_rcv_data_len_in_buf_con();
UINT8 * get_uart_rcv_buf_ptr_lock();
void free_uart_rcv_buf_lock(size_t len);
int uart_read(UINT8 * buf, size_t len);
int uart_transmit(UINT8 * buf, size_t len);

char bt_buf[WD_LEN];
static int bt_open(struct inode* inode, struct file* filp)
{
	return 0;
}

static ssize_t bt_read(struct file* filp, char __user* buf, size_t len, loff_t* pos)
{
	int size;

	if (len > WD_LEN)
		return 0;

	uart_read(bt_buf, len);
	if(copy_to_user(buf, bt_buf, len))
		return 0;

	return size;
}

static ssize_t bt_write(struct file* filp, const char __user* buf, size_t len, loff_t* pos)
{
	int size;

	if (len > WD_LEN)
		return 0;

	if(copy_from_user(bt_buf, buf, len))
		return 0;

	size = uart_transmit(bt_buf, len);
    return size;
}

static long bt_ioctl(struct file* filp, u_int cmd, u_long arg)
{
    int num, *targ, ret = 0;

    switch(cmd)
    {
		case BT_IOCTL_INIT_UART:
			init_uart((int)(arg&0xffffffff), (int)((arg>>32)&0xffffffff));
			break;
		
		case BT_IOCTL_INIT_BT_UART:
			init_bt_uart((int)(arg&0xffffffff), (int)((arg>>32)&0xffffffff));
			break;

		case BT_IOCTL_DEINIT:
			deinit_bt_uart();
			break;

		case BT_IOCTL_GET_RCV_LEN:
			num = get_uart_rcv_data_len_in_buf();
			targ = arg;
			*targ = num;
			break;

		case BT_IOCTL_GET_RCV_LEN_CON:
			num = get_uart_rcv_data_len_in_buf_con();
			targ = arg;
			*targ = num;
			break;

		case BT_IOCTL_GET_RCV_BUF_LOCK:
			num = get_uart_rcv_buf_ptr_lock();
			targ = arg;
			*targ = num;
			break;

		case BT_IOCTL_FREE_RCV_BUF_LOCK:
			free_uart_rcv_buf_lock(arg);
			break;

        default:
            break;
    }

    return ret;
}

static int bt_release(struct inode* inode, struct file* filp)
{
    return 0;
}

struct file_operations bt_fops = {
    .owner = THIS_MODULE,
    .open = bt_open,
    .read = bt_read,
    .write = bt_write,
    .unlocked_ioctl = bt_ioctl,
    .release = bt_release,
};

static struct miscdevice bt_misc = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = BT_DEV_NAME,
    .fops    = &bt_fops,
};

static int bt_probe(struct platform_device* device)
{
    int err;

    printk("####### silan bt_probe start \n");
   
	init_dmac();
    if (misc_register(&bt_misc) < 0)
    {
        printk("bt_probe err : misc_register failed \n");
        err = -EBUSY;
        goto failed;
	}

    printk("####### silan bt_probe done \n");
    return 0;

failed:

    printk("####### silan bt_probe failed \n");
    return err;
}

static int bt_remove(struct platform_device* device)
{
    printk("bt_remove \n");
    
    return 0;
}

static int bt_suspend(struct platform_device* device, pm_message_t msg)
{
    printk("bt_suspend \n");

    return 0;
}

static int bt_resume(struct platform_device* device)
{
    printk("bt_resume \n");
	
    return 0;
}

static struct platform_driver bt_driver = 
{
    .probe	= bt_probe,
    .remove = bt_remove,
    .suspend = bt_suspend,
    .resume = bt_resume,
    .driver = {
        .name = BT_DEV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init bt_init(void)
{
    int ret;

    printk("bt_init \n");
    ret = platform_driver_register(&bt_driver);
    if (ret)
        printk("bt_init failed \n");
    else
        printk("bt_init successful \n");

    return ret;
}

static void __exit bt_exit(void)
{
    printk("bt_exit \n");
    platform_driver_unregister(&bt_driver);
}

module_init(bt_init);
module_exit(bt_exit);

MODULE_LICENSE("GPL");





