#include <linux/module.h>
#include <silan_resources.h>
#include <silan_generic.h>
#include <silan_padmux.h>
#include <silan_gpio.h>

#define SILAN_PADMUX_CTRL            (SILAN_CR_BASE + 0x24)
#define SILAN_PADMUX2_CTRL           (SILAN_CR_BASE + 0x1c)

#define PAD2_SE    0x80
#define NONE       0xff

struct pad_mux_1{
    char *name;
    u8 lev0;
    u8 lev1;
    u8 lev2;
    u8 lev3;
    u8 lev4;
}padmuxall[] = {
    {"gpio1.0",  NONE, SILAN_PADMUX2_PWM0|PAD2_SE, NONE, SILAN_PADMUX_IISADC_FD0, NONE},
    {"gpio1.1",  NONE, NONE, SILAN_PADMUX_UART4_2, SILAN_PADMUX_IISADC, NONE},
    {"gpio1.2",  NONE, NONE, SILAN_PADMUX_UART4_2, SILAN_PADMUX_IISADC, NONE},
    {"gpio1.3",  NONE, NONE, NONE, SILAN_PADMUX_IISDAC_MCLK, NONE},
    {"gpio1.4",  NONE, NONE, NONE, SILAN_PADMUX_IISDAC, NONE},
    {"gpio1.5",  NONE, NONE, NONE, SILAN_PADMUX_IISDAC, NONE},
    {"gpio1.6",  NONE, NONE, NONE, SILAN_PADMUX_IISDAC_FD0, NONE},
    {"gpio1.7",  NONE, SILAN_PADMUX2_PWM2|PAD2_SE, NONE, SILAN_PADMUX_IISDAC_FD1, NONE},
    {"gpio1.8",  NONE, SILAN_PADMUX2_PWM3|PAD2_SE, NONE, SILAN_PADMUX_IISDAC_FD2, NONE},
    {"gpio1.9",  NONE, NONE, NONE, SILAN_PADMUX_SPDIF_IN0, NONE},
    {"gpio1.10", NONE, NONE, NONE, SILAN_PADMUX_SPDIF_IN1, NONE},
    {"gpio1.11", NONE, NONE, NONE, SILAN_PADMUX_SPDIF_IN2, NONE},
    {"gpio1.12", NONE, NONE, NONE, SILAN_PADMUX_SPDIF_IN3, NONE},
    {"gpio1.13", NONE, NONE, NONE, SILAN_PADMUX_UART3_1, NONE},
    {"gpio1.14", NONE, NONE, NONE, SILAN_PADMUX_UART3_1, NONE},
    {"gpio1.15", NONE, NONE, SILAN_PADMUX_SPI, SILAN_PADMUX_UART1, NONE},
    {"gpio1.16", NONE, NONE, SILAN_PADMUX_SPI_EX, SILAN_PADMUX_UART1, NONE},
    {"gpio1.17", NONE, NONE, SILAN_PADMUX_SPI, SILAN_PADMUX_I2C1, NONE},
    {"gpio1.18", NONE, NONE, SILAN_PADMUX_SPI, SILAN_PADMUX_I2C1, NONE},
    {"gpio1.19", NONE, NONE, SILAN_PADMUX_SF_DAT23, SILAN_PADMUX_UART2, NONE},
    {"gpio1.20", NONE, NONE, SILAN_PADMUX_SF_DAT23, SILAN_PADMUX_UART2, NONE},
    {"gpio1.21", NONE, SILAN_PADMUX2_PWM2_CH2|PAD2_SE, SILAN_PADMUX2_UART1_CH2, SILAN_PADMUX2_IISADC_FD1_CH2, NONE},
    {"gpio1.22", NONE, SILAN_PADMUX2_PWM3_CH2|PAD2_SE, SILAN_PADMUX2_UART1_CH2, SILAN_PADMUX2_IISADC_FD2_CH2|PAD2_SE, NONE},
    {"gpio1.23", NONE, NONE, SILAN_PADMUX2_PWM1|PAD2_SE, SILAN_PADMUX_SDMMCDET, NONE},
    {"gpio1.24", NONE, NONE, NONE, SILAN_PADMUX_SDMMCDAT, NONE},
    {"gpio1.25", NONE, NONE, NONE, SILAN_PADMUX_SDMMC, NONE},
    {"gpio1.26", NONE, NONE, NONE, SILAN_PADMUX_SDMMC, NONE},
    {"gpio1.27", NONE, NONE, NONE, SILAN_PADMUX_SDMMC, NONE},
    {"gpio1.28", NONE, NONE, NONE, SILAN_PADMUX_SDMMCDAT, NONE},
    {"gpio1.29", NONE, NONE, NONE, SILAN_PADMUX_SDMMCDAT, NONE},

    {"gpio2.0",  NONE, NONE, SILAN_PADMUX_SDIODET, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.1",  NONE, NONE, SILAN_PADMUX_SDIODAT, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.2",  NONE, NONE, SILAN_PADMUX_SDIO, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.3",  NONE, NONE, SILAN_PADMUX_SDIO, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.4",  NONE, NONE, SILAN_PADMUX_SDIO, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.5",  NONE, NONE, SILAN_PADMUX_SDIODAT, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.6",  NONE, NONE, SILAN_PADMUX_SDIODAT, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.7",  NONE, SILAN_PADMUX2_PWM6|PAD2_SE, NONE, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.8",  NONE, SILAN_PADMUX2_PWM4|PAD2_SE, SILAN_PADMUX_UART4_1, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.9",  NONE, SILAN_PADMUX2_PWM5|PAD2_SE, SILAN_PADMUX_UART4_1, SILAN_PADMUX_GMAC, NONE},
    {"gpio2.10", NONE, SILAN_PADMUX2_PWM7|PAD2_SE, NONE, SILAN_PADMUX_SPDIF, NONE},
    {"gpio2.11", NONE, SILAN_PADMUX_I2C2, SILAN_PADMUX_UART3_2, SILAN_PADMUX_IISADC_FD1, NONE},
    {"gpio2.12", NONE, SILAN_PADMUX_I2C2, SILAN_PADMUX_UART3_2, SILAN_PADMUX_IISADC_FD2, NONE},
    {"gpio2.13", NONE, SILAN_PADMUX2_PWM1_CH2|PAD2_SE, SILAN_PADMUX2_SPDIF_IN0_CH2|PAD2_SE, NONE, NONE},
    {"gpio2.14", NONE, NONE, SILAN_PADMUX2_SPDIF_IN1_CH2|PAD2_SE, NONE, NONE},
    {"gpio2.15", NONE, NONE, SILAN_PADMUX2_SPDIF_IN2_CH2|PAD2_SE, SILAN_PADMUX2_SDIO_DAT_CH2|PAD2_SE, NONE},
    {"gpio2.16", NONE, NONE, SILAN_PADMUX2_SPDIF_IN3_CH2|PAD2_SE, SILAN_PADMUX2_SDIO_CH2|PAD2_SE, NONE},
    {"gpio2.17", NONE, NONE, NONE, SILAN_PADMUX2_SDIO_CH2|PAD2_SE, NONE},
    {"gpio2.18", NONE, NONE, NONE, SILAN_PADMUX2_SDIO_CH2|PAD2_SE, NONE},
    {"gpio2.19", NONE, NONE, NONE, SILAN_PADMUX2_SDIO_DAT_CH2|PAD2_SE, NONE},
    {"gpio2.20", NONE, NONE, NONE, SILAN_PADMUX2_SDIO_DAT_CH2|PAD2_SE, NONE},

    {"pwm0",     SILAN_PADMUX2_PWM0|PAD2_SE, NONE, SILAN_PADMUX_IISADC_FD0, NONE, NONE},
    {"pwm2_ch1", SILAN_PADMUX2_PWM2|PAD2_SE, SILAN_PADMUX_IISDAC_FD1, NONE, NONE, NONE},
    {"pwm3_ch1", SILAN_PADMUX2_PWM3|PAD2_SE, NONE, SILAN_PADMUX_IISDAC_FD2, NONE},
    {"pwm2_ch2", SILAN_PADMUX2_PWM2_CH2|PAD2_SE, SILAN_PADMUX2_UART1_CH2, SILAN_PADMUX2_IISADC_FD1_CH2, NONE, NONE},
    {"pwm3_ch2", SILAN_PADMUX2_PWM3_CH2|PAD2_SE, SILAN_PADMUX2_UART1_CH2, SILAN_PADMUX2_IISADC_FD2_CH2|PAD2_SE, NONE, NONE},
    {"pwm6",     SILAN_PADMUX2_PWM6|PAD2_SE, NONE, SILAN_PADMUX_GMAC, NONE, NONE},
    {"pwm4",     SILAN_PADMUX2_PWM4|PAD2_SE, SILAN_PADMUX_UART4_1, SILAN_PADMUX_GMAC, NONE, NONE},
    {"pwm5",     SILAN_PADMUX2_PWM5|PAD2_SE, SILAN_PADMUX_UART4_1, SILAN_PADMUX_GMAC, NONE, NONE},
    {"pwm7",     SILAN_PADMUX2_PWM7|PAD2_SE, NONE, SILAN_PADMUX_SPDIF, NONE, NONE},
    {"i2c2",     SILAN_PADMUX_I2C2, SILAN_PADMUX_UART3_2, SILAN_PADMUX_IISADC_FD1,SILAN_PADMUX_IISADC_FD2, NONE},
    {"pwm1_ch2", SILAN_PADMUX2_PWM1_CH2|PAD2_SE, SILAN_PADMUX2_SPDIF_IN0_CH2|PAD2_SE, NONE, NONE, NONE},
    
    {"uart4_ch2",    SILAN_PADMUX_UART4_2, SILAN_PADMUX_IISADC, NONE, NONE, NONE},
    {"spi",          SILAN_PADMUX_SPI, SILAN_PADMUX_UART1, SILAN_PADMUX_I2C1, NONE, NONE},
    {"spi_miso",     SILAN_PADMUX_SPI_EX, SILAN_PADMUX_UART1, NONE, NONE, NONE},
    {"spiflash_data23", SILAN_PADMUX_SF_DAT23, SILAN_PADMUX_UART2, NONE, NONE, NONE},
    {"uart1_ch2",    SILAN_PADMUX2_UART1_CH2|PAD2_SE, SILAN_PADMUX2_IISADC_FD1_CH2, SILAN_PADMUX2_IISADC_FD2_CH2, NONE, NONE},
    {"pwm1_ch1",     SILAN_PADMUX2_PWM1|PAD2_SE, SILAN_PADMUX_SDMMCDET, NONE, NONE, NONE},
    {"sdio_det_ch1", SILAN_PADMUX_SDIODET, SILAN_PADMUX_GMAC, NONE, NONE, NONE},
    {"sdio_data_ch1", SILAN_PADMUX_SDIODAT, SILAN_PADMUX_GMAC, NONE, NONE, NONE},
    {"sdio_ch1",     SILAN_PADMUX_SDIO, SILAN_PADMUX_GMAC, NONE, NONE, NONE},
    {"uart4_ch1",    SILAN_PADMUX_UART4_1, SILAN_PADMUX_GMAC, NONE, NONE, NONE},
    {"uart3_ch2",    SILAN_PADMUX_UART3_2, SILAN_PADMUX_IISADC_FD1, SILAN_PADMUX_IISADC_FD2, NONE, NONE},
    {"spdif_i0_ch2", SILAN_PADMUX2_SPDIF_IN1_CH2|PAD2_SE, NONE, NONE, NONE, NONE},
    {"spdif_i1_ch2", SILAN_PADMUX2_SPDIF_IN1_CH2|PAD2_SE, NONE, NONE, NONE, NONE},
    {"spdif_i2_ch2", SILAN_PADMUX2_SPDIF_IN2_CH2|PAD2_SE, SILAN_PADMUX2_SDIO_DAT_CH2|PAD2_SE, NONE, NONE, NONE},
    {"spdif_i3_ch2", SILAN_PADMUX2_SPDIF_IN3_CH2|PAD2_SE, SILAN_PADMUX2_SDIO_CH2|PAD2_SE, NONE, NONE, NONE},
    
    {"iis_adc_sdi0", SILAN_PADMUX_IISADC_FD0, NONE, NONE, NONE, NONE},
    {"iis_adc",      SILAN_PADMUX_IISADC, NONE, NONE, NONE, NONE},
    {"iis_dac_mclk", SILAN_PADMUX_IISDAC_MCLK, NONE, NONE, NONE, NONE},
    {"iis_dac",      SILAN_PADMUX_IISDAC, NONE, NONE, NONE, NONE},
    {"iis_dac_sdo0", SILAN_PADMUX_IISDAC_FD0, NONE, NONE, NONE, NONE},
    {"iis_dac_sdo1", SILAN_PADMUX_IISDAC_FD1, NONE, NONE, NONE, NONE},
    {"iis_dac_sdo2", SILAN_PADMUX_IISDAC_FD2, NONE, NONE, NONE, NONE},
    {"spdif_i0_ch1", SILAN_PADMUX_SPDIF_IN1, NONE, NONE, NONE, NONE},
    {"spdif_i1_ch1", SILAN_PADMUX_SPDIF_IN1, NONE, NONE, NONE, NONE},
    {"spdif_i2_ch1", SILAN_PADMUX_SPDIF_IN2, NONE, NONE, NONE, NONE},
    {"spdif_i3_ch1", SILAN_PADMUX_SPDIF_IN3, NONE, NONE, NONE, NONE},
    {"uart3_ch1",    SILAN_PADMUX_UART3_1, NONE, NONE, NONE, NONE},
    {"uart1_ch1",    SILAN_PADMUX_UART1, NONE, NONE, NONE, NONE},
    {"i2c1",         SILAN_PADMUX_I2C1, NONE, NONE, NONE, NONE},
    {"uart2",        SILAN_PADMUX_UART2, NONE, NONE, NONE, NONE},
    {"iis_adc_sdi1_ch2", SILAN_PADMUX2_IISADC_FD1_CH2, NONE, NONE, NONE, NONE},
    {"iis_adc_sdi2_ch2", SILAN_PADMUX2_IISADC_FD2_CH2, NONE, NONE, NONE, NONE},
    {"sdmmc_det",    SILAN_PADMUX_SDMMCDET, NONE, NONE, NONE, NONE},
    {"sdmmc_data",   SILAN_PADMUX_SDMMCDAT, NONE, NONE, NONE, NONE},
    {"sdmmc",        SILAN_PADMUX_SDMMC, NONE, NONE, NONE, NONE},
    {"gmac",         SILAN_PADMUX_GMAC, NONE, NONE, NONE, NONE},
    {"spdif_o",      SILAN_PADMUX_SPDIF, NONE, NONE, NONE, NONE},
    {"iis_adc_sdi1_ch1", SILAN_PADMUX_IISADC_FD1, NONE, NONE, NONE, NONE},
    {"iis_adc_sdi2_ch1", SILAN_PADMUX_IISADC_FD2, NONE, NONE, NONE, NONE},
    {"sdio_data_ch2", SILAN_PADMUX2_SDIO_DAT_CH2|PAD2_SE, NONE, NONE, NONE, NONE},
    {"sdio_ch2",     SILAN_PADMUX2_SDIO_CH2|PAD2_SE, NONE, NONE, NONE, NONE},
    
};

int silan_padmux_ctrl(PADMUX pad, PAD_ON_OFF flag)
{
    unsigned int value = 0;

    if(pad < SILAN_PADMUX_START || pad >= SILAN_PADMUX_END)
        return -1;

    value = sl_readl(SILAN_PADMUX_CTRL);

    if(flag == PAD_OFF) 
        value &= ~(1 << (pad%32));
    else if(flag == PAD_ON)
        value |= (1 << (pad%32));
    else 
        return -1;

    sl_writel(value, SILAN_PADMUX_CTRL);

    return 0;
}
EXPORT_SYMBOL(silan_padmux_ctrl);

int silan_padmux2_ctrl(PADMUX pad, PAD_ON_OFF flag)
{
    unsigned int value = 0;

    if(pad < SILAN_PADMUX2_START || pad >= SILAN_PADMUX2_END)
        return -1;

    value = sl_readl(SILAN_PADMUX2_CTRL);

    if(flag == PAD_OFF) 
        value &= ~(1 << (pad%32));
    else if(flag == PAD_ON)
        value |= (1 << (pad%32));
    else 
        return -1;

    sl_writel(value, SILAN_PADMUX2_CTRL);

    return 0;
}
EXPORT_SYMBOL(silan_padmux2_ctrl);

static int silan_padmux_check(PADMUX pad)
{
    unsigned int value = 0;

    if(pad < SILAN_PADMUX_START || pad >= SILAN_PADMUX_END)
        return -1;

    value = sl_readl(SILAN_PADMUX_CTRL);

    value >>= pad;
    if((value & 0x00000001) == 0x1){
        return PAD_ON;
    }
    else{ 
        return PAD_OFF;
    }
}

static int silan_padmux2_check(PADMUX pad)
{
    unsigned int value = 0;

    if(pad < SILAN_PADMUX2_START || pad >= SILAN_PADMUX2_END)
        return -1;

    value = sl_readl(SILAN_PADMUX2_CTRL);

    value >>= pad;
    if((value & 0x00000001) == 0x1){
        return PAD_ON;
    }
    else{ 
        return PAD_OFF;
    }
}

static int check_lev(u8 pad)
{
    int res = 0;
    if(pad != NONE){
        if(pad < PAD2_SE){
            res = silan_padmux_check(pad); 
        }
        else{
            res = silan_padmux2_check(pad&0x3f); 
        }
    }
    return res;
}

static int silan_check_padmux(struct pad_mux_1 pad)
{
    int res = 0;
    int pad_num=1;
    //check self 
    res = check_lev(pad.lev0);
    if(res == PAD_OFF){
        printk("Please turn on the %s pad, self\n", pad.name);
        res = -1;
        goto end;
    }

    //check lev1
    res = check_lev(pad.lev1);
    if(res == PAD_ON){
        if(pad.lev1>=PAD2_SE){
            pad_num = 2;
            pad.lev1 &= 0x3f;
        }
        printk("Please turn off the pad%d:%d first, lev1\n",pad_num, pad.lev1);
        res = -1;
        goto end;
    }
    
    //check lev2
    res = check_lev(pad.lev2);
    if(res == PAD_ON){
        if(pad.lev2>=PAD2_SE){
            pad_num = 2;
            pad.lev2 &= 0x3f;
        }
        printk("Please turn off the pad%d:%d first, lev2\n",pad_num, pad.lev2);
        res = -1;
        goto end;
    }

    //check lev3
    res = check_lev(pad.lev3);
    if(res == PAD_ON){
        if(pad.lev3>=PAD2_SE){
            pad_num = 2;
            pad.lev3 &= 0x3f;
        }
        printk("Please turn off the pad%d:%d first, lev3\n",pad_num, pad.lev3);
        res = -1;
        goto end;
    }
    
    //check lev4
    res = check_lev(pad.lev4);
    if(res == PAD_ON){
        if(pad.lev4>=PAD2_SE){
            pad_num = 2;
            pad.lev4 &= 0x3f;
        }
        printk("Please turn off the pad%d:%d first\n",pad_num, pad.lev4);
        res = -1;
        goto end;
    }
end:
    return res; 
}

int silan_pad_check(char *name)
{
    int num = sizeof(padmuxall)/(sizeof(padmuxall[0])); 
    int i, flag = 1;
    int res = -1;
    for(i = 0; i < num && flag; i++){
        if(strcmp(name, padmuxall[i].name) == 0){
            flag = 0;
            break;
        }
    }
    if(flag == 0){
        res = silan_check_padmux(padmuxall[i]);
    }
    return res;
}
EXPORT_SYMBOL(silan_pad_check);

int silan_pad_enable(char *name)
{
    int num = sizeof(padmuxall)/(sizeof(padmuxall[0])); 
    int i, flag = 1;
    int res = -1;
    for(i = 0; i < num && flag; i++){
        if(strcmp(name, padmuxall[i].name) == 0){
            flag = 0;
            break;
        }
    }
    if(flag == 0){
        //check self 
        res = check_lev(padmuxall[i].lev0);
        if(res == PAD_OFF){
            if(padmuxall[i].lev0 < PAD2_SE)
                silan_padmux_ctrl(padmuxall[i].lev0, PAD_ON);
            else
                silan_padmux2_ctrl(padmuxall[i].lev0&0x3f, PAD_ON);
        }
        res = 0;
    }
    return res;
}
EXPORT_SYMBOL(silan_pad_enable);

int silan_pad_disable(char *name)
{
    int num = sizeof(padmuxall)/(sizeof(padmuxall[0])); 
    int i, flag = 1;
    int res = -1;
    for(i = 0; i < num && flag; i++){
        if(strcmp(name, padmuxall[i].name) == 0){
            flag = 0;
            break;
        }
    }
    if(flag == 0){
        //check self 
        res = check_lev(padmuxall[i].lev0);
        if(res == PAD_ON){
            if(padmuxall[i].lev0 < PAD2_SE)
                silan_padmux_ctrl(padmuxall[i].lev0, PAD_OFF);
            else
                silan_padmux2_ctrl(padmuxall[i].lev0&0x3f, PAD_OFF);
        }
        res = 0;
    }
    return res;
}
EXPORT_SYMBOL(silan_pad_disable);
