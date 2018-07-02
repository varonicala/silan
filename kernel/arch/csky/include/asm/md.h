/*
 * arch/csky/include/asm/md.h
 *
 * High speed xor_block operation for RAID4/5
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file COPYING in the main directory of
 * this archive for more details.
 *
 * (C) Copyright 2009, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
 *	   $Id: md.h,v 1.1 2012/04/05 06:57:31 hujunshan Exp $  
 */

#ifndef __ASM_MD_H
#define __ASM_MD_H

/* #define HAVE_ARCH_XORBLOCK */

#define MD_XORBLOCK_ALIGNMENT	sizeof(long)

#endif /* __ASM_MD_H */
