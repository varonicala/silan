/*
 *  Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
 *	JZ4740 SoC power management support
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/init.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/gpio.h>

#include <silan_def.h>
#include <silan_generic.h>
#include <silan_pmu.h>
#include <silan_regs.h>

#define SYSPLL_MANUAL_CFG		0xF
#define SYSPLL_FAST_CFG_324M	0xE
#define SYSPLL_FAST_CFG_300M	0xD
#define SYSPLL_FAST_CFG_282M	0xC
#define SYSPLL_FAST_CFG_264M	0xB
#define SYSPLL_FAST_CFG_258M	0xA
#define SYSPLL_FAST_CFG_240M	0x9
#define SYSPLL_FAST_CFG_220M	0x8
#define SYSPLL_FAST_CFG_216M	0x7
#define SYSPLL_FAST_CFG_200M	0x6
#define SYSPLL_FAST_CFG_180M	0x5
#define SYSPLL_FAST_CFG_162M	0x4
#define SYSPLL_FAST_CFG_150M	0x3
#define SYSPLL_FAST_CFG_120M	0x2
#define SYSPLL_FAST_CFG_90M	    0x1
#define SYSPLL_FAST_CFG_60M	    0x0

#define SILAN_SDRAM_REFCNT(m)   (((int)(m * 78 / 10)) & 0xFFFF)

static void silan_pmu_request(int pmu_cfg)
{
	sl_writel(pmu_cfg, SILAN_PMU_CFG);
}

static void silan_pmu_int_polarity(int pol)
{
	sl_writel(pol, SILAN_PMU_INTPOL);
}

static void silan_pmu_int_mask(int mask)
{
	sl_writel(mask, SILAN_PMU_INTMASK);
}

static void silan_pmu_int_clr(void)
{
	sl_writel(1, SILAN_PMU_INTCLR);
}

//int pmu_ctrl_reg12_data = 0;
int pmu_ctrl_reg13_data = 0;
static silan_pmu_ctrl(int mode)
{
    int reg;

    if (mode == 0)
    {
        // module clock shutdown
        //pmu_ctrl_reg12_data = sl_readl(SILAN_SYS_REG12);
        //reg = (1<<5) | (1<<13) | (1<<20) | (1<<23) | (1<<25);
        //sl_writel(reg, SILAN_SYS_REG12);

        // usb and audio pll pwdn
        reg = sl_readl(SILAN_SYS_REG15);
        reg &= ~(1<<16);
        reg &= ~(1<<17);
        sl_writel(reg, SILAN_SYS_REG15);

        // gpio drive strength to 2mA
        pmu_ctrl_reg13_data = sl_readl(SILAN_SYS_REG13);
        sl_writel(0, SILAN_SYS_REG13);

        //codec
        //reg = sl_readl(SILAN_SYS_REG2);
        //reg |= (1<<19);
        //sl_writel(reg, SILAN_SYS_REG2);
        //
        //reg = sl_readl(SILAN_CODEC_BASE + 0x20);
        //reg |= 0x40;
        //sl_writel(reg, SILAN_CODEC_BASE + 0x20);
    }
    else
    {
        // module clock restore
        //sl_writel(pmu_ctrl_reg12_data, SILAN_SYS_REG12);

        // usb and audio pll restore
        reg = sl_readl(SILAN_SYS_REG15);
        reg |= (1<<16);
        reg |= (1<<17);
        sl_writel(reg, SILAN_SYS_REG15);

        // gpio drive strength to 2mA
        sl_writel(pmu_ctrl_reg13_data, SILAN_SYS_REG13);

        // codec
        //reg = sl_readl(SILAN_CODEC_BASE + 0x20);
        //reg &= 0xBF;
        //sl_writel(reg, SILAN_CODEC_BASE + 0x20);
        //msleep(5);
    }
}

static suspend_state_t pmu_state_old = PM_SUSPEND_NORMALCLOCK;
static int silan_pmu_enter(suspend_state_t state)
{
	int value, err = 0;
	union silan_pmu_cfg pmu_cfg;
	union silan_pmu_pll_l pmu_pll_l;

    int hi, lo;
    int num = state>>8;
    state = state&0xff;
    hi = num/100;
    lo = num%100;
    if (hi == 1 && (lo >= 0 && lo <= 20))
        num = lo + 5;
    else if (hi == 2 && (lo >= 9 && lo <= 12))
        num = lo + 17;
    else
    {
        printk("silan_pmu_enter failed, gpio %d-%02d error\n", hi, lo);
        return -1;
    }

    hi = hi - 1;
    gpio_request(hi*32 + lo, "silan-gpio");
    gpio_direction_input(hi*32 + lo);

    //printk("#### %s %d s %d %d %d %d\n", __func__, __LINE__, state, num, hi, lo);
	pmu_cfg.data = 0;
	switch (state)
	{
		case PM_SUSPEND_CPUOFF:
			pmu_cfg.reg.mode = CLOCK_GATE;
			pmu_cfg.reg.sref_mode = 1;
			//value = 0xffffffff & (~(1 << SILAN_PMU_INT_SRC));
			value = 0xffffffff & (~(1 << num));
			silan_pmu_int_mask(value);
			silan_pmu_int_polarity(SILAN_PMU_INT_POL_L);
			break;

		case PM_SUSPEND_PLLOFF:
			silan_pmu_ctrl(0);
			pmu_cfg.reg.mode = PLL_POWERDOWN;
			pmu_cfg.reg.sref_mode = 1;
			//value = 0xffffffff & (~(1 << SILAN_PMU_INT_SRC));
			value = 0xffffffff & (~(1 << num));
			silan_pmu_int_mask(value);
			silan_pmu_int_polarity(SILAN_PMU_INT_POL_L);
			break;

		case PM_SUSPEND_OVERCLOCK:
			if (pmu_state_old == PM_SUSPEND_NORMALCLOCK)
			{
				pmu_cfg.reg.mode = PLL_UPDATE;
				pmu_cfg.reg.sref_mode = 1;
				pmu_cfg.reg.sref_cnt_en = 1;
				
				pmu_pll_l.data = sl_readl(SILAN_PMU_PLL_CFG_L);
				//pmu_pll_l.reg.pll_fast_cfg = SYSPLL_FAST_CFG_324M;
				pmu_pll_l.reg.pll_fast_cfg = SYSPLL_MANUAL_CFG;
				pmu_pll_l.reg.pll_dm = 0;
				pmu_pll_l.reg.pll_dn = 26;
				pmu_pll_l.reg.pll_cfg_sel = 1;
				sl_writel(pmu_pll_l.data, SILAN_PMU_PLL_CFG_L);
				sl_writel(SILAN_SDRAM_REFCNT(324/2), SILAN_PMU_PLL_CFG_H);
				pmu_state_old = state;
			}
			else
				err = -1;
			break;

		case PM_SUSPEND_NORMALCLOCK:
			if (pmu_state_old == PM_SUSPEND_OVERCLOCK)
			{
				pmu_cfg.reg.mode = PLL_UPDATE;
				pmu_cfg.reg.sref_mode = 1;
				pmu_cfg.reg.sref_cnt_en = 1;
				
				pmu_pll_l.data = sl_readl(SILAN_PMU_PLL_CFG_L);
				pmu_pll_l.reg.pll_cfg_sel = 0;
				sl_writel(pmu_pll_l.data, SILAN_PMU_PLL_CFG_L);
				sl_writel(SILAN_SDRAM_REFCNT(300/2), SILAN_PMU_PLL_CFG_H);
				pmu_state_old = state;
			}
			else if (pmu_state_old == PM_SUSPEND_UNDERCLOCK)
			{
				pmu_cfg.reg.mode = CLOCK_SWITCH;
				pmu_cfg.reg.clk_source = SYS_PLL;
				pmu_cfg.reg.clock_ratio = RATIO_2TO1;
				pmu_cfg.reg.sref_mode = 1;
				pmu_cfg.reg.sref_cnt_en = 1;
				sl_writel(SILAN_SDRAM_REFCNT(300/2), SILAN_PMU_PLL_CFG_H);
				pmu_state_old = state;
				//sl_writel(0x6012, 0xba000028);
			}
			else
				err = -1;
			break;

		case PM_SUSPEND_UNDERCLOCK:
			if (pmu_state_old == PM_SUSPEND_NORMALCLOCK)
			{
				pmu_cfg.reg.mode = CLOCK_SWITCH;
				pmu_cfg.reg.clk_source = OSC_12M;
				pmu_cfg.reg.clock_ratio = RATIO_1TO1;
				pmu_cfg.reg.sref_mode = 1;
				pmu_cfg.reg.sref_cnt_en = 1;
				sl_writel(SILAN_SDRAM_REFCNT(6), SILAN_PMU_PLL_CFG_H);
				pmu_state_old = state;
				//sl_writel(0x6322, 0xba000028);
			}
			else
				err = -1;
			break;

		default:
			err = -1;
			break;
	}
	
	if (err == 0)
	{
		pmu_cfg.reg.request = 1;
		silan_pmu_request(pmu_cfg.data);
	}

	return err;
}

static int silan_pmu_exit(suspend_state_t state)
{
	silan_pmu_int_clr();
	return 0;
}

static int silan_pm_enter(suspend_state_t state)
{
	int err;
	u32 reg_bk;

	printk("enter silan suspend mode\n");

	err = silan_pmu_enter(state);

	if (err == 0)
		__asm__("wait");

	if ((state&0xff) == PM_SUSPEND_PLLOFF)
		silan_pmu_ctrl(1);
	silan_pmu_exit(state);

	printk("enter silan resume mode \n");

	return 0;
}

static const struct platform_suspend_ops silan_pm_ops = 
{
	.valid		= suspend_valid_silan,
	.enter		= silan_pm_enter,
};

static int __init silan_pm_init(void) 
{
	suspend_set_ops(&silan_pm_ops);
	return 0;

}
late_initcall(silan_pm_init);


