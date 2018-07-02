/*
 * arch/csky/include/asm/io_mm.h
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file COPYING in the main directory of
 * this archive for more details.
 *
 * (C) Copyright 2009, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
 *  
 */

#ifndef _CSKY_IO_H
#define _CSKY_IO_H

#ifdef __KERNEL__

#include <linux/compiler.h>
#include <asm/virtconvert.h>
#include <asm/pgtable-bits.h>
#include <asm/cacheflush.h>
#include <asm/addrspace.h>
#include <asm-generic/iomap.h>
/*
 * These are for ISA/PCI shared memory _only_ and should never be used
 * on any other type of memory, including Zorro memory. They are meant to
 * access the bus in the bus byte order which is little-endian!.
 *
 * readX/writeX() are used to access memory mapped devices. On some
 * architectures the memory mapped IO stuff needs to be accessed
 * differently. On the csky architecture, we just read/write the
 * memory location directly.
 */
/* ++roman: The assignments to temp. vars avoid that gcc sometimes generates
 * two accesses to memory, which may be undesireable for some devices.
 */

/*
 * swap functions are sometimes needed to interface little-endian hardware
 */

/*
 * CHANGES
 * 
 * 020325   Added some #define's for the COBRA5272 board
 *          (hede)
 */
static inline unsigned short _swapw(volatile unsigned short v)
{
    return ((v << 8) | (v >> 8));
}

static inline unsigned int _swapl(volatile unsigned long v)
{
    return ((v << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | (v >> 24));
}

#define mmiowb()

#define readb(addr) \
    ({ unsigned char __v = (*(volatile unsigned char *) (addr)); __v; })
#define readw(addr) \
    ({ unsigned short __v = (*(volatile unsigned short *) (addr)); __v; })
#define readl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#define writeb(b,addr) (void)((*(volatile unsigned char *) (addr)) = (b))
#define writew(b,addr) (void)((*(volatile unsigned short *) (addr)) = (b))
#define writel(b,addr) (void)((*(volatile unsigned int *) (addr)) = (b))

/*
 * The following are some defines we need for MTD with our
 * COBRA5272 board.
 * Because I don't know if they break something I have
 * #ifdef'd them.
 * (020325 - hede)
 * As far as I can see they are safe, lets try
 * them always included - DAVIDM
 */
#define __raw_readb readb
#define __raw_readw readw
#define __raw_readl readl
#define __raw_writeb writeb
#define __raw_writew writew
#define __raw_writel writel

static inline void io_outsb(unsigned int addr, void *buf, int len)
{
	volatile unsigned char *ap = (volatile unsigned char *) addr;
	unsigned char *bp = (unsigned char *) buf;
	while (len--)
		*ap = *bp++;
}

static inline void io_outsw(unsigned int addr, void *buf, int len)
{
	volatile unsigned short *ap = (volatile unsigned short *) addr;
	unsigned short *bp = (unsigned short *) buf;
	while (len--)
		*ap = _swapw(*bp++);
}

static inline void io_outsl(unsigned int addr, void *buf, int len)
{
	volatile unsigned int *ap = (volatile unsigned int *) addr;
	unsigned int *bp = (unsigned int *) buf;
	while (len--)
		*ap = _swapl(*bp++);
}

static inline void io_insb(unsigned int addr, void *buf, int len)
{
	volatile unsigned char *ap = (volatile unsigned char *) addr;
	unsigned char *bp = (unsigned char *) buf;
	while (len--)
		*bp++ = *ap;
}

static inline void io_insw(unsigned int addr, void *buf, int len)
{
	volatile unsigned short *ap = (volatile unsigned short *) addr;
	unsigned short *bp = (unsigned short *) buf;
	while (len--)
		*bp++ = _swapw(*ap);
}

static inline void io_insl(unsigned int addr, void *buf, int len)
{
	volatile unsigned int *ap = (volatile unsigned int *) addr;
	unsigned int *bp = (unsigned int *) buf;
	while (len--)
		*bp++ = _swapl(*ap);
}

/*
 *	make the short names macros so specific devices
 *	can override them as required
 */

#define memset_io(a,b,c)	memset((void *)(a),(b),(c))
#define memcpy_fromio(a,b,c)	memcpy((a),(void *)(b),(c))
#define memcpy_toio(a,b,c)	memcpy((void *)(a),(b),(c))

#define inb(addr)      readb(addr)
#define inw(addr)    readw(addr)
#define inl(addr)    readl(addr)
#define outb(x,addr) ((void) writeb(x,addr))
#define outw(x,addr) ((void) writew(x,addr))
#define outl(x,addr) ((void) writel(x,addr))

#define inb_p(addr)    inb(addr)
#define inw_p(addr)    inw(addr)
#define inl_p(addr)    inl(addr)
#define outb_p(x,addr) outb(x,addr)
#define outw_p(x,addr) outw(x,addr)
#define outl_p(x,addr) outl(x,addr)

#define outsb(a,b,l) io_outsb(a,b,l)
#define outsw(a,b,l) io_outsw(a,b,l)
#define outsl(a,b,l) io_outsl(a,b,l)

#define insb(a,b,l) io_insb(a,b,l)
#define insw(a,b,l) io_insw(a,b,l)
#define insl(a,b,l) io_insl(a,b,l)

#define IO_SPACE_LIMIT 0xffff


/* Values for nocacheflag and cmode */
#define IOMAP_FULL_CACHING		0
#define IOMAP_NOCACHE_SER		1
#define IOMAP_NOCACHE_NONSER		2
#define IOMAP_WRITETHROUGH		3

extern void *__ioremap(phys_t phys_addr, phys_t size, unsigned long flags);
extern void __iounmap(void *addr, unsigned long size);
extern int remap_area_pages(unsigned long address, phys_t phys_addr,
        phys_t size, unsigned long flags);

extern void __iomem * __ioremap_mode(phys_t offset, unsigned long size,
	unsigned long flags);

extern inline void *ioremap(unsigned long physaddr, unsigned long size)
{
	return __ioremap_mode(physaddr, size, _CACHE_UNCACHED);
}
extern inline void *ioremap_nocache(unsigned long physaddr, unsigned long size)
{
	return __ioremap_mode(physaddr, size, _CACHE_UNCACHED);
}
extern inline void *ioremap_writethrough(unsigned long physaddr, unsigned long size)
{
	return __ioremap_mode(physaddr, size, IOMAP_WRITETHROUGH);
}
extern inline void *ioremap_fullcache(unsigned long physaddr, unsigned long size)
{
	return __ioremap_mode(physaddr, size, IOMAP_FULL_CACHING);
}
extern inline void *ioremap_cached(unsigned long physaddr, unsigned long size)
{
	return __ioremap_mode(physaddr, size, _CACHE_CACHED);
}

extern void iounmap(void *addr);

/* Nothing to do */

#ifdef CONFIG_CSKY_CACHE_LINE_FLUSH
#define dma_cache_inv(_start,_size)		__flush_dcache_range((unsigned long)(_start), (unsigned long)(_start) + (unsigned long)(_size))
#define dma_cache_wback(_start,_size)		clear_dcache_range((unsigned long)(_start), (unsigned long)(_size))
#define dma_cache_wback_inv(_start,_size)	__flush_dcache_range((unsigned long)(_start), (unsigned long)(_start) + (unsigned long)(_size))
#else
#define dma_cache_inv(_start,_size)		flush_dcache_all()
#define dma_cache_wback(_start,_size)		clear_dcache_all()
#define dma_cache_wback_inv(_start,_size)	flush_dcache_all()
#endif

#endif /* __KERNEL__ */

/*
 * Convert a physical pointer to a virtual kernel pointer for /dev/mem
 * access
 */
#define xlate_dev_mem_ptr(p)    __va(p)

/*
 * Convert a virtual cached pointer to an uncached pointer
 */
#define xlate_dev_kmem_ptr(p)   p


#endif /* _CSKY_IO_H */
