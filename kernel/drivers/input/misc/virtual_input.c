#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/gpio.h>
#include <linux/wait.h>
#include <linux/input.h>
#include <linux/init.h>

#define DEVNUM 12

static const int key_table[DEVNUM] =
{
	KEY_POWER,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_PLAY,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_ENTER,
	KEY_HOME,
	KEY_MENU,
	KEY_BACK
};

struct input_dev *button_dev;

static int __init button_init(void)
{
	int error, i;

	button_dev = input_allocate_device();
	if(!button_dev)
	{	
			printk(KERN_ERR "button: Not enough memory\n");
			error = -ENOMEM;
	}
	
	button_dev->evbit[0] = BIT_MASK(EV_KEY);
	
	for(i = 0; i < DEVNUM; i++)
		button_dev->keybit[BIT_WORD(key_table[i])] |= BIT_MASK(key_table[i]);

	button_dev->name = "silan_button";
	
	error = input_register_device(button_dev);
	if(error)
	{
			printk(KERN_ERR "button: Failed to register device\n");
			goto err_free_dev;
	}

	return 0;

err_free_dev:
	input_free_device(button_dev);

	return error;
}

static void __exit button_exit(void)
{
	input_unregister_device(button_dev);
}

module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Jianneng@silan.com");

