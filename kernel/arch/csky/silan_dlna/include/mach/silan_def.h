#ifndef __SILAN_DEF_H__
#define __SILAN_DEF_H__

#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

ulong get_silan_pllclk(void);
u32 prom_phy_mem_malloc(u32 size, int id);
u32 prom_pmem_start_addr(void);
void mips_reboot_setup(void);

#define SILAN_PROM_PHY_MEM_SIZE 0x8000000
#define SILAN_PMEM_SIZE			0x4000000
/* Total physical memory */
#define SILAN_PHY_MEM_SIZE			0x40000000
/* Start address of the register space */
#define SILAN_REG_SPACE_START		0x1FA00000
/* Size of register space */
#define SILAN_REG_SPACE_SIZE		0x600000

extern struct amba_device *silan_amba_devs[3];

extern struct platform_device silan_soc_dma;
extern struct platform_device silan_spdif_device;
extern struct platform_device silan_spdcodec_device; 
extern struct platform_device silan_spdif_in_device;
extern struct platform_device silan_iis_device;
//extern struct platform_device silan_hdmi_audio_device;
//extern struct platform_device silan_soc_hdmi_dma;
//extern struct platform_device silan_iis_hdmi_device;

extern struct spi_board_info silan_spi_board_info[2];
#endif
