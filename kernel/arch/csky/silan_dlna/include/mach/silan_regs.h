/*
 * Register address definition
 */
#ifndef _SILAN_REGS_H
#define _SILAN_REGS_H

/*
 * CTR
 */
#define SILAN_SYS_REG0				(SILAN_CR_BASE)
#define SILAN_SYS_REG1				(SILAN_CR_BASE + 0x04)
#define SILAN_SYS_REG2				(SILAN_CR_BASE + 0x08)
#define SILAN_SYS_REG3				(SILAN_CR_BASE + 0x0C)
#define SILAN_SYS_REG4				(SILAN_CR_BASE + 0x10)
#define SILAN_SYS_REG5				(SILAN_CR_BASE + 0x14)
#define SILAN_SYS_REG6				(SILAN_CR_BASE + 0x18)
#define SILAN_SYS_REG7				(SILAN_CR_BASE + 0x1c)
#define SILAN_SYS_REG8				(SILAN_CR_BASE + 0x20)
#define SILAN_SYS_REG9				(SILAN_CR_BASE + 0x24)
#define SILAN_SYS_REG10				(SILAN_CR_BASE + 0x28)
#define SILAN_SYS_REG11				(SILAN_CR_BASE + 0x2c)
#define SILAN_SYS_REG12				(SILAN_CR_BASE + 0x30)
#define SILAN_SYS_REG13				(SILAN_CR_BASE + 0x34)
#define SILAN_SYS_REG14				(SILAN_CR_BASE + 0x38)
#define SILAN_SYS_REG15				(SILAN_CR_BASE + 0x3c)
#define SILAN_SYS_REG30				(SILAN_CR_BASE + 0x78)
#define SILAN_SYS_REG32				(SILAN_CR_BASE + 0x80)
#define SILAN_SYS_REG33				(SILAN_CR_BASE + 0x84)
#define SILAN_GPIOPAD_BASE			(SILAN_CR_BASE + 0x90)

/*
 * LSP Misc
 */
#define SILAN_LSP_CTRL				SILAN_LSPMISC_BASE
/* Bit definition */
#define LSP_CTRL_TIMCLK_EN0			(1 << 1)
#define LSP_CTRL_TIMCLK_EN1			(1 << 2)

/*
 * PMU
 */
/* PMU configure register */
#define SILAN_PMU_CFG				SILAN_PMU_BASE
/* PLL configure register low */
#define SILAN_PMU_PLL_CFG_L			(SILAN_PMU_BASE + 0x04)
/* PLL configure register high */
#define SILAN_PMU_PLL_CFG_H			(SILAN_PMU_BASE + 0x08)
/* PMU external interrupt mask register */
#define SILAN_PMU_INT_MASK_CFG		(SILAN_PMU_BASE + 0x0c)
/* PMU external interrupt polarity register */
#define SILAN_PMU_INT_POLAR_CFG		(SILAN_PMU_BASE + 0x10)
/* PMU external interrupt configuration register */
#define SILAN_PMU_INT_CFG			(SILAN_PMU_BASE + 0x14) 
/* PMU interrupt status register */
#define SILAN_PMU_INT_STATUS		(SILAN_PMU_BASE + 0x18) 
/* PMU external interrupt level count register */
#define SILAN_PMU_INT_LEVEL_CNT_CFG	(SILAN_PMU_BASE + 0x1c) 
/* PMU status register */
#define SILAN_PMU_STATUS			(SILAN_PMU_BASE + 0x20)
/* PLL status register low */
#define SILAN_PMU_PLL_STATUS_L		(SILAN_PMU_BASE + 0x24)
/* PLL status register high */
#define SILAN_PMU_PLL_STATUS_H		(SILAN_PMU_BASE + 0x28)
/* PMU state change wait count register */
#define SILAN_PMU_WAIT_CNT			(SILAN_PMU_BASE + 0x2c)

/* 
 * SEC_AR
 */
#define SILAN_SEC_AR_STATUS			(SILAN_SEC_AR_BASE + 0x40)

#endif /* _SILAN_REGS_H */

