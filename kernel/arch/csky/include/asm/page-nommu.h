/*
 * linux/arch/csky/include/asm/page-nommu.h
 *
 * Definitions for page handling
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2011 by Hangzhou C-SKY Microsystem co.,ltd
 *
 */
#ifndef __ASM_PAGE_NOMMU_H
#define __ASM_PAGE_NOMMU_H

/*
 * PAGE_SHIFT determines the page size
 */
#define PAGE_SHIFT	12

#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))
#define THREAD_SIZE (8192)

/*
 * NOTE: virtual isn't really correct, actually it should be the offset into the
 * memory node, but we have no highmem, so that works for now.
 * TODO: implement (fast) pfn<->pgdat_idx conversion functions, this makes lots
 * of the shifts unnecessary.
 */

#ifndef __ASSEMBLY__

#include <linux/pfn.h>

#define PHY_OFFSET				CK_RAM_BASE
#define ARCH_PFN_OFFSET 		PFN_UP(PHY_OFFSET)

#define virt_to_pfn(kaddr)      (__pa(kaddr) >> PAGE_SHIFT)
#define pfn_to_virt(pfn)        __va((pfn) << PAGE_SHIFT)

#define virt_addr_valid(kaddr)  ((void *)(kaddr) >= (void *)PAGE_OFFSET && \
                                 (void *)(kaddr) < high_memory)
#define pfn_valid(pfn)          virt_addr_valid(pfn_to_virt(pfn))

#define clear_page(page)        memset((page), 0, PAGE_SIZE)
#define copy_page(to,from)      memcpy((to), (from), PAGE_SIZE)

extern unsigned long shm_align_mask;

static inline unsigned long pages_do_alias(unsigned long addr1,
	unsigned long addr2)
{
	return (addr1 ^ addr2) & shm_align_mask;
}

struct page;
#define clear_user_page(addr, vaddr, page)      \
        do {                                    \
          clear_page(addr);                     \
        } while (0)

#define copy_user_page(to, from, vaddr, page)   \
        do {                                    \
          copy_page(to, from);                  \
        } while (0)

struct vm_area_struct;

/*
 * These are used to make use of C type-checking..
 */
typedef struct { unsigned long pte_low; } pte_t;
#define pte_val(x)    ((x).pte_low)

typedef struct { unsigned long pmd; } pmd_t;
typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long pgprot; } pgprot_t;
typedef struct page *pgtable_t;

#define pmd_val(x)	((x).pmd)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define ptep_buddy(x)	((pte_t *)((unsigned long)(x) ^ sizeof(pte_t)))

#define __pte(x)	((pte_t) { (x) } )
#define __pmd(x)	((pmd_t) { (x) } )
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )

#endif /* !__ASSEMBLY__ */

/*
 * This handles the memory map.
 * We handle pages at KSEG0 for kernels with 32 bit address space.
 */
#define	__PAGE_OFFSET	0x0
#define PHYS_OFFSET	0x0
/*
 * Memory above this physical address will be considered highmem.
 */
#define HIGHMEM_START   0xFFFFFFFFUL
#define PAGE_OFFSET	(__PAGE_OFFSET)

#define __pa(x)		((unsigned long) (x) - PAGE_OFFSET)
#define __va(x)		((void *)((unsigned long) (x) + PAGE_OFFSET))
#define __pa_symbol(x)  __pa(RELOC_HIDE((unsigned long)(x), 0))

#define MAP_NR(addr)       (((unsigned long)(addr)-PAGE_OFFSET-CK_RAM_BASE) \
                               >> PAGE_SHIFT)

#define virt_to_page(kaddr)	(mem_map + ((__pa(kaddr)-CK_RAM_BASE) \
                                    >> PAGE_SHIFT))

#define VALID_PAGE(page)	((page - mem_map) < max_mapnr)

#define VM_DATA_DEFAULT_FLAGS  (VM_READ | VM_WRITE | VM_EXEC | \
				VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)

#define UNCACHE_ADDR(addr)	((addr) - PAGE_OFFSET)
#define CACHE_ADDR(addr)		((addr) + PAGE_OFFSET)

/*
 * main RAM and kernel working space are coincident at 0x80000000, but to make
 * life more interesting, there's also an uncached virtual shadow at 0xb0000000
 * - these mappings are fixed in the MMU
 */

#define pfn_to_kaddr(pfn)       __va((pfn) << PAGE_SHIFT)

#include <asm-generic/memory_model.h>
#include <asm-generic/getorder.h>

#endif /* __ASM_PAGE_NOMMU_H */
