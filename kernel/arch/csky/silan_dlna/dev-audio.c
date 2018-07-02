#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <silan_resources.h>
#include <silan_irq.h>

static u64 silan_audio_dmamask = DMA_BIT_MASK(32);

struct platform_device silan_soc_dma = {
	.name = "silan-pcm",
	.id = -1,
	.dev = {
		.dma_mask = &silan_audio_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
EXPORT_SYMBOL(silan_soc_dma);

static struct resource silan_spdif_resource[] = {
	[0] = {
		.start	= SILAN_SPDIF_PHY_BASE,
		.end	= SILAN_SPDIF_PHY_BASE + SILAN_SPDIF_SIZE,
		.flags	= IORESOURCE_MEM, 
	},
	[1] = {
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_DMA,
	},
};

struct platform_device silan_spdif_device = {
	.name = "silan-spdif",
	.id = -1,
	.num_resources = ARRAY_SIZE(silan_spdif_resource),
	.resource = silan_spdif_resource,
	.dev = {
		.dma_mask = &silan_audio_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
EXPORT_SYMBOL(silan_spdif_device);

struct platform_device silan_spdcodec_device = {
	.name	        = "silan-spdcodec",
	.id		        = -1,
};
EXPORT_SYMBOL(silan_spdcodec_device);

static struct resource silan_spdif_in_resource[] = {
	[0] = {
		.start	= SILAN_SPDIF_IN_PHY_BASE,
		.end	= SILAN_SPDIF_IN_PHY_BASE + SILAN_SPDIF_IN_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= 11,
		.end	= 11,
		.flags	= IORESOURCE_DMA,
	},
};

struct platform_device silan_spdif_in_device = {
	.name = "silan-spdif-in",
	.id = -1,
	.num_resources = ARRAY_SIZE(silan_spdif_in_resource),
	.resource = silan_spdif_in_resource,
	.dev = {
		.dma_mask = &silan_audio_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
EXPORT_SYMBOL(silan_spdif_in_device);

static struct resource silan_iis_resource[] = {
	{
		.start	= SILAN_IISDAC_PHY_BASE,
		.end	= SILAN_IISDAC_PHY_BASE + SILAN_IISDAC_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SILAN_IISADC_PHY_BASE,
		.end	= SILAN_IISADC_PHY_BASE + SILAN_IISADC_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= 1,
		.end	= 1,
		.flags	= IORESOURCE_DMA,
	},
	{
		.start	= 8,
		.end	= 8,
		.flags	= IORESOURCE_DMA,
	},
};

struct platform_device silan_iis_device = {
	.name = "silan-i2s-hdmi",
	.id = -1,
	.num_resources = ARRAY_SIZE(silan_iis_resource),
	.resource = silan_iis_resource,
	.dev = {
		.dma_mask = &silan_audio_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
EXPORT_SYMBOL(silan_iis_device);

