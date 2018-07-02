#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/spi/ads7846.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/partitions.h>

#define SPI_FLASH_OVERLAY

#define SZ_1K (1024)
#define SZ_64K (64*1024)
#define SZ_1M (1024*1024)

#ifdef SPI_FLASH_OVERLAY
static struct mtd_partition silan_partitions[] = {
    {
        .name   = "Bootloader",
        .offset = 0,
        .size   = 4*SZ_64K
    },
    {
        .name   = "kernel",
        .offset = 4*SZ_64K,
        .size   = 46*SZ_64K
    },
    {
        .name   = "rootfs",
        .offset = 46*SZ_64K + 4*SZ_64K,
        .size   = 46*SZ_64K
    },
	{
        .name   = "rootfs_data",
        .offset = 46*SZ_64K + 4*SZ_64K + 46*SZ_64K,
        .size   = 32*SZ_64K
    },
};
#endif

static struct flash_platform_data silan_spiflash_info=
{
	.name		 = "w25p64",
#ifdef SPI_FLASH_OVERLAY	
	.parts	 = silan_partitions,
	.nr_parts	 = 4,
#else
	.parts		 = NULL,
	.nr_parts	 = 0,
#endif
	.type		 = "w25x64",
};

#ifdef SPI_FLASH_OVERLAY
static struct mtd_partition silan_partitions_ex[] = {
    {
        .name   = "flash_data",
        .offset = 0,
        .size   = 64*SZ_64K
    },
};
#endif

static struct flash_platform_data silan_spiflash_info_ex = {
	.name		 = "w25p64",
#ifdef SPI_FLASH_OVERLAY
	.parts	 = silan_partitions_ex,
	.nr_parts	 = 1,
#else
	.parts		 = NULL,
	.nr_parts	 = 0,
#endif
	.type		 = "w25x64",
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
#if 0
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
#else
	[1] = {
		.modalias        = "w25q32",
		.platform_data   = &silan_spiflash_info_ex,
		.mode            = SPI_MODE_0,
		.irq             = 0,
		.max_speed_hz    = 4*1000*1000,
		.bus_num         = 2,
		.chip_select     = 0,
		},
#endif
	[2] = {
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


