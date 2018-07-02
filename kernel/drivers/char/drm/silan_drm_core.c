#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_def.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/miscdevice.h>

#include "silan_drm_core.h"
#include "silan_drm.h"

#ifdef CONFIG_SILAN_DLNA
#include <asm/io.h>
#endif

static void buf_set(struct silan_drm *drm, u32 buf_num)
{
	u32 inpmode, outmode;
	void __iomem *base = (void __iomem *)drm->buf_ctrl_base;
	
	if(buf_num == 0){
		inpmode = (3 << 16) | SECDMX_BUF_DMA_MODE  | SECDMX_BUF_DMA_CHANNEL;
		outmode = (3 << 16) | SECDMX_BUF_FIFO_MODE | SECDMX_BUF_DRM_CIPHER_CHANNEL;

		writel(SECDMX_BUF_RESET, base + BUF0_CTRL);
		writel(SECDMX_BUF_UNRESET, base + BUF0_CTRL);
		
		writel(inpmode, base + BUF0_INP_CFG);
		writel(outmode, base + BUF0_OUT_CFG);

		writel(0x1, base + BUF_CFG);
	}
	else{
		inpmode =  (3 << 16) | SECDMX_BUF_FIFO_MODE | SECDMX_BUF_DRM_CIPHER_CHANNEL;
#ifdef DRM_ENGINE_CPU_READ
        outmode =  (3 << 16) | SECDMX_BUF_RAM_MODE  | SECDMX_BUF_CPU_CHANNEL; // RAM BUF1 CPU READ 
#else
		outmode =  (3 << 16) | SECDMX_BUF_DMA_MODE  | SECDMX_BUF_DMA_CHANNEL;
#endif

		writel(SECDMX_BUF_RESET, base + BUF1_CTRL);
		writel(SECDMX_BUF_UNRESET, base + BUF1_CTRL);

		writel(inpmode, base + BUF1_INP_CFG);
		writel(outmode, base + BUF1_OUT_CFG);

		//writel(((drm->param.sector_size/4) << 16) | (1 << 15), base + BUF1_DMA_RD_CFG);
	}
}

static void init_secret_key(struct silan_drm *drm, u32 *key)
{
	writel(key[0], &drm->drm_core_base->key_reg0);
	writel(key[1], &drm->drm_core_base->key_reg1);
	writel(key[2], &drm->drm_core_base->key_reg2);
	writel(key[3], &drm->drm_core_base->key_reg3);
	//writel(key[4], &drm->drm_core_base->key_reg4);
	//writel(key[5], &drm->drm_core_base->key_reg5);
}

static void init_vector(struct silan_drm *drm, u32 *vec)
{
	writel(vec[0], &drm->drm_core_base->iv_reg0);
	writel(vec[1], &drm->drm_core_base->iv_reg1);
	writel(vec[2], &drm->drm_core_base->iv_reg2);
	writel(vec[3], &drm->drm_core_base->iv_reg3);
}

static void init_count(struct silan_drm *drm, u32 *cnt)
{
	writel(cnt[0], &drm->drm_core_base->cnt_reg0);
	writel(cnt[1], &drm->drm_core_base->cnt_reg1);
	writel(cnt[2], &drm->drm_core_base->cnt_reg2);
	writel(cnt[3], &drm->drm_core_base->cnt_reg3);
}

static void drm_core_reset(struct silan_drm *drm)
{
	u32 val;

	writel(DRM_CTRL_REG_CLEAR, &drm->drm_core_base->ctrl_reg);

	val = readl(&drm->drm_core_base->ctrl_reg);
	val &= (~DRM_CTRL_REG_CLEAR);

	writel(val, &drm->drm_core_base->ctrl_reg);
}

static void drm_core_init_param(struct silan_drm *drm, u32 *key, u32 *vec, u32 *cnt)
{
	init_secret_key(drm, key);
	init_vector(drm, vec);
	init_count(drm, cnt);
}

static void drm_core_set_config(struct silan_drm *drm, enum patten pa, enum type tp, enum mode mo)
{
	u32 reg = 0;
	reg |= pa;
	reg |= DRM_CFG_REG_TYPE(tp);
	reg |= DRM_CFG_REG_MODE(mo);

	writel(reg, &drm->drm_core_base->cfg_reg);
}

static void drm_core_set_sector_size(struct silan_drm *drm, u32 size)
{
	if(size > 0){
		writel(size, &drm->drm_core_base->sector_size_reg);
	}
}

static void init_enable(struct silan_drm *drm)
{
	u32 val = 0;
	val = readl(&drm->drm_core_base->ctrl_reg);
	val |= (DRM_CTRL_REG_INIT_EN);
	writel(val, &drm->drm_core_base->ctrl_reg);
}

static void drm_core_sector_start(struct silan_drm *drm)
{
	writel(DRM_CMD_REG_SECTOR_START, &drm->drm_core_base->cmd_reg);
}

#if 0
static void drm_core_block_start(struct silan_drm *drm)
{
	writel(DRM_CMD_REG_BLOCK_START, &drm->drm_core_base->cmd_reg);
}
#endif

static void drm_core_interrupt_unmask(struct silan_drm *drm)
{
	writel(DRM_INTERRUPT_UNMASK, &drm->drm_core_base->mask_reg);
}

static int sl_drm_cipher_init(struct silan_drm *drm, enum patten pa)
{
	buf_set(drm, 0);
	
	buf_set(drm, 1);
	
	drm_core_reset(drm);
	
	drm_core_init_param(drm, drm->param.key, drm->param.vec, drm->param.cnt);

	drm_core_set_config(drm, pa, drm->config.tp, drm->config.mo);

	drm_core_set_sector_size(drm, drm->param.size / 16);
 
	drm_core_interrupt_unmask(drm);
	
	init_enable(drm);
	
	return 0;
}

static void sl_drm_cipher_start(struct silan_drm *drm)
{
	drm_core_sector_start(drm);
}

static void sl_drm_cipher_complete(struct silan_drm *drm)
{
	//printk("drm_cipher_complete\n");
}

static struct silan_drm_cipher_ops sl_drm_cipher_ops = {
	.init     = sl_drm_cipher_init,
	.start    = sl_drm_cipher_start,
	.complete = sl_drm_cipher_complete,
};

void silan_cipher_ops_init(struct silan_drm *drm)
{
	drm->cipher_ops = &sl_drm_cipher_ops;
}
