/*
 * arch/csky/cskyevb/timer.c --- timer handles
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2004, Kang Sun (sunk@vlsi.zju.edu.cn)
 * Copyright (C) 2004, Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009, Hu Junshan (junshan_hu@c-sky.com)
 * Copyright (C) 2015, Chen Linfei (linfei_chen@c-sky.com)
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/rtc.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>

#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/csky.h>
#include <mach/cktimer.h>
#include <asm/delay.h>
#include <mach/silan_irq.h>

extern unsigned int get_silan_busclk(void);

unsigned int timer_busclk;
unsigned int timer_busclk_hz;

static cycle_t notrace csky_clocksource_read(struct clocksource *unused)
{
	volatile unsigned long  *timerp;

	timerp = (volatile unsigned long *) (CKTIMER_BASE);

	return timerp[CKTIMER_TCN2_CVR];
}

static struct clocksource csky_clocksource = {
	.name	= "csky_timer2",
	.rating	= 300,
	.read	= csky_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __init csky_timer1_hw_init(void)
{
	volatile unsigned long  *timerp;

	timerp = (volatile unsigned long *) (CKTIMER_BASE);

	/*
	 * Set up TIMER 1 as poll clock
	 * Set timer control register, 8 bits, clock = APB bus clock
	 * The work mode of timer1 is user-defined running mode
	 * Enable timer1 and tiemr1 interrupt
	 */
	timerp[CKTIMER_TCN1_CR] = 0x1;

	/*
	 *  set the init value of timer1 load counter, 24bits
	 *  when the counter overflow, interrupt occurs
	 */
	timerp[CKTIMER_TCN1_LDCR] = (timer_busclk_hz);
}

static void __init csky_timer2_hw_init(void)
{
	volatile unsigned long  *timerp;

	timerp = (volatile unsigned long *) (CKTIMER_BASE);

	/*
	 * Set up TIMER 1 as poll clock
	 * Set timer control register, 8 bits, clock = APB bus clock
	 * The work mode of timer1 is user-defined running mode
	 * Enable timer1 and tiemr1 interrupt
	 */
	//timerp[CKTIMER_TCN2_CR] = CKTIMER_TCR_IM | CKTIMER_TCR_EN;
	timerp[CKTIMER_TCN2_CR] = 0x1;
	/*
	 *  set the init value of timer1 load counter, 24bits
	 *  when the counter overflow, interrupt occurs
	 */
	timerp[CKTIMER_TCN2_LDCR] = 0xffffffffu;
	//timerp[CKTIMER_TCN2_LDCR] = 0x0u;
}

static int csky_timer_set_next_event(unsigned long cycles, struct clock_event_device *evt)
{
	volatile unsigned long  *timerp;

	timerp = (volatile unsigned long *) (CKTIMER_BASE);
	timerp[CKTIMER_TCN1_LDCR] = cycles;
	timerp[CKTIMER_TCN1_CVR] = 0x0;

	return 0;
}

static void csky_timer_set_mode(enum clock_event_mode mode,
                              struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device csky_clockevent = {
	.name 		= "csky_timer1",
	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_next_event	= csky_timer_set_next_event,
	.set_mode	= csky_timer_set_mode,
};


static irqreturn_t csky_timer_interrupt(int irq, void *dev_id)
{
	volatile unsigned long  *timerp;
	unsigned long   temp;
	struct clock_event_device *evt = &csky_clockevent;

	timerp = (volatile unsigned long *) (CKTIMER_BASE);

	temp = timerp[CKTIMER_TCN1_CR];
	temp &= ~0x2;
	timerp[CKTIMER_TCN1_CR] = temp;

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction csky_timer_irq = {
	.name           = "csky timer1",
	.flags          = IRQF_DISABLED,
	.handler        = csky_timer_interrupt,
};

void __init csky_timer_init(void)
{
	unsigned int csky_clock_rate;
    timer_busclk = get_silan_busclk();
    timer_busclk_hz = timer_busclk/HZ;
    csky_clock_rate = timer_busclk;

	csky_timer1_hw_init();
	csky_timer2_hw_init();
	setup_irq(PIC_IRQ_TO0, &csky_timer_irq);
	clockevents_calc_mult_shift(&csky_clockevent, csky_clock_rate, 4);
	csky_clockevent.max_delta_ns = clockevent_delta2ns((timer_busclk_hz)/10, &csky_clockevent);
	csky_clockevent.min_delta_ns = clockevent_delta2ns(10, &csky_clockevent);
	csky_clockevent.cpumask      = cpumask_of(0);

	clockevents_register_device(&csky_clockevent);

	clocksource_register_hz(&csky_clocksource, csky_clock_rate);
}

#if 0
/*
 *  Clears the interrupt from timer1.
 */
void csky_tick(void)
{
	volatile unsigned long	*timerp;
	unsigned long	temp;

	/* Ack and Clear the interrupt from timer1 */
	timerp = (volatile unsigned long *) (CKTIMER_BASE);
	temp = timerp[CKTIMER_TCN1_CR];
	temp &= ~0x2;
	timerp[CKTIMER_TCN1_CR] = temp;
}

unsigned long csky_timer_offset(void)
{
	volatile unsigned long  *timerp;
	unsigned long       trr, tcn, offset;

	timerp = (volatile unsigned long *) (CKTIMER_BASE);
	tcn = timerp[CKTIMER_TCN1_CVR];
	trr = timerp[CKTIMER_TCN1_LDCR];

//	tcn = trr - tcn;

	offset = ((tcn * (10000 / HZ)) / (trr / 100));
	return offset;
}

/*
 *  set or get the real time clock. But now, we don't support.
 */
int csky_hwclk(int set, struct rtc_time *t)
{
	t->tm_year = 1980;
	t->tm_mon  = 1;
	t->tm_mday = 1;
	t->tm_hour = 0;
	t->tm_min  = 0;
	t->tm_sec  = 0;	

	return 0;
}
#endif
