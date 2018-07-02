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


#define LEADER_MIN   1330*10  //us
#define LEADER_MAX   1370*10
#define DATA_1_MIN   205*10
#define DATA_1_MAX   245*10
#define DATA_0_MIN   92*10
#define DATA_0_MAX   132*10
 
#define REPEAT_MIN      1100*10
#define REPEAT_MAX      1150*10

#define IR_IDLE             0
#define IR_LEADER_LOW           1
#define IR_LEADER_HIGH          2
#define IR_CUSTOM           3
#define IR_REPEAT               4
         
#define cpuclk 10
#define CLK_LEADER_MIN  (cpuclk * 1300)  /* Leader minimum */
#define CLK_LEADER_MAX  (cpuclk * 1390)  /* Leader maximum */
#define CLK_DATA_1_MIN  (cpuclk * 195)   /* Data 1 minimum  */
#define CLK_DATA_1_MAX  (cpuclk * 255)   /* Data 1 maximum */
#define CLK_DATA_0_MIN  (cpuclk * 82)    /* Data 0 minimum */
#define CLK_DATA_0_MAX  (cpuclk * 142)   /* Data 0 maximum  */

#define CLK_REP_MIN  (cpuclk * 1000)     /* Data 0 minimum */
#define CLK_REP_MAX  (cpuclk * 1150)     /* Data 0 maximum  */

DECLARE_WAIT_QUEUE_HEAD(ir_wq);

#define CODENUM 24

static const struct
{ 
   unsigned char  ircode;
   int  keycode;
} ir_key_table[] = 
{
    { 0x82, KEY_0 },
    { 0x90, KEY_1 },
    { 0xa0, KEY_2 },
    { 0x80, KEY_3 },
    { 0xd2, KEY_4 },
    { 0xe2, KEY_5 },
    { 0xc2, KEY_6 },
    { 0x52, KEY_7 },
    { 0x62, KEY_8 },
    { 0x42, KEY_9 },
    { 0x70, KEY_BACK },
    { 0x30, KEY_POWER },
    { 0x10, KEY_MUTE },
    { 0x0a, KEY_VOLUMEUP },
    { 0x8a, KEY_VOLUMEDOWN },
    { 0x3a, KEY_PLAY },
    { 0xea, KEY_PAUSE },
    { 0xda, KEY_STOP },
    { 0x50, KEY_MENU },
    { 0x22, KEY_UP },
    { 0x12, KEY_DOWN },
    { 0x32, KEY_LEFT },
    { 0x02, KEY_RIGHT },
    { 0x60, KEY_ENTER },
    { 0x00, KEY_EJECTCD},
};



/* Private data structure */
struct ir_private {
    unsigned long *key_table;    /* Table for repetition keys */
    spinlock_t lock;        /* Spin lock */
    unsigned long last_jiffies;    /* Timestamp for last reception */
    unsigned char *ir_devname;
    unsigned int width;
    struct timeval     base_time;
    int previous_data;
};


struct input_dev *ir_dev;
struct ir_private ir_priv;
struct delayed_work ir_work;
static unsigned int width = 0;
static int ir_irq = 38;
static int gpiobase = 96;


static void ir_produce(void);
static irqreturn_t ir_isr(int irq, void *dev_id);
static int __init ir_init(void);
static void __exit ir_exit(void);


/* Produce data */
static void ir_produce()
{
    unsigned int data = 0;
    unsigned char check, key;

    struct timeval tm_p, tm_n;
    int value_n, flag = 0, data1;
    bool keycod[33];
    unsigned char i = 0;
    disable_irq(gpiobase+ir_irq);

#ifdef IR_DEBUG_IRQ
    printk("ir_produce, width %d\n", width);
#endif
   
     /*
    * If 4438's clock overflows, then reset. The only exception is
     * when REP is considered!!
     */
    if ((width >= LEADER_MIN) && (width <= LEADER_MAX))
    {
#ifdef IR_DEBUG_IRQ
    printk("leader\n");
#endif

        //stop and waiting for reading the nec code
        flag = 0;
        i = 0;
        do_gettimeofday(&tm_p);
        while(i < 32)
        {
            value_n = gpio_get_value(ir_irq);
            if(value_n == 1)
            {
                flag = 1;
            }
            if(flag == 1 && value_n == 0)
            {
                flag = 0;
                do_gettimeofday(&tm_n);
                width = 1000000 * (tm_n.tv_sec - tm_p.tv_sec)
                    + tm_n.tv_usec - tm_p.tv_usec;

                tm_p = tm_n;
                if ((width >= DATA_1_MIN) && (width <= DATA_1_MAX))
                    keycod[i++] = 1;
                else if ((width >= DATA_0_MIN) && (width <= DATA_0_MAX))
                    keycod[i++] = 0;
                else 
                {
                    enable_irq(gpiobase+ir_irq);
                    return ;
                }
            }
        }
        //code_valid = 1;
        for(i = 0; i < 32; i++)
            data = ((data << 1) | keycod[i]);

        ir_priv.previous_data = data;
        data1 = data & 0xff;
        check = ~((data >> 8) & 0xff);
        if (data1 != check)     /*do the double check */
        {
            enable_irq(gpiobase+ir_irq);
            return ;
        }
#ifdef IR_DEBUG_IRQ
        printk("$$$$$$data: 0x00%x\n", data);
        for(i = 0; i < 32; i++)
            printk("%d", keycod[i]);
        printk("\n");
#endif
     }
 
     else if ((width >= REPEAT_MIN) && (width <= REPEAT_MAX))
     {
          /* if the width is 2.25 ms, it is repeat code leader */
        // codeIR = previous_data | 0x100; /* Indicate a new code */
         data = ir_priv.previous_data;
#ifdef IR_DEBUG
         printk("repeat\n");
#endif
     }
     
#ifdef IR_DEBUG_IRQ
    if(data == 0)
    {
        printk("data error\n");
    }
    else 
    {
        printk("keycode : %x\n", data);
    }
#endif
    key = ~(data & 0xff);
    for(i = 0; i < CODENUM; i++)
    {
        if(key == ir_key_table[i].ircode)
        {    
            input_report_key(ir_dev, ir_key_table[i].keycode, 1);  
            input_report_key(ir_dev, ir_key_table[i].keycode, 0);  
            input_sync(ir_dev);
        }
    }    
    enable_irq(gpiobase+ir_irq);
return ;
}

static irqreturn_t ir_isr(int irq, void *dev_id)
{
    struct ir_private *priv = (struct ir_private *) dev_id;
    unsigned int overflow;
    struct timeval tv;
    
    do_gettimeofday(&tv);
    width = 1000000 * (tv.tv_sec - priv->base_time.tv_sec)
            + tv.tv_usec - priv->base_time.tv_usec;
    overflow = 0;

    //store current time
    priv->base_time.tv_sec = tv.tv_sec;
    priv->base_time.tv_usec = tv.tv_usec;

    if ((width >= CLK_LEADER_MIN) && (width <= CLK_LEADER_MAX))
    {
         width = LEADER_MAX;
    }
    else
    {
        if ((width >= CLK_REP_MIN) && (width <= CLK_REP_MAX))
        {
            width = REPEAT_MAX;
        }
        else
        {
            overflow = 1;
        }
    }
     if ((width >= LEADER_MIN) && (width <= LEADER_MAX))
        schedule_delayed_work(&ir_work, 0);

    return IRQ_HANDLED;
}

static void ir_work_fn(struct work_struct *work)
{
    return ir_produce();
}

static int __init ir_init(void)
{
    int error, i;
    
    gpio_request(ir_irq, "silan-gpio");
     gpio_direction_input(ir_irq);

    //hook up isr
    if(request_irq(gpiobase+ir_irq, ir_isr, IRQF_DISABLED, "sl_ir", &ir_priv) != 0)
    {
        printk(KERN_ERR "ir_input.c: Can't allocate irq %d\n", ir_irq);
        return (-EIO);
    }

    INIT_DELAYED_WORK(&ir_work, ir_work_fn);

    ir_dev = input_allocate_device();    //allocate space for ir_dev

    if (!ir_dev) 
    {                
        printk(KERN_ERR "ir_input.c: Not enough memory\n");
        error = -ENOMEM;
        goto err_free_irq;
    }
    
    ir_dev->evbit[0] = BIT_MASK(EV_KEY);        //set key type
    
    ir_dev->name = "sl-ir";
    ir_dev->dev.init_name = "ir_input";        
    
    //set key code
    for(i = 0; i < CODENUM; i++)
    {
        ir_dev->keybit[BIT_WORD(ir_key_table[i].keycode)] |= BIT_MASK(ir_key_table[i].keycode);
    }

    error = input_register_device(ir_dev);        //register the ir_input device

    if (error) 
    {
        printk(KERN_ERR "ir_input.c: Failed to register device\n");
        goto err_free_dev;
    }
    return 0;

err_free_dev:
    input_free_device(ir_dev);
err_free_irq:
    free_irq(gpiobase+ir_irq, ir_isr);
    return error;
}

static void __exit ir_exit(void)
{

    input_unregister_device(ir_dev);    //remove device
    free_irq(gpiobase+ir_irq, ir_isr);            //free ir_irq
}

module_init(ir_init);
module_exit(ir_exit);
MODULE_LICENSE("GPL");


