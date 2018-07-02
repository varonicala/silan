/*
 * arch/csky/ck6408evb/irq.c ---IRQ vector handles
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2009 by Hu Junshan <junshan_hu@c-sky.com> 
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/ckpic.h>

extern void ck_gpio_irq_init(void);

/*
 *  Mask the interrupt and the irp number is irqno.
 */
static void
csky_irq_mask(struct irq_data *d)
{
	((volatile unsigned long *)(CKPIC_BASE))[CKPIC_NIER] &= 
                                                ~(1 << d->irq);
}

/*
 *  Unmask the interrupt and the irp number is irqno.
 */
static void
csky_irq_unmask(struct irq_data *d)
{
	((volatile unsigned long *)(CKPIC_BASE))[CKPIC_NIER] |= 
                                           (1 << d->irq);
}

struct irq_chip csky_irq_chip = {
        .name           = "csky",
        .irq_mask           = csky_irq_mask,
        .irq_unmask         = csky_irq_unmask,
};

unsigned int csky_get_auto_irqno(void)
{
	volatile unsigned int *icrp;
	unsigned int irqno;

	icrp = (volatile unsigned int *) (CKPIC_BASE);
	
	irqno = ((unsigned int)icrp[CKPIC_ISR]) & 0x7f;
	return irqno;
}

/*
 *  Initial the interrupt controller of c-sky.
 */
void __init csky_init_IRQ(void)
{
	volatile unsigned int *icrp;
	int i;
	
	icrp = (volatile unsigned int *) (CKPIC_BASE);

	/*
	 * Initial the interrupt control register.
	 * 	1. Program the vector to be an auto-vectored.
	 * 	2. Mask all Interrupt.
	 * 	3. Unique vector numbers for fast vectored interrupt requests and fast 
     *  	vectored interrupts Number are 64-95.
	 */
	icrp[CKPIC_ICR] = 0x40000000;

	/*
	 * Initial the Interrupt source priority level registers
	 */
	icrp[CKPIC_PR0] = 0x00010203;
	icrp[CKPIC_PR4] = 0x04050607;
	icrp[CKPIC_PR8] = 0x08090a0b;
	icrp[CKPIC_PR12] = 0x0c0d0e0f;
	icrp[CKPIC_PR16] = 0x10111213;
	icrp[CKPIC_PR20] = 0x14151617;
	icrp[CKPIC_PR24] = 0x18191a1b;
	icrp[CKPIC_PR28] = 0x1c1d1e1f;

	for (i = 0; i < NR_IRQS; i++)
	{
		irq_set_chip_and_handler(i, &csky_irq_chip, handle_level_irq);
	}
	ck_gpio_irq_init();	
}


