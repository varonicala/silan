/* mailbox.h */

#ifndef _SILAN_UNICOM_H
#define _SILAN_UNICOM_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/blkdev.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>

#define CXC_UNICOM_INT_STATUS  (0x40C)
#define CXC_UNICOM_INT_MASK    (0x400)
#define CXC_UNICOM_INT_SET     (0x404)
#define CXC_UNICOM_INT_CLR     (0x408)
#define CXC_HOST_INT_STATUS    (0x30C)
#define CXC_HOST_INT_MASK      (0x300)
#define CXC_HOST_INT_SET       (0x304)
#define CXC_HOST_INT_CLR       (0x308)

#define UNICOM_MUTEX(n)    (0x200 + 4*(n))
#define UNICOM_DATA(n)     (0x000 + 4*(n))

#define CXC_SPIFLASH_ADDR0		(0x80ff00)
#define CXC_SPIFLASH_ADDR1		(0x3AD999)
#define CXC_SPIFLASH_ADDR2		(0x3C1999)
#define CXC_SPIFLASH_ADDR3		(0x3D5999)
#define CXC_SPIFLASH_ADDR4		(0x3E9999)
//#define UNICOM_DATALEN_PER_MODULE (4)


typedef enum UNICOM_CXC_MODULE_ID{
	UNICOM_CXC_MODULE_RTC = 0,
	UNICOM_CXC_MODULE_IR,
	UNICOM_CXC_MODULE_SPIFLASH,
	UNICOM_CXC_MODULE_KEYPAD,
	UNICOM_CXC_MODULE_INFRA,
	UNICOM_CXC_MODULE_MAX = 7
}UNICOM_CXC_MODULE_ID;


typedef enum UNICOM_CXC_CMD{
	UNICOM_CXC_GET_RTC = 0,
	UNICOM_CXC_SET_RTC,
	
	UNICOM_CXC_CMD_MAX
}UNICOM_CXC_CMD;

typedef struct UNICOM_CXC_CMD_ST
{
//	u32 unicom_module_id;
//	UNICOM_CXC_CMD unicom_cxc_cmd;
	union {
		struct rtc_time rtc;
		u32 framebuf_dma;
	}u;
}UNICOM_CXC_CMD_ST;


typedef struct SPIFLASH_DATA
{
	u32	spiflash_data_size;
	void *spiflash_data_addr;
}spiflash_data;

struct silan_unicom_cxc 
{
	char *name;
	u32 rtc_irq;
	u32 ir_irq;
	u32 keypad_irq;
	u32 spiflash_irq;
	struct silan_unicom_cxc_ops *ops;
	struct device dev;
	void *priv;
	void (*err_notify)(void);
};

//struct silan_unicom;

struct silan_unicom_cxc_ops 
{
	int (*fifo_read)(struct silan_unicom_cxc *unicom, unsigned int *buffer,int module_id,int i, int num);
	int (*fifo_write)(struct silan_unicom_cxc *unicom, unsigned int *buffer,int module_id,int i,int num);
    void (*irq_clr)(void);
};

int silan_unicom_cxc_get_time(void);
int silan_unicom_cxc_set_time(void);
int cxc_set_mips2unicom_rtc_int(void);
int cxc_set_mips2unicom_spiflash_int(void);
int cxc_set_mcu2unicom_rtc_clr(void);
int cxc_set_mcu2unicom_ir_clr(void);
int cxc_set_mcu2unicom_keypad_clr(void);
int cxc_set_mcu2unicom_spiflash_clr(void);
int silan_unicom_cxc_probe(struct platform_device *pdev);
struct silan_unicom_cxc* silan_unicom_cxc_get(const char *name);

#endif
