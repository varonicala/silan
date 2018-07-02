/*
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012  Hangzhou C-SKY Microsystems co.,ltd.
 */

#ifndef __CKRTC_H__
#define __CKRTC_H__

#include <asm/addrspace.h>
#include <mach/ck_iomap.h>

/* RTC registers */
#define CK_RTC_CCVR_L	0x00
#define CK_RTC_CMR_L	0x04
#define CK_RTC_CLR_L	0x08
#define CK_RTC_CCR	0x0c
#define CK_RTC_STAT	0x10
#define CK_RTC_RSTAT	0x14
#define CK_RTC_EOI	0x18
#define CK_RTC_COMP_VERSION	0x1c

#define CK_RTC_CCVR_H	0x20
#define CK_RTC_CMR_H	0x24
#define CK_RTC_CLR_H	0x28

#endif
