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
#include "asm/mach-silan/dlna/silan_unicom.h"

#define DEVNUM 20

static const int key_table[DEVNUM] =
{
	KEY_A, KEY_B, KEY_C, KEY_D,
	KEY_E, KEY_F, KEY_G, KEY_H,
	KEY_I, KEY_J, KEY_K, KEY_L,
	KEY_M, KEY_N, KEY_O, KEY_P,
	KEY_Q, KEY_R, KEY_S, KEY_T
};

#define gpio_base 96

static int n;

static struct input_dev *key_dev;
static struct delayed_work keypad_work;
static struct silan_unicom *silan_unicom_keypad;
static irqreturn_t keypad_isr(int irq, void *dev_id);

static int __init unicom_key_init(void);
static void __exit unicom_key_exit(void);


static void input_the_key()
{
	input_report_key(key_dev, key_table[n], 1);  
	input_report_key(key_dev, key_table[n], 0);  
	input_sync(key_dev);
}

static void keypad_work_fn(struct work_struct *work)
{
	disable_irq(gpio_base);	

	UNICOM_CMD_ST keypad = {0};
	
	silan_unicom_keypad->ops->fifo_read(silan_unicom_keypad, &keypad, UNICOM_MODULE_KEYPAD);

	input_the_key();
	enable_irq(gpio_base);	
}

static irqreturn_t keypad_isr(int irq, void *dev_id)
{
	//printk("#### %s  irq: %d ####\n", __func__, irq);
	schedule_delayed_work(&keypad_work, 0);
	return IRQ_HANDLED;
}

static int __init unicom_key_init(void)
{
	int error, i;

		if(request_irq(gpio_base, keypad_isr, IRQF_DISABLED, "silan_keypad", NULL) != 0){
	//		printk(KERN_ERR "silan_keypad: Can't allocate irq %d\n", keypad[i]);
		}

	key_dev = input_allocate_device();
	if(!key_dev){	
		printk(KERN_ERR "key: Not enough memory\n");
		error = -ENOMEM;
		goto err_free_irq;
	}

	key_dev->evbit[0] = BIT_MASK(EV_KEY);
	key_dev->name = "silan_keypad";
	
	for(i = 0; i < DEVNUM; i++){
		key_dev->keybit[BIT_WORD(key_table[i])] |= BIT_MASK(key_table[i]);
	}

	error = input_register_device(key_dev);
	if(error){
		printk(KERN_ERR "key: Failed to register device\n");
		goto err_free_dev;
	}

	INIT_DELAYED_WORK(&keypad_work, keypad_work_fn);

	printk("########Silan key regester ok\n");
	return 0;

err_free_dev:
	input_free_device(key_dev);
err_free_irq:
		free_irq(gpio_base, keypad_isr);

	return error;
}

static void __exit unicom_key_exit(void)
{
	input_unregister_device(key_dev);	//remove device
	free_irq(gpio_base, keypad_isr);
}

module_init(unicom_key_init);
module_exit(unicom_key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("chenwangxin@silan.com");
