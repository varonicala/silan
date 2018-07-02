#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <silan_resources.h>
#include <asm/io.h>

#define ADKEY_DIS  10
#define POLL_TIME  200

static const struct
{ 
   u32  adcode;
   u32  keycode;
} ad_keys_table[] = 
{
    { 0xC10, KEY_1 },
    { 0xA20, KEY_2 },
    { 0xE65, KEY_3 },
    { 0x654, KEY_4 },
    { 0x37A, KEY_5 },
#if 0
    { 0x0, KEY_0 },
    { 0x0, KEY_1 },
    { 0x0, KEY_2 },
    { 0x0, KEY_3 },
    { 0x0, KEY_4 },
    { 0x0, KEY_5 },
    { 0x0, KEY_6 },
    { 0x0, KEY_7 },
    { 0x0, KEY_8 },
    { 0x0, KEY_9 },
    { 0x0, KEY_POWER },
    { 0x0, KEY_VOLUMEUP },
    { 0x0, KEY_VOLUMEDOWN },
    { 0x0, KEY_PLAYPAUSE },
    { 0x0, KEY_MUTE },
    { 0x0, KEY_BACK },
    { 0x0, KEY_STOP },
#endif
};

struct sl_ad_keys {
	struct input_dev *input;
	struct mutex lock;
	struct delayed_work input_work;
    unsigned int poll_time;
};

#define MAX_ADC_CH_NUM 7
char ad_keys_ch_table[MAX_ADC_CH_NUM];
struct sl_ad_keys *sl_ad_keys_g = NULL;

static int key_math(int val, int key){
    int val1, val2;
    if(key+ADKEY_DIS >= 4096){
        val1 = 4096;
    }
    else{
        val1 = key+ADKEY_DIS;
    }

    if(key-ADKEY_DIS<=0){
        val2 =  0;
    }
    else{
        val2 = key-ADKEY_DIS;
    }

    if(val >= val2 && val <= val1)
        return 0;
    return -1;
}

static void report_key(struct input_dev *input_dev, u32 key, int ch)
{
    int len = (sizeof(ad_keys_table)/(sizeof(ad_keys_table[0])));
    int i;
    for(i = 0; i < len; i++){
        if(!key_math(key, ad_keys_table[i].adcode)){
            //printk("##### report_key %x, ch %d\n", ad_keys_table[i].keycode, ch);
            input_report_key(input_dev, ad_keys_table[i].keycode + 10*ch, 1);
            input_report_key(input_dev, ad_keys_table[i].keycode + 10*ch, 0);
            //input_report_key(input_dev, ad_keys_table[i].keycode, 1);
            //input_report_key(input_dev, ad_keys_table[i].keycode, 0);
            input_sync(input_dev);
        }
    }
}
#define ADC_SIN_FINISH     (1<<1)
static int read_adc_value(struct sl_ad_keys *sl_ad_keys)
{
    u32 val = readl(SILAN_ADC_BASE + 0x000);
    val |= (1<<8);
    writel(val, SILAN_ADC_BASE + 0x000);
    //udelay(10);
    while ((readl(SILAN_ADC_BASE + 0xc) & ADC_SIN_FINISH) == 0);
    writel(ADC_SIN_FINISH, SILAN_ADC_BASE + 0x18);

    return readl(SILAN_ADC_BASE + 0x008);
}

static void sl_ad_keys_input_work_func(struct work_struct *work)
{
	struct sl_ad_keys *sl_ad_keys;
	int i, val;

    sl_ad_keys = container_of((struct delayed_work *)work, struct sl_ad_keys, input_work);

    mutex_lock(&sl_ad_keys->lock);
    for (i = 0; i < MAX_ADC_CH_NUM; i++)
    {
        if (ad_keys_ch_table[i])
        {
            val = readl(SILAN_ADC_BASE + 0x000);
            val &= 0xFFFFFF8F; 
            val |= (i<<4);
            writel(val, SILAN_ADC_BASE + 0x000);

            val = read_adc_value(sl_ad_keys);
            if (val < 0)
                printk("Read the adc value error!\n");
            else
            {
                //printk("value is 0x%x ch %d\n", val, i);
                report_key(sl_ad_keys->input, val, i);
            }
        }
    }
	schedule_delayed_work(&sl_ad_keys->input_work, msecs_to_jiffies(sl_ad_keys->poll_time));
	mutex_unlock(&sl_ad_keys->lock);
}

static void sl_adkeys_init(struct sl_ad_keys *sl_ad_keys, int ch)
{
    int i;

    INIT_DELAYED_WORK(&sl_ad_keys->input_work, sl_ad_keys_input_work_func);

	mutex_init(&sl_ad_keys->lock);
#if 1
    // Init ADC
    u32 val = readl(SILAN_ADC_BASE + 0x000);
    val |= (1<<10);
    val &= ~(1<<0);
    //val &= 0xFFFFFF8F; 
    //val |= (4<<4);
    writel(val, SILAN_ADC_BASE + 0x000);
    //writel(0x1, SILAN_CR_BASE + 0x00c);
    //writel(1<<4, SILAN_CR_BASE + 0x00c);
    writel(ch, SILAN_CR_BASE + 0x00c);

    for (i = 0; i < MAX_ADC_CH_NUM; i++)
    {
        ad_keys_ch_table[i] = ch&0x1;
        ch >>= 1;
    }

    i = readl(SILAN_ADC_BASE + 0xc);
    writel(i, SILAN_ADC_BASE + 0x18);
#endif
}

static void sl_ad_keys_enable(struct sl_ad_keys *sl_ad_keys)
{
	schedule_delayed_work(&sl_ad_keys->input_work, msecs_to_jiffies(sl_ad_keys->poll_time));
}

static void sl_ad_keys_disable(struct sl_ad_keys *sl_ad_keys)
{
	cancel_delayed_work_sync(&sl_ad_keys->input_work);
}

static int *sl_ad_keys_event(struct input_dev *dev, unsigned int type, unsigned int code, int value)
{
    if (sl_ad_keys_g == NULL)
        return NULL;

    if (code == 1)
    {
        sl_adkeys_init(sl_ad_keys_g, value);
        sl_ad_keys_enable(sl_ad_keys_g);
    }
    else
        sl_ad_keys_disable(sl_ad_keys_g);

    return (int *)sl_ad_keys_g;
}

static int __devinit sl_ad_keys_probe(struct platform_device *pdev)
{
	int err = -ENOMEM, i, len, j;
	struct sl_ad_keys *sl_ad_keys;
	struct input_dev *input_dev;

	sl_ad_keys = kmalloc(sizeof(struct sl_ad_keys), GFP_KERNEL);
	if (!sl_ad_keys)
		return err;

	//sl_ad_keys->sl_ad = dev_get_drvdata(pdev->dev.parent);

	input_dev = input_allocate_device();
	if (!input_dev)
		goto fail;

	sl_ad_keys->input = input_dev;

	platform_set_drvdata(pdev, sl_ad_keys);
	input_dev->evbit[0] = BIT_MASK(EV_KEY);
	input_dev->name = "sl_ad-keys";
	//input_dev->phys = "sl_ad-keys/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &pdev->dev;
    sl_ad_keys->poll_time = POLL_TIME;

    len = (sizeof(ad_keys_table)/sizeof(ad_keys_table[0]));
    for(i = 0; i < len; i++){
        for (j = 0; j < MAX_ADC_CH_NUM; j++)
            input_dev->keybit[BIT_WORD(ad_keys_table[i].keycode + 10*j)] |= BIT_MASK(ad_keys_table[i].keycode + 10*j);
    }
    input_dev->event = sl_ad_keys_event;

	err = input_register_device(input_dev);
	if (err){
		goto fail_allocate;
    }
    sl_ad_keys_g = sl_ad_keys;
#if 0
    sl_adkeys_init(sl_ad_keys);

	sl_ad_keys_enable(sl_ad_keys);
#endif
    printk("###### %s %d #####\n", __func__, __LINE__);
    
    return 0;

fail_register:
	input_unregister_device(input_dev);
	goto fail;
fail_allocate:
	input_free_device(input_dev);
fail:
	kfree(sl_ad_keys);
	return err;
}

static int __devexit sl_ad_keys_remove(struct platform_device *pdev)
{
	struct sl_ad_keys *sl_ad_keys = platform_get_drvdata(pdev);

	input_unregister_device(sl_ad_keys->input);
	kfree(sl_ad_keys);

	return 0;
}

static struct platform_driver sl_ad_keys_device_driver = {
	.probe		= sl_ad_keys_probe,
	.remove		= __devexit_p(sl_ad_keys_remove),
	.driver		= {
		.name	= "sl_ad-keys",
		.owner	= THIS_MODULE,
	}
};

static int __init sl_ad_keys_init(void)
{
	return platform_driver_register(&sl_ad_keys_device_driver);
};

static void __exit sl_ad_keys_exit(void)
{
	platform_driver_unregister(&sl_ad_keys_device_driver);
};

module_init(sl_ad_keys_init);
module_exit(sl_ad_keys_exit);

MODULE_DESCRIPTION("Silan AD keys driver");
MODULE_AUTHOR("Chen Jianneng <chenjianneng@silan.com.cn>");
MODULE_LICENSE("GPL");
