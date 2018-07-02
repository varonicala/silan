#ifndef _SILAN_NEW_RTC_H_
#define _SILAN_NEW_RTC_H_

/*
 * Registers offset
 */

#define PWM_PSC         0x00
#define PWM_0D          0x04
#define PWM_1D          0x08
#define PWM_2D          0x0C
#define PWM_3D          0x10
#define PWM_4D          0x14
#define PWM_5D          0x18
#define PWM_6D          0x1C
#define PWM_7D          0x20
#define PWM_P01         0x24
#define PWM_P23         0x28
#define PWM_P45         0x2C
#define PWM_P67         0x30
#define PWM_CON         0x34
#define PWM_READSEL0    0
#define PWM_READSEL2    1
#define PWM_READSEL4    2
#define PWM_READSEL6    3
#define PWM_0EN         4
#define PWM_1EN         5
#define PWM_2EN         6
#define PWM_3EN         7
#define PWM_4EN         8
#define PWM_5EN         9
#define PWM_6EN         10
#define PWM_7EN         11

#if 1//CONFIG_CKCPU_MMU
#define PWM_BASE SILAN_PWM_BASE
#else
#define PWM_BASE SILAN_PWM_PHY_BASE
#endif

#endif
