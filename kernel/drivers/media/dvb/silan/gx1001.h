#ifndef __GX1001_H__
#define __GX1001_H__

/*
 * Abbreviation
    GX        --    GUOXIN 
    IF        --    intermediate frequency
    RF        --      radiate frequency
    SNR        --    signal to noise ratio
    OSC        --    oscillate
    SPEC    --    spectrum
    FREQ    --    frequency
*/

#define SUCCESS   1
#define FAILURE  -1    //don't set 0 

#define NEWONE    1
#define OLDONE   -1

#define ENABLE    1
#define DISABLE   0

/*-- Register Address Defination begin ---------------*/

#define GX_CHIP_ID                 0x00
#define GX_MAN_PARA                0x10
#define GX_INT_PO1_SEL             0x11
#define GX_SYSOK_PO2_SEL           0x12
#define GX_STATE_IND               0x13
#define GX_TST_SEL                 0x14
#define GX_I2C_RST                 0x15
#define GX_MAN_RST                 0x16
#define GX_BIST                    0x18
#define GX_MODE_AGC                0x20
#define GX_AGC_PARA                0x21
#define GX_AGC2_THRES              0x22
#define GX_AGC12_RATIO             0x23
#define GX_AGC_STD                 0x24
#define GX_SCAN_TIME               0x25
#define GX_DCO_CENTER_H            0x26
#define GX_DCO_CENTER_L            0x27
#define GX_BBC_TST_SEL             0x28
#define GX_AGC_ERR_MEAN            0x2B
#define GX_FREQ_OFFSET_H           0x2C
#define GX_FREQ_OFFSET_L           0x2D
#define GX_AGC1_CTRL               0x2E
#define GX_AGC2_CTRL               0x2F
#define GX_FSAMPLE_H               0x40
#define GX_FSAMPLE_M               0x41
#define GX_FSAMPLE_L               0x42
#define GX_SYMB_RATE_H             0x43
#define GX_SYMB_RATE_M             0x44
#define GX_SYMB_RATE_L             0x45
#define GX_TIM_LOOP_CTRL_L         0x46
#define GX_TIM_LOOP_CTRL_H         0x47
#define GX_TIM_LOOP_BW             0x48
#define GX_EQU_CTRL                0x50
#define GX_SUM_ERR_POW_L           0x51
#define GX_SUM_ERR_POW_H           0x52
#define GX_EQU_BYPASS              0x53
#define GX_EQU_TST_SEL             0x54
#define GX_EQU_COEF_L              0x55
#define GX_EQU_COEF_M              0x56
#define GX_EQU_COEF_H              0x57
#define GX_EQU_IND                 0x58
#define GX_RSD_CONFIG              0x80
#define GX_ERR_SUM_1               0x81
#define GX_ERR_SUM_2               0x82
#define GX_ERR_SUM_3               0x83
#define GX_ERR_SUM_4               0x84
#define GX_RSD_DEFAULT             0x85
#define GX_OUT_FORMAT              0x90

/*---GX1001B New Address Defination Start--*/
#define GX_CHIP_IDD                0x01
#define GX_CHIP_VERSION            0x02
#define GX_PLL_M                   0x1A    
#define GX_PLL_N                   0x1B     
#define GX_PLL_L                   0x1B    
#define GX_MODE_SCAN_ENA           0x29    
#define GX_FM_CANCEL_CTRL          0x30    
#define GX_SF_DECT_FM              0x30
#define    GX_FM_CANCEL_ON         0x30
#define GX_FM_SUB_ENA              0x30    
#define GX_FM_ENA_FM               0x30
#define GX_FM_UNLOCK_TIME          0x30
#define GX_FM_SUBENA_TIME          0x30    
#define GX_FM_CANCEL_LMT           0x31
#define GX_FM_UNLOCK_LMT           0x31
#define GX_FM_SUBENA_LMT           0x31        
#define GX_SF_CANCEL_CTRL          0x32        
#define GX_SF_DECT_SF              0x32    
#define GX_SF_BUSY                 0x32    
#define GX_SF_LOCKED               0x32        
#define GX_FM_ENA_SF               0x32        
#define GX_SF_BW_SEL               0x32        
#define GX_SCAN_TOUT_SET           0x32    
#define GX_SF_LMT_FM               0x33        
#define GX_SF_LMT_H                0x34        
#define GX_SF_LMT_L                0x35        
#define GX_AGC_SET                 0x39        
#define    GX_AGC_HOLD             0x39    
#define GX_RF_SET_EN               0x39    
#define GX_IF_SET_EN               0x39    
#define GX_RF_SET_DAT              0x3A        
#define GX_IF_SET_DAT              0x3B        
#define GX_RF_MIN                  0x3C        
#define    GX_RF_MAX               0x3D        
#define    GX_IF_MIN               0x3E        
#define GX_IF_MAX                  0x3F        
#define GX_SF_FREQ_OUT_H           0xA0        
#define GX_SF_FREQ_OUT_L           0xA1        
#define GX_FM_FREQ_OUT_H           0xA2        
#define GX_FM_FREQ_OUT_L           0xA3      
#define GX_AUTO_THRESH             0xA6        
#define GX_THRESH_AUTO             0xA6    
#define GX_THRESH_STEP             0xA6
#define GX_THRESH_OUT              0xA7        
#define GX_TIM_JIT_BOUND           0x49        
#define GX_TIM_SCAN_SPEED          0x4A        
#define GX_TIM_SCAN_ENA            0x4B        
#define GX_AGC_LMT_2DELTA          0x4C        
#define GX_AGC_LMT_3DELTA          0x4D        
#define GX_DIGITAL_AGC_ON          0x4E        
#define GX_agc_amp_ditter_on       0x4E
#define GX_SPECTRUM_INV            0x60        
#define GX_PIN_SEL_1               0x91        
#define GX_PIN_SEL_2               0x92
#define GX_PIN_SEL_3               0x93
#define GX_PIN_SEL_4               0x94
#define GX_PIN_SEL_5               0x95
#define GX_PIN_SEL_6               0x96

/*--GX1001B New Address Defination End-----*/

/*-- Register Address Defination end ---------------*/

u8  GX_CHIP_ADDRESS                = 0x18;    /*GX1001 chip address*/

unsigned long  GX_OSCILLATE_FREQ   = 28636;    //(oscillate frequency) ( Unit: KHz ) 
unsigned long  GX_IF_FREQUENCY     = 36125;    //(tuner carrier center frequency) ( Unit: KHz ) 

unsigned int  GX_TS_OUTPUT_MODE    = 1;        // 1: Parallel output,  0: Serial output
int  GX_PLL_M_VALUE                = 0x0b; // This parameter is effective only for GX1001B
int  GX_PLL_N_VALUE                = 0x00; // This parameter is effective only for GX1001B
int  GX_PLL_L_VALUE                = 0x05; // This parameter is effective only for GX1001B
int  GX_RF_AGC_MIN_VALUE           = 0x00; // This parameter is effective only for GX1001B
int  GX_RF_AGC_MAX_VALUE           = 0xff; // This parameter is effective only for GX1001B
int  GX_IF_AGC_MIN_VALUE           = 0x00; // This parameter is effective only for GX1001B
int  GX_IF_AGC_MAX_VALUE           = 0xff; // This parameter is effective only for GX1001B
int sfenable = 0;
int fmenable = 0;

#endif
