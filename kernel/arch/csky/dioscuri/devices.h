/* linux/arch/csky/dioscuri/devices.h
 *
 * Copyright (C) 2010 C-SKY Microsystems Co., Ltd.
 * Author: Hu junshan <junshan_hu@c-sky.com> 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ARCH_CSKY_MACH_DIOSCURI_DEVICES_H
#define __ARCH_CSKY_MACH_DIOSCURI_DEVICES_H

extern struct platform_device csky_device_uart1;
extern struct platform_device csky_device_uart2;

extern struct platform_device csky_device_nand;
extern struct platform_device csky_device_usb;
extern struct platform_device csky_device_lcd;
extern struct platform_device csky_device_nor;

extern struct platform_device csky_gpio_keys_device;

extern struct platform_device csky_device_sdhc;
extern struct platform_device csky_maceth_device;
extern struct platform_device csky_macphy_device;

extern struct platform_device csky_device_rtc;
extern struct platform_device csky_device_i2c1;
extern struct platform_device csky_device_i2c2;
#endif
