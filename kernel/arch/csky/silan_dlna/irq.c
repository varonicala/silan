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
#include <mach/silan_generic.h>
#include <mach/silan_irq.h>

#if 0
/* Every board describes its IRQ mapping with this table.*/
typedef struct silan_irqmap 
{
    int im_irq;
    int im_pri;
} silan_irq_map_t;
silan_irq_map_t silan_irq_map[] = 
{
/* LSP */
	{PIC_IRQ_CAN,		5},
	{PIC_IRQ_GPIO5,		6}, 
	{PIC_IRQ_GPIO4,		7},	
	{PIC_IRQ_GPIO3,		8},
	{PIC_IRQ_GPIO2,		9},
	{PIC_IRQ_GPIO1,		10},
	{PIC_IRQ_UART2,		11},
	{PIC_IRQ_UART1,		12}, 
	{PIC_IRQ_SCI,		13},   
	{PIC_IRQ_RTC,		14}, 
	{PIC_IRQ_I2C3,		15}, 
	{PIC_IRQ_I2C2,		16}, 
	{PIC_IRQ_I2C1,		17},
	{PIC_IRQ_TIMER1,    18},
	{PIC_IRQ_WDG1,		19},
/* HSP */
	{PIC_IRQ_UART6,     21},
	{PIC_IRQ_UART5,		22},
	{PIC_IRQ_I2C5,		23},
	{PIC_IRQ_DMAC0,		24}, 
	{PIC_IRQ_SSP2,		25},
	{PIC_IRQ_SSP1,		26},
	{PIC_IRQ_GMAC,		27},
	{PIC_IRQ_SDIO,		28},
	{PIC_IRQ_SD,		29},
	{PIC_IRQ_MMC,		30},
	{PIC_IRQ_I2C4,		31},
	//{PIC_IRQ_SATA,		INTC_INT_HIGH_LEVEL,    0},
	{PIC_IRQ_USB_HOST,	32},
	{PIC_IRQ_USB_OTG,	33},
	{PIC_IRQ_UART4,		34},
	{PIC_IRQ_UART3,		35},
/* Audio */
	{PIC_IRQ_APWM,		39},     
	{PIC_IRQ_SPDIF,     40},
	{PIC_IRQ_DMAC1,     41},
#ifdef CONFIG_IVS
	{PIC_IRQ_MPU,		38},
#endif
/* BB */
	{PIC_IRQ_FDIP,		45},
	{PIC_IRQ_JPEG,		46},
	{PIC_IRQ_CEC,		47},
	{PIC_IRQ_HDMI,		48},
	{PIC_IRQ_VPU,		49},
	{PIC_IRQ_VPRE,		0},
	{PIC_IRQ_VENC,		0},
	{PIC_IRQ_DIT,		50},
	{PIC_IRQ_GPU,		51},
	{PIC_IRQ_VIU,		52},
	{PIC_IRQ_VPP,		53},
	{MIPS_DDR_ERR_IRQ,  54},
	{MIPS_PUB_CXC_IRQ,	55},
	{MIPS_XPU_CXC_IRQ,	56},
	{MIPS_DSP_CXC_IRQ,	57},
	{MIPS_WDG_IRQ,		58},
	{MIPS_TIMER_IRQ,	59},
};
#endif
int silan_pic_nr_irqs = 0; //sizeof(silan_irq_map)/sizeof(silan_irq_map_t);

DEFINE_RAW_SPINLOCK(irq_lock);
/*
 *  Mask the interrupt and the irp number is irqno.
 */
static void csky_irq_mask(struct irq_data *d)
{
	unsigned long flags;
	u32 data,reg,shift;

	raw_spin_lock_irqsave(&irq_lock, flags);

	reg = CKPIC_BASE + IRQ_INTMASK;
	shift = d->irq;
	data = sl_readl(reg);
	data |= (1UL << shift);
	sl_writel(data, reg);

	raw_spin_unlock_irqrestore(&irq_lock, flags);
}

/*
 *  Unmask the interrupt and the irp number is irqno.
 */
static void csky_irq_unmask(struct irq_data *d)
{
	unsigned long flags;
	u32 data,reg,shift;

	raw_spin_lock_irqsave(&irq_lock, flags);

	reg = CKPIC_BASE + IRQ_INTMASK;
	shift = d->irq;
	data = sl_readl(reg);
	data &= ~(1UL << shift);
	sl_writel(data, reg);

	raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static void csky_irq_disable(struct irq_data *d)
{
	unsigned long flags;
	u32 data,reg,shift;

	raw_spin_lock_irqsave(&irq_lock, flags);

	reg = CKPIC_BASE + IRQ_INTEN;
	shift = d->irq;
	data = sl_readl(reg);
	data &= ~(1UL << shift);
	sl_writel(data, reg);

	raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static void csky_irq_enable(struct irq_data *d)
{
	unsigned long flags;
	u32 data,reg,shift;

	raw_spin_lock_irqsave(&irq_lock, flags);

	reg = CKPIC_BASE + IRQ_INTEN;
	shift = d->irq;
	data = sl_readl(reg);
	data |= (1UL << shift);
	sl_writel(data, reg);

	raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static void csky_irq_ack(struct irq_data *d)
{
	unsigned long flags;
	u32 data,reg,shift;

	raw_spin_lock_irqsave(&irq_lock, flags);

	reg = CKPIC_BASE + IRQ_INTEN;
	shift = d->irq;
	data = sl_readl(reg);
	data |= (1UL << shift);
	//data &= ~(1UL << shift);
	sl_writel(data, reg);

	raw_spin_unlock_irqrestore(&irq_lock, flags);
}

struct irq_chip csky_irq_chip = 
{
        .name           = "csky",
        .irq_mask           = csky_irq_mask,
        .irq_mask_ack       = csky_irq_mask,
        .irq_unmask         = csky_irq_unmask,
        .irq_eoi            = csky_irq_unmask,
		.irq_enable         = csky_irq_enable,
		.irq_disable        = csky_irq_disable,
		.irq_ack            = csky_irq_ack,
};

unsigned int find_bit(int irq)
{
	int i;
	for (i = 0; i < 32; i ++) 
	{   
		if (irq & (1 << i)) 
		return i;
	}   
}


unsigned int csky_get_auto_irqno(void)
{
	unsigned int irqno;

	irqno = sl_readl(CKPIC_BASE + IRQ_MASKSTATUS);
	irqno = find_bit(irqno);
	
	return irqno;
}

#if 0
static unsigned int csky_get_priority(int start_irq,int end_irq)
{
	unsigned int value = 0;	
	int i;

	for(i = 0; i<silan_pic_nr_irqs; i++)
	{
		if((silan_irq_map[i].im_irq >= start_irq) &&
           (silan_irq_map[i].im_irq <= end_irq))
			value |= (silan_irq_map[i].im_pri << (i - start_irq) * 8);
	}
	return value;
}

static void cksy_set_priority(void)
{
	unsigned data;
	/*
	 * Initial the Interrupt source priority level registers
	 */
	data = csky_get_priority(0,3);
	sl_writel(data,CKPIC_BASE + CKPIC_PR0);
	data = csky_get_priority(4,7);
	sl_writel(data,CKPIC_BASE + CKPIC_PR4);
	data = csky_get_priority(8,11);
	sl_writel(data,CKPIC_BASE + CKPIC_PR8);
	data = csky_get_priority(12,15);
	sl_writel(data,CKPIC_BASE + CKPIC_PR12);
	data = csky_get_priority(16,19);
	sl_writel(data,CKPIC_BASE + CKPIC_PR16);
	data = csky_get_priority(20,23);
	sl_writel(data,CKPIC_BASE + CKPIC_PR20);
	data = csky_get_priority(24,27);
	sl_writel(data,CKPIC_BASE + CKPIC_PR24);
	data = csky_get_priority(28,31);
	sl_writel(data,CKPIC_BASE + CKPIC_PR28);

	data = csky_get_priority(32,35);
	sl_writel(data,CKPIC_BASE + CKPIC_PR32);
	data = csky_get_priority(36,39);
	sl_writel(data,CKPIC_BASE + CKPIC_PR36);
	data = csky_get_priority(40,43);
	sl_writel(data,CKPIC_BASE + CKPIC_PR40);
	data = csky_get_priority(44,47);
	sl_writel(data,CKPIC_BASE + CKPIC_PR44);
	data = csky_get_priority(48,51);
	sl_writel(data,CKPIC_BASE + CKPIC_PR48);
	data = csky_get_priority(52,55);
	sl_writel(data,CKPIC_BASE + CKPIC_PR52);
	data = csky_get_priority(56,59);
	sl_writel(data,CKPIC_BASE + CKPIC_PR56);
	data = csky_get_priority(60,63);
	sl_writel(data,CKPIC_BASE + CKPIC_PR60);
}
#endif

/*
 *  Initial the interrupt controller of c-sky.
 */
void __init csky_init_IRQ(void)
{
	int i;
	unsigned data;
	
	volatile unsigned int *icrp;
	icrp = (volatile unsigned int *) (CKPIC_BASE);
	/*
	 * Initial the interrupt control register.
	 * 	1. Program the vector to be an auto-vectored.
	 * 	2. Mask all Interrupt.
	 * 	3. Unique vector numbers for fast vectored interrupt requests and fast 
     *  	vectored interrupts Number are 64-95.
	 */
//	data = CKPIC_ICR_AVE;
//	sl_writel(data,CKPIC_BASE + CKPIC_ICR);

#if 0
	for(i = 0; i < silan_pic_nr_irqs; i++){
		irq_set_chip_and_handler(silan_irq_map[i].im_irq, &csky_irq_chip, handle_level_irq);
	}
#endif

	for(i = 0; i < NR_IRQS; i++)
	{
		irq_set_chip_and_handler(i, &csky_irq_chip, handle_level_irq);
	}
	
	//sl_writel(0x0, 0xba090008);
}


