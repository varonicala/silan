/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000, 2001, 2004 MIPS Technologies, Inc.
 * Copyright (C) 2001 Ralf Baechle
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * Routines for generic manipulation of the interrupts found on the SILAN SUV
 * PLATFORM.
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel_stat.h>
#include <linux/kernel.h>
#include <linux/random.h>

#include <asm/traps.h>
#include <asm/irq_cpu.h>
#include <asm/irq_regs.h>
#include <silan_irq.h>
#include <silan_resources.h>
#include <silan_generic.h>

/* DW INTERRUPT CONTROLLER REGISTER */
#define DW_IRQ_INTEN_L                  (SILAN_ICTL1_BASE + 0x000)
#define DW_IRQ_INTEN_H                  (SILAN_ICTL1_BASE + 0x004)
#define DW_IRQ_INTMASK_L                (SILAN_ICTL1_BASE + 0x008)
#define DW_IRQ_INTMASK_H                (SILAN_ICTL1_BASE + 0x00C)
#define DW_IRQ_INTFORCE_L               (SILAN_ICTL1_BASE + 0x010)
#define DW_IRQ_INTFORCE_H               (SILAN_ICTL1_BASE + 0x014)
#define DW_IRQ_RAWSTATUS_L              (SILAN_ICTL1_BASE + 0x018)
#define DW_IRQ_RAWSTATUS_H              (SILAN_ICTL1_BASE + 0x01C)
#define DW_IRQ_STATUS_L                 (SILAN_ICTL1_BASE + 0x020)
#define DW_IRQ_STATUS_H                 (SILAN_ICTL1_BASE + 0x024)
#define DW_IRQ_MASKSTATUS_L             (SILAN_ICTL1_BASE + 0x028)
#define DW_IRQ_MASKSTATUS_H             (SILAN_ICTL1_BASE + 0x02C)
#define DW_IRQ_FINALSTATUS_L            (SILAN_ICTL1_BASE + 0x030)
#define DW_IRQ_FINALSTATUS_H            (SILAN_ICTL1_BASE + 0x034)
#define DW_IRQ_VECTOR                   (SILAN_ICTL1_BASE + 0x038)
#define DW_IRQ_VECTOR0                  (SILAN_ICTL1_BASE + 0x040)
#define DW_IRQ_VECTOR1                  (SILAN_ICTL1_BASE + 0x048)
#define DW_IRQ_VECTOR2                  (SILAN_ICTL1_BASE + 0x050)
#define DW_IRQ_VECTOR3                  (SILAN_ICTL1_BASE + 0x058)
#define DW_IRQ_VECTOR4                  (SILAN_ICTL1_BASE + 0x060)
#define DW_IRQ_VECTOR5                  (SILAN_ICTL1_BASE + 0x068)
#define DW_IRQ_VECTOR6                  (SILAN_ICTL1_BASE + 0x070)
#define DW_IRQ_VECTOR7                  (SILAN_ICTL1_BASE + 0x078)
#define DW_IRQ_VECTOR8                  (SILAN_ICTL1_BASE + 0x080)
#define DW_IRQ_VECTOR9                  (SILAN_ICTL1_BASE + 0x088)
#define DW_IRQ_VECTOR10                 (SILAN_ICTL1_BASE + 0x090)
#define DW_IRQ_VECTOR11                 (SILAN_ICTL1_BASE + 0x098)
#define DW_IRQ_VECTOR12                 (SILAN_ICTL1_BASE + 0x0A0)
#define DW_IRQ_VECTOR13                 (SILAN_ICTL1_BASE + 0x0A8)
#define DW_IRQ_VECTOR14                 (SILAN_ICTL1_BASE + 0x0B0)
#define DW_IRQ_VECTOR15                 (SILAN_ICTL1_BASE + 0x0B8)

#define DW_FIQ_INTEN                    (SILAN_ICTL1_BASE + 0x0C0)
#define DW_FIQ_INTMASK                  (SILAN_ICTL1_BASE + 0x0C4)
#define DW_FIQ_INTFORCE                 (SILAN_ICTL1_BASE + 0x0C8)
#define DW_FIQ_RAWSTATUS                (SILAN_ICTL1_BASE + 0x0CC)
#define DW_FIQ_STATUS                   (SILAN_ICTL1_BASE + 0x0D0)
#define DW_FIQ_FINALSTATUS              (SILAN_ICTL1_BASE + 0x0D4)

#define DW_IRQ_PLEVEL                   (SILAN_ICTL1_BASE + 0x0D8)
#define DW_IRQ_INTERNAL_PLEVEL          (SILAN_ICTL1_BASE + 0x0DC)

#define DW_PR(x)                        (SILAN_ICTL1_BASE + 0x0E8 + (x << 2))
#define DW_ICTL_COMP_PARAMS2            (SILAN_ICTL1_BASE + 0x3F0)
#define DW_ICTL_COMP_PARAMS1            (SILAN_ICTL1_BASE + 0x3F4)
#define DW_ICTL_COMP_VERSION            (SILAN_ICTL1_BASE + 0x3FC)
#define DW_ICTL_COMP_TYPE               (SILAN_ICTL1_BASE + 0x3FC)

int gic_present;

DEFINE_RAW_SPINLOCK(irq_lock);

#ifdef CONFIG_MIPS_MT_SMP
static int cpu_ipi_resched_irq, cpu_ipi_call_irq;

static void ipi_resched_dispatch(void)
{
    do_IRQ(MIPS_CPU_IRQ_BASE + MIPS_CPU_IPI_RESCHED_IRQ);
}

static void ipi_call_dispatch(void)
{
    do_IRQ(MIPS_CPU_IRQ_BASE + MIPS_CPU_IPI_CALL_IRQ);
}

static irqreturn_t ipi_resched_interrupt(int irq, void *dev_id)
{
    scheduler_ipi();

    return IRQ_HANDLED;
}

static irqreturn_t ipi_call_interrupt(int irq, void *dev_id)
{
    smp_call_function_interrupt();

    return IRQ_HANDLED;
}

static struct irqaction irq_resched = {
    .handler    = ipi_resched_interrupt,
    .flags        = IRQF_DISABLED|IRQF_PERCPU,
    .name        = "IPI_resched"
};

static struct irqaction irq_call = {
    .handler    = ipi_call_interrupt,
    .flags        = IRQF_DISABLED|IRQF_PERCPU,
    .name        = "IPI_call"
};
#endif /* CONFIG_MIPS_MT_SMP */

silan_irq_map_t silan_irq_map[] = 
{
    {PIC_IRQ_USB_OTG,    INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_SPDIF,      INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_DRM,        INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_DAC_IIS,    INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_ADC_IIS,    INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_SDMMC,      INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_SDIO,       INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_GPIO1,      INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_GPIO2,      INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_GMAC,       INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_I2C1,       INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_USB_HOST,   INTC_INT_HIGH_LEVEL,    0},
    {MIPS_DSP_CXC_IRQ,   INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_UART1,      INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_UART2,      INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_DMAC0,      INTC_INT_HIGH_LEVEL,    0},
	{PIC_IRQ_CODEC,      INTC_INT_HIGH_LEVEL,    0},

    {PIC_IRQ_UNICOM_CXC_RTC,        INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_UNICOM_CXC_IR,         INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_UNICOM_CXC_SPIFLASH,   INTC_INT_HIGH_LEVEL,    0},
    {PIC_IRQ_UNICOM_CXC_KEYPAD,     INTC_INT_HIGH_LEVEL,    0},
};

int silan_pic_nr_irqs = sizeof(silan_irq_map)/sizeof(silan_irq_map_t);

static void dw_enable_irq(struct irq_data *d)
{
    u32 bit;
    u32 reg;    
    u32 data;
    u32 shift;
    ulong flags;
    u32 irq_nr;;
    
    raw_spin_lock_irqsave(&irq_lock, flags);

    irq_nr = d->irq;
    bit = irq_nr - MIPS_PIC_IRQ_BASE;
    reg = (bit >= 32)?DW_IRQ_INTMASK_H:DW_IRQ_INTMASK_L;
    shift = (bit >= 32)?(bit - 32):bit;
    
    data = sl_readl(reg);
    data &= ~(1UL << shift);
    sl_writel(data, reg);
    
    raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static void dw_disable_irq(struct irq_data *d)
{
    u32 bit;
    u32 reg;    
    u32 data;
    u32 shift;
    ulong flags;
    u32 irq_nr;
    
    raw_spin_lock_irqsave(&irq_lock, flags);

    irq_nr = d->irq;
    bit = irq_nr - MIPS_PIC_IRQ_BASE;
    reg = (bit >= 32)?DW_IRQ_INTMASK_H:DW_IRQ_INTMASK_L;
    shift = (bit >= 32)?(bit - 32):bit;
    
    data = sl_readl(reg);
    data |= (1UL << shift);
    sl_writel(data, reg);
    
    raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static void dw_ack_irq(struct irq_data *d)
{
    u32 bit;
    u32 reg;    
    u32 data;
    u32 shift;
    ulong flags;
    u32 irq_nr;
    
    raw_spin_lock_irqsave(&irq_lock, flags);

    irq_nr = d->irq;
    bit = irq_nr - MIPS_PIC_IRQ_BASE;
    reg = (bit >= 32)?DW_IRQ_INTMASK_H:DW_IRQ_INTMASK_L;
    shift = (bit >= 32)?(bit - 32):bit;
    
    data = sl_readl(reg);
    data |= (1UL << shift);
    sl_writel(data, reg);
    
    raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static void dw_mask_irq(struct irq_data *d)
{
    u32 bit;
    u32 reg;    
    u32 data;
    u32 shift;
    ulong flags;
    u32 irq_nr;

    raw_spin_lock_irqsave(&irq_lock, flags);

    irq_nr = d->irq;
    bit = irq_nr - MIPS_PIC_IRQ_BASE;
    reg = (bit >= 32)?DW_IRQ_INTMASK_H:DW_IRQ_INTMASK_L;
    shift = (bit >= 32)?(bit - 32):bit;
    
    data = sl_readl(reg);
    data |= (1UL << shift);
    sl_writel(data, reg);
    
    raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static void dw_unmask_irq(struct irq_data *d)
{
    u32 bit;
    u32 reg;    
    u32 data;
    u32 shift;
    ulong flags;
    u32 irq_nr;
    
    raw_spin_lock_irqsave(&irq_lock, flags);
    
    irq_nr = d->irq;
    bit = irq_nr - MIPS_PIC_IRQ_BASE;
    reg = (bit >= 32)?DW_IRQ_INTMASK_H:DW_IRQ_INTMASK_L;
    shift = (bit >= 32)?(bit - 32):bit;
    
    data = sl_readl(reg);
    data &= ~(1UL << shift);
    sl_writel(data, reg);
    
    raw_spin_unlock_irqrestore(&irq_lock, flags);
}

static struct irq_chip silan_pic =
{
    .name     = "SILAN-IRQ",
    .irq_enable     = dw_enable_irq,
    .irq_disable     = dw_disable_irq,
    .irq_ack         = dw_ack_irq,
    .irq_mask        = dw_mask_irq,
    .irq_mask_ack    = dw_mask_irq,
    .irq_unmask        = dw_unmask_irq,
    .irq_eoi        = dw_unmask_irq,
};

static void setup_dw_irq(u32 irq_nr, int type, int int_req)
{
    switch (type) {
        /* this interrupt controller looks like only support the high level irq */
        case INTC_INT_HIGH_LEVEL:
            irq_set_chip_and_handler(irq_nr, &silan_pic, handle_level_irq);
            break;
            
        case INTC_INT_LOW_LEVEL:
        case INTC_INT_RISE_EDGE:
        case INTC_INT_FALL_EDGE:
        default:
            printk("unsupported int type %d (irq %d)\n", type, irq_nr);
            return;
    }
}

static void silan_dw_ictl_init(void)
{
    int i;
    u32 bit;
    int reg;
    u32 data;
    u32 shift;
    silan_irq_map_t *imp;
    
    /* Initial PIC-IRQ, disable all irq */
    sl_writel(0x00000000, DW_IRQ_INTEN_L);
    sl_writel(0x00000000, DW_IRQ_INTEN_H);
    sl_writel(0x00000000, DW_FIQ_INTEN);

    sl_writel(0xFFFFFFFF, DW_IRQ_INTMASK_L);
    sl_writel(0xFFFFFFFF, DW_IRQ_INTMASK_H);
    sl_writel(0xFFFFFFFF, DW_FIQ_INTMASK);

    /* Initialize IC0, which is fixed per processor */
    imp = silan_irq_map;
    for ( i = 0; i < silan_pic_nr_irqs; i++) {
        bit = imp->im_irq - MIPS_PIC_IRQ_BASE;
        reg = (bit >= 32)?DW_IRQ_INTEN_H:DW_IRQ_INTEN_L;
        shift = (bit >= 32)?(bit - 32):bit;
        
        data = sl_readl(reg);
        data |= 1UL << shift;
        sl_writel(data, reg);
        
        setup_dw_irq(imp->im_irq, imp->im_type, imp->im_request );
        imp++;
    }
    
    write_c0_status(read_c0_status() | STATUSF_IP2);
}

static inline int clz(u32 x)
{
    __asm__(
    "    .set    push                \n"
    "    .set    mips32                \n"
    "    clz    %0, %1                    \n"
    "    .set    pop                    \n"
    : "=r" (x)
    : "r" (x));

    return x;
}

/*
 * Version of ffs that only looks at bits 12..15.
 */
static inline unsigned int irq_ffs(u32 pending)
{
#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
    return -clz(pending) + 31 - CAUSEB_IP;
#else
    u32 a0 = 7;
    u32 t0;

    t0 = pending & 0xf000;
    t0 = t0 < 1;
    t0 = t0 << 2;
    a0 = a0 - t0;
    pending = pending << t0;

    t0 = pending & 0xc000;
    t0 = t0 < 1;
    t0 = t0 << 1;
    a0 = a0 - t0;
    pending = pending << t0;

    t0 = pending & 0x8000;
    t0 = t0 < 1;
    /* t0 = t0 << 2; */
    a0 = a0 - t0;
    /* pending = pending << t0; */

    return a0;
#endif
}

static void silan_pic_irqdispatch(void)
{
    int irq;
    unsigned int pending;
    
    pending = sl_readl(DW_IRQ_FINALSTATUS_L);
#if 0
    if(pending&(1<<(PIC_IRQ_VPP-MIPS_PIC_IRQ_BASE)))
    {
        irq = PIC_IRQ_VPP;
        do_IRQ(irq);
        return;
    }
#endif

    if(pending) {
        irq = 31 - clz(pending) + MIPS_PIC_IRQ_BASE;
        do_IRQ(irq);
        return;
    }

    pending = sl_readl(DW_IRQ_FINALSTATUS_H);
    if(pending) {
        irq = 31 - clz(pending) + MIPS_PIC_IRQ_BASE + 32;
        do_IRQ(irq);
        return;
    }
}

asmlinkage void plat_irq_dispatch(void)
{
    u32 pending = read_c0_status() & read_c0_cause() & ST0_IM;
    int irq;    

    irq = irq_ffs(pending);

    if (irq == 2)
        silan_pic_irqdispatch();
    else if(irq >= 0)
        do_IRQ(MIPS_CPU_IRQ_BASE + irq);
    else
        spurious_interrupt();
    
    return;
}

void __init arch_init_ipiirq(int irq, struct irqaction *action)
{
    setup_irq(irq, action);
    irq_set_handler(irq, handle_percpu_irq);
}

void __init arch_init_irq(void)
{
    gic_present = 0;
    
    /* initialize the 1st-level CPU based interrupt controller */
    mips_cpu_irq_init();
      
#ifdef CONFIG_MIPS_MT_SMP
    if (cpu_has_vint) {
        set_vi_handler (MIPS_CPU_IPI_RESCHED_IRQ, ipi_resched_dispatch);
        set_vi_handler (MIPS_CPU_IPI_CALL_IRQ, ipi_call_dispatch);
        set_vi_handler (MIPS_CPU_IRQ_PIC, silan_pic_irqdispatch);
    }
    cpu_ipi_resched_irq = MIPS_CPU_IRQ_BASE + MIPS_CPU_IPI_RESCHED_IRQ;
    cpu_ipi_call_irq = MIPS_CPU_IRQ_BASE + MIPS_CPU_IPI_CALL_IRQ;

    arch_init_ipiirq(cpu_ipi_resched_irq, &irq_resched);
    arch_init_ipiirq(cpu_ipi_call_irq, &irq_call);
#endif    
    silan_dw_ictl_init();
}

