#ifndef _CMD_H
#define _CMD_H

#include "common.h"

struct dsp_mbox_msg
{
    struct mbcmd *mcmd;
    struct mbcmd_st *mcmd_st;
};

struct dsp_mbox
{
    struct dsp_mbox_msg msg;
    struct work_struct tx_work;
    struct work_struct rx_work;
};

extern int dsp_send_cmd(struct mbcmd *mcmd);
extern int dsp_mbox_init(void);
extern irqreturn_t silan_dsp_interrupt(int irq, void *p);

#endif

