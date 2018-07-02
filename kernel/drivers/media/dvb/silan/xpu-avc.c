/*
 * XPUDTV driver
 *
 * Copyright (C) 2012 panjianguang <panjianguang@silan.com.cn>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation; either version 2 of
 *    the License, or (at your option) any later version.
 */

#include <linux/bug.h>
#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/stringify.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <dvb_frontend.h>

#include "xpudtv.h"
#include "xpucxc.h"

static void get_mutex(struct xpudtv *xdtv)
{
    unsigned int tmp;    
    do {
        tmp = readl(xdtv->base_addr + CXC_MUTEX(0));
    }while(tmp);
}

static void release_mutex(struct xpudtv *xdtv)
{
    writel(0, xdtv->base_addr + CXC_MUTEX(0));
}

static int xpu_fw_write(struct xpudtv *xdtv, u8 *data, size_t length)
{
    unsigned char i;
    unsigned int tmp;
    struct xpu_avc_command_frame *c;
    c = (struct xpu_avc_command_frame *)data;
    if(c->opcode != XPU_OPCODE_REV_SEC && c->opcode != XPU_OPCODE_REV_PES)
        get_mutex(xdtv);
    for(i = 0; i < XPU_CXC_MAX_FIFO; i+=4) {
        tmp = 0;
        tmp |= ((data[i]   << 24)&0xff000000);
        tmp |= ((data[i+1] << 16)&0x00ff0000);
        tmp |= ((data[i+2] <<  8)&0x0000ff00);
        tmp |= ((data[i+3])&0xff);
        writel(tmp, xdtv->base_addr + CXC_MAIL_BOX(i / 4));
    }
    writel(0x1, xdtv->base_addr + CXC_HOST_INT_MASK);
    writel(0x1, xdtv->base_addr + CXC_HOST_INT_SET);
    return 0;
}

static int xpu_fw_read(struct xpudtv *xdtv,u8 *data, size_t length)
{
    unsigned char i;
    unsigned int tmp;
    struct xpu_avc_response_frame *r;
    r = (struct xpu_avc_response_frame*)data;
    for    (i = 0; i < XPU_CXC_MAX_FIFO; i+=4) {
        tmp = readl(xdtv->base_addr + CXC_MAIL_BOX(i / 4));
        data[i]   = (tmp >> 24)&0xff;
        data[i+1] = (tmp >> 16)&0xff;
        data[i+2] = (tmp >>  8)&0xff;
        data[i+3] = (tmp &0xff);
    }
    
    {
        if(r->sync == XPU_RESPONSE_SYNC && (r->opcode == XPU_OPCODE_REV_SEC || r->opcode == XPU_OPCODE_REV_PES))
            return 0;
    }
    release_mutex(xdtv);
    return 0;
} 
static int xpu_avc_write(struct xpudtv *xdtv)
{
    int err, retry;
    xdtv->avc_reply_received = false;
    
    err = xpu_fw_write(xdtv,xdtv->avc_data, xdtv->avc_data_length);
    if (err) 
    {
        dev_err(xdtv->device, "XPU command write failed\n");
        return err;
    }

    for (retry = 0; retry < 6; retry++) 
    {
         /*
         * AV/C specs say that answers should be sent within 150 ms.
         * Time out after 200 ms.
         */
        if (wait_event_timeout(xdtv->avc_wait,
                       xdtv->avc_reply_received,
                       msecs_to_jiffies(200)) != 0) {
            return 0;
        }
    }
    dev_err(xdtv->device, "SILAN XPU response timed out\n");

    return -ETIMEDOUT;
}

/****************************************************************************
 * IRQ handling
 ****************************************************************************/
static int xpu_DvbDmxFilterCallback(u8 *buffer1, size_t buffer1_len,struct dvb_demux_filter *dvbdmxfilter)
{
    if (!dvbdmxfilter->feed->demux->dmx.frontend) {
        printk("%s: not frontend!\n", __func__);
        return 0;
    }

    if (dvbdmxfilter->feed->demux->dmx.frontend->source == DMX_MEMORY_FE) {
        printk("%s: DMX_MEMORY_FE!\n", __func__);
        return 0;
    }

    switch (dvbdmxfilter->type) {
    case DMX_TYPE_SEC:
        return dvbdmxfilter->feed->cb.sec(buffer1, buffer1_len,
                            NULL, 0,
                            &dvbdmxfilter->filter,
                            DMX_OK);
    case DMX_TYPE_TS:
        return dvbdmxfilter->feed->cb.ts(buffer1, buffer1_len,
                            NULL, 0,
                            &dvbdmxfilter->feed->feed.ts,
                            DMX_OK);
    default:
        return 0;
    }
}

int xpu_avc_recv(struct xpudtv *xdtv)
{
    struct xpu_avc_response_frame *r;
    struct xpu_avc_command_frame c;
    struct dvb_demux_filter *dvbdmxfilter;
    int ret;

    xpu_fw_read(xdtv,xdtv->avc_data,XPU_CXC_MAX_FIFO);
    xdtv->avc_data_length = XPU_CXC_MAX_FIFO;

    r = (struct xpu_avc_response_frame*)xdtv->avc_data;

    if (xdtv->avc_reply_received)
    {
        if (r->opcode != XPU_OPCODE_REV_SEC && r->opcode != XPU_OPCODE_REV_PES) {
            dev_err(xdtv->device, "XPU out-of-order AVC response, ignored, opcode = %d\n", r->opcode);
            return -EIO;
        }
    }

    if(r->sync != XPU_RESPONSE_SYNC)
    {
        dev_err(xdtv->device, "XPU response sync error, ignored\n");
        return -EIO;
    }

    switch(r->opcode)
    {
        case XPU_OPCODE_SET_PID:
        case XPU_OPCODE_CLR_PID:
            if(r->u.pid_filter_respond.err)
            {
                dev_err(xdtv->device, "XPU response set/clr pid status error, ignored\n");
                return -EIO;
            }
            break;
        case XPU_OPCODE_CARESET:
            if(r->u.ca_status.err)
            {
                dev_err(xdtv->device, "XPU response ca status error, ignored\n");
                return -EIO;
            }
            break;
        case XPU_OPCODE_REV_SEC:
        case XPU_OPCODE_REV_PES:
            dvbdmxfilter = xdtv->handle2filter[r->u.rev_sec_respond.channel];
            ret = xpu_DvbDmxFilterCallback(r->u.rev_sec_respond.buf,r->u.rev_sec_respond.len,dvbdmxfilter);

            memset((char*)&c, 0, XPU_CXC_MAX_FIFO);
            c.sync = XPU_COMMAND_SYNC;
            c.opcode = r->opcode;
            c.size = sizeof(struct xpu_rev_sec);
            c.u.rev_sec.channel = r->u.rev_sec_respond.channel;
            c.u.rev_sec.pid = r->u.rev_sec_respond.pid;
            c.u.rev_sec.count = r->u.rev_sec_respond.count;
            c.u.rev_sec.start_index = r->u.rev_sec_respond.start_index;
            c.u.rev_sec.err = 0;
            xpu_fw_write(xdtv,(u8*)&c,sizeof(c));
            return 0;;
        default:
            break;
    }
    xdtv->avc_reply_received = true;
    wake_up(&xdtv->avc_wait);
    return 0;
}

int xpu_avc_set_pid(struct xpudtv *xdtv, unsigned char pidc, u16 pid[], u16 type, struct dmx_section_filter filter)
{
    struct xpu_avc_command_frame *c = (void *)xdtv->avc_data;
    int ret;
    int i;

    if (pidc >= XPU_DTV_MAX_CHANNEL)
        return -EINVAL;

    mutex_lock(&xdtv->avc_mutex);

    memset((char*)c, 0, XPU_CXC_MAX_FIFO);
    c->sync = XPU_COMMAND_SYNC;
    c->opcode = XPU_OPCODE_SET_PID;
    c->size = sizeof(struct xpu_pidfilter);
    c->u.pid_filter.pid = pid[pidc];
    c->u.pid_filter.channel = pidc;
    c->u.pid_filter.type = type;

    c->u.pid_filter.filter[0] = filter.filter_value[0];
    c->u.pid_filter.mask[0] = filter.filter_mask[0];
    c->u.pid_filter.mode[0] = filter.filter_mode[0];
    for (i = 3; i < SEC_FILTER_BITS + 2; i++)
    {
        c->u.pid_filter.filter[i - 2] = filter.filter_value[i];
        c->u.pid_filter.mask[i - 2] = filter.filter_mask[i];
        c->u.pid_filter.mode[i - 2] = filter.filter_mode[i];
        printk("0x%x ", filter.filter_value[i]);
    }
    printk("\n");

    xdtv->avc_data_length = XPU_CXC_MAX_FIFO;
    ret = xpu_avc_write(xdtv);

    mutex_unlock(&xdtv->avc_mutex);

    return ret;
}

int xpu_avc_clr_pid(struct xpudtv *xdtv, unsigned char pidc, u16 pid[])
{
    struct xpu_avc_command_frame *c = (void *)xdtv->avc_data;
    int ret;

    if (pidc >= XPU_DTV_MAX_CHANNEL)
        return -EINVAL;

    mutex_lock(&xdtv->avc_mutex);

    memset((char*)c, 0, XPU_CXC_MAX_FIFO);
    c->sync = XPU_COMMAND_SYNC;
    c->opcode = XPU_OPCODE_CLR_PID;
    c->size = sizeof(struct xpu_pidfilter);
    c->u.pid_filter.pid = pid[pidc];
    c->u.pid_filter.channel = pidc;

    xdtv->avc_data_length = XPU_CXC_MAX_FIFO;
    ret = xpu_avc_write(xdtv);

    mutex_unlock(&xdtv->avc_mutex);

    return ret;
}

int xpu_avc_ca_reset(struct xpudtv *xdtv)
{
    struct xpu_avc_command_frame *c = (void *)xdtv->avc_data;
    int ret;

    mutex_lock(&xdtv->avc_mutex);

    memset((char*)c, 0, XPU_CXC_MAX_FIFO);
    c->sync = XPU_COMMAND_SYNC;
    c->opcode = XPU_OPCODE_CARESET;
    c->size = 0;

    xdtv->avc_data_length = XPU_CXC_MAX_FIFO;
    ret = xpu_avc_write(xdtv);

    mutex_unlock(&xdtv->avc_mutex);

    return ret;
}

int xpu_start_streaming(struct xpudtv *xdtv)
{
    int ret;
    struct xpu_avc_command_frame *c = (void *)xdtv->avc_data;

    mutex_lock(&xdtv->avc_mutex);

    memset((char*)c, 0, XPU_CXC_MAX_FIFO);
    c->sync = XPU_COMMAND_SYNC;
    c->opcode = XPU_OPCODE_RESUME;
    c->size = 0;

    xdtv->avc_data_length = XPU_CXC_MAX_FIFO;
    ret = xpu_avc_write(xdtv);

    mutex_unlock(&xdtv->avc_mutex);
    return ret;
}

int xpu_stop_streaming(struct xpudtv *xdtv)
{
    int ret;
    struct xpu_avc_command_frame *c = (void *)xdtv->avc_data;

    mutex_lock(&xdtv->avc_mutex);

    memset((char*)c, 0, XPU_CXC_MAX_FIFO);
    c->sync = XPU_COMMAND_SYNC;
    c->opcode = XPU_OPCODE_SUSPEND;
    c->size = 0;

    xdtv->avc_data_length = XPU_CXC_MAX_FIFO;
    ret = xpu_avc_write(xdtv);

    mutex_unlock(&xdtv->avc_mutex);
    return ret;
}

