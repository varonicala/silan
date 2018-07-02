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

/* We originally used an mutex here, but some contexts (see resume)
 * are calling functions such as clk_set_parent() with IRQs disabled
 * causing an BUG to be triggered.
 */
DEFINE_SPINLOCK(clocks_lock);

/* enable and disable calls for use with the clk struct */
#define SILAN_CLK_GET_FS_SD(x)      (((x)>>8)&0xf)
#define SILAN_CLK_SET_FS_SD(x)      ((x)<<8)
#define SILAN_CLK_GET_FS_EMMC(x)    (((x)>>12)&0xf)
#define SILAN_CLK_SET_FS_EMMC(x)    ((x)<<12)
#define SILAN_CLK_GET_FS_SDIO(x)    (((x)>>16)&0xf)
#define SILAN_CLK_SET_FS_SDIO(x)    ((x)<<16)

#define SD_CCLK_SEL(_nr)      ((_nr)<<8)
#define SD_CCLK_EN            (0x1<<11)
#define SDIO_CCLK_SEL(_nr)    ((_nr)<<12)
#define SDIO_CCLK_EN          (0x1<<15)
#define MC_CCLK_SEL(_nr)      ((_nr)<<16)    
#define MC_CCLK_EN            (0x1<<19)    

#define CH1_BCLK_EN           (0x1<<1)
#define CH1_MCLK_EN           (0x1<<2)
#define CH1_SPDIF_EN          (0x1<<3)
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

    SILAN_CLK_USBHOST,
    SILAN_CLK_USBOTG,
    SILAN_CLK_GMAC,
    SILAN_CLK_DMAC0,
    SILAN_CLK_DSP0,
    SILAN_CLK_SPI,
    SILAN_CLK_UART,
    SILAN_CLK_I2C,
    SILAN_CLK_GPIO,
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
        .id = 0,
        .clk_id = SILAN_CLK_UART,
    },    
    {
        .name = "uart1",
        .id = 1,
        .clk_id = SILAN_CLK_UART,
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
};

unsigned int get_silan_pllclk(void)
{
    unsigned int cpu_speed;
#ifndef CONFIG_MIPS_SILAN_FPGA
    unsigned int regval = sl_readl(SILAN_STS_REG0);
    regval = (regval & 0x0000000f);
#endif
    spin_lock(&clocks_lock);
#if defined(CONFIG_MIPS_SILAN_FPGA)
    cpu_speed = 100000000;
#else
    switch(regval)
    {
        case 0:
            cpu_speed = 60000000;
            break;
        case 1:
            cpu_speed = 90000000;
            break;
        case 2:
            cpu_speed = 120000000;
            break;
        case 3:
            cpu_speed = 150000000;
            break;
        case 4:
            cpu_speed = 162000000;
            break;
        case 5:
            cpu_speed = 180000000;
            break;
        case 6:
            cpu_speed = 200000000;
            break;
        case 7:
            cpu_speed = 216000000;
            break;
        case 8:
            cpu_speed = 220000000;
            break;
        case 9:
            cpu_speed = 240000000;
            break;
        case 10:
            cpu_speed = 258000000;
            break;
        case 11:
            cpu_speed = 264000000;
            break;
        case 12:
            cpu_speed = 282000000;
            break;
        case 13:
            cpu_speed = 300000000;
            break;
        case 14:
            cpu_speed = 324000000;
            break;
        default:
            cpu_speed = 0;
            break;
    }
#endif
    spin_unlock(&clocks_lock);
	
    return cpu_speed;
}
EXPORT_SYMBOL(get_silan_pllclk);

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

        default:
            break;
    }
    return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
    if(clk->clk_id == SILAN_CLK_UART)
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
        default:
            break;
    }
}
EXPORT_SYMBOL(clk_disable);

static unsigned long clk_set_i2s_out_rate(unsigned long rate, int mode, int inout)
{
    u32 reg0_value, reg1_value, reg2_value;

    reg0_value = sl_readl(SILAN_SYS_REG4);
    reg2_value = sl_readl(SILAN_SYS_REG5);

    reg1_value = 0;
    if(inout == 0){
        reg2_value &= 0xff00;
        reg2_value |= 0x04;
    }
    else if(inout == 1){
        reg2_value &= 0x00ff;
        reg2_value |= 0x0400;
    }
    
    if(mode == 1){
        if(rate == 176400 || rate == 128000 || rate == 192000){
            printk("Do not support the sample rate: %ld, while mclk=256fs\n", rate);
            return -1;
        }
    }
    else if(mode == 2){
        if(inout == 0)
            reg2_value |= 0x08;
        else if(inout == 1)
            reg2_value |= 0x0800;
    }

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
        reg1_value <<= 4;
    }
    reg1_value |= reg2_value;

    sl_writel(reg0_value, SILAN_SYS_REG4);
    sl_writel(reg1_value, SILAN_SYS_REG5);

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
#if 0
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
    return clk_set_i2s_out_rate(rate, 2, 0);
}

static unsigned long clk_get_sdmmc(int id, unsigned long pll)
{
    u32 reg_value,fs = 0;    
    spin_lock(&clocks_lock);
    reg_value = sl_readl(SILAN_SYS_REG3);    
    if(id == 0)
        fs = SILAN_CLK_GET_FS_SD(reg_value);
    else if(id == 1)
        fs = SILAN_CLK_GET_FS_EMMC(reg_value);
    else if(id == 2)
        fs = SILAN_CLK_GET_FS_SDIO(reg_value);
    spin_unlock(&clocks_lock);

    switch(fs)
    {
        case 0:
            return pll;
            break;
        case 1:
            return pll*3/4;
            break;
        case 2:
            return pll/10;
            break;
        case 3:
            return pll/12;
            break;
        default:
            break;
    }

    return pll;
}

unsigned long clk_get_rate(struct clk *clk)
{
    unsigned long silan_pll = get_silan_pllclk();

    switch (clk->clk_id)
    {
        case SILAN_CLK_UART:
            return silan_pll / 2;
            break;
        case SILAN_CLK_I2C:
            break;
        case SILAN_CLK_SDMMC:
            return clk_get_sdmmc(0,silan_pll);
            break;
        case SILAN_CLK_SDIO:
            return clk_get_sdmmc(2,silan_pll);
            break;
        default:
            break;
    }

    return silan_pll/2;
}
EXPORT_SYMBOL(clk_get_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
    //unsigned long silan_pll = get_silan_pllclk();

    spin_lock(&clocks_lock);
    switch (clk->clk_id)
    {
        case SILAN_CLK_IIS_OUT:
            clk_set_i2s_out_rate1(rate);
            break;
        case SILAN_CLK_IIS_IN:
            clk_set_i2s_in_rate1(rate);
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
