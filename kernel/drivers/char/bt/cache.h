/****************************************************************************
 * arch/csky/src/cskyv1/cache.h
 *
  * Copyright (C) 2015 The YunOS Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

#ifndef __ARCH_CSKY_SRC_CSKYV1_CACHE_H
#define __ARCH_CSKY_SRC_CSKYV1_CACHE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Defintiions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void flush_cache_all(void);
void flush_icache_all(void);
void flush_dcache_all(void);
void clear_dcache_all(void);

#define flush_cache_range(start, end)   flush_cache_all()
#define flush_icache_range(start, end)  flush_icache_all()
#define flush_dcache_range(start, end)  flush_dcache_all()
#define clear_dcache_range(start, end)  clear_dcache_all()

#endif /* __ARCH_CSKY_SRC_CSKYV1_CACHE_H */

