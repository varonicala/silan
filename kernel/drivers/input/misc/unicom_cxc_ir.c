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


DECLARE_WAIT_QUEUE_HEAD(ir_wq);

#define IR_DEV_NAME        "unicom_cxc_ir"
#define CODENUM 5

static const struct
{ 
   unsigned char  ircode;
   int  keycode;
} ir_key_table[] = 
{
    { 0x02, KEY_0 },
    { 0x0f, KEY_1 },
    { 0x5f, KEY_2 },
    { 0x06, KEY_3 },
    { 0x12, KEY_4 },
};

/* Private data structure */
struct ir_private {
    unsigned long *key_table;    /* Table for repetition keys */
    spinlock_t lock;        /* Spin lock */
    unsigned long last_jiffies;    /* Timestamp for last reception */
    unsigned char *ir_devname;
};

struct input_dev *ir_dev;
struct platform_device *pdev;
struct silan_unicom_cxc *irUnicom;
struct ir_private ir_priv;
struct delayed_work ir_work;

static void ir_produce(void);
static irqreturn_t ir_isr(int irq, void *dev_id);
//static int __init ir_init(void);
//static void __exit ir_exit(void);

static void ir_produce()
{
	unsigned int buf_key[2];
	unsigned char buf_ir;
	int i;
	irUnicom->ops->fifo_read(irUnicom, buf_key, UNICOM_CXC_MODULE_IR, 2, 3);
	buf_ir = ((unsigned char)(buf_key[0]))&0x000000ff;

    for(i = 0; i < CODENUM; i++)
    {
        if(buf_ir == ir_key_table[i].ircode)
		{
            input_report_key(ir_dev, ir_key_table[i].keycode, 1);  
            input_report_key(ir_dev, ir_key_table[i].keycode, 0);  
            input_sync(ir_dev);
			printk("send successful! \n");
        }
    }    
}
static irqreturn_t ir_isr(int ir_irq, void *dev_id)
{
	cxc_set_mcu2unicom_ir_clr();	
    schedule_delayed_work(&ir_work, 0);
}
static void ir_work_fn(struct work_struct *work)
{
    return ir_produce();
}

static int ir_probe(struct platform_device* pdev)
{
    int error, i;

	silan_unicom_cxc_probe(pdev);
    printk("unicom_cxc probe successful \n");
	irUnicom = silan_unicom_cxc_get("silan_unicom_cxc");	
    printk("unicom_cxc_probe ok and get_unicom_cxc \n");

    //hook up isr
    if(request_irq(irUnicom->ir_irq, ir_isr, 0, "unicom_ir", NULL) != 0)
    {
        printk(KERN_ERR "ir_input.c: Can't allocate irq %d\n", irUnicom->ir_irq);
        return (-EIO);
    }

    INIT_DELAYED_WORK(&ir_work, ir_work_fn);

    ir_dev = input_allocate_device();    //allocate space for ir_dev

    if (!ir_dev) 
    {                
        printk(KERN_ERR "unicom_cxc_ir.c: Not enough memory\n");
        error = -ENOMEM;
        goto err_free_irq;
    }
    
    ir_dev->evbit[0] = BIT_MASK(EV_KEY);        //set key type
    
    ir_dev->name = "unicom-ir";
    ir_dev->dev.init_name = "unicom_cxc_ir";        
    
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
    free_irq(irUnicom->ir_irq, ir_isr);
    return error;
	
}
static int ir_remove(struct platform_device* device)
{

	printk(" ir_device_remove\n");

    input_unregister_device(ir_dev);    //remove device
    free_irq(irUnicom->ir_irq, ir_isr);            //free ir_irq
    return 0;
}

static struct platform_driver ir_driver = 
{
    .probe	= ir_probe,
    .remove = ir_remove,
    .driver = {
		.name = IR_DEV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init unicom_ir_init(void)
{
    int ret;
    printk("unicom_cxc_ir_init! \n");
    ret = platform_driver_register(&ir_driver);
    if (ret)
        printk("unicom_cxc_ir_init failed \n");
    else
        printk("unicom_cxc_ir_init successful! \n");
	return ret;
}

static void __exit unicom_ir_exit(void)
{
    printk("ir_exit \n");
    platform_driver_unregister(&ir_driver);
}

module_init(unicom_ir_init);
module_exit(unicom_ir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chenwangxin");
MODULE_DESCRIPTION("UNICOM_CXC_IR driver");


