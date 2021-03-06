#ifndef __SL_PADMUX_H
#define __SL_PADMUX_H

typedef enum {
    SILAN_PADMUX_START = 0,
    
    SILAN_PADMUX_UART1 = SILAN_PADMUX_START,
    SILAN_PADMUX_UART2,
    SILAN_PADMUX_IISDAC_MCLK,
    SILAN_PADMUX_UART4_1,
    SILAN_PADMUX_UART4_2,
    SILAN_PADMUX_SPI = 5,
    SILAN_PADMUX_SPI_EX,
    SILAN_PADMUX_SDMMC,
    SILAN_PADMUX_SDIO,
    SILAN_PADMUX_SDMMCDET,
    SILAN_PADMUX_SDIODET,
    SILAN_PADMUX_SF_DAT23,
    SILAN_PADMUX_IISADC = 12,
    SILAN_PADMUX_UART3_1 = 13,
    SILAN_PADMUX_UART3_2 = 14,
    SILAN_PADMUX_IISDAC,
    SILAN_PADMUX_IISDAC_FD0,
    SILAN_PADMUX_IISDAC_FD1,
    SILAN_PADMUX_IISDAC_FD2,
    SILAN_PADMUX_IISADC_FD0,
    SILAN_PADMUX_IISADC_FD1,
    SILAN_PADMUX_IISADC_FD2,
    SILAN_PADMUX_SPDIF,
    SILAN_PADMUX_SDMMCDAT,
    SILAN_PADMUX_SDIODAT,
    SILAN_PADMUX_I2C2,
    SILAN_PADMUX_GMAC,
    SILAN_PADMUX_I2C1,
    SILAN_PADMUX_SPDIF_IN0,
    SILAN_PADMUX_SPDIF_IN1,
    SILAN_PADMUX_SPDIF_IN2,
    SILAN_PADMUX_SPDIF_IN3,

    SILAN_PADMUX_END
}PADMUX;

typedef enum {
    SILAN_PADMUX2_START = 0,
    
    SILAN_PADMUX2_PWM0 = SILAN_PADMUX2_START,
    SILAN_PADMUX2_PWM1,
    SILAN_PADMUX2_PWM2,
    SILAN_PADMUX2_PWM3,
    SILAN_PADMUX2_PWM4,
    SILAN_PADMUX2_PWM5,
    SILAN_PADMUX2_PWM6,
    SILAN_PADMUX2_PWM7,
    SILAN_PADMUX2_SPDIF_IN0_CH2,
    SILAN_PADMUX2_SPDIF_IN1_CH2,
    SILAN_PADMUX2_SPDIF_IN2_CH2,
    SILAN_PADMUX2_SPDIF_IN3_CH2 = 11,
    SILAN_PADMUX2_IISADC_FD1_CH2,
    SILAN_PADMUX2_IISADC_FD2_CH2,
    SILAN_PADMUX2_UART1_CH2,   
    SILAN_PADMUX2_IISADC_MODE = 16,
    SILAN_PADMUX2_IISDAC_MODE,
    SILAN_PADMUX2_CODEC_ADC_SEL,
    SILAN_PADMUX2_CODEC_DAC_SEL,
    SILAN_PADMUX2_SPDIF_IN0_SEL,
    SILAN_PADMUX2_SPDIF_IN1_SEL,
    SILAN_PADMUX2_PWM2_CH2 = 24,
    SILAN_PADMUX2_PWM3_CH2,
    SILAN_PADMUX2_PWM1_CH2,
    SILAN_PADMUX2_SDIO_CH2,
    SILAN_PADMUX2_SDIO_DAT_CH2,

    SILAN_PADMUX2_END
}PADMUX2;

typedef enum {
    PAD_OFF,
    PAD_ON,
}PAD_ON_OFF;

int silan_padmux_ctrl(PADMUX pad, PAD_ON_OFF flag);
int silan_padmux2_ctrl(PADMUX pad, PAD_ON_OFF flag);
int silan_pad_check(char *name);
int silan_pad_enable(char *name);
int silan_pad_disable(char *name);

#endif
