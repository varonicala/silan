#include <linux/module.h>
#include <silan_resources.h>
#include <silan_generic.h>
#include <silan_padmux.h>
#include <silan_gpio.h>

#define SILAN_PADMUX_CTRL            (SILAN_CR_BASE + 0x24)

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

