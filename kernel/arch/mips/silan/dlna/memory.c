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

#include <asm/bootinfo.h>
#include <asm/page.h>
#include <asm/sections.h>

#include <asm/mips-boards/prom.h>
#include <silan_def.h>
#include <silan_memory.h>

/* determined physical memory size, not overridden by command line args  */
static u32 physical_memsize = 0L;
static u32 prom_phy_mem_start = 0;
static u32 prom_phy_mem_size = SILAN_PROM_PHY_MEM_SIZE;

void __init prom_meminit(void)
{
	char *memsize_str;
	char *argptr;
	int have_mem_region = 0;
	unsigned int memsize;    
	
	/* otherwise look in the environment */
	memsize_str = prom_getenv("memsize");
	if (!memsize_str) 
	{
		argptr = prom_getcmdline();
		memsize_str = strstr(argptr,"mem=");
		if(!memsize_str)
		{
			physical_memsize = 0x04000000;
			have_mem_region = 0;
		}
		else
		{
			memsize_str += strlen("mem=");
			physical_memsize = memparse(memsize_str,NULL);
			have_mem_region = 1;
		}
	}
	else 
	{
		physical_memsize = simple_strtol(memsize_str, NULL, 0);
	}	

	memsize = physical_memsize;
	prom_phy_mem_start = physical_memsize;
	if(have_mem_region == 0)
		add_memory_region(0, memsize, BOOT_MEM_RAM);
}

void __init prom_free_prom_memory(void)
{
	unsigned long addr;
	int i;

	for (i = 0; i < boot_mem_map.nr_map; i++) {
		if (boot_mem_map.map[i].type != BOOT_MEM_ROM_DATA)
			continue;

		addr = boot_mem_map.map[i].addr;
		free_init_pages("prom memory",
				addr, addr + boot_mem_map.map[i].size);
	}
}    

u32 prom_phy_mem_malloc(u32 size, int id)
{
	u32 phy_addr;
	size = (size+PAGE_SIZE-1)&PAGE_MASK;
	if(size <= prom_phy_mem_size)
	{
		phy_addr = prom_phy_mem_start;
		prom_phy_mem_start += size;
		prom_phy_mem_size -= size;
		return phy_addr;
	}
	return 0;
}
EXPORT_SYMBOL(prom_phy_mem_malloc);

u32 prom_pmem_start_addr(void)
{
	return (physical_memsize+SILAN_PROM_PHY_MEM_SIZE);
}

