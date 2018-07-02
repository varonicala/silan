/*
 * arch/csky/include/asm/kmap_types.h
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file COPYING in the main directory of
 * this archive for more details.
 *
 * (C) Copyright 2009, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
 *  
 */

#ifndef __ASM_CSKY_KMAP_TYPES_H
#define __ASM_CSKY_KMAP_TYPES_H

#ifdef CONFIG_DEBUG_HIGHMEM
#define  __WITH_KM_FENCE
#endif

#include <asm-generic/kmap_types.h>

#undef __WITH_KM_FENCE


#endif	/* __ASM_CSKY_KMAP_TYPES_H */
