/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009  Hangzhou C-SKY Microsystems co.,ltd.
 */

#ifndef _CSKY_CACHECTL_H
#define _CSKY_CACHECTL_H

/*
 * Options for cacheflush system call
 *
 * cacheflush() is currently fluch_cache_all().
 */
#define ICACHE  (1<<0)      /* flush instruction cache        */
#define DCACHE  (1<<1)      /* writeback and flush data cache */
#define BCACHE  (ICACHE|DCACHE) /* flush both caches              */

/*
 * Caching modes for the cachectl(2) call
 *
 * cachectl(2) is currently not supported and returns ENOSYS.
 */
#define CACHEABLE   0   /* make pages cacheable */
#define UNCACHEABLE 1   /* make pages uncacheable */

#endif /* _CSKY_CACHECTL_H */
