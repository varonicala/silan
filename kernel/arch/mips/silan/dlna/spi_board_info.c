#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/spi/ads7846.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>

#define SILAN_TS_GPIO	(4*32+14)

static struct flash_platform_data silan_spiflash_info=
{
	.name		 = "w25q32",
	.parts		 = NULL,
	.nr_parts	 = 0,
	.type		 = "w25q32",
};

static const struct ads7846_platform_data silan_ts_info = {
	.model            = 7843,
	.vref_delay_usecs = 100,
	.keep_vref_on	  = 1,
	.pressure_max     = 1024,
	.debounce_max     = 0,
	.debounce_tol     = 3,
	.debounce_rep     = 1,
	.gpio_pendown     = SILAN_TS_GPIO,
};

struct spi_board_info silan_spi_board_info[]  __initdata = {
	[0] = { 
		.modalias		 = "w25q32",
		.platform_data	 = &silan_spiflash_info,
		.mode			 = SPI_MODE_0,
		.irq			 = 0,
		.max_speed_hz 	 = 4*1000*1000,
		.bus_num		 = 1,
		.chip_select	 = 0,
		},
	[1] = {
		//.modalias		 = "spidev",
		.modalias		 = "lcd-spi",
		.platform_data   = NULL,
		.mode			 = SPI_MODE_0,
		.irq			 = 0,
		.max_speed_hz    = 4*1000*1000,
		//.bus_num 		 = 1,
		.bus_num 		 = 2,
		//.chip_select 	 = 1,
		.chip_select 	 = 0,
		},
	[2]={
		.modalias        = "ads7846",
		.bus_num         = 1,
		.max_speed_hz    = 2600000, 
		.irq             = (96+SILAN_TS_GPIO), 
		.platform_data   = &silan_ts_info,
		.controller_data = NULL,
		.chip_select 	 = 2,
		},
	[3] = {
		.modalias		 = "silan-wmcodec",
		.platform_data   = NULL,
		.mode			 = SPI_MODE_0,
		.irq			 = 0,
		.max_speed_hz    = 4*1000*1000,
		.bus_num 		 = 1,
		.chip_select 	 = 3,
		.bits_per_word	 = 16,
		},
};
EXPORT_SYMBOL(silan_spi_board_info);

static int __init silan_spi_board_init(void)
{
	int ret;
	ret =spi_register_board_info(silan_spi_board_info,ARRAY_SIZE(silan_spi_board_info));
	return ret;
}

static void __exit silan_spi_board_exit(void)
{
	
}
module_init(silan_spi_board_init);
module_exit(silan_spi_board_exit);


