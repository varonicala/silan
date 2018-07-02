/* mailbox.h */

#ifndef _MAILBOX_H
#define _MAILBOX_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/blkdev.h>
#include <linux/interrupt.h>
#include "dsp_mbcmd.h"
#include "dsp_core.h"

typedef enum
{
	DSPNAME_START =0,
	SILAN_DSP0 = DSPNAME_START,
	SILAN_DSP1,
	DSPNAME_END,
}SILAN_DSPNAME;

#ifdef CONFIG_MIPS_SILAN_SUVII
/*cxc reg1*/
#define SET_DSP0_RUNST_MASK			(0x1<<0)
#define SET_DSP1_RUNST_MASK			(0x1<<16)

/*cxc reg2*/
#define SET_DSP0_FIFO_RESET			(0x1<<0)
#define SET_DSP1_FIFO_RESET			(0x1<<16)

/*cxc reg3*/
#define SET_DSP0_FIFO_WDAT(x)		((x)<<0)

/*cxc reg4*/
#define SET_DSP1_FIFO_WDAT(x)		((x)<<0)

/*cxc reg5*/
#define DSP1_FIFO_EMPTY				(0x1<<17)
#define DSP1_FIFO_FULL				(0x1<<16)
#define DSP0_FIFO_EMPTY				(0x1<<1)
#define DSP0_FIFO_FULL				(0x1<<0)

/*cxc reg6*/
#define GET_DSP1_RDAT(x)			(((x)>>16)&0xffff)
#define GET_DSP0_RDAT(x)			((x)&0xffff)

/*cxc reg7*/
#define SET_DSP1_IRQ_CLR			(0x1<<1)
#define SET_DSP0_IRQ_CLR			(0x1<<0)

#endif

#define CXC_REG_BASE 0
#define CXC_DSP_INT_STATUS  (CXC_REG_BASE + 0x0)
#define CXC_DSP_INT_MASK    (CXC_REG_BASE + 0x4)
#define CXC_DSP_INT_SET     (CXC_REG_BASE + 0x8)
#define CXC_DSP_INT_CLR     (CXC_REG_BASE + 0xc)
#define CXC_HOST_INT_STATUS (CXC_REG_BASE + 0x10)
#define CXC_HOST_INT_MASK   (CXC_REG_BASE + 0x14)
#define CXC_HOST_INT_SET    (CXC_REG_BASE + 0x18)
#define CXC_HOST_INT_CLR    (CXC_REG_BASE + 0x1c)

#define CXC_MUTEX(n)        (CXC_REG_BASE + 0x20 + 4*(n))
#define CXC_MAIL_BOX(n)     (CXC_REG_BASE + 0x100 + 4*(n))

#define CXC_DLNA_SET_END_FLAG       (58)
#define CXC_DLNA_GET_DECODE_STATE   (59)
#define CXC_DLNA_IN_RIDX            (60)
#define CXC_DLNA_IN_WIDX            (61)
#define CXC_DLNA_OUT_RIDX           (62)
#define CXC_DLNA_OUT_WIDX           (63)


#define MAILBOX_LEN_PER_TASK 0x20
#define MIPS_TO_DSP_INT 0x1
#define MIPS2DSP_INT_MASK(x) (1<<(x))
#define MIPS2DSP_INT_SET(x) (1<<(x))

typedef struct _mbox_msg
{
	struct mbcmd *cmd;
	struct mbcmd_st cmdst;
	struct list_head list;	
}mbox_msg_t;

struct silan_mbox;

struct silan_mbox_ops 
{
	int (*startup)(struct silan_mbox *mbox);
	int (*shutdown)(struct silan_mbox *mbox);

	/* fifo */
	int	(*create_fifo_read)(struct silan_mbox *mbox,struct dspdev_filter *dspfilter,SILAN_DSPNAME dspname);
	int (*fifo_read)(struct silan_mbox *mbox,SILAN_DSPNAME dspname,mbox_msg_t *msg,int task_num);
	int (*fifo_read_empty)(struct silan_mbox *mbox,SILAN_DSPNAME dspname,int task_num);
	int	(*fifo_read_full)(struct silan_mbox *mbox,SILAN_DSPNAME dspname);

	int	(*create_fifo_write)(struct silan_mbox *mbox,SILAN_DSPNAME dspname,dma_addr_t dspmap_addr,u32 size);
	int (*fifo_write)(struct silan_mbox *mbox,SILAN_DSPNAME dspname,mbox_msg_t *msg);
	int	(*fifo_write_empty)(struct silan_mbox *mbox,SILAN_DSPNAME dspname);
	int	(*fifo_write_full)(struct silan_mbox *mbox,SILAN_DSPNAME dspname);

	/* irq */
	SILAN_DSPNAME (*irq_status)(struct silan_mbox *mbox);
	void (*irq_clr)(struct silan_mbox *mbox);
};

struct silan_mbox_queue 
{
	spinlock_t lock;
	struct list_head queue;
	struct work_struct work;
	int	(*callback)(void *);
	struct silan_mbox	*mbox;
};

struct silan_mbox_priv 
{
	u8* dsp2mipsAddr;
	u32 size;
};

struct silan_mbox 
{
	char *name;
	u32 irq;
	struct silan_mbox_queue *txq,*rxq;
	struct silan_mbox_ops *ops;
	mbox_msg_t seq_snd,seq_rcv;
	struct device dev;
	void *priv;
	void (*err_notify)(void);
};

/* Mailbox FIFO handle functions */
static inline int mbox_fifo_create_read(struct silan_mbox *mbox,struct dspdev_filter *dspfilter,SILAN_DSPNAME dspname)
{
	return mbox->ops->create_fifo_read(mbox,dspfilter,dspname);
}

static inline int mbox_fifo_read(struct silan_mbox *mbox,SILAN_DSPNAME dspname,mbox_msg_t *msg,int task_num)
{
	return mbox->ops->fifo_read(mbox,dspname,msg,task_num);
}

static inline int mbox_fifo_write(struct silan_mbox *mbox,SILAN_DSPNAME dspname,mbox_msg_t *msg)
{
	return mbox->ops->fifo_write(mbox,dspname,msg);
}

static inline int mbox_fifo_read_empty(struct silan_mbox *mbox,SILAN_DSPNAME dspname,int task_num)
{
	return mbox->ops->fifo_read_empty(mbox,dspname,task_num);
}

static inline int mbox_fifo_read_full(struct silan_mbox *mbox,SILAN_DSPNAME dspname)
{
	return mbox->ops->fifo_read_full(mbox,dspname);
}

static inline int mbox_fifo_write_empty(struct silan_mbox *mbox,SILAN_DSPNAME dspname)
{
	return mbox->ops->fifo_write_empty(mbox,dspname);
}

static inline int mbox_fifo_write_full(struct silan_mbox *mbox,SILAN_DSPNAME dspname)
{
	return mbox->ops->fifo_write_full(mbox,dspname);
}

static inline SILAN_DSPNAME mbox_irq_status(struct silan_mbox *mbox)
{
	return mbox->ops->irq_status(mbox);
}

static inline void mbox_irq_clr(struct silan_mbox *mbox)
{
	return mbox->ops->irq_clr(mbox);
}

struct silan_mbox* silan_mbox_get(const char *name);
int silan_mbox_put(const char *name);
int silan_mbox_probe(struct platform_device *pdev);

int cxc_set_mips2dsp_int(void);

int silan_cxc_get_idx(int *pidx, int offset);
int silan_cxc_set_idx(int idx, int offset);

#endif /* MAILBOX_H */
