/************************************************************************************
 *arch/cskyv1/src/cskyv1/up_cache.c
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
 ************************************************************************************/


void flush_cache_all(void)
{
    int value = 0x33;
    __asm__ __volatile__("mtcr %0,cr17\n\t"
                         "sync\n\t"
                         : :"r" (value));
}

void flush_icache_all(void)
{
    int value = 0x11;
    __asm__ __volatile__("mtcr %0,cr17\n\t"
                         "sync\n\t"
                         : :"r" (value));
}

void flush_dcache_all(void)
{
    int value = 0x32;
    __asm__ __volatile__("mtcr %0,cr17\n\t"
                         "sync\n\t"
                         : :"r" (value));
}

void clear_dcache_all(void)
{
    int value = 0x22;
    __asm__ __volatile__("mtcr %0,cr17\n\t"
                         "sync\n\t"
                         : :"r" (value));
}
