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

#define DEVNUM 7
#define KEY_NUM  7

#define KEYDOWN_TIMEOUT 5*HZ

static const int key_table[DEVNUM] =
{
    KEY_VOLUMEUP,
    KEY_VOLUMEDOWN,
    KEY_PLAYPAUSE,
    KEY_PREVIOUSSONG,
    KEY_NEXTSONG,
    KEY_1,
    KEY_2
};

static int gpio_base = 96;

#define volup      17
#define voldown   13
#define play      (32+14)
#define pre       18
#define next      14
#define input     16
#define tone      15

static int key_gpio[KEY_NUM] = {volup, voldown, play, pre, next, input, tone};

struct input_dev *button_dev;
struct delayed_work volup_work;
struct delayed_work voldown_work;
struct delayed_work play_work;
struct delayed_work pre_work;
struct delayed_work next_work;
struct delayed_work input_work;
struct delayed_work tone_work;

static irqreturn_t button_isr(int irq, void *dev_id);

static int __init button_init(void);
static void __exit button_exit(void);

static int button_delay(int button)
{
    int val = 0;
    unsigned long delay;

    val = gpio_get_value(button);
    if(val == 0){
        msleep(80);
        val = gpio_get_value(button);
        if(val == 0){
            delay = jiffies + KEYDOWN_TIMEOUT;
            while(val == 0 && time_before(jiffies, delay)){
                val = gpio_get_value(button);
                msleep(30);
            }
            return 0;
        }
    }

    return -1;        
}

static void volup_work_fn(struct work_struct *work)
{
    disable_irq(key_gpio[0]+gpio_base);
    if(!button_delay(volup)){
        printk(" %s %d \n", __func__, __LINE__);
        input_report_key(button_dev, key_table[0], 1);  
        input_report_key(button_dev, key_table[0], 0);  
        input_sync(button_dev);
    }
    enable_irq(key_gpio[0]+gpio_base);    
}

static void voldown_work_fn(struct work_struct *work)
{
    disable_irq(key_gpio[1]+gpio_base);    
    if(!button_delay(voldown)){
        printk(" %s %d \n", __func__, __LINE__);
        input_report_key(button_dev, key_table[1], 1);  
        input_report_key(button_dev, key_table[1], 0);  
        input_sync(button_dev);
    }
    enable_irq(key_gpio[1]+gpio_base);    
}

static void play_work_fn(struct work_struct *work)
{
    disable_irq(key_gpio[2]+gpio_base);    
    if(!button_delay(play)){
        printk(" %s %d \n", __func__, __LINE__);
        input_report_key(button_dev, key_table[2], 1);  
        input_report_key(button_dev, key_table[2], 0);  
        input_sync(button_dev);
    }
    enable_irq(key_gpio[2]+gpio_base);    
}

static void pre_work_fn(struct work_struct *work)
{
    disable_irq(key_gpio[3]+gpio_base);    
    if(!button_delay(pre)){
        printk(" %s %d \n", __func__, __LINE__);
        input_report_key(button_dev, key_table[3], 1);  
        input_report_key(button_dev, key_table[3], 0);  
        input_sync(button_dev);
    }
    enable_irq(key_gpio[3]+gpio_base);    
}

static void next_work_fn(struct work_struct *work)
{
    disable_irq(key_gpio[4]+gpio_base);    
    if(!button_delay(next)){
        printk(" %s %d \n", __func__, __LINE__);
        input_report_key(button_dev, key_table[4], 1);  
        input_report_key(button_dev, key_table[4], 0);  
        input_sync(button_dev);
    }
    enable_irq(key_gpio[4]+gpio_base);    
}

static void input_work_fn(struct work_struct *work)
{
    disable_irq(key_gpio[5]+gpio_base);    
    if(!button_delay(input)){
        printk(" %s %d \n", __func__, __LINE__);
        input_report_key(button_dev, key_table[5], 1);  
        input_report_key(button_dev, key_table[5], 0);  
        input_sync(button_dev);
    }
    enable_irq(key_gpio[5]+gpio_base);    
}

static void tone_work_fn(struct work_struct *work)
{
    disable_irq(key_gpio[6]+gpio_base);    
    if(!button_delay(tone)){
        printk(" %s %d \n", __func__, __LINE__);
        input_report_key(button_dev, key_table[6], 1);  
        input_report_key(button_dev, key_table[6], 0);  
        input_sync(button_dev);
    }
    enable_irq(key_gpio[6]+gpio_base);    
}

static irqreturn_t button_isr(int irq, void *dev_id)
{
    irq -= gpio_base;
    switch (irq){
        case volup:
            //schedule_work(&volup_work);
            schedule_delayed_work(&volup_work, 0);
            break;
        case voldown:
            schedule_delayed_work(&voldown_work, 0);
            break;
        case play:
            schedule_delayed_work(&play_work, 0);
            break;
        case pre:
            schedule_delayed_work(&pre_work, 0);
            break;
        case next:
            schedule_delayed_work(&next_work, 0);
            break;
        case input:
            schedule_delayed_work(&input_work, 0);
            break;
        case tone:
            schedule_delayed_work(&tone_work, 0);
            break;
        default:
            break;
    }    
    return IRQ_HANDLED;
}

static void request_all_gpios(void)
{
    int i = 0;
    for(i = 0; i < KEY_NUM; i++){
        gpio_request(key_gpio[i], "silan-gpio");
        gpio_direction_input(key_gpio[i]);
    }
}

static int request_all_irqs(void)
{
    int i = 0;
    for(i = 0; i < KEY_NUM; i++){
        if(request_irq(gpio_base+key_gpio[i], button_isr, IRQF_DISABLED, "silan_button", NULL) != 0){
            printk(KERN_ERR "silan_button: Can't allocate irq %d\n", key_gpio[i]);
            return (-EIO);
        }
    }
    return 0;
}

static void free_all_irqs(void)
{
    int i = 0;
    for(i = 0; i < KEY_NUM; i++){
        free_irq(gpio_base+key_gpio[i], button_isr);
    }
}

static void init_all_delayedwork(void)
{
    INIT_DELAYED_WORK(&volup_work,   volup_work_fn);
    INIT_DELAYED_WORK(&voldown_work, voldown_work_fn);
    INIT_DELAYED_WORK(&play_work,    play_work_fn);
    INIT_DELAYED_WORK(&pre_work,     pre_work_fn);
    INIT_DELAYED_WORK(&next_work,    next_work_fn);
    INIT_DELAYED_WORK(&input_work,   input_work_fn);
    INIT_DELAYED_WORK(&tone_work,    tone_work_fn);
}

static int __init button_init(void)
{
    int error, i;
    
    init_all_delayedwork();
    
    request_all_gpios();

    if(request_all_irqs())
        return (-EIO);
    
    button_dev = input_allocate_device();
    if(!button_dev){    
        printk(KERN_ERR "button: Not enough memory\n");
        error = -ENOMEM;
        goto err_free_irq;
    }

    button_dev->evbit[0] = BIT_MASK(EV_KEY);
    button_dev->name = "silan_button";
    
    for(i = 0; i < DEVNUM; i++){
        button_dev->keybit[BIT_WORD(key_table[i])] |= BIT_MASK(key_table[i]);
    }

    error = input_register_device(button_dev);
    if(error){
        printk(KERN_ERR "button: Failed to register device\n");
        goto err_free_dev;
    }

    printk("########Silan button regester ok\n");
    return 0;

err_free_dev:
    input_free_device(button_dev);
err_free_irq:
    free_all_irqs();

    return error;
}

static void __exit button_exit(void)
{
    input_unregister_device(button_dev);    //remove device
    
    free_all_irqs();
}

module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChenJianneng@silan.com");
