/*
 * XPUDTV driver
 *
 * Copyright (C) 2012 panjianguang <panjianguang@silan.com.cn>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation; either version 2 of
 *	the License, or (at your option) any later version.
 */

#include <linux/device.h>
#include <linux/dvb/ca.h>
#include <linux/fs.h>
#include <linux/module.h>

#include <dvbdev.h>

#include "xpudtv.h"

static int xpu_ca_reset(struct xpudtv *xdtv)
{
	return xpu_avc_ca_reset(xdtv) ? -EFAULT : 0;
}

static int xpu_ca_get_caps(void *arg)
{
	struct ca_caps *cap = arg;

	cap->slot_num = 1;
	cap->slot_type = CA_CI;
	cap->descr_num = 1;
	cap->descr_type = CA_ECD;
	return 0;
}

static int xpu_ca_get_slot_info(struct xpudtv *xdtv, void *arg)
{
	struct ca_slot_info *slot = arg;

	if (slot->num != 0)
		return -EFAULT;

	slot->type = CA_CI;
	slot->flags = CA_CI_MODULE_PRESENT | CA_CI_MODULE_READY;
	return 0;
}

static int xdtv_ca_get_msg(struct xpudtv *xdtv, void *arg)
{

	return 0;
}

static int xdtv_ca_send_msg(struct xpudtv *xdtv, void *arg)
{

	return 0;
}

static int xpu_ca_ioctl(struct file *file, unsigned int cmd, void *arg)
{
	struct dvb_device *dvbdev = file->private_data;
	struct xpudtv *xdtv = dvbdev->priv;
	int err = 0;

	switch (cmd) {
	case CA_RESET:
		err = xpu_ca_reset(xdtv);
		break;
	case CA_GET_CAP:
		err = xpu_ca_get_caps(arg);
		break;
	case CA_GET_SLOT_INFO:
		err = xpu_ca_get_slot_info(xdtv,arg);
		break;
	case CA_GET_MSG:
		err = xdtv_ca_get_msg(xdtv,arg);
		break;
	case CA_SEND_MSG:
		err = xdtv_ca_send_msg(xdtv,arg);
		break;
	default:
		dev_info(xdtv->device, "unhandled CA ioctl %u\n", cmd);
		err = -EOPNOTSUPP;
	}

	return err;
}

static unsigned int xpu_ca_io_poll(struct file *file, poll_table *wait)
{
	return POLLIN;
}

static const struct file_operations xpu_ca_fops = 
{
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= dvb_generic_ioctl,
	.open		= dvb_generic_open,
	.release	= dvb_generic_release,
	.poll		= xpu_ca_io_poll,
	.llseek		= noop_llseek,
};

static struct dvb_device xpu_ca = 
{
	.users		= 1,
	.readers	= 1,
	.writers	= 1,
	.fops		= &xpu_ca_fops,
	.kernel_ioctl	= xpu_ca_ioctl,
};

int xpu_ca_register(struct xpudtv *xdtv)
{
	int err;

	err = dvb_register_device(&xdtv->adapter, &xdtv->cadev,
				  &xpu_ca, xdtv, DVB_DEVICE_CA);

	return err;
}

void xpu_ca_release(struct xpudtv *xdtv)
{
	if (xdtv->cadev)
		dvb_unregister_device(xdtv->cadev);
}
