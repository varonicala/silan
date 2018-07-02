/*
 * arch/arm/mach-vt8500/pwm.c
 *
 *  Copyright (C) 2010 Alexey Charkov <alchark@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/pwm.h>
#include <linux/delay.h>
#include <silan_padmux.h>
#include <asm/div64.h>

#include <linux/clk.h>

#define PWMSRC 0x0
#define PWM0D  0x4
#define PWM1D  0x8
#define PWM2D  0xc
#define PWM3D  0x10
#define PWMP   0x14
#define PWMCN  0x18


#define SILAN_PWMS				4

//#define SILAN_PWM_DEBUG

static DEFINE_MUTEX(pwm_lock);
static LIST_HEAD(pwm_list);

struct pwm_device {
	struct list_head	node;
	struct platform_device	*pdev;
	struct resource		 *pwm_mem;
	struct clk			*clk;	
	const char	*label;

	void __iomem	*regbase;

	unsigned int	use_count;
	unsigned int	pwm_id;
};

static struct pwm_device *pwms;

#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)
static inline void pwm_busy_wait(void __iomem *reg, u8 bitmask)
{
	int loops = msecs_to_loops(10);
	while ((readb(reg) & bitmask) && --loops)
		cpu_relax();

	if (unlikely(!loops))
		pr_warning("Waiting for status bits 0x%x to clear timed out\n",
			   bitmask);
}

int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	unsigned long long c;
	unsigned long period_cycles, prescale, pv, dc;
	
	if (pwm == NULL || period_ns == 0 || duty_ns > period_ns)
		return -EINVAL;
#ifdef SILAN_PWM_DEBUG
	printk("@@@@@@@@@@@@@duty is %x, period is %x\n", duty_ns, period_ns);
#endif
	c = clk_get_rate(pwm->clk); //200MHz
	c = c * period_ns;
	do_div(c, 1000000000);
	period_cycles = c;
	if (period_cycles < 1)
		period_cycles = 1;
	prescale = (period_cycles - 1) / 256;
	pv = period_cycles / (prescale + 1) - 1;
	if (pv > 256)
		pv = 255;

	if (prescale > 256)
		return -EINVAL;

	c = (unsigned long long)pv * duty_ns;
	do_div(c, period_ns);
	dc = c;
	

	writel(prescale, pwm->regbase+PWMSRC);

//	pwm_busy_wait(pwm->regbase + 0x40 + pwm->pwm_id, (1 << 2));
//	writel(pv, pwm->regbase + 0x8 + (pwm->pwm_id << 4));
	writel(pv, pwm->regbase + PWMP);
	
//	pwm_busy_wait(pwm->regbase + 0x40 + pwm->pwm_id, (1 << 3));
	writel(dc, pwm->regbase + 4*(pwm->pwm_id+1));

	return 0;
}
EXPORT_SYMBOL(pwm_config);

int pwm_enable(struct pwm_device *pwm)
{
	int cr;
	cr = readl(pwm->regbase + PWMCN);
#ifdef SILAN_PWM_DEBUG
	printk("pwm_enable %x, %x!\n", pwm->regbase, pwm->pwm_id);
#endif
	if(pwm->pwm_id == 0)
		cr |= (1<<4);
	if(pwm->pwm_id == 1)
		cr |= (1<<5);
	if(pwm->pwm_id == 2)
		cr |= (1<<6);
	if(pwm->pwm_id == 3)
		cr |= (1<<7);
	writel(cr, pwm->regbase+PWMCN);
	return 0;
}
EXPORT_SYMBOL(pwm_enable);

void pwm_disable(struct pwm_device *pwm)
{
	int cr;
	cr = readl(pwm->regbase + PWMCN);
	if(pwm->pwm_id == 0)
		cr &= ~(1<<4);
	if(pwm->pwm_id == 1)
		cr &= ~(1<<5);
	if(pwm->pwm_id == 2)
		cr |= ~(1<<6);
	if(pwm->pwm_id == 3)
		cr |= ~(1<<7);
	writel(cr, pwm->regbase+PWMCN);

}
EXPORT_SYMBOL(pwm_disable);

struct pwm_device *pwm_request(int pwm_id, const char *label)
{
	struct pwm_device *pwm;
	int found = 0;

	mutex_lock(&pwm_lock);

	list_for_each_entry(pwm, &pwm_list, node) {
		if (pwm->pwm_id == pwm_id) {
			found = 1;
			break;
		}
	}
#ifdef SILAN_PWM_DEBUG
	printk("found in pwm_request %x, %x, %x, %p\n", found, pwm->pwm_id, pwm->use_count, pwm->clk);
#endif
	if (found) {
		if (pwm->use_count == 0) {
			pwm->use_count++;
			pwm->label = label;
		} else {
			pwm = ERR_PTR(-EBUSY);
		}
	} else {
		pwm = ERR_PTR(-ENOENT);
	}

	mutex_unlock(&pwm_lock);
	return pwm;
}
EXPORT_SYMBOL(pwm_request);

void pwm_free(struct pwm_device *pwm)
{
	mutex_lock(&pwm_lock);

	if (pwm->use_count) {
		pwm->use_count--;
		pwm->label = NULL;
	} else {
		pr_warning("PWM device already freed\n");
	}

	mutex_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_free);

static inline void __add_pwm(struct pwm_device *pwm)
{
	mutex_lock(&pwm_lock);
	list_add_tail(&pwm->node, &pwm_list);
	mutex_unlock(&pwm_lock);
}

static int __devinit pwm_probe(struct platform_device *pdev)
{
	struct resource *r;
	int ret = 0,size;
	int i;
	struct clk *clk;


	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&pdev->dev, "no memory resource defined\n");
		ret = -ENODEV;
		goto err_free;
	}
	
	size = (r->end - r->start) + 1 ; 

	pwms = kzalloc(sizeof(struct pwm_device) * SILAN_PWMS, GFP_KERNEL);
	if (pwms == NULL) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	pwms->pwm_mem = request_mem_region(r->start, size, pdev->name);
	if (pwms->pwm_mem == NULL) {
		dev_err(&pdev->dev, "failed to request memory resource\n");
		ret = -EBUSY;
		goto err_free;
	}

	pwms[0].regbase = ioremap(r->start, size);
	if (pwms[0].regbase == NULL) {
		dev_err(&pdev->dev, "failed to ioremap() registers\n");
		ret = -ENODEV;
		goto err_free_mem;
	}

	clk = clk_get(&pdev->dev,"pwm");
	clk_enable(clk);
	//silan_padmux_ctrl(SILAN_PADMUX_PWM1,PAD_ON);
	silan_padmux_ctrl(SILAN_PADMUX_PWM0,PAD_ON);

	for (i = 0; i < SILAN_PWMS; i++) {
		pwms[i].use_count = 0;
		pwms[i].pwm_id = i;
		pwms[i].pdev = pdev;
		pwms[i].regbase = pwms[0].regbase;
		pwms[i].clk		= clk;
		__add_pwm(&pwms[i]);

	}

	printk("pwm probe ok!\n");
	platform_set_drvdata(pdev, pwms);
	return 0;

err_free_mem:
	release_mem_region(r->start, size);
err_free:
	kfree(pwms);
	return ret;
}

static int __devexit pwm_remove(struct platform_device *pdev)
{
	struct pwm_device *pwms;
	struct resource *r;
	int i;

	pwms = platform_get_drvdata(pdev);
	if (pwms == NULL)
		return -ENODEV;

	mutex_lock(&pwm_lock);

	for (i = 0; i < SILAN_PWMS; i++)
		list_del(&pwms[i].node);
	mutex_unlock(&pwm_lock);

	iounmap(pwms[0].regbase);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(r->start, resource_size(r));

	kfree(pwms);
	return 0;
}

static struct platform_driver pwm_driver = {
	.driver		= {
		.name	= "silan-pwm",
		.owner	= THIS_MODULE,
	},
	.probe		= pwm_probe,
	.remove		= __devexit_p(pwm_remove),
};

static int __init pwm_init(void)
{
   	return platform_driver_register(&pwm_driver);
}
arch_initcall(pwm_init);

static void __exit pwm_exit(void)
{
	platform_driver_unregister(&pwm_driver);
}
module_exit(pwm_exit);

MODULE_LICENSE("GPL");
