#ifndef _SILAN_NEW_I2C_H_
#define _SILAN_NEW_I2C_H_

#include <linux/i2c.h>
#include <linux/circ_buf.h>

#define I2C4_ENABLE	0xbfba9048
#define I2C4_CLK	0xbfba9078

/*
 * Registers offset
 */
#define DW_IC_CON				0x00
#define DW_IC_TAR				0x04
#define DW_IC_SAR				0x08
#define DW_HS_MADDR				0x0c
#define DW_IC_DATA_CMD			0x10
#define DW_IC_SS_SCL_HCNT		0x14
#define DW_IC_SS_SCL_LCNT		0x18
#define DW_IC_FS_SCL_HCNT		0x1c
#define DW_IC_FS_SCL_LCNT		0x20
#define DW_IC_HS_SCL_LCNT		0x24
#define DW_IC_HS_SCL_HCNT		0x28
#define DW_IC_INTR_STAT			0x2c
#define DW_IC_INTR_MASK			0x30
#define DW_IC_RAW_INTR_STAT		0x34
#define DW_IC_RX_TL				0x38
#define DW_IC_TX_TL				0x3c
#define DW_IC_CLR_INTR			0x40
#define DW_IC_CLR_RX_UNDER		0x44
#define DW_IC_CLR_RX_OVER		0x48
#define DW_IC_CLR_TX_OVER		0x4c
#define DW_IC_CLR_RD_REQ		0x50
#define DW_IC_CLR_TX_ABRT		0x54
#define DW_IC_CLR_RX_DONE		0x58
#define DW_IC_CLR_ACTIVITY		0x5c
#define DW_IC_CLR_STOP_DET		0x60
#define DW_IC_CLR_START_DET		0x64
#define DW_IC_CLR_GEN_CALL		0x68
#define DW_IC_ENABLE			0x6c
#define DW_IC_STATUS			0x70
#define DW_IC_TXFLR				0x74
#define DW_IC_RXFLR				0x78
#define DW_IC_TX_ABRT_SOURCE	0x80
#define DW_IC_DMA_CR			0x88
#define DW_IC_DMA_TDLR			0x8c
#define DW_IC_DMA_RDLR			0x90
#define DW_IC_COMP_PARAM_1		0xf4
#define DW_IC_COMP_VERSION		0xf8
#define DW_IC_COMP_TYPE			0xfc

#define DW_IC_CON_MASTER			0x01
#define DW_IC_CON_SPEED_STD			0x02
#define DW_IC_CON_SPEED_FAST		0x04
#define DW_IC_CON_10BITADDR_MASTER	0x10
#define DW_IC_CON_RESTART_EN		0x20
#define DW_IC_CON_SLAVE_DISABLE		0x40

#define DW_IC_INTR_RX_UNDER		0x001
#define DW_IC_INTR_RX_OVER		0x002
#define DW_IC_INTR_RX_FULL		0x004
#define DW_IC_INTR_TX_OVER		0x008
#define DW_IC_INTR_TX_EMPTY		0x010
#define DW_IC_INTR_RD_REQ		0x020
#define DW_IC_INTR_TX_ABRT		0x040
#define DW_IC_INTR_RX_DONE		0x080
#define DW_IC_INTR_ACTIVITY		0x100
#define DW_IC_INTR_STOP_DET		0x200
#define DW_IC_INTR_START_DET	0x400
#define DW_IC_INTR_GEN_CALL		0x800

#define DW_IC_INTR_DEFAULT_MASK		(DW_IC_INTR_RX_FULL | \
					 DW_IC_INTR_TX_EMPTY | \
					 DW_IC_INTR_TX_ABRT | \
					 DW_IC_INTR_STOP_DET)

#define DW_IC_STATUS_ACTIVITY		0x1

#define DW_IC_ERR_TX_ABRT			0x1

/*
 * status codes
 */
#define STATUS_IDLE					0x0
#define STATUS_WRITE_IN_PROGRESS	0x1
#define STATUS_READ_IN_PROGRESS		0x2


#define RDMAE						0x1
#define TDMAE						0x2
#define TIMEOUT						20 /* ms */

#define I2C_DMA_BUFFER_SIZE         PAGE_SIZE
#define I2C_XMIT_SIZE				PAGE_SIZE
#define FIFO_DEPTH					32
/*
 * harsilanare abort codes from the DW_IC_TX_ABRT_SOURCE register
 *
 * only expected abort codes are listed here
 * refer to the datasheet for the full list
 */
#define ABRT_7B_ADDR_NOACK	0
#define ABRT_10ADDR1_NOACK	1
#define ABRT_10ADDR2_NOACK	2
#define ABRT_TXDATA_NOACK	3
#define ABRT_GCALL_NOACK	4
#define ABRT_GCALL_READ		5
#define ABRT_SBYTE_ACKDET	7
#define ABRT_SBYTE_NORSTRT	9
#define ABRT_10B_RD_NORSTRT	10
#define ABRT_MASTER_DIS		11
#define ARB_LOST			12

#define DW_IC_TX_ABRT_7B_ADDR_NOACK	 (1UL << ABRT_7B_ADDR_NOACK)
#define DW_IC_TX_ABRT_10ADDR1_NOACK	 (1UL << ABRT_10ADDR1_NOACK)
#define DW_IC_TX_ABRT_10ADDR2_NOACK	 (1UL << ABRT_10ADDR2_NOACK)
#define DW_IC_TX_ABRT_TXDATA_NOACK	 (1UL << ABRT_TXDATA_NOACK)
#define DW_IC_TX_ABRT_GCALL_NOACK	 (1UL << ABRT_GCALL_NOACK)
#define DW_IC_TX_ABRT_GCALL_READ	 (1UL << ABRT_GCALL_READ)
#define DW_IC_TX_ABRT_SBYTE_ACKDET	 (1UL << ABRT_SBYTE_ACKDET)
#define DW_IC_TX_ABRT_SBYTE_NORSTRT	 (1UL << ABRT_SBYTE_NORSTRT)
#define DW_IC_TX_ABRT_10B_RD_NORSTRT (1UL << ABRT_10B_RD_NORSTRT)
#define DW_IC_TX_ABRT_MASTER_DIS	 (1UL << ABRT_MASTER_DIS)
#define DW_IC_TX_ARB_LOST		     (1UL << ARB_LOST)

#define DW_IC_TX_ABRT_NOACK		(DW_IC_TX_ABRT_7B_ADDR_NOACK | \
					 DW_IC_TX_ABRT_10ADDR1_NOACK | \
					 DW_IC_TX_ABRT_10ADDR2_NOACK | \
					 DW_IC_TX_ABRT_TXDATA_NOACK | \
					 DW_IC_TX_ABRT_GCALL_NOACK)

#define i2c_circ_empty(circ)	((circ)->head == (circ)->tail)
#define i2c_circ_clear(circ)	((circ)->head = (circ)->tail = 0)

#define i2c_circ_chars_pending(circ) \
	(CIRC_CNT((circ)->head, (circ)->tail, I2C_XMIT_SIZE))

#define i2c_circ_chars_free(circ) \
	(CIRC_SPACE((circ)->head, (circ)->tail, I2C_XMIT_SIZE))

struct i2c_dma_data{
	struct dma_chan	   *chan;
	struct scatterlist sg;
	u32			*buf;
	bool		queued;
};

struct i2c_icount{
	u32  tx;
	u32	 rx;
};

struct silan_i2c_dev;
struct silan_i2c_dma_ops {
	int (*dma_init)(struct silan_i2c_dev *dev);
	int (*dma_start)(struct silan_i2c_dev *dev);
	void (*dma_exit)(struct silan_i2c_dev *dev);
	int (*dma_read)(struct silan_i2c_dev *dev, int cs_change);
	int (*dma_write)(struct silan_i2c_dev *dev, int cs_change);
	int (*dma_transfer)(struct silan_i2c_dev *dev, int cs_change);
};

/**
 * struct silan_i2c_dev - private i2c-designware data
 * @dev: driver model device node
 * @base: IO registers pointer
 * @cmd_complete: tx completion indicator
 * @lock: protect this struct and IO registers
 * @clk: input reference clock
 * @cmd_err: run time hasilanare error code
 * @msgs: points to an array of messages currently being transferred
 * @msgs_num: the number of elements in msgs
 * @msg_write_idx: the element index of the current tx message in the msgs
 *	array
 * @tx_buf_len: the length of the current tx buffer
 * @tx_buf: the current tx buffer
 * @msg_read_idx: the element index of the current rx message in the msgs
 *	array
 * @rx_buf_len: the length of the current rx buffer
 * @rx_buf: the current rx buffer
 * @msg_err: error status of the current transfer
 * @status: i2c master status, one of STATUS_*
 * @abort_source: copy of the TX_ABRT_SOURCE register
 * @irq: interrupt number for the i2c master
 * @adapter: i2c subsystem adapter node
 * @tx_fifo_depth: depth of the harsilanare tx fifo
 * @rx_fifo_depth: depth of the harsilanare rx fifo
 */
struct silan_i2c_dev {
	struct device		*dev;
	void __iomem		*base;
	void __iomem		*phy_base;
	struct completion	cmd_complete;
	struct mutex		lock;
	struct clk		*clk;
	int			cmd_err;
	struct i2c_msg		*msgs;
	int			msgs_num;
	int			msg_write_idx;
	u32			tx_buf_len;
	u8			*tx_buf;
	int			msg_read_idx;
	u32			rx_buf_len;
	u8			*rx_buf;
	int			msg_err;
	unsigned int		status;
	u32			abort_source;
	int			irq;
	struct i2c_adapter	adapter;
	unsigned int		tx_fifo_depth;
	unsigned int		rx_fifo_depth;
	struct i2c_icount	icount;
	
	/* Driver message queue */
	struct workqueue_struct	*workqueue;
	struct work_struct	pump_messages;
	spinlock_t		spinlock;
	struct list_head	queue;
	int			busy;
	int			run;
	u32 stat;
	struct tasklet_struct	tasklet;

#ifdef CONFIG_I2C_DMA_MODE
	struct  circ_buf   xmit;
	struct	i2c_dma_data  dmarx;
	struct  i2c_dma_data  dmatx;
	dma_addr_t  dma_addr;
	struct device		*dma_dev;
	int dma_inited;
	struct silan_i2c_dma_ops   *dma_ops;
	int dma_chan_done;
	int dmarx_ch;
	int dmatx_ch;
	int dmatx_done;
#endif
};

int silan_i2c_mid_init(struct silan_i2c_dev *dev);

#endif //_SILAN_NEW_I2C_H_
