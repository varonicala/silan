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
#include <silan_def.h>
#include <silan_resources.h>

//#define IR_DEBUG

#define TIMER_COUNTER 0
#define TIMER_COMPARE 4
#define TIMER_CONTROL 8

#define LEADER_MIN   1330*10  //us
#define LEADER_MAX   1370*10
#define DATA_1_MIN   205*10
#define DATA_1_MAX   245*10
#define DATA_0_MIN   92*10
#define DATA_0_MAX   132*10
#define REPEAT_MIN   1100*10
#define REPEAT_MAX   1150*10

#define cpuclk 10
#define CLK_LEADER_MIN  (cpuclk * 1300)  /* Leader minimum */
#define CLK_LEADER_MAX  (cpuclk * 1390)  /* Leader maximum */
#define CLK_DATA_1_MIN  (cpuclk * 195)   /* Data 1 minimum  */
#define CLK_DATA_1_MAX  (cpuclk * 255)   /* Data 1 maximum */
#define CLK_DATA_0_MIN  (cpuclk * 82)    /* Data 0 minimum */
#define CLK_DATA_0_MAX  (cpuclk * 142)   /* Data 0 maximum  */
#define CLK_REP_MIN     (cpuclk * 1000)  /* REP  0 minimum */
#define CLK_REP_MAX     (cpuclk * 1150)  /* REP  0 maximum  */

#define IR_BIT_NUM 32

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
    { 0x30, KEY_POWER },
    { 0x0a, KEY_VOLUMEUP },
    { 0x8a, KEY_VOLUMEDOWN },
    { 0x3a, KEY_PLAYPAUSE },
    { 0x10, KEY_MUTE },
    { 0x70, KEY_BACK },
    { 0xda, KEY_STOP },
    { 0x22, KEY_UP },
    { 0x12, KEY_DOWN },
    { 0x60, KEY_ENTER },
	{ 0x28, KEY_PREVIOUSSONG },
	{ 0x08, KEY_NEXTSONG },
};

#define CODENUM (sizeof(ir_key_table)/sizeof(ir_key_table[0]))

#define TIMER_BASE (SILAN_TIMER_BASE + 0x30)

struct input_dev *ir_dev;
struct delayed_work ir_work;
static int ir_irq = 46;
static int gpiobase = 96;
static int ir_code_cnt = 0;
static int ir_code_tbl[IR_BIT_NUM] = {0};

static int read_timer(void);
static void ir_produce(void);
static irqreturn_t ir_isr(int irq, void *dev_id);
static int __init ir_init(void);
static void __exit ir_exit(void);

static int read_timer(void)
{
    int val, cnt, usec;

    val = readl(TIMER_BASE + TIMER_COUNTER);
    cnt = get_silan_pllclk()/1000000;
    usec = val/cnt;

    return 2*usec;
}

static void ir_produce(void)
{
    unsigned int data = 0, i;
    unsigned char data1, check, key;

    disable_irq(gpiobase + ir_irq);
    for (i = 0; i < IR_BIT_NUM; i++)
        data = ((data << 1) | ir_code_tbl[i]);
#ifdef IR_DEBUG
    printk("ir_produce data %x\n", data);
#endif
    data1 = data & 0xff;
    check = ~((data >> 8) & 0xff);
#ifdef IR_DEBUG
    printk("ir_produce data1 %x check %x\n", data1, check);
#endif
    if (data1 == check)
    {
        key = ~(data & 0xff);
        //printk("keycode : %x\n", key);
        for (i = 0; i < CODENUM; i++)
        {
            if (key == ir_key_table[i].ircode)
            {
                input_report_key(ir_dev, ir_key_table[i].keycode, 1);
                input_report_key(ir_dev, ir_key_table[i].keycode, 0);
                input_sync(ir_dev);
            }
        }
    }

    ir_code_cnt = 0;
    memset(ir_code_tbl, 0, IR_BIT_NUM);
    writel(0, TIMER_BASE + TIMER_CONTROL);
    writel(0, TIMER_BASE + TIMER_COUNTER);
    writel(0xffffffff, TIMER_BASE + TIMER_COMPARE);

    enable_irq(gpiobase + ir_irq);
    return;
}

static irqreturn_t ir_isr(int irq, void *dev_id)
{
    unsigned int width, usec;
    static unsigned int prev_usec = 0;

    if (ir_code_cnt >= IR_BIT_NUM)
        return IRQ_HANDLED;

    writel(1, TIMER_BASE + TIMER_CONTROL);
    usec = read_timer();
    if (prev_usec > usec)
        prev_usec = 0;

    width = usec - prev_usec;
    prev_usec = usec;
    if ((width >= CLK_DATA_1_MIN) && (width <= CLK_DATA_1_MAX))
        ir_code_tbl[ir_code_cnt++] = 1;
    else if ((width >= CLK_DATA_0_MIN) && (width <= CLK_DATA_0_MAX))
        ir_code_tbl[ir_code_cnt++] = 0;
    else if ((width >= CLK_LEADER_MIN) && (width <= CLK_LEADER_MAX))
    {
         ir_code_cnt = 0;
         memset(ir_code_tbl, 0, IR_BIT_NUM);
    }

    if (ir_code_cnt >= IR_BIT_NUM)
        schedule_delayed_work(&ir_work, 0);

    return IRQ_HANDLED;
}

static void ir_work_fn(struct work_struct *work)
{
    ir_produce();
}

static int __init ir_init(void)
{
    int error, i;

    gpio_request(ir_irq, "silan-gpio");
    gpio_direction_input(ir_irq);

    if (request_irq(gpiobase+ir_irq, ir_isr, IRQF_DISABLED, "sl_ir", NULL) != 0)
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
    for (i = 0; i < CODENUM; i++)
    {
        ir_dev->keybit[BIT_WORD(ir_key_table[i].keycode)] |= BIT_MASK(ir_key_table[i].keycode);
    }

    error = input_register_device(ir_dev);        //register the ir_input device
    if (error)
    {
        printk(KERN_ERR "ir_input.c: Failed to register device\n");
        goto err_free_dev;
    }

    writel(0, TIMER_BASE + TIMER_CONTROL);
    writel(0, TIMER_BASE + TIMER_COUNTER);
    writel(0xffffffff, TIMER_BASE + TIMER_COMPARE);
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
    free_irq(gpiobase+ir_irq, ir_isr);  //free ir_irq
}

module_init(ir_init);
module_exit(ir_exit);
MODULE_LICENSE("GPL");


