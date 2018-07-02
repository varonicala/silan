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

#ifndef _XPUDTV_H
#define _XPUDTV_H

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/list.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>
#include <linux/spinlock_types.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <demux.h>
#include <dmxdev.h>
#include <dvb_demux.h>
#include <dvb_frontend.h>
#include <dvb_net.h>
#include <dvbdev.h>

#include "xpucxc.h"

#define IRQ_CMD           0x1
#define IRQ_PES_CB        0x2
#define IRQ_SEC_CB        0x3
#define TS_PACKET_SIZE    188
#define MAX_SEC_PACKET    100
#define MAX_PES_PACKET    2000
#define SEC_MESSAGE_COUNT 20
#define PES_MESSAGE_COUNT 100

struct dma_buf_t {
    u32 base;
    u32 phy_addr;
    u32 size;
};

struct xpudtv 
{
    struct device *device;
    struct list_head list;

    struct dvb_adapter    adapter;
    struct dmxdev        dmxdev;
    struct dvb_demux    demux;
    struct dmx_frontend    frontend;
    struct dvb_net        dvbnet;
    struct i2c_adapter    *i2c;
    struct dvb_frontend   *fe;

    struct dvb_device *cadev;
    int    ca_last_command;
    int    ca_time_interval;

    struct mutex avc_mutex;
    wait_queue_head_t avc_wait;
    bool avc_reply_received;


    struct mutex demux_mutex;
    unsigned long channel_active;
    u16    channel_pid[XPU_DTV_MAX_CHANNEL];
    struct dvb_demux_filter *handle2filter[XPU_DTV_MAX_CHANNEL];

    int    avc_data_length;
    u8    avc_data[XPU_CXC_MAX_FIFO];

    void __iomem* base_addr;
    int cxc_irq;

    struct tasklet_struct recv_taskq;

    struct clk *clk;
};


/* xpu-avc.c */
int xpu_avc_set_pid(struct xpudtv *xdtv, unsigned char pidc, u16 pid[], u16 type, struct dmx_section_filter filter);
int xpu_avc_clr_pid(struct xpudtv *xdtv, unsigned char pidc, u16 pid[]);
int xpu_avc_recv(struct xpudtv *xdtv);
int xpu_avc_start_push_ts(struct xpudtv *xdtv);
int xpu_avc_start_capture(struct xpudtv *xdtv, unsigned int pes_phy_addr, unsigned int sec_phy_addr);
int xpu_avc_pes_cb(struct xpudtv *xdtv);
int xpu_avc_sec_cb(struct xpudtv *xdtv);
int xpu_avc_ca_reset(struct xpudtv *xdtv);
int xpu_start_streaming(struct xpudtv *xdtv);
int xpu_stop_streaming(struct xpudtv *xdtv);

/* xpu-ci.c */
int xpu_ca_register(struct xpudtv *fdtv);
void xpu_ca_release(struct xpudtv *fdtv);

/* xpu-dvb.c */
int xpu_start_feed(struct dvb_demux_feed *dvbdmxfeed);
int xpu_stop_feed(struct dvb_demux_feed *dvbdmxfeed);
int xpu_dvb_register(struct xpudtv *xdtv);
void xpu_dvb_unregister(struct xpudtv *xdtv);

#endif /* _XPUDTV_H */
