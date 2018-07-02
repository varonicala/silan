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

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <dmxdev.h>
#include <dvb_demux.h>
#include <dvbdev.h>
#include <dvb_frontend.h>

#include "xpudtv.h"

extern struct dvb_frontend *gx1001_attach(struct i2c_adapter *i2c_adap, u8 i2c_addr);

static int xpu_alloc_channel(struct xpudtv *xdtv)
{
    int i;

    for (i = 0; i < XPU_DTV_MAX_CHANNEL; i++)
        if (!__test_and_set_bit(i, &xdtv->channel_active))
            break;
    return i;
}

static void xpu_collect_channels(struct xpudtv *xdtv, int *pidc, u16 pid[])
{
    int i, n;

    for (i = 0, n = 0; i < XPU_DTV_MAX_CHANNEL; i++)
        if (test_bit(i, &xdtv->channel_active)) {
            pid[n++] = xdtv->channel_pid[i];
        }
    *pidc = (n == 0 ? n : n -1);
}

static inline void xpu_dealloc_channel(struct xpudtv *xdtv, int i)
{
    __clear_bit(i, &xdtv->channel_active);
}

int xpu_start_feed(struct dvb_demux_feed *dvbdmxfeed)
{
    struct xpudtv *xdtv = dvbdmxfeed->demux->priv;
    int pidc, c, ret;
    u16 pids[XPU_DTV_MAX_CHANNEL];
    u16 type = 0;

    switch (dvbdmxfeed->type) {
    case DMX_TYPE_TS:
    case DMX_TYPE_SEC:
        type = dvbdmxfeed->type;
        break;
    default:
        dev_err(xdtv->device, "can't start dmx feed: invalid type %u\n",
            dvbdmxfeed->type);
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&xdtv->demux_mutex))
        return -EINTR;

    if (dvbdmxfeed->type == DMX_TYPE_TS) {
        switch (dvbdmxfeed->pes_type) {
        case DMX_TS_PES_VIDEO:
        case DMX_TS_PES_AUDIO:
        case DMX_TS_PES_TELETEXT:
        case DMX_TS_PES_PCR:
        case DMX_TS_PES_OTHER:
            c = xpu_alloc_channel(xdtv);
            type = dvbdmxfeed->type | 0x80;
            break;
        default:
            dev_err(xdtv->device,
                "can't start dmx feed: invalid pes type %u\n",
                dvbdmxfeed->pes_type);
            ret = -EINVAL;
            goto out;
        }
    } else {
        c = xpu_alloc_channel(xdtv);
    }

    if (c > 15) {
        dev_err(xdtv->device, "can't start dmx feed: busy\n");
        ret = -EBUSY;
        goto out;
    }

    dvbdmxfeed->priv =(typeof(dvbdmxfeed->priv)) (unsigned long)c;
    xdtv->channel_pid[c] = dvbdmxfeed->pid;
    xdtv->handle2filter[c] =dvbdmxfeed->filter; 
    xpu_collect_channels(xdtv, &pidc, pids);

    ret = xpu_avc_set_pid(xdtv, pidc, pids, type, dvbdmxfeed->filter->filter);
    if (ret) {
        xpu_dealloc_channel(xdtv, c);
        dev_err(xdtv->device, "can't set PIDs\n");
        goto out;
    }

out:
    mutex_unlock(&xdtv->demux_mutex);

    return ret;
}

int xpu_stop_feed(struct dvb_demux_feed *dvbdmxfeed)
{
    struct dvb_demux *demux = dvbdmxfeed->demux;
    struct xpudtv *xdtv = demux->priv;
    int pidc, c, ret;
    u16 pids[XPU_DTV_MAX_CHANNEL];

    if (dvbdmxfeed->type == DMX_TYPE_TS &&
        !((dvbdmxfeed->ts_type & TS_PACKET) &&
          (demux->dmx.frontend->source != DMX_MEMORY_FE))) {

        if (dvbdmxfeed->ts_type & TS_DECODER) {
            if (dvbdmxfeed->pes_type >= DMX_TS_PES_OTHER ||
                !demux->pesfilter[dvbdmxfeed->pes_type])
                return -EINVAL;

            demux->pids[dvbdmxfeed->pes_type] |= 0x8000;
            demux->pesfilter[dvbdmxfeed->pes_type] = NULL;
        }

        if (!(dvbdmxfeed->ts_type & TS_DECODER &&
              dvbdmxfeed->pes_type < DMX_TS_PES_OTHER))
            return 0;
    }

    if (mutex_lock_interruptible(&xdtv->demux_mutex))
        return -EINTR;

    c = (unsigned long)dvbdmxfeed->priv;
    xpu_collect_channels(xdtv, &pidc, pids);
    xpu_dealloc_channel(xdtv, c);
    xdtv->handle2filter[c] =NULL; 

    ret = xpu_avc_clr_pid(xdtv, pidc, pids);

    mutex_unlock(&xdtv->demux_mutex);

    return ret;
}

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

int xpu_dvb_register(struct xpudtv *xdtv)
{
    int err;

    err = dvb_register_adapter(&xdtv->adapter, "SILAN XPU",
                   THIS_MODULE, xdtv->device, adapter_nr);
    if (err < 0)
        goto fail_log;

    /*DMX_TS_FILTERING | DMX_SECTION_FILTERING*/
    xdtv->demux.dmx.capabilities = 0;

    xdtv->demux.priv    = xdtv;
    xdtv->demux.filternum    = XPU_DTV_MAX_CHANNEL;
    xdtv->demux.feednum    = XPU_DTV_MAX_CHANNEL;
    xdtv->demux.start_feed    = xpu_start_feed;
    xdtv->demux.stop_feed    = xpu_stop_feed;
    xdtv->demux.write_to_decoder = NULL;

    err = dvb_dmx_init(&xdtv->demux);
    if (err)
        goto fail_unreg_adapter;

    xdtv->dmxdev.filternum    = XPU_DTV_MAX_CHANNEL;
    xdtv->dmxdev.demux        = &xdtv->demux.dmx;
    xdtv->dmxdev.capabilities = 0;

    err = dvb_dmxdev_init(&xdtv->dmxdev, &xdtv->adapter);
    if (err)
        goto fail_dmx_release;

    xdtv->frontend.source = DMX_FRONTEND_0;

    err = xdtv->demux.dmx.add_frontend(&xdtv->demux.dmx, &xdtv->frontend);
    if (err)
        goto fail_dmxdev_release;

    err = xdtv->demux.dmx.connect_frontend(&xdtv->demux.dmx,
                           &xdtv->frontend);
    if (err)
        goto fail_rem_frontend;

    dvb_net_init(&xdtv->adapter, &xdtv->dvbnet, &xdtv->demux.dmx);

    xdtv->fe = dvb_attach(gx1001_attach, xdtv->i2c, 0x18);
 
    err = dvb_register_frontend(&xdtv->adapter, xdtv->fe);
    if (err)
        goto fail_net_release;

    err = xpu_ca_register(xdtv);
    if (err)
        dev_info(xdtv->device,
             "Conditional Access Module not enabled\n");
    return 0;

fail_net_release:
    dvb_net_release(&xdtv->dvbnet);
    xdtv->demux.dmx.close(&xdtv->demux.dmx);
fail_rem_frontend:
    xdtv->demux.dmx.remove_frontend(&xdtv->demux.dmx, &xdtv->frontend);
fail_dmxdev_release:
    dvb_dmxdev_release(&xdtv->dmxdev);
fail_dmx_release:
    dvb_dmx_release(&xdtv->demux);
fail_unreg_adapter:
    dvb_unregister_adapter(&xdtv->adapter);
fail_log:
    dev_err(xdtv->device, "DVB initialization failed\n");
    return err;
}

void xpu_dvb_unregister(struct xpudtv *xdtv)
{
    xpu_ca_release(xdtv);
    dvb_unregister_frontend(xdtv->fe);
    dvb_net_release(&xdtv->dvbnet);
    xdtv->demux.dmx.close(&xdtv->demux.dmx);
    xdtv->demux.dmx.remove_frontend(&xdtv->demux.dmx, &xdtv->frontend);
    dvb_dmxdev_release(&xdtv->dmxdev);
    dvb_dmx_release(&xdtv->demux);
    dvb_unregister_adapter(&xdtv->adapter);
}
