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

#include <silan_resources.h>
#include <silan_generic.h>

void dwc_usb_board_init(void)
{
#if defined(CONFIG_MIPS_SILAN_FPGA)
#else /* !FPGA */

	//struct clk *clk;
	//open clk
	//clk = clk_get(NULL, "usbhost");
	//clk_enable(clk);

	//clk = clk_get(NULL, "usbdevice");
	//clk_enable(clk);

	//reset module
	//silan_module_rst(SILAN_SR_USB);
	//msleep(80);
#endif /* FPGA */
	u32 val = 0;
	val = sl_readl(0xba00002c);
	val |= ((1<<16) | (1<<17) | (1<<22));
	sl_writel(val, 0xba00002c);

}
EXPORT_SYMBOL(dwc_usb_board_init);
