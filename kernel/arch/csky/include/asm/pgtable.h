/*
 * linux/arch/include/asm/pgtable_mm.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009  Hangzhou C-SKY Microsystemsi,.Ltd
 * Copyright (C) 2009  Ye Yun (yun_ye@c-sky.com)
 */

#ifndef _ASM_PGTABLE_H
#define _ASM_PGTABLE_H

#ifndef CONFIG_MMU

#include "pgtable-nommu.h"

#else

#include <asm/addrspace.h>
#include <asm-generic/4level-fixup.h>
#include <asm/fixmap.h>

#ifndef __ASSEMBLY__

#include <asm/pgtable-bits.h>
#include <asm/virtconvert.h>
/*
 * This flag is used to indicate that the page pointed to by a pte
 * is dirty and requires cleaning before returning it to the user.
 */
#define PG_dcache_dirty			PG_arch_1

#define Page_dcache_dirty(page)		\
	test_bit(PG_dcache_dirty, &(page)->flags)
#define SetPageDcacheDirty(page)	\
	set_bit(PG_dcache_dirty, &(page)->flags)
#define ClearPageDcacheDirty(page)	\
	clear_bit(PG_dcache_dirty, &(page)->flags)

/*
 * Certain architectures need to do special things when pte's
 * within a page table are directly modified.  Thus, the following
 * hook is made available.
 */
#ifdef CONFIG_MMU_HARD_REFILL

#define set_pte(pteptr, pteval)                                 \
        do{                                                     \
                *(pteptr) = (pteval);                           \
                clear_dcache_range((unsigned long)pteptr, 4);   \
        } while(0)
#define set_pte_at(mm,addr,ptep,pteval) set_pte(ptep,pteval)

static inline pte_t *pmd_page_vaddr(pmd_t pmd)
{
	unsigned long ptr;

	ptr = pmd_val(pmd);
	
	return __va(ptr);
}

#define pmd_phys(pmd)           pmd_val(pmd)
#else   /* CONFIG_MMU_HARD_REFILL */

#define set_pte(pteptr, pteval)                                 \
        do{                                                     \
                *(pteptr) = (pteval);                           \
        } while(0)
#define set_pte_at(mm,addr,ptep,pteval) set_pte(ptep,pteval)

static inline pte_t *pmd_page_vaddr(pmd_t pmd)
{
	unsigned long ptr;

	ptr = pmd_val(pmd);

	return (pte_t *)ptr;
}

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */
#define pmd_phys(pmd)           virt_to_phys((void *)pmd_val(pmd))
#endif /* CONFIG_MMU_HARD_REFILL */

#define pmd_page(pmd)           (pfn_to_page(pmd_phys(pmd) >> PAGE_SHIFT))

#endif /* !defined (__ASSEMBLY__) */

/* 
 * Basically we have the same two-level (which is the logical three level
 * Linux page table layout folded) page tables as the i386.  Some day
 * when we have proper page coloring support we can have a 1% quicker
 * tlb refill handling mechanism, but for now it is a bit slower but
 * works even with the cache aliasing problem the R4k and above have.
 */

#ifndef PUD_SHIFT
#define PUD_SHIFT      22
#endif
/* PMD_SHIFT determines the size of the area a second-level page table can map*/
#define PMD_SHIFT      22 
#define PMD_SIZE	(1UL << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE-1))

/* PGDIR_SHIFT determines what a third-level page table entry can map */
#define PGDIR_SHIFT	PMD_SHIFT
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

#define USER_PTRS_PER_PGD	(0x80000000UL/PGDIR_SIZE)
#define FIRST_USER_ADDRESS      0

#define VMALLOC_START     (KSEG2 + 0x8000)

#define PKMAP_BASE		(0xfe000000UL)

#ifdef CONFIG_HIGHMEM
# define VMALLOC_END	(PKMAP_BASE-2*PAGE_SIZE)
#else
# define VMALLOC_END	(FIXADDR_START-2*PAGE_SIZE)
#endif
/*
 * traditional  two-level paging structure:
 */
#define PGD_ORDER       0
#define PTE_ORDER       0

#define pte_ERROR(e) \
        printk("%s:%d: bad pte %08lx.\n", __FILE__, __LINE__, (e).pte_low)
#define pmd_ERROR(e) \
        printk("%s:%d: bad pmd %08lx.\n", __FILE__, __LINE__, pmd_val(e))
#define pgd_ERROR(e) \
        printk("%s:%d: bad pgd %08lx.\n", __FILE__, __LINE__, pgd_val(e))

/* Find an entry in the third-level page table.. */
#define __pte_offset_t(address)                                           \
        (((address) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
#define pte_offset_kernel(dir, address)                                 \
        (pmd_page_vaddr(*(dir)) + __pte_offset_t(address))
#define pte_offset_map(dir, address)                                    \
        ((pte_t *)page_address(pmd_page(*(dir))) + __pte_offset_t(address))

#define PAGE_NONE	__pgprot(_PAGE_PRESENT | PAGE_CACHABLE_DEFAULT)
#define PAGE_SHARED     __pgprot(_PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | \
			PAGE_CACHABLE_DEFAULT)
#define PAGE_COPY       __pgprot(_PAGE_PRESENT | _PAGE_READ | \
			PAGE_CACHABLE_DEFAULT)
#define PAGE_READONLY   __pgprot(_PAGE_PRESENT | _PAGE_READ | \
			PAGE_CACHABLE_DEFAULT)
#define PAGE_KERNEL	__pgprot(_PAGE_PRESENT | __READABLE | __WRITEABLE | \
			_PAGE_GLOBAL | PAGE_CACHABLE_DEFAULT)
#define PAGE_USERIO     __pgprot(_PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | \
			PAGE_CACHABLE_DEFAULT)
#define PAGE_KERNEL_UNCACHED __pgprot(_PAGE_PRESENT | __READABLE | \
			__WRITEABLE | _PAGE_GLOBAL | _CACHE_UNCACHED)

#define pte_clear(mm,addr,ptep)         set_pte((ptep), __pte(0))
#define pte_none(pte)           (!pte_val(pte))
#define pte_present(pte)        (pte_val(pte) & _PAGE_PRESENT )
#define pte_pfn(x)              ((unsigned long)((x).pte_low >> PAGE_SHIFT))
#define pfn_pte(pfn, prot)    __pte(((unsigned long long)(pfn) << PAGE_SHIFT)\
                                    | pgprot_val(prot))
#ifdef CONFIG_CPU_MMU_V1
static inline unsigned long pte_to_pgoff(pte_t pte)
{
        return pte.pte_low >> 4;
}

static inline pte_t pgoff_to_pte(unsigned off)
{
        pte_t pte = { (off << 4) + _PAGE_FILE };
        return pte;
}
#else
/*
 * Bits 10(_PAGE_PRESENT) and 11(_PAGE_FILE)are taken,
 * split up 30 bits of offset into this range:
 */
#define pte_to_pgoff(_pte)		\
	(((_pte).pte_low & 0x3ff) | (((_pte).pte_low >> 12) << 10))
#define pgoff_to_pte(off)		\
	((pte_t) {((off) & 0x3ff) | (((off) >> 10) << 12) | _PAGE_FILE})
#endif

#define PTE_FILE_MAX_BITS 28

#define pte_unmap(pte) ((void)(pte))

#define __swp_type(x)           (((x).val >> 4) & 0xff)
#define __swp_offset(x)         ((x).val >> 12)
#define __swp_entry(type, offset)    ((swp_entry_t) { ((type) << 4) | \
                                        ((offset) << 12) })
#define __pte_to_swp_entry(pte) ((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)   ((pte_t) { (x).val })

#define pte_page(x)             pfn_to_page(pte_pfn(x))
#define __mk_pte(page_nr,pgprot)    __pte(((page_nr) << PAGE_SHIFT) | \
                                     pgprot_val(pgprot))

/*
 * CSKY can't do page protection for execute, and considers that the same like
 * read. Also, write permissions imply read permissions. This is the closest
 * we can get by reasonable means..
 */
#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY
#define __P100	PAGE_READONLY
#define __P101	PAGE_READONLY
#define __P110	PAGE_COPY
#define __P111	PAGE_COPY

#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY
#define __S010	PAGE_SHARED
#define __S011	PAGE_SHARED
#define __S100	PAGE_READONLY
#define __S101	PAGE_READONLY
#define __S110	PAGE_SHARED
#define __S111	PAGE_SHARED

#ifndef __ASSEMBLY__

extern unsigned long empty_zero_page;
extern unsigned long zero_page_mask;

#define ZERO_PAGE(vaddr)    (virt_to_page(empty_zero_page + \
                               (((unsigned long)(vaddr)) & zero_page_mask)))

extern void load_pgd(unsigned long pg_dir);

extern pmd_t invalid_pte_table[PAGE_SIZE/sizeof(pmd_t)];

static inline int pte_special(pte_t pte)        { return 0; }
static inline pte_t pte_mkspecial(pte_t pte)    { return pte; }

/*
 * Empty pgd/pmd entries point to the invalid_pte_table.
 */
#ifdef CONFIG_MMU_HARD_REFILL
#include <asm/cacheflush.h>
static inline void set_pmd(pmd_t *pmdptr, pmd_t pmdval)
{
	*pmdptr = pmdval;
	clear_dcache_range((unsigned long)pmdptr, 4);
}

static inline void set_pgd(pgd_t *pgdptr, pgd_t pgdval)
{
	*pgdptr = pgdval;
	clear_dcache_range((unsigned long)pgdptr, 4);
}

static inline int pmd_none(pmd_t pmd)
{
	return pmd_val(pmd) == __pa(invalid_pte_table);
}

#define pmd_bad(pmd)        (pmd_val(pmd) & ~PAGE_MASK)

static inline int pmd_present(pmd_t pmd)
{
        return (pmd_val(pmd) != __pa(invalid_pte_table));
}

static inline void pmd_clear(pmd_t *pmdp)
{
        pmd_val(*pmdp) = (__pa(invalid_pte_table));
	clear_dcache_range((unsigned long)pmdp, 4);
}
#else

/*
 * (pmds are folded into pgds so this doesn't get actually called,
 * but the define is needed for a generic inline function.)
 */
#define set_pmd(pmdptr, pmdval) (*(pmdptr) = pmdval)
#define set_pgd(pgdptr, pgdval) (*(pgdptr) = pgdval)

static inline int pmd_none(pmd_t pmd)
{
	return pmd_val(pmd) == (unsigned long) invalid_pte_table;
}

#define pmd_bad(pmd)        (pmd_val(pmd) & ~PAGE_MASK)

static inline int pmd_present(pmd_t pmd)
{
	return (pmd_val(pmd) != (unsigned long) invalid_pte_table);
}

static inline void pmd_clear(pmd_t *pmdp)
{
	pmd_val(*pmdp) = ((unsigned long) invalid_pte_table);
}
#endif

/*
 * The "pgd_xxx()" functions here are trivial for a folded two-level
 * setup: the pgd is never bad, and a pmd always exists (as it's folded
 * into the pgd entry)
 */
#define  pgd_none(pgd)                (0)
#define  pgd_bad(pgd)                (0)
static inline int pgd_present(pgd_t pgd)	{ return 1; }
static inline void pgd_clear(pgd_t *pgdp)	{ }

/*
 * The following only work if pte_present() is true.
 * Undefined behaviour if not..
 */

static inline int pte_read(pte_t pte)	
{
    return (pte).pte_low & _PAGE_READ; 
}

static inline int pte_write(pte_t pte)	
{
    return (pte).pte_low & _PAGE_WRITE; 
}

static inline int pte_dirty(pte_t pte)	
{ 
    return (pte).pte_low & _PAGE_MODIFIED; 
}

static inline int pte_young(pte_t pte)	
{ 
    return (pte).pte_low & _PAGE_ACCESSED; 
}

static inline int pte_file(pte_t pte)   
{
    return pte_val(pte) & _PAGE_FILE; 
}

static inline pte_t pte_wrprotect(pte_t pte)    
{ 
    pte_val(pte) &= ~(_PAGE_WRITE | _PAGE_SILENT_WRITE);
    return pte; 
}

static inline pte_t pte_mkclean(pte_t pte)      
{
    pte_val(pte) &= ~(_PAGE_MODIFIED|_PAGE_SILENT_WRITE);
    return pte; 
}

static inline pte_t pte_mkold(pte_t pte)        
{ 
    pte_val(pte) &= ~(_PAGE_ACCESSED|_PAGE_SILENT_READ);
    return pte; 
}

static inline pte_t pte_mkwrite(pte_t pte)      
{ 
    pte_val(pte) |= _PAGE_WRITE;
    if (pte_val(pte) & _PAGE_MODIFIED)
        pte_val(pte) |= _PAGE_SILENT_WRITE;
    return pte; 
}

static inline pte_t pte_mkdirty(pte_t pte)      
{ 
    pte_val(pte) |= _PAGE_MODIFIED;
    if (pte_val(pte) & _PAGE_WRITE)
        pte_val(pte) |= _PAGE_SILENT_WRITE;
    return pte; 
}

static inline pte_t pte_mkyoung(pte_t pte)      
{ 
    pte_val(pte) |= _PAGE_ACCESSED;
    if (pte_val(pte) & _PAGE_READ)
        pte_val(pte) |= _PAGE_SILENT_READ;
    return pte; 
}

#define PGD_T_LOG2	ffz(~sizeof(pgd_t))
#define PMD_T_LOG2	ffz(~sizeof(pmd_t))
#define PTE_T_LOG2	ffz(~sizeof(pte_t))

#define PTRS_PER_PGD	((PAGE_SIZE << PGD_ORDER) / sizeof(pgd_t))
#define PTRS_PER_PMD	1
#define PTRS_PER_PTE	((PAGE_SIZE << PTE_ORDER) / sizeof(pte_t))

#define page_pte(page) page_pte_prot(page, __pgprot(0))

#define __pgd_offset(address)	pgd_index(address)
#define __pud_offset(address)	(((address) >> PUD_SHIFT) & (PTRS_PER_PUD-1))
#define __pmd_offset(address)	(((address) >> PMD_SHIFT) & (PTRS_PER_PMD-1))

/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(address) pgd_offset(&init_mm, address)

#define pgd_index(address)	((address) >> PGDIR_SHIFT)

/*
 * Macro to make mark a page protection value as "uncacheable".  Note
 * that "protection" is really a misnomer here as the protection value
 * contains the memory attribute bits, dirty bits, and various other
 * bits as well.
 */
#define pgprot_noncached pgprot_noncached

static inline pgprot_t pgprot_noncached(pgprot_t _prot)
{
        unsigned long prot = pgprot_val(_prot);

        prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;

        return __pgprot(prot);
}


/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */
#define mk_pte(page, pgprot)    pfn_pte(page_to_pfn(page), (pgprot))
static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
        return __pte((pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot));
}

/* to find an entry in a page-table-directory */
static inline pgd_t *pgd_offset(struct mm_struct *mm, unsigned long address)
{
	return mm->pgd + pgd_index(address);
}

/* Find an entry in the second-level page table.. */
static inline pmd_t *pmd_offset(pgd_t *dir, unsigned long address)
{
	return (pmd_t *) dir;
}

/* Find an entry in the third-level page table.. */
static inline pte_t *pte_offset(pmd_t * dir, unsigned long address)
{
	return (pte_t *) (pmd_page_vaddr(*dir)) +
	       ((address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1));
}

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];
extern void paging_init(void);

extern void __update_tlb(struct vm_area_struct *vma, unsigned long address,
	pte_t pte);
extern void __update_cache(struct vm_area_struct *vma, unsigned long address,
	pte_t pte);
extern void show_jtlb_table(void);

static inline void update_mmu_cache(struct vm_area_struct *vma,
	unsigned long address, pte_t *ptep)
{
	pte_t pte = *ptep;
	__update_tlb(vma, address, pte);
	__update_cache(vma, address, pte);
}

/* Needs to be defined here and not in linux/mm.h, as it is arch dependent */
#define PageSkip(page)		(0)
#define kern_addr_valid(addr)	(1)

#include <asm-generic/pgtable.h>

#endif /* ifndef (__ASSEMBLY__) */

/*
 * We provide our own get_unmapped area to cope with the virtual aliasing
 * constraints placed on us by the cache architecture.
 */
#define HAVE_ARCH_UNMAPPED_AREA

/*
 * No page table caches to initialise
 */
#define pgtable_cache_init()	do { } while (0)

#define io_remap_pfn_range(vma, vaddr, pfn, size, prot)         \
                remap_pfn_range(vma, vaddr, pfn, size, prot)

#endif /* CONFIG_MMU */

#endif /* _ASM_PGTABLE_H */
