/* linux/arch/csky/ck6408ecb/gpio.c
 *
 * Copyright (C) 2012 , Chen Linfei<linfei_chen@c-sky.com>.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h> 
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#include <mach/ckgpio.h>

extern void irq_startup(struct irq_desc* desc);

static void ck_gpiolib_set(struct gpio_chip *chip, unsigned offset, int val);
static int ck_gpiolib_get(struct gpio_chip *chip, unsigned offset);
static int ck_gpiolib_direction_output(struct gpio_chip *chip,
					unsigned offset, int val);
static int ck_gpiolib_direction_input(struct gpio_chip *chip, unsigned offset);
static int ck_gpiolib_to_irq(struct gpio_chip* chip, unsigned offset);


struct ck_gpio_chip
{
	struct gpio_chip chip;
	unsigned int *regbase;
};

#define CK_GPIO_CHIP(name, base_gpio, nr_gpio)	\
	{					\
		.chip = {			\
			.label = name,		\
                        .base  = base_gpio,             \
                        .ngpio = nr_gpio,               \
			.direction_input  = ck_gpiolib_direction_input,  \
			.direction_output = ck_gpiolib_direction_output, \
			.to_irq		  = ck_gpiolib_to_irq,		 \
			.get   = ck_gpiolib_get,	\
			.set   = ck_gpiolib_set,	\
			},				\
	}

static struct ck_gpio_chip gpio_chips[] = {

	CK_GPIO_CHIP("GPIOA", 0x00, 11),
	CK_GPIO_CHIP("GPIOB", 0x20, 10),
};

/*
 * set GPIO direction: input
 */
static int ck_gpiolib_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct ck_gpio_chip *ck_gpio = to_ck_gpio_chip(chip);
	void __iomem *pio = ck_gpio->regbase;
	unsigned int temp = __raw_readl(pio + CKGPIO_PADDR);
	temp &= ~(1 << offset);

	__raw_writel(temp, pio + CKGPIO_PADR);
	return 0;
}

/*
 * set GPIO direction: output
 */
static int ck_gpiolib_direction_output(struct gpio_chip *chip,
                                        unsigned offset, int val)
{
        struct ck_gpio_chip *ck_gpio = to_ck_gpio_chip(chip);
        void __iomem *pio = ck_gpio->regbase;
        unsigned int temp = __raw_readl(pio + CKGPIO_PADDR);
	unsigned int dat  = __raw_readl(pio + CKGPIO_PADR);
	
	if (val)
	{
		dat |= 1 << offset;
	}

	__raw_writel(dat, pio + CKGPIO_PADR);

        temp |= 1 << offset;

        __raw_writel(temp, pio + CKGPIO_PADDR); 
	return 0;
}

/*
 * get data from GPIO, return 0 or 1
 */
static int ck_gpiolib_get(struct gpio_chip *chip, unsigned offset)
{
	struct ck_gpio_chip *ck_gpio = to_ck_gpio_chip(chip);
	void __iomem *pio = ck_gpio->regbase;
	unsigned int temp;
        if(pio == (void __iomem*) CK_GPIO_BASE )
                temp = __raw_readl(pio + CKGPIO_EXTPORTA);
        else
                temp = __raw_readl(pio + CKGPIO_EXTPORTA - 0x08);

	temp >>= offset;
	temp &= 1;
	
	return temp;
}

/*
 * set data(one bit in GPIOA or GPIOB)
 * val is the initial value
 */
static void ck_gpiolib_set(struct gpio_chip *chip, unsigned offset, int val)
{
	struct ck_gpio_chip *ck_gpio = to_ck_gpio_chip(chip);
	void __iomem *pio = ck_gpio->regbase;
	unsigned int temp = __raw_readl(pio + CKGPIO_PADR);

	if (val)
		temp |= 1 << offset;
	else
		temp &= ~(1 << offset);

	__raw_writel(temp, pio + CKGPIO_PADR);
}

/*
 * get the GPIO irq, GPIOA has irqs
 */
static int ck_gpiolib_to_irq(struct gpio_chip* chip, unsigned offset)
{
	if(chip->base == 0x00 && offset <= CSKY_GPIOA10_IRQ )
	{
		return offset;
	}
	else
	{
		return -1;
	}
}

/*
 * init GPIO in Linux
 */
void __init ck_gpio_init(void)
{
	int i;
	struct ck_gpio_chip *ck_gpio;

	for(i = 0; i < GPIO_BANK_NUM; i++)
	{
		ck_gpio = &gpio_chips[i];	
		ck_gpio->regbase = (void __iomem*)(CK_GPIO_BASE + 0x0c*i);
		gpiochip_add(&(ck_gpio->chip));
	}
}



/*
 * enable GPIO interrupts
 */
static void ck_gpio_irq_enable(struct irq_data *d)
{
	void __iomem* pio = (void __iomem *)CK_GPIO_BASE;	
	unsigned int temp = __raw_readl(pio + CKGPIO_INTEN);
	temp |= (1 << d->irq);

	__raw_writel(temp, pio + CKGPIO_INTEN);
}

/*
 * disable GPIO interrupts
 */
static void ck_gpio_irq_disable(struct irq_data* d)
{
        void __iomem *pio = (void __iomem *)CK_GPIO_BASE;
        unsigned int temp = __raw_readl(pio + CKGPIO_INTEN);
        temp &= ~(1 << d->irq);         

        __raw_writel(temp, pio + CKGPIO_INTEN);
}

/*
 * mask GPIO interrupts
 */
static void ck_gpio_irq_mask(struct irq_data *d)
{
        void __iomem *pio = (void __iomem *)CK_GPIO_BASE;
        unsigned int temp = __raw_readl(pio + CKGPIO_INTMASK);
        temp |= (1 << d->irq);

        __raw_writel(temp, pio + CKGPIO_INTMASK);
}

/*
 * unmask GPIO interrupts
 */
static void ck_gpio_irq_unmask(struct irq_data *d)
{
        void __iomem *pio = (void __iomem *)CK_GPIO_BASE;
        unsigned int temp = __raw_readl(pio + CKGPIO_INTMASK);
        temp &= ~(1 << d->irq);

        __raw_writel(temp, pio + CKGPIO_INTMASK);
}

/*
 * clear GPIO interrupts
 */
static void ck_gpio_irq_eoi(struct irq_data *d)
{
        void __iomem *pio = (void __iomem *)CK_GPIO_BASE;
        unsigned int temp = __raw_readl(pio + CKGPIO_PORTAEOI);
        temp |= (1 << d->irq);

        __raw_writel(temp, pio + CKGPIO_PORTAEOI);
}

/*
 * set interrupts trigger type
 */
static int ck_gpio_irq_setType(struct irq_data *d, unsigned trigger)
{
	void __iomem *pio = (void __iomem *)CK_GPIO_BASE;
	unsigned int type = __raw_readl(pio + CKGPIO_INTTYPE);
	unsigned int pola = __raw_readl(pio + CKGPIO_INTPOLA);
	
	switch(trigger)	
	{
		case IRQ_TYPE_EDGE_RISING:
			type |= (1 << d->irq);
			pola |= (1 << d->irq);
			__raw_writel(type, pio + CKGPIO_INTTYPE);
			__raw_writel(pola, pio + CKGPIO_INTPOLA);
			break;
		case IRQ_TYPE_EDGE_FALLING:
			type |= (1 << d->irq);
			pola &= ~(1 << d->irq);
                        __raw_writel(type, pio + CKGPIO_INTTYPE); 
                        __raw_writel(pola, pio + CKGPIO_INTPOLA);
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			type &= ~(1 << d->irq);
			pola |= (1 << d->irq);
                        __raw_writel(type, pio + CKGPIO_INTTYPE); 
                        __raw_writel(pola, pio + CKGPIO_INTPOLA);
			break;
		case IRQ_TYPE_LEVEL_LOW:
			type &= ~(1 << d->irq);
			pola &= ~(1 << d->irq);
                        __raw_writel(type, pio + CKGPIO_INTTYPE); 
                        __raw_writel(pola, pio + CKGPIO_INTPOLA);
			break;
		default:
			break;
	}

	return 0;
}

static struct irq_chip ck_gpio_irqchip = {

	.name 	= "GPIO",
	.irq_enable	= ck_gpio_irq_enable,
	.irq_disable	= ck_gpio_irq_disable,
	.irq_mask 	= ck_gpio_irq_mask,
	.irq_unmask	= ck_gpio_irq_unmask,
	.irq_set_type	= ck_gpio_irq_setType,
	.irq_eoi	= ck_gpio_irq_eoi,
};

/*
 * enable the interrupt controller
 * init GPIO interrupts in linux
 */
void __init ck_gpio_irq_init(void)
{
	int irq;

	for(irq = CSKY_GPIOA0_IRQ; irq <= CSKY_GPIOA10_IRQ; irq++)
	{
		struct irq_desc *desc = irq_to_desc(irq);
                irq_startup(desc);
                irq_set_chip_and_handler(irq, &ck_gpio_irqchip,
                                               handle_fasteoi_irq);
		irq_set_chip_data(irq, (void*)irq);
		irq_set_irq_type(irq,IRQ_TYPE_EDGE_FALLING);
	}
}

