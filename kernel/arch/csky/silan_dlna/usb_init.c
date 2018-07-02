#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <asm/io.h>

#include <silan_resources.h>
#include <silan_generic.h>

void dwc_usb_board_init(void)
{
    //printk("###### %s ######\n", __func__);
    u32 val = readl(SILAN_CR_BASE+0x3c);
    val &= (~(1<<18)) & (~(1<<16));
    writel(val, SILAN_CR_BASE+0x3c);
    udelay(500);
    val |= (1<<18) | (1<<16);
    writel(val, SILAN_CR_BASE+0x3c);
    udelay(1000);

}
EXPORT_SYMBOL(dwc_usb_board_init);

