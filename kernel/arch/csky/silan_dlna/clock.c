/*
 * Setting up the clock on MSP SOCs.  No RTC typically.
 *
 * Pan jianguang, panjianguang@silan.com.cn
 * Copyright (C) 1999,2010 Silan.  All rights reserved.
 *
 * ########################################################################
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
 * ########################################################################
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <silan_resources.h>
#include <silan_generic.h>

#include <silan_pmu.h>

/* We originally used an mutex here, but some contexts (see resume)
 * are calling functions such as clk_set_parent() with IRQs disabled
 * causing an BUG to be triggered.
 */
DEFINE_SPINLOCK(clocks_lock);
#define OSC_CLK            12000000
/* enable and disable calls for use with the clk struct */
#define SILAN_CLK_GET_FS_SD(x)      (((x)>>8)&0x7)
#define SILAN_CLK_SET_FS_SD(x)      ((x)<<8)
//#define SILAN_CLK_GET_FS_EMMC(x)    (((x)>>12)&0xf)
//#define SILAN_CLK_SET_FS_EMMC(x)    ((x)<<12)
#define SILAN_CLK_GET_FS_SDIO(x)    (((x)>>12)&0x7)
#define SILAN_CLK_SET_FS_SDIO(x)    ((x)<<12)
#define SILAN_CLK_GET_FS_MC(x)      (((x)>>16)&0x7)
#define SILAN_CLK_SET_FS_MC(x)      ((x)<<16)

#define SD_CCLK_SEL(_nr)      ((_nr)<<8)
#define SD_CCLK_EN            (0x1<<11)
#define SDIO_CCLK_SEL(_nr)    ((_nr)<<12)
#define SDIO_CCLK_EN          (0x1<<15)
#define MC_CCLK_SEL(_nr)      ((_nr)<<16)    
#define MC_CCLK_EN            (0x1<<19)    

#define CH1_BCLK_EN           (0x1<<1)
#define CH1_MCLK_EN           (0x1<<2)
#define CH1_SPDIF_EN          (0x1<<3)
#define SPDIF_IN_EN           (0x1<<4)
#define CH2_BCLK_EN           (0x1<<9)
#define CH2_MCLK_EN           (0x1<<10)
#define DAC_BYPASS_EN         (0x1<<24)
#define DAC_IIS_ACT_EN        (0x1<<25)
#define ADC_BYPASS_EN         (0x1<<26)
#define ADC_IIS_ACT_EN        (0x1<<27)
#define AUDIO_MODE            (0x1<<31)

//SD/SDIO ext clock configure
#define SILAN_SYS_REG1            (SILAN_CR_BASE+4)
//SD/SDIO bus clock configure
#define SILAN_SYS_REG2            (SILAN_CR_BASE+8)
//system pll control
#define SILAN_SYS_REG3            (SILAN_CR_BASE+0xC)
//audio control
#define SILAN_SYS_REG4            (SILAN_CR_BASE+0x10)
//audio clock configure
#define SILAN_SYS_REG5            (SILAN_CR_BASE+0x14)
//module software reset
#define SILAN_SYS_REG6            (SILAN_CR_BASE+0x18)
//system configure
#define SILAN_SYS_REG7            (SILAN_CR_BASE+0x1c)
//system miscellaneous control
#define SILAN_SYS_REG8            (SILAN_CR_BASE+0x20)
//pad mutiplex select 
#define SILAN_SYS_REG9            (SILAN_CR_BASE+0x24)
//dram clock control
#define SILAN_SYS_REG10           (SILAN_CR_BASE+0x28)
//usb phy configure
#define SILAN_SYS_REG11           (SILAN_CR_BASE+0x2c)

#define SILAN_SYS_REG12           (SILAN_CR_BASE+0x30)

//uart4 control
#define SILAN_SYS_REG14           (SILAN_CR_BASE+0x38)
//codec control
#define SILAN_SYS_REG15           (SILAN_CR_BASE+0x3c)
//usb1.1 control
#define SILAN_SYS_REG16           (SILAN_CR_BASE+0x40)

//system status 
#define SILAN_STS_REG0            (SILAN_CR_BASE+0x100)
//chip id 
#define SILAN_STS_REG2            (SILAN_CR_BASE+0x88)

typedef enum
{
    SILAN_CLK_START = 0,
    SILAN_CLK_SDMMC = SILAN_CLK_START,
    SILAN_CLK_SDIO,
    SILAN_CLK_MC,
    SILAN_CLK_IIS_OUT,
    SILAN_CLK_SPDIF,
    SILAN_CLK_IIS_IN,
    SILAN_CLK_ADC,

    SILAN_CLK_BUS,
    
    SILAN_CLK_USBHOST,
    SILAN_CLK_USBOTG,
    SILAN_CLK_GMAC,
    SILAN_CLK_DMAC0,
    SILAN_CLK_DSP0,
    SILAN_CLK_SPI,
    SILAN_CLK_UART1,
    SILAN_CLK_UART2,
    SILAN_CLK_UART3,
    SILAN_CLK_UART4,
    SILAN_CLK_I2C,
    SILAN_CLK_GPIO,
    SILAN_CLK_WTG,
    SILAN_CLK_SPDIF_IN,
    SILAN_CLK_END, 
}SILAN_CLK_ID;

struct clk{
    char name[16];
    int id;
    SILAN_CLK_ID clk_id;
    unsigned long rate;
    unsigned long (*get_rate)(struct clk *);
};

static struct clk silan_clk[]=
{
    {
        .name = "sdmmc",
        .id = -1,
        .clk_id = SILAN_CLK_SDMMC,
    },
    {
        .name = "sdio",
        .id = -1,
        .clk_id = SILAN_CLK_SDIO,
    },
    {
        .name = "mc",
        .id = -1,
        .clk_id = SILAN_CLK_MC,
    },
    {
        .name = "i2s_out",
        .id = -1,
        .clk_id = SILAN_CLK_IIS_OUT,
    },
    {
        .name = "spdif",
        .id = -1,
        .clk_id = SILAN_CLK_SPDIF,
    },    
    {
        .name = "i2s_in",
        .id = -1,
        .clk_id = SILAN_CLK_IIS_IN,
    },    
    {
        .name = "usbhost",
        .id = -1,
        .clk_id = SILAN_CLK_USBHOST,
    },    
    {
        .name = "usbdevice",
        .id = -1,
        .clk_id = SILAN_CLK_USBOTG,
    },    
    {
        .name = "gmac",
        .id = -1,
        .clk_id = SILAN_CLK_GMAC,
    },
    {
        .name = "dmac0",
        .id = -1,
        .clk_id = SILAN_CLK_DMAC0,
    },
    {
        .name = "dsp0",
        .id = -1,
        .clk_id = SILAN_CLK_DSP0,
    },    
    { .name = "spi",
        .id = -1,
        .clk_id = SILAN_CLK_SPI,
    },
    {
        .name = "uart0",
        .id = 1,
        .clk_id = SILAN_CLK_UART1,
    }, 
    {
        .name = "uart2",
        .id = 1,
        .clk_id = SILAN_CLK_UART2,
    }, 
    {
        .name = "uart3",
        .id = 1,
        .clk_id = SILAN_CLK_UART3,
    },
    {
        .name = "uart4",
        .id = 1,
        .clk_id = SILAN_CLK_UART4,
    }, 
    {
        .name = "i2c",
        .id = -1,
        .clk_id = SILAN_CLK_I2C,
    },    
    {
        .name = "gpio",
        .id = -1,
        .clk_id = SILAN_CLK_GPIO,
    },
    {
        .name = "watchdog",
        .id = -1,
        .clk_id = SILAN_CLK_WTG,
    },
    {
        .name = "spdif_in",
        .id = -1,
        .clk_id = SILAN_CLK_SPDIF_IN,
    },
    {
        .name = "adc",
        .id = -1,
        .clk_id = SILAN_CLK_ADC,
    },
    {
        .name = "bus",
        .id = -1,
        .clk_id = SILAN_CLK_BUS,
    },
};

unsigned int get_silan_pllclk(void)
{
    unsigned int pll_speed, regval_pmu, regval;
    union silan_pmu_pll_l pll_status_l;

	spin_lock(&clocks_lock);

	pll_status_l.data = sl_readl(SILAN_PMU_PLL_STATUS_L);
	if (pll_status_l.reg.pll_cfg_sel)
	{
		if (pll_status_l.reg.pll_fast_cfg == 0xF) {
			pll_speed = 12000000 * (pll_status_l.reg.pll_dn + 1) / (pll_status_l.reg.pll_dm + 1);
			//printk("##### %s, %d , %d #####\n", __func__, __LINE__, pll_speed);
			return pll_speed;
		}
		else {
			regval = (regval_pmu>>28) & 0xf;
		}
	}
	else {
		regval = sl_readl(SILAN_STS_REG0);
		regval = (regval & 0xf);
	}

    switch(regval)
    {
        case 1:
            pll_speed = 90000000;
            break;
        case 3:
            pll_speed = 150000000;
            break;
        case 5:
            pll_speed = 180000000;
            break;
        case 7:
            pll_speed = 216000000;
            break;
        case 9:
            pll_speed = 240000000;
            break;
        case 11:
            pll_speed = 264000000;
            break;
        case 13:
            pll_speed = 300000000;
            break;
        case 14:
            pll_speed = 324000000;
            break;
        case 15:
            pll_speed = -1;
            break;
        default:
            pll_speed = 0;
            break;
    }
    spin_unlock(&clocks_lock);

    return pll_speed;
}
EXPORT_SYMBOL(get_silan_pllclk);

unsigned int get_silan_cpuclk(void)
{
    unsigned int core_speed, div_speed, regval_pmu;
    
    //system clock source from osc12m/2
    regval_pmu = sl_readl(SILAN_PMU_STATUS);
    if (regval_pmu & (1<<3))
    {
        core_speed = 12000000/2;
    }
    else
    {
        core_speed = get_silan_pllclk();
    }

    switch (regval_pmu & 0x3) {
        case 0: div_speed = core_speed; break;
        case 1: div_speed = core_speed/2; break;
        case 2: div_speed = core_speed/3; break;
        case 3: div_speed = core_speed/4; break;
        default: div_speed = core_speed; break;
    }

    return div_speed;
}
EXPORT_SYMBOL(get_silan_cpuclk);

unsigned int get_silan_busclk(void)
{
    unsigned int core_speed, div_speed, bus_speed, regval_pmu;
    
    // system clock source from osc12m/2 ?
	regval_pmu = sl_readl(SILAN_PMU_STATUS);
	if (regval_pmu & (1<<3))
	{
		core_speed = 12000000/2;
	}
	else {
		core_speed = get_silan_pllclk();
	}

    switch (regval_pmu & 0x3) {
        case 0: div_speed = core_speed; break;
        case 1: div_speed = core_speed/2; break;
        case 2: div_speed = core_speed/3; break;
        case 3: div_speed = core_speed/4; break;
        default: div_speed = core_speed; break;
    }
	// clk_ratio	
	if (regval_pmu & (1<<6))
	{
		bus_speed = div_speed/2;
	}
	else {
		bus_speed = div_speed;
	}
	
    return bus_speed;
}
EXPORT_SYMBOL(get_silan_busclk);

void __init silan_clocks_init(void)
{
    unsigned long int busclk, cpuclk, pllclk;
    cpuclk = get_silan_cpuclk();
    busclk = get_silan_busclk();
    pllclk = get_silan_pllclk();
	
    pr_info("Clocks: CPU:%lu.%03luMHz, DSP:%lu.%03luMHz, SDR:%lu.%03luMHz, PLL:%lu.%03luMHz\n",
		 cpuclk / 1000000,
		(cpuclk / 1000) % 1000,
		 busclk / 1000000,
		(busclk / 1000) % 1000,
		 busclk / 1000000,
		(busclk / 1000) % 1000,
		 pllclk / 1000000,
		(pllclk / 1000) % 1000);
}

struct clk *clk_get(struct device *dev, const char *id)
{
    int i,found = 0;
    struct clk *p=NULL;
#if 0    
    int idno;
    if (dev == NULL || dev->bus != &platform_bus_type)
        idno = -1;
    else
        idno = to_platform_device(dev)->id;
#endif
    spin_lock(&clocks_lock);
    for(i=0;i<sizeof(silan_clk)/sizeof(struct clk);i++) 
    {
        p= &silan_clk[i];
        if (strcmp(id, p->name) == 0)
        {
            found = 1;
            break;
        }
    }
    spin_unlock(&clocks_lock);

    if(found)
        return p;
    else
        return ERR_PTR(-ENOENT);
}

EXPORT_SYMBOL(clk_get);

int _clk_set_sdmmc_sdio(int id, int fre)
{
    u32 reg0_value;

    switch(fre) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            fre -= 1;
            break;
        case 8:
            fre = 6;
            break;
        case 16:
            fre = 7;
            break;
        default:
            return -1;
    }
    
    spin_lock(&clocks_lock);

    reg0_value = sl_readl(SILAN_SYS_REG2);
    switch(id){
        case SILAN_CLK_SDMMC:
            reg0_value &= ~(0x7<<8);
            reg0_value |= SD_CCLK_SEL(fre);
            break;
        case SILAN_CLK_SDIO:
            reg0_value &= ~(0x7<<12);
            reg0_value |= SDIO_CCLK_SEL(fre);
            break;
        case SILAN_CLK_MC:
            reg0_value &= ~(0x7<<16);
            reg0_value |= MC_CCLK_SEL(fre);
            break;
    }
    sl_writel(reg0_value, SILAN_SYS_REG2);
    
    spin_unlock(&clocks_lock);
    
    return 0;
}

int clk_set_sdmmc(int fre)
{    
    return _clk_set_sdmmc_sdio(SILAN_CLK_SDMMC, fre);
}

int clk_set_sdio(int fre)
{    
    return _clk_set_sdmmc_sdio(SILAN_CLK_SDIO, fre);
}

int clk_set_mc(int fre)
{    
    return _clk_set_sdmmc_sdio(SILAN_CLK_MC, fre);
}

static int _clk_disable_sdmmc_sdio(int id)
{
    u32 reg0_value;
    spin_lock(&clocks_lock);

    reg0_value = sl_readl(SILAN_SYS_REG2);    
    switch(id){
        case SILAN_CLK_SDMMC:
            reg0_value &= ~SD_CCLK_EN;
            break;
        case SILAN_CLK_SDIO:
            reg0_value &= ~SDIO_CCLK_EN;
            break;
        case SILAN_CLK_MC:
            reg0_value &= ~MC_CCLK_EN;
            break;
    }
    sl_writel(reg0_value, SILAN_SYS_REG2);
    
    spin_unlock(&clocks_lock);

    return 0;
}

int _clk_enable_sdmmc_sdio(int id)
{
    u32 reg0_value;    
    spin_lock(&clocks_lock);
    
    reg0_value = sl_readl(SILAN_SYS_REG2);    
    switch(id){
        case SILAN_CLK_SDMMC:
            reg0_value |= SD_CCLK_EN;
            break;
        case SILAN_CLK_SDIO:
            reg0_value |= SDIO_CCLK_EN;
            break;
        case SILAN_CLK_MC:
            reg0_value |= MC_CCLK_EN;
            break;
    }
    sl_writel(reg0_value, SILAN_SYS_REG2);
    
    spin_unlock(&clocks_lock);

    return 0;
}

static int clk_enable_sdmmc(void)
{
    return _clk_enable_sdmmc_sdio(SILAN_CLK_SDMMC);
}

static int clk_disable_sdmmc(void)
{
    return _clk_disable_sdmmc_sdio(SILAN_CLK_SDMMC);
}

static int clk_enable_sdio(void)
{
    return _clk_enable_sdmmc_sdio(SILAN_CLK_SDIO);
}

static int clk_disable_sdio(void)
{
    return _clk_disable_sdmmc_sdio(SILAN_CLK_SDIO);
}

static int clk_enable_mc(void)
{
    return _clk_enable_sdmmc_sdio(SILAN_CLK_MC);
}

static int clk_disable_mc(void)
{
    return _clk_disable_sdmmc_sdio(SILAN_CLK_MC);
}

static int _clk_enable_audio(int id)
{
    u32 reg0_value;    
    spin_lock(&clocks_lock);
    
    reg0_value = sl_readl(SILAN_SYS_REG4);    
    switch(id){
        case SILAN_CLK_IIS_OUT:
            reg0_value |= (CH1_BCLK_EN | CH1_MCLK_EN);
            break;
        case SILAN_CLK_SPDIF:
            reg0_value |= CH1_SPDIF_EN;
            break;
        case SILAN_CLK_IIS_IN:
            reg0_value |= (CH2_BCLK_EN | CH2_MCLK_EN);
            break;
        case SILAN_CLK_SPDIF_IN:
            reg0_value |= SPDIF_IN_EN;
            sl_writel((sl_readl(SILAN_SYS_REG12)) | (0x1 << 29), SILAN_SYS_REG12);
            break;
    }
    sl_writel(reg0_value, SILAN_SYS_REG4);
    
    spin_unlock(&clocks_lock);

    return 0;
}

static int _clk_disable_audio(int id)
{
    u32 reg0_value;    
    spin_lock(&clocks_lock);
    
    reg0_value = sl_readl(SILAN_SYS_REG4);    
    switch(id){
        case SILAN_CLK_IIS_OUT:
            reg0_value &= ~(CH1_BCLK_EN | CH1_MCLK_EN);
            break;
        case SILAN_CLK_SPDIF:
            reg0_value &= ~CH1_SPDIF_EN;
            break;
        case SILAN_CLK_IIS_IN:
            reg0_value &= ~(CH2_BCLK_EN | CH2_MCLK_EN);
            break;
        case SILAN_CLK_SPDIF_IN:
            sl_writel((sl_readl(SILAN_SYS_REG12)) & (~(0x1 << 29)), SILAN_SYS_REG12);
            break;
    }
    sl_writel(reg0_value, SILAN_SYS_REG4);
    
    spin_unlock(&clocks_lock);

    return 0;
}

static int clk_enable_iis_out(void)
{
    return _clk_enable_audio(SILAN_CLK_IIS_OUT);
}

static int clk_disable_iis_out(void)
{
    return _clk_disable_audio(SILAN_CLK_IIS_OUT);
}

static int clk_enable_iis_in(void)
{
    return _clk_enable_audio(SILAN_CLK_IIS_IN);
}

static int clk_disable_iis_in(void)
{
    return _clk_disable_audio(SILAN_CLK_IIS_IN);
}

static int clk_enable_spdif(void)
{
    return _clk_enable_audio(SILAN_CLK_SPDIF);
}

static int clk_disable_spdif(void)
{
    return _clk_disable_audio(SILAN_CLK_SPDIF);
}

static int clk_enable_spdif_in(void)
{
    return _clk_enable_audio(SILAN_CLK_SPDIF_IN);
}

static int clk_disable_spdif_in(void)
{
    return _clk_disable_audio(SILAN_CLK_SPDIF_IN);
}

static int clk_enable_watchdog()
{
	u32 val = sl_readl(SILAN_SYS_REG15);	
	val |= (1<<30);
    sl_writel(val, SILAN_SYS_REG15);
}

static int clk_enable_uart()
{
	u32 val = sl_readl(SILAN_SYS_REG14);	
	val |= 1<<0;
    sl_writel(val, SILAN_SYS_REG14);
}

int clk_enable(struct clk *clk)
{
    switch(clk->clk_id)
    {
        case SILAN_CLK_SDMMC:
            clk_enable_sdmmc();
            break;
        case SILAN_CLK_SDIO:
            clk_enable_sdio();
            break;
        case SILAN_CLK_MC:
            clk_enable_mc();
            break;
        case SILAN_CLK_IIS_OUT:
            clk_enable_iis_out();
            break;
        case SILAN_CLK_SPDIF:
            clk_enable_spdif();
            break;
        case SILAN_CLK_IIS_IN:
            clk_enable_iis_in();
            break;
        case SILAN_CLK_SPDIF_IN:
            clk_enable_spdif_in();
            break;
        case SILAN_CLK_WTG:
            clk_enable_watchdog();
            break;
        case SILAN_CLK_UART4:
            clk_enable_uart(clk);
            break;
        default:
            break;
    }
    return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
    if(clk->clk_id == SILAN_CLK_UART1)
        return ;

    switch(clk->clk_id)
    {
        case SILAN_CLK_SDMMC:
            clk_disable_sdmmc();
            break;
        case SILAN_CLK_SDIO:
            clk_disable_sdio();
            break;
        case SILAN_CLK_MC:
            clk_disable_mc();
            break;
        case SILAN_CLK_IIS_OUT:
            clk_disable_iis_out();
            break;
        case SILAN_CLK_SPDIF:
            clk_disable_spdif();
            break;
        case SILAN_CLK_IIS_IN:
            clk_disable_iis_in();
            break;
        case SILAN_CLK_SPDIF_IN:
            clk_disable_spdif_in();
            break;
        default:
            break;
    }
}
EXPORT_SYMBOL(clk_disable);

static unsigned long clk_set_i2s_out_rate(unsigned long rate, int mode, int inout)
{
    u32 reg0_value, reg1_value, reg2_value;

    reg0_value = sl_readl(SILAN_SYS_REG15);
    reg2_value = sl_readl(SILAN_SYS_REG5);

    reg1_value = 0;
    if(inout == 0){
        reg2_value &= 0xffffff00;
        reg2_value |= 0x04;
    }
    else if(inout == 1){
        reg2_value &= 0xffff00ff;
        reg2_value |= 0x0400;
    }
    if(mode == 1){
        if(rate == 176400 || rate == 128000 || rate == 192000){
            printk("Do not support the sample rate: %ld, while mclk=256fs\n", rate);
            return -1;
        }
    }
    else if(mode == 2){
        if(inout == 0){
            reg2_value &= 0xffffff00;
            reg2_value |= 0x02;
        }
        else if(inout == 1){
            reg2_value &= 0xffff00ff;
            reg2_value |= 0x0200;
            //reg2_value |= 0x0800;
        }
    }

	//printk("###### SILAN_SYS_REG4:%x SILAN_SYS_REG5:%x  %x\n", reg0_value, reg1_value, reg2_value);
    switch(rate){
        case 11025:
            reg0_value |= AUDIO_MODE;
            reg1_value |= (0x3 << 4);
            break;
        case 22050:
            reg0_value |= AUDIO_MODE;
            reg1_value |= (0x2 << 4);
            break;
        case 44100:
            reg0_value |= AUDIO_MODE;
            reg1_value |= (0x1 << 4);
            break;
        case 88200:
            reg0_value |= AUDIO_MODE;
            reg1_value |= (0x0 << 4);
            break;
        case 176400:
            reg0_value |= AUDIO_MODE;
            reg1_value |= (0x0 << 4);
            reg1_value &= ~(0x1 << 3);
            break;
        case 8000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x1 << 6);
            reg1_value |= (0x3 << 4);
            break;
        case 16000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x1 << 6);
            reg1_value |= (0x2 << 4);
            break;
        case 32000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x1 << 6);
            reg1_value |= (0x1 << 4);
            break;
        case 64000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x1 << 6);
            reg1_value |= (0x0 << 4);
            break;
        case 128000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x1 << 6);
            reg1_value |= (0x0 << 4);
            reg1_value &= ~(0x1 << 3);
            break;
        case 12000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x3 << 4);
            break;
        case 24000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x2 << 4);
            break;
        case 48000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x1 << 4);
            break;
        case 96000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x0 << 4);
            break;
        case 192000:
            reg0_value &= ~AUDIO_MODE;
            reg1_value |= (0x0 << 4);
            reg1_value &= ~(0x1 << 3);
            break;
        default:
            return -1;
    }

    if(inout == 1){
        reg1_value <<= 8;
    }
    reg1_value |= reg2_value;
	//printk("###### SILAN_SYS_REG4:%x SILAN_SYS_REG5:%x  %x\n", reg0_value, reg1_value, reg2_value);

    sl_writel(reg0_value, SILAN_SYS_REG15);
    sl_writel(reg1_value, SILAN_SYS_REG5);
    //sl_writel(0x402, SILAN_SYS_REG5);

    return 0;
}
#if 1
static unsigned long clk_set_i2s_out_rate1(unsigned long rate)
{
    return clk_set_i2s_out_rate(rate, 1, 0);
}

static unsigned long clk_set_i2s_in_rate1(unsigned long rate)
{
    return clk_set_i2s_out_rate(rate, 1, 1);
}
#endif
#if 1
static unsigned long clk_set_i2s_out_rate2(unsigned long rate)
{
    return clk_set_i2s_out_rate(rate, 2, 0);
}

static unsigned long clk_set_i2s_in_rate2(unsigned long rate)
{
    return clk_set_i2s_out_rate(rate, 2, 1);
}
#endif

static unsigned long clk_set_spdif_rate(unsigned long rate)
{
    //return clk_set_i2s_out_rate(rate, 2, 0);
    u32 reg0_value, reg1_value;

    reg0_value = sl_readl(SILAN_SYS_REG5);
    reg1_value = sl_readl(SILAN_SYS_REG15);

    switch (rate)
    {
        case 44100:
            reg0_value &= ~(0x7 << 20);
            reg0_value |= (0x2 << 20);
            reg1_value |= (0x1 << 31);
            break;

        case 48000:
            reg0_value &= ~(0x7 << 20);
            reg0_value |= (0x2 << 20);
            reg1_value &= ~(0x1 << 31);
            break;

        case 32000:
            reg0_value &= ~(0x7 << 20);
            reg0_value |= (0x6 << 20);
            reg1_value &= ~(0x1 << 31);
            break;

        case 96000:
            reg0_value &= ~(0x7 << 20);
            reg0_value |= (0x1 << 20);
            reg1_value &= ~(0x1 << 31);
            break;

        default:
            printk("clk_set_spdif_rate err, unsupport rate \n");
            break;
    }

    sl_writel(reg0_value, SILAN_SYS_REG5);
    sl_writel(reg1_value, SILAN_SYS_REG15);

    return 0;
}

static unsigned long clk_set_spdif_in_rate(unsigned long rate)
{
    u32 reg0_value, reg1_value;

    reg0_value = sl_readl(SILAN_SYS_REG15);
    reg1_value = sl_readl(SILAN_SYS_REG5);

    reg1_value &= ~0x18000; /* spdif_sample_sel[16:15] */

    switch (rate) {
        case 44://44100:
            reg0_value |= (0x1 << 0);
            reg1_value |= (0x2 << 15);
            break;
        case 88://88200:
            reg0_value |= (0x1 << 0);
            reg1_value |= (0x1 << 15);
            break;
        case 176://176400:
            reg0_value |= (0x1 << 0);
            reg1_value |= (0x0 << 15);
            break;

        case 48://48000:
            reg0_value &= ~(0x1 << 0);
            reg1_value |= (0x2 << 15);
            break;
        case 96://96000:
            reg0_value &= ~(0x1 << 0);
            reg1_value |= (0x1 << 15);
            break;
        case 192://192000:
            reg0_value &= ~(0x1 << 0);
            reg1_value |= (0x0 << 15);
            break;
        default:
            return -1;
    }

    //printk("clk set spdif in rate: %ld reg0_value: 0x%x, reg1_value: 0x%x \n", rate, reg0_value, reg1_value);
    sl_writel(reg0_value, SILAN_SYS_REG15);
    sl_writel(reg1_value, SILAN_SYS_REG5);

    return 0;
}

static unsigned long clk_get_sdmmc(int id, unsigned long pll)
{
    u32 reg_value,fs = 0;    
    spin_lock(&clocks_lock);
    
	reg_value = sl_readl(SILAN_SYS_REG2);    
    if(id == 0)
        fs = SILAN_CLK_GET_FS_SD(reg_value);
    else if(id == 1)
        fs = SILAN_CLK_GET_FS_MC(reg_value);
    else if(id == 2)
        fs = SILAN_CLK_GET_FS_SDIO(reg_value);
    spin_unlock(&clocks_lock);

    switch(fs)
    {
        case 0:
            return pll/2;
            break;
        case 1:
            return pll/2;
            break;
        case 2:
            return pll/3;
            break;
        case 3:
            return pll/4;
            break;
        case 4:
            return pll/5;
            break;
        case 5:
            return pll/6;
            break;
        case 6:
            return pll/8;
            break;
        case 7:
            return pll/16;
            break;
        default:
            return pll/2;
            break;
    }

    return pll;
}

unsigned long clk_get_rate(struct clk *clk)
{
    unsigned long silan_pll = get_silan_pllclk();

    switch (clk->clk_id)
    {
        case SILAN_CLK_SDMMC:
            return clk_get_sdmmc(0, silan_pll);
            break;
        case SILAN_CLK_SDIO:
            return clk_get_sdmmc(2, silan_pll);
            break;
        case SILAN_CLK_WTG:
            return OSC_CLK / 2;
            break;
        default:
            break;
    }

    return get_silan_busclk();
}
EXPORT_SYMBOL(clk_get_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
    //unsigned long silan_pll = get_silan_pllclk();

    spin_lock(&clocks_lock);
    switch (clk->clk_id)
    {
        case SILAN_CLK_IIS_OUT:
            if(rate == 176400 || rate == 192000){
                clk_set_i2s_out_rate2(rate);
            }
            else{
                clk_set_i2s_out_rate1(rate);
            }
            break;
        case SILAN_CLK_IIS_IN:
            if(rate == 176400 || rate == 192000){
                clk_set_i2s_in_rate2(rate);
            }
            else{
                clk_set_i2s_in_rate1(rate);
            }
            break;
        case SILAN_CLK_SPDIF:
            clk_set_spdif_rate(rate);
            break;
        default:
            break;
    }
    spin_unlock(&clocks_lock);

    return 0;
}
EXPORT_SYMBOL(clk_set_rate);

void clk_put(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_put);
