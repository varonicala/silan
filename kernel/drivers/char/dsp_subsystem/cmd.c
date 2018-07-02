#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include "common.h"
#include "cxc.h"
#include "cmd.h"
#include "netlink.h"

#define DSP_MSG_TIMEOUT    (2*HZ)
static DECLARE_WAIT_QUEUE_HEAD(cfg_wait_q);
struct dsp_mbox *mbox_info, mbox_info_s;

static void mbox_tx_work(struct work_struct *t)
{
    struct dsp_mbox *mbox;
    struct dsp_mbox_msg *msg;

    mbox = (struct dsp_mbox *)container_of(t, struct dsp_mbox, tx_work);
    msg = &mbox->msg;

    if (cxc_write_mips2dsp_fifo(msg->mcmd) == DSP_ERROR_NONE)
        cxc_set_mips2dsp_int();
    else
        LOGE("mbox_tx_work failed \n");
}

static int dsp_recv_cmd(struct mbcmd_st *mcmd_st)
{
    if (!waitqueue_active(&cfg_wait_q))
    {
        LOGE("dsp_recv_cmd failed, waitqueue_active error \n");
        return DSP_ERROR_CMD;
    }

    LOGD("dsp_recv_cmd %d\n", mcmd_st->cmd);
    wake_up_interruptible(&cfg_wait_q);
    LOGD("dsp_recv_cmd %d end\n", mcmd_st->cmd);

    return DSP_ERROR_NONE;
}

static void mbox_rx_work(struct work_struct *t)
{
    struct dsp_mbox *mbox;
    struct mbcmd_st mcmd_st;

    mbox = (struct dsp_mbox *)container_of(t, struct dsp_mbox, rx_work);
    cxc_read_dsp2mips_fifo(&mcmd_st);
    mbox->msg.mcmd_st = &mcmd_st;
	if(mcmd_st.cmd <= POST_CMD_SET_EQ){
		dsp_recv_cmd(mbox->msg.mcmd_st);
	}
	else {
		printk("recv dsp cmd: %d!\n", mcmd_st.cmd);
	}
}

int dsp_send_cmd(struct mbcmd *mcmd)
{
    int ret;

    LOGD("dsp_send_cmd %d\n", mcmd->cmd);
    DEFINE_WAIT(wait);
    prepare_to_wait(&cfg_wait_q, &wait, TASK_INTERRUPTIBLE);
    mbox_info->msg.mcmd = mcmd;
    schedule_work(&mbox_info->tx_work);
    ret = schedule_timeout(DSP_MSG_TIMEOUT);
    LOGD("dsp_send_cmd %d end, ret %d\n", mcmd->cmd, ret);

    if (ret == 0)
    {
        remove_wait_queue(&cfg_wait_q, &wait);
        return DSP_ERROR_CMD;
    }

    return DSP_ERROR_NONE;
}

irqreturn_t silan_dsp_interrupt(int irq, void *p)
{
    schedule_work(&mbox_info->rx_work);
	//printk("## %s ##\n", __func__);
    cxc_clr_dsp2mips_int();

    return IRQ_HANDLED;
}

int dsp_mbox_init(void)
{
    int ret = DSP_ERROR_NONE;

    mbox_info = &mbox_info_s;
    INIT_WORK(&mbox_info->tx_work, mbox_tx_work);
    INIT_WORK(&mbox_info->rx_work, mbox_rx_work);

    return ret;
}

