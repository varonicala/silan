/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Ye Yun (yun_ye@c-sky.com)
 */
#ifndef __ASM_OUTERCACHE_MM_H
#define __ASM_OUTERCACHE_MM_H

#include <linux/compiler.h>
#include <asm/cache.h>

#ifdef CONFIG_CSKY_L2_CACHE
/* 
 * Cache flushing:
 *  - flush_icache_range(start, end) flush a range of instructions
 */

extern void __flush_l2_all(unsigned long value);
extern void __flush_l2_cache_range(unsigned long start, unsigned long end, unsigned long value);
extern void __flush_l2_disable(void);

static inline void outer_inv_range(unsigned long start, unsigned long end){
	__flush_l2_cache_range(start, end, L2_CACHE_INV);
}

static inline void outer_clean_range(unsigned long start, unsigned long end){
	__flush_l2_cache_range(start, end, L2_CACHE_CLR);
}

static inline void outer_flush_range(unsigned long start, unsigned long end){
	__flush_l2_cache_range(start, end, L2_CACHE_CLR | L2_CACHE_INV);
}

static inline void outer_flush_all(void){
	__flush_l2_all(L2_CACHE_CLR | L2_CACHE_INV);
}

static inline void outer_inv_all(void){
	__flush_l2_all(L2_CACHE_INV);
}

static inline void outer_disable(void){
	__flush_l2_disable();
}

#endif
#endif /* __ASM_OUTERCACHE_MM_H */
