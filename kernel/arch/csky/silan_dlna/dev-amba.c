#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_def.h>

#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/amba/pl08x.h>

static struct pl08x_channel_data pl080_slave_data0[] = {
	{//0
		.bus_id = "spdif",
		.min_signal = 14,
		.max_signal = 14,
		.muxval = 0x00,
	},
	{//8
		.bus_id = "iis adc",
		.min_signal = 11,
		.max_signal = 11,
		.muxval = 0x00,
	},
	{//1
		.bus_id = "iis dac",
		.min_signal = 9,
		.max_signal = 9,
		.muxval = 0x00,
	},
	{//2
		.bus_id = "uart1-rx",
		.min_signal = 0,
		.max_signal = 0,
		.muxval = 0x00,
	},
	{//4
		.bus_id = "uart2-rx",
		.min_signal = 3,
		.max_signal = 3,
		.muxval = 0x00,
	},
	{//5
		.bus_id = "uart2-tx",
		.min_signal = 4,
		.max_signal = 4,
		.muxval = 0x00,
	},
	{//3
		.bus_id = "uart1-tx",
		.min_signal = 1,
		.max_signal = 1,
		.muxval = 0x00,
	},
	{//6
		.bus_id = "i2c-rx",
		.min_signal = 2,
		.max_signal = 2,
		.muxval = 0x00,
	},
	{//7
		.bus_id = "i2c-tx",
		.min_signal = 7,
		.max_signal = 7,
		.muxval = 0x00,
	},
	{//9
		.bus_id = "spi-tx",
		.min_signal = 12,
		.max_signal = 12,
		.muxval = 0x00,
	},
	{//10
		.bus_id = "spi-rx",
		.min_signal = 13,
		.max_signal = 13,
		.muxval = 0x00,
	},
	{//11
		.bus_id = "spdif-rx",
		.min_signal = 15,
		.max_signal = 15,
		.muxval = 0x00,
	},

};

static int pl080_get_signal(struct pl08x_dma_chan *ch)
{
	struct pl08x_channel_data *cd = ch->cd;
	return cd->min_signal;
}

static void pl080_put_signal(struct pl08x_dma_chan *ch)
{
}

static struct pl08x_platform_data pl080_plat_data0 = {
	.slave_channels = pl080_slave_data0,
	.num_slave_channels = ARRAY_SIZE(pl080_slave_data0),
	.memcpy_channel = {
		.bus_id = "memcpy0",
	},
	.get_signal = pl080_get_signal,
	.put_signal = pl080_put_signal,
	.lli_buses = PL08X_AHB1,
	.mem_buses = PL08X_AHB1,
};

static struct amba_device silan_dma_device0 = {
	.dev = {
		.init_name = DMAC0_NAME,
		.platform_data = &pl080_plat_data0,
	},
	.res = {
		.start	= SILAN_DMAC0_PHY_BASE,
		.end	= SILAN_DMAC0_PHY_BASE + SILAN_DMAC0_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	.irq = { PIC_IRQ_DMAC0, NO_IRQ },
	.periphid = 0x00141080,
};

static struct pl011_dma_params uart1_dma_rx = {
	.dma_name = DMAC0_NAME,
	.dma_channel = 3,
};

static struct pl011_dma_params uart1_dma_tx = {
	.dma_name = DMAC0_NAME,
	.dma_channel = 6,
};

static struct amba_pl011_data silan_uart1_plat = {
	.dma_filter = pl011_filter,
	.dma_rx_param = &uart1_dma_rx,
	.dma_tx_param = &uart1_dma_tx,
};

static struct amba_device silan_uart1_device = {
	.dev = {
		.init_name = "silan-uart-dma:1",
		.platform_data = &silan_uart1_plat,
	},
	.res = {
		.start	= SILAN_UART1_PHY_BASE,
		.end	= SILAN_UART1_PHY_BASE + SILAN_UART1_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	.irq = { PIC_IRQ_UART1, NO_IRQ },
	.periphid = 0x00041011,
};

static struct pl011_dma_params uart2_dma_rx = {
	.dma_name = DMAC0_NAME,
	.dma_channel = 4,
};

static struct pl011_dma_params uart2_dma_tx = {
	.dma_name = DMAC0_NAME,
	.dma_channel = 5,
};

static struct amba_pl011_data silan_uart2_plat = {
	.dma_filter = pl011_filter,
	.dma_rx_param = &uart2_dma_rx,
	.dma_tx_param = &uart2_dma_tx,
};

static struct amba_device silan_uart2_device = {
	.dev = {
		.init_name = "silan-uart-dma:2",
		.platform_data = &silan_uart2_plat,
	},
	.res = {
		.start	= SILAN_UART2_PHY_BASE,
		.end	= SILAN_UART2_PHY_BASE + SILAN_UART2_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	.irq = { PIC_IRQ_UART2, NO_IRQ },
	.periphid = 0x00041011,
};

struct amba_device *silan_amba_devs[] __initdata = {
	&silan_dma_device0,
	&silan_uart1_device,
	&silan_uart2_device,
};
EXPORT_SYMBOL(silan_amba_devs);
