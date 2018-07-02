/*linux/drivers/gpio/silan_gpio.c
 *Copyright(c)2011 Silan
 * lifei <lifei@silan.com.cn>
 *
 * Silan GPIO Controller driver
 *
 *Changelog:
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/hardirq.h>

#include <linux/bitops.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <linux/slab.h>

#include <silan_gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <silan_padmux.h>


#define IRQF_VALID (1 << 0)
//#define GPIO_DEBUG

#define GPIODAT 0x000
#define GPIODIR 0x400
#define GPIOIS  0x404
#define GPIOIBE 0x408
#define GPIOIEV 0x40C
#define GPIOIE  0x410
#define GPIORIS 0x414
#define GPIOMIS 0x418
#define GPIOIC  0x41C

#define SL_GPIO_NR	32
//#define SL_GPIO_IRQBASE 64+32
struct sl_gpio {
	/* We use a list of pl061_gpio structs for each trigger IRQ in the main
	 * interrupts controller of the system. We need this to support systems
	 * in which more that one PL061s are connected to the same IRQ. The ISR
	 * interates through this list to find the source of the interrupt.
	 */
	struct list_head	list;

	/* Each of the two spinlocks protects a different set of hardware
	 * regiters and data structurs. This decouples the code of the IRQ from
	 * the GPIO code. This also makes the case of a GPIO routine call from
	 * the IRQ code simpler.
	 */
	spinlock_t		lock;		/* GPIO registers */
	spinlock_t		irq_lock;	/* IRQ registers */
	
	struct resource 	*area;
	
	void __iomem		*base;
	unsigned		irq_base;
	unsigned 		gpio_irqbase;
	struct gpio_chip	gc;
};

//global variable
struct sl_gpio chip;

static int sl_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct sl_gpio *chip = container_of(gc, struct sl_gpio, gc);
	unsigned long flags;
	unsigned int gpiodir;
#ifdef GPIO_DEBUG
	printk("enter sl_direction_input offset %x\n", offset);
#endif
	if (offset >= gc->ngpio)
		return -EINVAL;

	spin_lock_irqsave(&chip->lock, flags);
	gpiodir = readl(chip->base + GPIODIR);
	gpiodir &= ~(1 << offset);
	writel(gpiodir, chip->base + GPIODIR);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int sl_direction_output(struct gpio_chip *gc, unsigned offset,
		int value)
{
	struct sl_gpio *chip = container_of(gc, struct sl_gpio, gc);
	unsigned long flags;
	unsigned int gpiodir;
	unsigned int gpiodata;
#ifdef GPIO_DEBUG
	printk("enter sl_direction_output, offset %x, value %x\n", offset, value);
#endif
	if (offset >= gc->ngpio)
		return -EINVAL;

	spin_lock_irqsave(&chip->lock, flags);
	gpiodata = readl(chip->base + GPIODAT);

	gpiodata &= ~(1 << offset);
	gpiodata |= value << offset;
	writel(gpiodata, chip->base + GPIODAT);
	gpiodir = readl(chip->base + GPIODIR);
	gpiodir |= 1 << offset;
	writel(gpiodir, chip->base + GPIODIR);

	/*
	 * gpio value is set again, because sl doesn't allow to set value of
	 * a gpio pin before configuring it in OUT mode.
	 */
	writel(gpiodata, chip->base + GPIODAT);
	//writel(!!value << offset, chip->base + GPIODAT);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int sl_get_value(struct gpio_chip *gc, unsigned offset)
{

	struct sl_gpio *chip = container_of(gc, struct sl_gpio, gc);
#ifdef GPIO_DEBUG
	printk("enter sl_get_value, offset %x\n", offset);
#endif
	return (readl(chip->base + GPIODAT)&(1<<offset)) >> offset;
}

static void sl_set_value(struct gpio_chip *gc, unsigned offset, int value)
{

	struct sl_gpio *chip = container_of(gc, struct sl_gpio, gc);
	unsigned int value_new;
#ifdef GPIO_DEBUG
	printk("enter sl_set_value, offset %x , value %x\n", offset, value);
#endif
	value_new = readl(chip->base + GPIODAT);
	value_new &= ~(1 << offset);
	value_new |= (!!value) << offset;
	writel(value_new, chip->base + GPIODAT);
//	writel(value << offset, chip->base + (1 << (offset + 2)));
}

static int sl_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct sl_gpio *chip = container_of(gc, struct sl_gpio, gc);
#ifdef GPIO_DEBUG_IRQ
	printk("enter sl_to_irq, offset %x\n", offset);
#endif

	if (chip->gpio_irqbase == (unsigned) -1)
		return -EINVAL;

	return chip->gpio_irqbase + offset;
}

/*
 * sl GPIO IRQ
 */
static void sl_irq_disable(struct irq_data *d)
{

	struct sl_gpio *chip = irq_data_get_irq_chip_data(d);
	int offset = d->irq - chip->gpio_irqbase;
	unsigned long flags;
	u32 gpioie;
#ifdef GPIO_DEBUG
	printk("enter sl_irq_disable, irq %x\n", d->irq);
#endif
	spin_lock_irqsave(&chip->irq_lock, flags);
	gpioie = readl(chip->base + GPIOIE);
	gpioie &= ~(1 << offset);
	writel(gpioie, chip->base + GPIOIE);
	spin_unlock_irqrestore(&chip->irq_lock, flags);
}

static void sl_irq_enable(struct irq_data *d)
{

	struct sl_gpio *chip = irq_data_get_irq_chip_data(d);
	int offset = d->irq - chip->gpio_irqbase;
	unsigned long flags;
	u32 gpioie;
#ifdef GPIO_DEBUG
	printk("enter sl_irq_enable, irq %x\n", d->irq);
#endif
	spin_lock_irqsave(&chip->irq_lock, flags);
	gpioie = readl(chip->base + GPIOIE);
	gpioie |= 1 << offset;
	writel(gpioie, chip->base + GPIOIE);
	spin_unlock_irqrestore(&chip->irq_lock, flags);
}

static int sl_irq_type(struct irq_data *d, unsigned int trigger)
{
	struct sl_gpio *chip = irq_data_get_irq_chip_data(d);

	int offset = d->irq - chip->gpio_irqbase;
	unsigned long flags;
	u32 gpiois, gpioibe, gpioiev;
#ifdef GPIO_DEBUG
	printk("enter sl_irq_type, irq %x, trigger %x\n", d->irq, trigger);
#endif
	if (offset < 0 || offset >= SL_GPIO_NR)
		return -EINVAL;

	spin_lock_irqsave(&chip->irq_lock, flags);

	gpioiev = readl(chip->base + GPIOIEV);

	gpiois = readl(chip->base + GPIOIS);
	if (trigger & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW)) {
		gpiois |= 1 << offset;
		if (trigger & IRQ_TYPE_LEVEL_HIGH)
			gpioiev |= 1 << offset;
		else
			gpioiev &= ~(1 << offset);
	} else
		gpiois &= ~(1 << offset);
	writel(gpiois, chip->base + GPIOIS);

	gpioibe = readl(chip->base + GPIOIBE);
	if ((trigger & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH)
		gpioibe |= 1 << offset;
	else {
		gpioibe &= ~(1 << offset);
		if (trigger & IRQ_TYPE_EDGE_RISING)
			gpioiev |= 1 << offset;
		else if (trigger & IRQ_TYPE_EDGE_FALLING)
			gpioiev &= ~(1 << offset);
	}
	writel(gpioibe, chip->base + GPIOIBE);

	writel(gpioiev, chip->base + GPIOIEV);

	spin_unlock_irqrestore(&chip->irq_lock, flags);

	return 0;
}

static struct irq_chip sl_irqchip = {
	.name		= "GPIO",
	.irq_enable	= sl_irq_enable,
	.irq_disable	= sl_irq_disable,
	.irq_set_type	= sl_irq_type,
};

static void sl_irq_handler(unsigned irq, struct irq_desc *desc)
{
	struct list_head *chip_list = irq_get_handler_data(irq);
	struct list_head *ptr;
	struct sl_gpio *chip;
#ifdef GPIO_DEBUG_IRQ
	printk("enter sl_irq_handler, irq %x\n", irq);
#endif
	desc->irq_data.chip->irq_ack(&desc->irq_data);
	list_for_each(ptr, chip_list) 
	{
		unsigned long pending;
		int offset;

		chip = list_entry(ptr, struct sl_gpio, list);
		pending = readl(chip->base + GPIOMIS);
		writel(pending, chip->base + GPIOIC);

		if (pending == 0)
			continue;

		for_each_set_bit(offset, &pending, SL_GPIO_NR)
			generic_handle_irq(sl_to_irq(&chip->gc, offset));
	}
	desc->irq_data.chip->irq_unmask(&desc->irq_data);
}

static int sl_gpio_remove(struct platform_device *pdev)
{
	struct sl_gpio *chip = platform_get_drvdata(pdev);
	
	platform_set_drvdata(pdev, NULL);
	
	if(chip == NULL)
		return 0;
	
	/*free the common resources */
	if(chip->base != NULL) 
	{
		iounmap(chip->base);
		chip->base = NULL;
	}
	
	if(chip->area != NULL) 
	{
		release_resource(chip->area);
		kfree(chip->area);
		chip->area = NULL;
	}
	
/*	if(chip->list != NULL) 
	{
		kfree(chip->list);
		chip->list = NULL;		
	}*/
	
	kfree(chip);
	
	return 0;
	
}

static int sl_gpio_probe(struct platform_device *pdev)
{
	struct pl061_platform_data *plat;
	struct sl_gpio *chip;
	struct list_head *chip_list;
	int ret, irq, i;
	struct resource *res;
	int size;
	struct clk *clk;
	static DECLARE_BITMAP(init_irq, NR_IRQS);

	clk = clk_get(NULL, "gpio");
	if (IS_ERR(clk)) 
	{
		printk( KERN_ERR "Failed to set vpp clk\n");
		return PTR_ERR(clk);
	}
	clk_enable(clk);

//	silan_padmux_ctrl(SILAN_PADMUX_GPIO5, PAD_ON);

	plat = pdev->dev.platform_data;
	if (plat == NULL)
		return -ENODEV;
	silan_padmux_ctrl(plat->padmux_gpio, PAD_ON);
	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	memset(chip, 0, sizeof(*chip));
	platform_set_drvdata(pdev, chip);

	spin_lock_init(&chip->lock);
	spin_lock_init(&chip->irq_lock);
	INIT_LIST_HEAD(&chip->list);
		
	res = pdev->resource;
	size = res->end - res->start + 1;
	
	chip->area = request_mem_region(res->start, size, pdev->name);
	
	if (chip->area == NULL) 
	{
		dev_err(&pdev->dev, "cannot reserve register region\n");
		ret = -ENOENT;
		goto exit_error;
	}

	chip->base =ioremap(res->start, size);
	
	if(chip->base == NULL) 
	{
		dev_err(&pdev->dev, "cannot reserve register region\n");
		ret = -EIO;
		goto exit_error;
	}
		
	dev_dbg(&pdev->dev, "mapped registers at %p\n", chip->base);	
    
	chip->gc.direction_input = sl_direction_input;
	chip->gc.direction_output = sl_direction_output;
	chip->gc.get = sl_get_value;
	chip->gc.set = sl_set_value;
	chip->gc.to_irq = sl_to_irq;
	chip->gc.base = plat->gpio_base;
	chip->gc.ngpio = plat->gpio_num;//SL_GPIO_NR;
	chip->gc.label = dev_name(&pdev->dev);
	chip->gc.dev = &pdev->dev;
	chip->gc.owner = THIS_MODULE;

	chip->irq_base = plat->irq_base;

	chip->gpio_irqbase = plat->gpio_irqbase;//SL_GPIO_IRQBASE;
	

	ret = gpiochip_add(&chip->gc);
	if (ret)
		goto exit_error;

	/*
	 * irq_chip support
	 */

	if (chip->irq_base == (unsigned) -1)
		return 0;

	writel(0, chip->base + GPIOIE); /* disable irqs */
	irq = chip->irq_base;
	if (irq < 0) 
	{
		ret = -ENODEV;
		goto exit_error;
	}
	irq_set_chained_handler(irq, sl_irq_handler);
	if (!test_and_set_bit(irq, init_irq)) 
	{ /* list initialized? */
		chip_list = kmalloc(sizeof(*chip_list), GFP_KERNEL);
		if (chip_list == NULL) 
		{
			clear_bit(irq, init_irq);
			ret = -ENOMEM;
			goto exit_error;
		}
		INIT_LIST_HEAD(chip_list);
		irq_set_handler_data(irq, chip_list);
	} else
		chip_list = irq_get_handler_data(irq);
	list_add(&chip->list, chip_list);
//	chip->gpio_irqbase = SL_GPIO_IRQBASE;
	//plat->gpio_num;
	for (i = 0; i < plat->gpio_num /*SL_GPIO_NR*/; i++) 
	{
		if (plat->directions & (1 << i)){
			sl_direction_output(&chip->gc, i,
					(plat->values >> i) & 0x1);
		}
		else
			sl_direction_input(&chip->gc, i);
		irq_set_chip_and_handler(i + chip->gpio_irqbase, &sl_irqchip, handle_simple_irq);
//		set_irq_flags(i+chip->irq_base, IRQF_VALID);
		irq_set_chip_data(i+chip->gpio_irqbase, chip);
	}


	printk("silan gpio probe ok\n");
	return 0;

exit_error:
	sl_gpio_remove(pdev);
	
	if(ret == 0)
		ret = -EINVAL;

	return ret;
}

//PM support
#ifdef CONFIG_PM

static int sl_gpio_suspend(struct platform_device *dev, pm_message_t pm)
{

	
	return 0;
}

static int sl_gpio_resume(struct platform_device *dev)
{

		
	return 0;
}
#else
#define sl_gpio_suspend NULL
#define sl_gpio_resume  NULL
#endif

static struct platform_driver sl_gpio_driver1 = {
	.probe 		= sl_gpio_probe,
	.remove 	= sl_gpio_remove,
	.suspend 	= sl_gpio_suspend,
	.resume		= sl_gpio_resume,
	.driver		= {
		.name 	= "silan-gpio1",
		.owner 	= THIS_MODULE,
	},
};
static struct platform_driver sl_gpio_driver2 = {
	.probe 		= sl_gpio_probe,
	.remove 	= sl_gpio_remove,
	.suspend 	= sl_gpio_suspend,
	.resume		= sl_gpio_resume,
	.driver		= {
		.name 	= "silan-gpio2",
		.owner 	= THIS_MODULE,
	},
};
static struct platform_driver sl_gpio_driver3 = {
	.probe 		= sl_gpio_probe,
	.remove 	= sl_gpio_remove,
	.suspend 	= sl_gpio_suspend,
	.resume		= sl_gpio_resume,
	.driver		= {
		.name 	= "silan-gpio3",
		.owner 	= THIS_MODULE,
	},
};
static struct platform_driver sl_gpio_driver4 = {
	.probe 		= sl_gpio_probe,
	.remove 	= sl_gpio_remove,
	.suspend 	= sl_gpio_suspend,
	.resume		= sl_gpio_resume,
	.driver		= {
		.name 	= "silan-gpio4",
		.owner 	= THIS_MODULE,
	},
};
static struct platform_driver sl_gpio_driver5 = {
	.probe 		= sl_gpio_probe,
	.remove 	= sl_gpio_remove,
	.suspend 	= sl_gpio_suspend,
	.resume		= sl_gpio_resume,
	.driver		= {
		.name 	= "silan-gpio5",
		.owner 	= THIS_MODULE,
	},
};

static int __init sl_gpio_init(void)
{
    platform_driver_register(&sl_gpio_driver1);
    platform_driver_register(&sl_gpio_driver2);
    platform_driver_register(&sl_gpio_driver3);
    platform_driver_register(&sl_gpio_driver4);
    return platform_driver_register(&sl_gpio_driver5);
}

static void __exit sl_gpio_exit(void)
{
    platform_driver_unregister(&sl_gpio_driver1);
    platform_driver_unregister(&sl_gpio_driver2);
    platform_driver_unregister(&sl_gpio_driver3);
    platform_driver_unregister(&sl_gpio_driver4);
    platform_driver_unregister(&sl_gpio_driver5);
}


module_init(sl_gpio_init);
module_exit(sl_gpio_exit);

MODULE_AUTHOR("Li Fei <lifei@silan.com.cn>");
MODULE_DESCRIPTION("Silan GPIO driver");
MODULE_LICENSE("GPL");
