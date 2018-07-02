/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * PROM library functions for acquiring/using memory descriptors given to
 * us from the YAMON.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/pfn.h>
#include <linux/string.h>
#include <linux/spinlock.h>

#include <asm/page.h>
#include <silan_def.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
extern struct android_pmem_platform_data android_pmem_pdata;
#endif

extern char * __init prom_getcmdline(void);

/* Spinlock for prom allocator */
DEFINE_SPINLOCK(prom_lock);
/* determined physical memory size, not overridden by command line args  */
static u32 prom_phy_mem_start = 0;
static u32 prom_phy_mem_size = 0;

void __init prom_meminit(void)
{
	char *memsize_str;
	char *argptr;
	int have_mem_region = 0;
	u32 physical_memsize = 0L;
	u32 start_addr = 0x0;
	u32 mem_size = (SILAN_PHY_MEM_SIZE > HIGHMEM_START) ? HIGHMEM_START : SILAN_PHY_MEM_SIZE;

	/* Reserve the register space and reset vector */
	if (mem_size > SILAN_REG_SPACE_START)
		mem_size = SILAN_REG_SPACE_START;

	/* Get user configured low memory size */
    argptr = prom_getcmdline();
    memsize_str = strstr(argptr,"mem=");
    if (!memsize_str) {
        physical_memsize = 0x04000000;
        have_mem_region = 0;
    } else {
        memsize_str += strlen("mem=");
        physical_memsize = memparse(memsize_str,NULL);
        have_mem_region = 1;
    }
	
	/* User configured low memory */
	printk("User configured memsize 0x%x\n", physical_memsize);
	if (physical_memsize > mem_size) {
		printk("WARN: cut user configured memsize 0x%x to 0x%x\n",
				physical_memsize, mem_size);
		physical_memsize = mem_size;
	}
	if (have_mem_region == 0)
		add_memory_region(start_addr, physical_memsize, BOOT_MEM_RAM);
	mem_size -= physical_memsize;
	start_addr += physical_memsize;

	/* pmem */
#ifdef CONFIG_ANDROID_PMEM
	if (android_pmem_pdata.size > mem_size) {
		printk("WARN: cut android pmem size 0x%lx to 0x%x\n",
				android_pmem_pdata.size, mem_size);
		android_pmem_pdata.size = mem_size;
	}
	android_pmem_pdata.start = start_addr;
	printk("pmem start addr 0x%lx, size 0x%lx\n",
			android_pmem_pdata.start, android_pmem_pdata.size);
	mem_size -= android_pmem_pdata.size;
	start_addr += android_pmem_pdata.size;
#endif

	/* prom phy mem */
	prom_phy_mem_start = start_addr;
	prom_phy_mem_size = mem_size;
	printk("prom start addr 0x%x, size 0x%x\n",
			prom_phy_mem_start, prom_phy_mem_size);
}

u32 prom_phy_mem_malloc(u32 size, int id)
{
	u32 phy_addr;

	size = (size+PAGE_SIZE-1)&PAGE_MASK;
	if (size <= prom_phy_mem_size) {
		spin_lock(&prom_lock);
		phy_addr = prom_phy_mem_start;
		prom_phy_mem_start += size;
		prom_phy_mem_size -= size;
		spin_unlock(&prom_lock);
		printk("prom malloc: free pool = %d Kbytes\n", prom_phy_mem_size/1024);
		return phy_addr;
	}
	printk("Error: unbale to alloc 0x%x bytes from prom mem pool = 0x%x bytes\n", size, prom_phy_mem_size);
	return 0;
}
EXPORT_SYMBOL(prom_phy_mem_malloc);

