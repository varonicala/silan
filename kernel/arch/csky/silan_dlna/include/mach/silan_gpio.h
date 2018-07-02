#ifndef __SILAN_GPIO_H__
#define __SILAN_GPIO_H__

#include <linux/types.h>
#include <silan_generic.h>

#define GPIO_IOC_MAGIC    0xF5
#define GPIO_IOCRESET     _IO(GPIO_IOC_MAGIC, 0)
#define GPIO_IOCOUT_H     _IOW(GPIO_IOC_MAGIC, 1, unsigned long)
#define GPIO_IOCOUT_L     _IOW(GPIO_IOC_MAGIC, 2, unsigned long)

#define GPIO_CTRL         (SILAN_GPIO2_BASE|0x400)
#define GPIO_DATA         (SILAN_GPIO2_BASE|0x000)

#define OUT_GPIO(n)       sl_writel(sl_readl(GPIO_CTRL) | (1UL<<(n)), GPIO_CTRL)
#define IN_GPIO(n)        sl_writel(sl_readl(GPIO_CTRL) & ~(1UL<<(n)), GPIO_CTRL)

#define SET_GPIO(n)       sl_writel(sl_readl(GPIO_DATA) | (1UL<<(n)), GPIO_DATA)
#define CLR_GPIO(n)       sl_writel(sl_readl(GPIO_DATA) & ~(1UL<<(n)), GPIO_DATA)
#define GET_GPIO(n)       (sl_readl(GPIO_DATA) & (1UL<<(n)))

#define SILAN_GPIO1_BASENUM    0
#define SILAN_GPIO1_GPIONUM    32
#define SILAN_GPIO1_IRQBASE    96

#define SILAN_GPIO2_BASENUM    (SILAN_GPIO1_BASENUM + SILAN_GPIO1_GPIONUM)
#define SILAN_GPIO2_GPIONUM    32
#define SILAN_GPIO2_IRQBASE    (SILAN_GPIO1_IRQBASE+SILAN_GPIO1_GPIONUM)

/* platform data for the PL061 GPIO driver */

struct pl061_platform_data 
{
    /* number of the first GPIO */
    unsigned    gpio_base;

    /* number of the first IRQ.
     * If the IRQ functionality in not desired this must be set to
     * (unsigned) -1.
     */
    unsigned    irq_base;
    unsigned    gpio_irqbase;
    unsigned    gpio_num;
    u32         directions;     /* startup directions, 1: out, 0: in */
    u32         values;         /* startup values */
    u32          padmux_gpio;
};

#endif
