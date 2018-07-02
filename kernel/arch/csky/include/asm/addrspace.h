/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009  Hangzhou C-SKY Microsystems co.,ltd.
 *
 * Copyright (C) 1996 by Ralf Baechle
 * Copyright (C) 2000, 2002  Maciej W. Rozycki
 *
 * Definitions for the address spaces of the CSKY CPUs.
 */
#ifndef __ASM_CK640_ADDRSPACE_H
#define __ASM_CK640_ADDRSPACE_H

#ifdef CONFIG_MMU
/*
 *  Configure language
 */
#ifdef __ASSEMBLY__
#define _ATYPE_
#define _ATYPE32_
#else
#define _ATYPE_		__PTRDIFF_TYPE__
#define _ATYPE32_	int
#endif

/*
 *  32-bit address spaces
 */
#ifdef __ASSEMBLY__
#define _ACAST32_
#else
#define _ACAST32_		(_ATYPE_)(_ATYPE32_)	/* widen if necessary */
#endif

/*
 * Memory segments (32bit kernel mode addresses)
 */
#define KUSEG			0x00000000ul
#define KSEG0			0x80000000ul
#define KSEG1			0xa0000000ul
#define KSEG2			0xc0000000ul
#define KSEG3			0xe0000000ul

/*
 * Returns the kernel segment base of a given address
 */
#define KSEGX(a)		((_ACAST32_ (a)) & 0xe0000000)

/*
 * Returns the physical address of a KSEG0/KSEG1 address
 */
#define CPHYSADDR(a)		((_ACAST32_ (a)) & 0x1fffffff)

#ifndef __ASSEMBLY__
#define PHYSADDR(a)		CPHYSADDR(a)
#endif

/*
 * Map an address to a certain kernel segment
 */
#define KSEG0ADDR(a)		(CPHYSADDR(a) | KSEG0)
#define KSEG1ADDR(a)		(CPHYSADDR(a) | KSEG1)
#define KSEG2ADDR(a)		(CPHYSADDR(a) | KSEG2)
#define KSEG3ADDR(a)		(CPHYSADDR(a) | KSEG3)

/*
 * Cache modes for XKPHYS address conversion macros
 */
#define K_CALG_COH_EXCL1_NOL2	0
#define K_CALG_COH_SHRL1_NOL2	1
#define K_CALG_UNCACHED		2
#define K_CALG_NONCOHERENT	3
#define K_CALG_COH_EXCL		4
#define K_CALG_COH_SHAREABLE	5
#define K_CALG_NOTUSED		6
#define K_CALG_UNCACHED_ACCEL	7

#define TO_PHYS_MASK			0xfffffffffULL		/* 36 bit */


#endif /* CONFIG_MMU */

#endif /* __ASM_ADDRSPACE_H */

