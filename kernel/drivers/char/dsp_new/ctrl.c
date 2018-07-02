#include <linux/module.h>
#include <linux/types.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <asm/io.h>
#include "silan_memory.h"
#include "common.h"
#include "ctrl.h"

#define DSP_CTL_REG                 0
#define DSP_STUS_REG                4
#define DSP_AREA0_REG               8
#define DSP_AREA0_LEN_REG           12
#define DSP_AREA1_REG               16
#define DSP_AREA1_LEN_REG           20

#define DSP_CONFIG_STALL            (0x1<<1)
#define DSP_CONFIG_VECTOR           (0x1<<2)
#define DSP_CONFIG_SW_INT           (0x1<<8)

static int dsp_base;

static inline unsigned int dspreg_read(unsigned int reg)
{
    return __raw_readl((void __iomem *)(dsp_base + reg));
}

static inline void dspreg_write(unsigned int val, unsigned int reg)
{
    __raw_writel(val, (void __iomem *)(dsp_base + reg));
}

int dsp_set_stall(int enable)
{
    unsigned int regval;

    regval = dspreg_read(DSP_CTL_REG);

    if (enable)
        regval |= DSP_CONFIG_STALL;
    else
        regval &= ~DSP_CONFIG_STALL;

    dspreg_write(regval, DSP_CTL_REG);

    return 0;
}

static int dsp_set_area0_attr(int offset, int size)
{
    dspreg_write(offset, DSP_AREA0_REG);
    dspreg_write(size, DSP_AREA0_LEN_REG);

    return 0;
}

static int dsp_set_area1_attr(int offset, int size)
{
    dspreg_write(offset, DSP_AREA1_REG);
    dspreg_write(size, DSP_AREA1_LEN_REG);

    return 0;
}

int dsp_ctrl_init(struct dsp_dev_info *dsp_info)
{
    dsp_base = dsp_info->dsp_base;

    dsp_info->area0_dma = (dma_addr_t)prom_phy_mem_malloc(DSP_FIRMWARE_SIZE, 0);
    if (dsp_info->area0_dma == 0)
    {
        LOGE("dsp_area_buf_init failed, area0_dma malloc err\n");
        return DSP_ERROR_DEVICE;
    }

    dsp_info->area0_cpu = (int)ioremap_nocache(dsp_info->area0_dma, DSP_FIRMWARE_SIZE);
    if (dsp_info->area0_cpu == 0)
    {
        LOGE("dsp_area_buf_init failed, area0_cpu malloc err\n");
        return DSP_ERROR_DEVICE;
    }
    dsp_info->area0_len = DSP_FIRMWARE_SIZE;
    dsp_set_area0_attr(dsp_info->area0_dma,dsp_info->area0_len);

    dsp_info->area1_dma = (dma_addr_t)prom_phy_mem_malloc(DSP_RING_BUF_SIZE, 0);
    if (dsp_info->area1_dma == 0)
    {
        LOGE("dsp_area_buf_init failed, area1_dma malloc err\n");
        return DSP_ERROR_DEVICE;
    }

    dsp_info->area1_cpu = (int)ioremap_nocache(dsp_info->area1_dma, DSP_RING_BUF_SIZE);
    if (dsp_info->area1_cpu == 0)
    {
        LOGE("dsp_area_buf_init failed, area1_cpu malloc err\n");
        return DSP_ERROR_DEVICE;
    }
    dsp_info->area1_len = DSP_RING_BUF_SIZE;
    dsp_set_area1_attr(dsp_info->area1_dma, dsp_info->area1_len);

    return DSP_ERROR_NONE;
}

