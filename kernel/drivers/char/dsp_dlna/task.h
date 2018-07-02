#ifndef _TASK_H_
#define _TASK_H_
#include "dsp_io.h"
#include "dsp_mbcmd.h"
#include "ringbuffer.h"
#include "dsp_core.h"

#define DSP_MMAP_AREA1_OFFSET 0x8000000

typedef enum
{
	DSP_ST_FREE = 0,
	DSP_ST_ALLOC = 1,
	DSP_ST_STOP = 0x2,
	DSP_ST_CFG = 0x4,
	DSP_ST_START = 0x8,
}DSPSTATE;

struct dsptask
{
	u32 tid;
	DSPSTATE state;
	char* name;
	u32	codec_type; 
	struct mbcmd_st cmdst;
};

void mbox_tadd(struct mbcmd_st *cmdst);
void mbox_tdel(struct mbcmd_st *cmdst);
void mbox_tcfg(struct mbcmd_st *cmdst);
void mbox_tstop(struct mbcmd_st *cmdst);
void mbox_send(struct mbcmd_st *cmdst);
void mbox_request(struct mbcmd_st *cmdst);
void mbox_send_async(struct mbcmd_st *cmdst);
void mbox_set_pp(struct mbcmd_st *cmdst);
void mbox_codec_type(struct mbcmd_st *cmdst);
void mbox_codec_cfg(struct mbcmd_st *cmdst);
void mbox_codec_start(struct mbcmd_st *cmdst);
void mbox_codec_stop(struct mbcmd_st *cmdst);
void mbox_codec_pause(struct mbcmd_st *cmdst);
void mbox_codec_info(struct mbcmd_st *cmdst);
void mbox_system_attr(struct mbcmd_st *cmdst);
void mbox_runlevel(struct mbcmd_st *cmdst);
void mbox_debug(struct mbcmd_st *cmdst);
void mbox_poll(struct mbcmd_st *cmdst);
void mbox_suspend(struct mbcmd_st *cmdst);
void mbox_resume(struct mbcmd_st *cmdst);

#ifdef CONFIG_MIPS_SILAN_DLNA
void mbox_common_rcv(struct mbcmd_st *cmdst);
#endif

int dsp_set_suspend(void);
int dsp_set_resume(void);

int dsp_set_system_addr(struct mbcmd *cmd);
long silan_task_ioctl(struct file *fp,unsigned int cmd,unsigned long arg);
void dsp_send_cmd_async(struct dspdev_filter *dspfilter,struct dsp_ringbuffer *rbuf);
void dsp_rbuffer_update(struct dsp_ringbuffer *wbuf,struct mbcmd_st *cmdst);
#endif
