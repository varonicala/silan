#ifndef _SILAN_DRM_H_
#define _SILAN_DRM_H_

#include <linux/poll.h>

#include "silan_drm_core.h"
#include "silan_dmac.h"

#define SILAN_DRM_NAME  "silan-drm"

#define DRM_MAGIC 'd'

#define DRM_IOCTL_READ           _IO(DRM_MAGIC, 1)
#define DRM_IOCTL_SET_KEY        _IO(DRM_MAGIC, 2)
#define DRM_IOCTL_SET_VECTOR     _IO(DRM_MAGIC, 3)
#define DRM_IOCTL_SET_COUNT      _IO(DRM_MAGIC, 4)
#define DRM_IOCTL_SET_SIZE       _IO(DRM_MAGIC, 5)
#define DRM_IOCTL_SET_TYPE       _IO(DRM_MAGIC, 6)
#define DRM_IOCTL_SET_MODE       _IO(DRM_MAGIC, 7)
#define DRM_IOCTL_CIPHER         _IO(DRM_MAGIC, 8)
#define DRM_IOCTL_DECIPHER       _IO(DRM_MAGIC, 9)
#define DRM_IOCTL_RSA_C			 _IO(DRM_MAGIC, 10)
#define DRM_IOCTL_RSA_M          _IO(DRM_MAGIC, 11)
#define DRM_IOCTL_RSA_N          _IO(DRM_MAGIC, 12)
#define DRM_IOCTL_RSA_D			 _IO(DRM_MAGIC, 13)
#define DRM_IOCTL_RSA_E			 _IO(DRM_MAGIC, 14)
#define DRM_IOCTL_RSA_E_D_WIDTH	 _IO(DRM_MAGIC, 15)
#define DRM_IOCTL_RSA_N_WIDTH	 _IO(DRM_MAGIC, 16)
#define DRM_IOCTL_RSA_ENDIAN	 _IO(DRM_MAGIC, 17)

#define DRM_IN_BUF_SIZE   0x10000
#define DRM_OUT_BUF_SIZE  0x10000
#define DRM_MMAP_SIZE     (DRM_IN_BUF_SIZE + DRM_OUT_BUF_SIZE)

#define TX_CHAN		0
#define RX_CHAN		1

#define ID_BUF0_WR	0
#define ID_BUF0_RD	1
#define ID_BUF1_WR	2
#define ID_BUF1_RD	3

#define DRM_ENGINE_CPU_READ

struct drm_data{
	u32 size;
	u32 key[6];
	u32 vec[4];
	u32 cnt[4];
};

enum patten{
	CIPHER,
	DECIPHER,
};

enum type{
	AES,
	SMS4,
	DES,
	SDES,
	C2,
};

enum mode{
	EBC,
	CBC,
	CFB,
	OFB,
	CTR,
};

struct cipher_config{
	enum type tp;
	enum mode mo;
};

enum drm_direction{
	DMA_FROM_BUF,
	DMA_TO_BUF,
};

struct silan_drm
{
	void __iomem *dmac_base;
	u32 buf_ctrl_base;
	DRM_CORE *drm_core_base;

	//u8 *mmap_cpu;
	u32 *mmap_cpu;
	u32 mmap_size;
	struct device *dev;
	dma_addr_t map_dma;
	wait_queue_head_t irq_wait;
	atomic_t irqc;
	struct mutex mutex;
	int irq;
	struct drm_data param;
	struct cipher_config config;
	struct silan_drm_dma_ops *dma_ops;
	struct silan_drm_cipher_ops *cipher_ops;
	struct dma_chan tx_chan;
	struct dma_chan rx_chan;
};

struct silan_drm_dma_ops{
	int (*init)(struct silan_drm *drm);
	void (*start)(struct silan_drm *drm, enum drm_direction dir);
	void (*complete)(struct silan_drm *drm, enum drm_direction dir);
	void (*stop)(struct silan_drm *drm, enum drm_direction dir);
	void (*cleanup)(struct silan_drm *drm);
	void (*exit)(struct silan_drm *drm);
};

struct silan_drm_cipher_ops{
	int (*init)(struct silan_drm *drm, enum patten pa);
	void (*start)(struct silan_drm *drm);
	void (*complete)(struct silan_drm *drm);
	void (*exit)(struct silan_drm *drm);
};

extern void silan_dmac_ops_init(struct silan_drm *drm);
extern void silan_cipher_ops_init(struct silan_drm *drm);

#endif
