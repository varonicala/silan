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

#define DEVNUM 20
#define KEYDOWN_TIMEOUT 5*HZ

static const int key_table[DEVNUM] =
{
	KEY_A, KEY_B, KEY_C, KEY_D,
	KEY_E, KEY_F, KEY_G, KEY_H,
	KEY_I, KEY_J, KEY_K, KEY_L,
	KEY_M, KEY_N, KEY_O, KEY_P,
	KEY_Q, KEY_R, KEY_S, KEY_T
};

#define gpio_base 96

#define row1 (32+22)
#define row2 (32+23)
#define row3 (32+24)
#define row4 (32+25)

//#define volumn1 (32*3+4)
#define volumn1 (32+18)
#define volumn2 (32+19)
#define volumn3 (32+20)
#define volumn4 (32+21)

static int keypad[] = {volumn1, volumn2, volumn3, volumn4};
static struct input_dev *key_dev;
static struct delayed_work keypad_work;

static irqreturn_t keypad_isr(int irq, void *dev_id);

#ifdef CONFIG_SILAN_DLNA
static int __init silan_key_init(void);
static void __exit silan_key_exit(void);
#else
static int __init key_init(void);
static void __exit key_exit(void);
#endif
static void set_row_low(int val1, int val2, int val3, int val4)
{
	gpio_set_value(row1, val1);
	gpio_set_value(row2, val2);
	gpio_set_value(row3, val3);
	gpio_set_value(row4, val4);
	return ;
}

static int read_volumn(void)
{
	int i, val, ret = 0;
	unsigned long delay;
	
	for(i = 0; i < 4; i++){
		val = gpio_get_value(keypad[i]);
		if(val == 0){
			msleep(80);
			val = gpio_get_value(keypad[i]);
			if(val == 0){
				ret = i + 1;
				delay = jiffies + KEYDOWN_TIMEOUT;
				while(val == 0 && time_before(jiffies, delay)){
					val = gpio_get_value(keypad[i]);
					msleep(30);
				}
				break;
			}
		}
	}

	return ret;
}

static void input_the_key(int row, int volumn)
{
	int n = (row - 1) * 4 + volumn - 1;

	if(volumn){
		//printk("$$$$ %d, %d, key_table: %d $$$$\n", row, volumn, key_table[n]);
		input_report_key(key_dev, key_table[n], 1);  
		input_report_key(key_dev, key_table[n], 0);  
		input_sync(key_dev);
	}
}

static void keypad_work_fn(struct work_struct *work)
{
	int volumn, row;
	int i;
	int val1, val2, val3, val4;

	for(i = 0; i < 4; i++)
		disable_irq(keypad[i]+gpio_base);	

	set_row_low(1, 1, 1, 1);
	row = 5;
	volumn = read_volumn();
	if(!volumn){
		for(i = 1; i < 5; i++){
			row = i;
			volumn = 0;
			val1 = val2 = val3 = val4 = 1;
			switch(i){
				case 1:
					val1 = 0;
					break;
				case 2:
					val2 = 0;
					break;
				case 3: 
					val3 = 0;
					break;
				case 4:
					val4 = 0;
					break;
			}
			set_row_low(val1, val2, val3, val4);
			volumn = read_volumn();
			if(volumn)
				break;
		}
	}

	input_the_key(row, volumn);
	set_row_low(0, 0, 0, 0);
	for(i = 0; i < 4; i++)
		enable_irq(keypad[i]+gpio_base);	
}

static irqreturn_t keypad_isr(int irq, void *dev_id)
{
	//printk("#### %s  irq: %d ####\n", __func__, irq);
	schedule_delayed_work(&keypad_work, 0);
	return IRQ_HANDLED;
}

#ifdef CONFIG_SILAN_DLNA
static int __init silan_key_init(void)
#else
static int __init key_init(void)
#endif
{
	int error, i;
	
    INIT_DELAYED_WORK(&keypad_work, keypad_work_fn);

	for(i = 0; i < 4; i++){
		if(request_irq(gpio_base+keypad[i], keypad_isr, IRQF_DISABLED, "silan_keypad", NULL) != 0){
			printk(KERN_ERR "silan_keypad: Can't allocate irq %d\n", keypad[i]);
			return (-EIO);
		}
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

	gpio_request(row1, "silan-gpio");
 	gpio_direction_output(row1, 0);

	gpio_request(row2, "silan-gpio");
 	gpio_direction_output(row2, 0);
	
	gpio_request(row3, "silan-gpio");
 	gpio_direction_output(row3, 0);
	
	gpio_request(row4, "silan-gpio");
 	gpio_direction_output(row4, 0);

	printk("########Silan key regester ok\n");
	return 0;

err_free_dev:
	input_free_device(key_dev);
err_free_irq:
	for(i = 0; i < 4; i++)
		free_irq(gpio_base+keypad[i], keypad_isr);

	return error;
}

#ifdef CONFIG_SILAN_DLNA
static void __exit silan_key_exit(void)
#else
static void __exit key_exit(void)
#endif
{
	int i;
	input_unregister_device(key_dev);	//remove device
	for(i = 0; i < 4; i++)
		free_irq(gpio_base+keypad[i], keypad_isr);
}

#ifdef CONFIG_SILAN_DLNA
module_init(silan_key_init);
module_exit(silan_key_exit);
#else
module_init(key_init);
module_exit(key_exit);
#endif
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChenJianneng@silan.com");
