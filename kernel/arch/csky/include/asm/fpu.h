/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009  Hangzhou C-SKY Microsystems co.,ltd.
 */

#ifndef __CSKY_FPU_H
#define __CSKY_FPU_H

#ifndef __ASSEMBLY__ /* C source */
 
/*
 * Define the fesr bit for fpe handle. 
 */
#define  FPE_ILLE  (1 << 16)    /* Illegal instruction  */
#define  FPE_FEC   (1 << 7)     /* Input float-point arithmetic exception */
#define  FPE_IDC   (1 << 5)     /* Input denormalized exception */
#define  FPE_IXC   (1 << 4)     /* Inexact exception */
#define  FPE_UFC   (1 << 3)     /* Underflow exception */
#define  FPE_OFC   (1 << 2)     /* Overflow exception */
#define  FPE_DZC   (1 << 1)     /* Divide by zero exception */
#define  FPE_IOC   (1 << 0)     /* Invalid operation exception */

#ifdef CONFIG_OPEN_FPU_IDE
#define IDE_STAT   (1 << 5)
#else 
#define IDE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_IXE
#define IXE_STAT   (1 << 4)
#else 
#define IXE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_UFE
#define UFE_STAT   (1 << 3)
#else
#define UFE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_OFE
#define OFE_STAT   (1 << 2)
#else
#define OFE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_DZE
#define DZE_STAT   (1 << 1)
#else
#define DZE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_IOE
#define IOE_STAT   (1 << 0)
#else
#define IOE_STAT   0
#endif

#define FMFS_FPU_REGS(frw, frx, fry, frz)       \
	"fmfs   %0, "#frw" \n\r"        \
	"fmfs   %1, "#frx" \n\r"        \
	"fmfs   %2, "#fry" \n\r"        \
	"fmfs   %3, "#frz" \n\r"

#define FMTS_FPU_REGS(frw, frx, fry, frz)       \
	"fmts   %0, "#frw" \n\r"        \
	"fmts   %1, "#frx" \n\r"        \
	"fmts   %2, "#fry" \n\r"        \
	"fmts   %3, "#frz" \n\r"

#define FMFVR_FPU_REGS(vrx, vry)        \
	"fmfvrl %0, "#vrx" \n\r"        \
	"fmfvrh %1, "#vrx" \n\r"        \
	"fmfvrl %2, "#vry" \n\r"        \
	"fmfvrh %3, "#vry" \n\r"

#define FMTVR_FPU_REGS(vrx, vry)        \
	"fmtvrl "#vrx", %0 \n\r"        \
	"fmtvrh "#vrx", %1 \n\r"        \
	"fmtvrl "#vry", %2 \n\r"        \
	"fmtvrh "#vry", %3 \n\r"

#define STW_FPU_REGS(a, b, c, d)        \
	"stw    %0, (%4, "#a") \n\r"    \
	"stw    %1, (%4, "#b") \n\r"    \
	"stw    %2, (%4, "#c") \n\r"    \
	"stw    %3, (%4, "#d") \n\r"

#define LDW_FPU_REGS(a, b, c, d)        \
	"ldw    %0, (%4, "#a") \n\r"    \
	"ldw    %1, (%4, "#b") \n\r"    \
	"ldw    %2, (%4, "#c") \n\r"    \
	"ldw    %3, (%4, "#d") \n\r"

/* enable and init FPU */
static inline void init_fpu(void)
{
	unsigned long flg;
	unsigned long cpwr, fcr;

	cpwr = 0xf0000007; // set for reg CPWR(cp15): ie, ic, ec, rp, wp, en = 1 
	fcr = (IDE_STAT | IXE_STAT | UFE_STAT | OFE_STAT | DZE_STAT | IOE_STAT);
	local_save_flags(flg);	
#if defined(CONFIG_CPU_CSKYV1)
	__asm__ __volatile__("cpseti  1 \n\t"
	                     "mtcr    %0, cr15 \n\t"
	                     "cpwcr   %1, cpcr1 \n\t"
	                     ::"r"(cpwr), "b"(fcr)
	                     );
#else
	__asm__ __volatile__("mtcr    %0, cr<1, 2> \n\t"
			     ::"r"(fcr)
			    );
#endif

	local_irq_restore(flg);
}
 
static inline void save_fp_to_thread(unsigned long  * fpregs, 
	   unsigned long * fcr, unsigned long * fsr, unsigned long * fesr)
{
	unsigned long flg;
	unsigned long tmp1, tmp2, tmp3, tmp4;
	
	local_save_flags(flg);	

#if defined(CONFIG_CPU_CSKYV1)   
	__asm__ __volatile__("cpseti 1 \n\t"
	                     "cprcr  r7, cpcr1 \n\t"
	                     "mov    %0, r7    \n\t"
	                     "cprcr  r7, cpcr2 \n\t"
	                     "mov    %1, r7    \n\t"
	                     "cprcr  r7, cpcr4 \n\t"
	                     "mov    %2, r7    \n\t"
	                     :"=a"(tmp1), "=a"(tmp2), "=a"(tmp3)
	                     : :"r7");
	*fcr = tmp1;
	*fsr = tmp2;
	*fesr = tmp3;
 
	__asm__ __volatile__("cpseti 1 \n\t"
	                     FMFS_FPU_REGS(fr0, fr1, fr2, fr3)
	                     STW_FPU_REGS(0, 4, 8, 12)
                         FMFS_FPU_REGS(fr4, fr5, fr6, fr7)
                         STW_FPU_REGS(16, 20, 24, 28)
                         FMFS_FPU_REGS(fr8, fr9, fr10, fr11)
                         STW_FPU_REGS(32, 36, 40, 44)
                         FMFS_FPU_REGS(fr12, fr13, fr14, fr15)
                         STW_FPU_REGS(48, 52, 56, 60)
	                     "addi   %4,32 \n\t"
	                     "addi   %4,32 \n\t"
	                     FMFS_FPU_REGS(fr16, fr17, fr18, fr19)
	                     STW_FPU_REGS(0, 4, 8, 12)
	                     FMFS_FPU_REGS(fr20, fr21, fr22, fr23)
	                     STW_FPU_REGS(16, 20, 24, 28)
	                     FMFS_FPU_REGS(fr24, fr25, fr26, fr27)
	                     STW_FPU_REGS(32, 36, 40, 44)
	                     FMFS_FPU_REGS(fr28, fr29, fr30, fr31)
	                     STW_FPU_REGS(48, 52, 56, 60)
	                     :"=a"(tmp1), "=a"(tmp2), "=a"(tmp3), "=a"(tmp4),
	                       "+a"(fpregs));
#elif defined(CONFIG_CPU_CSKYV2)
	__asm__ __volatile__("mfcr    %0, cr<1, 2> \n\r"
	                     "mfcr    %1, cr<2, 2> \n\r"
	                     :"+r"(tmp1), "+r"(tmp2) : );
	*fcr = tmp1;
	*fsr = 0;      // not use in fpuv2
	*fesr = tmp2;
	__asm__ __volatile__(FMFVR_FPU_REGS(vr0, vr1)
	                     STW_FPU_REGS(0, 4, 8, 12)
	                     FMFVR_FPU_REGS(vr2, vr3)
	                     STW_FPU_REGS(16, 20, 24, 28)
	                     FMFVR_FPU_REGS(vr4, vr5)
	                     STW_FPU_REGS(32, 36, 40, 44)
	                     FMFVR_FPU_REGS(vr6, vr7)
	                     STW_FPU_REGS(48, 52, 56, 60)
	                     "addi    %4, 32 \n\r"
	                     "addi    %4, 32 \n\r"
	                     FMFVR_FPU_REGS(vr8, vr9)
	                     STW_FPU_REGS(0, 4, 8, 12)
	                     FMFVR_FPU_REGS(vr10, vr11)
	                     STW_FPU_REGS(16, 20, 24, 28)
	                     FMFVR_FPU_REGS(vr12, vr13)
	                     STW_FPU_REGS(32, 36, 40, 44)
	                     FMFVR_FPU_REGS(vr14, vr15)
	                     STW_FPU_REGS(48, 52, 56, 60)
	                     :"=a"(tmp1), "=a"(tmp2), "=a"(tmp3), "=a"(tmp4),
	                       "+a"(fpregs));
#endif
	local_irq_restore(flg);
}

#else  /* __ASSEMBLY__ */

#include <asm/asm-offsets.h>

.macro  FPU_SAVE_REGS
#if defined(CONFIG_CPU_CSKYV1)
	cpseti   1             /* select fpu */
	/* Save FPU control regs task struct */
	cprcr    r6, cpcr1
	stw      r6, (r5, THREAD_FCR)
	cprcr    r6, cpcr2
	cprcr    r7, cpcr4
	stw      r6, (a3, THREAD_FSR)
	stw      r7, (a3, THREAD_FESR)
	/* Save FPU general regs task struct */
	lrw      r10, THREAD_FPREG
	add      r10, a3
	fmfs     r6, fr0
	fmfs     r7, fr1
	fmfs     r8, fr2
	fmfs     r9, fr3
	stw      r6, (r10, 0)
	stw      r7, (r10, 4)
	stw      r8, (r10, 8)
	stw      r9, (r10, 12)
	fmfs     r6, fr4
	fmfs     r7, fr5
	fmfs     r8, fr6
	fmfs     r9, fr7
	stw      r6, (r10, 16)
	stw      r7, (r10, 20)
	stw      r8, (r10, 24)
	stw      r9, (r10, 28)
	fmfs     r6, fr8
	fmfs     r7, fr9
	fmfs     r8, fr10 
	fmfs     r9, fr11
	stw      r6, (r10, 32)
	stw      r7, (r10, 36)
	stw      r8, (r10, 40)
	stw      r9, (r10, 44)
	fmfs     r6, fr12 
	fmfs     r7, fr13
	fmfs     r8, fr14 
	fmfs     r9, fr15
	stw      r6, (r10, 48)
	stw      r7, (r10, 52)
	stw      r8, (r10, 56)
	stw      r9, (r10, 60)
	movi     r11, 64
	add      r10, r11
	fmfs     r6, fr16
	fmfs     r7, fr17
	fmfs     r8, fr18
	fmfs     r9, fr19
	stw      r6, (r10, 0)
	stw      r7, (r10, 4)
	stw      r8, (r10, 8)
	stw      r9, (r10, 12)
	fmfs     r6, fr20
	fmfs     r7, fr21
	fmfs     r8, fr22
	fmfs     r9, fr23
	stw      r6, (r10, 16)
	stw      r7, (r10, 20)
	stw      r8, (r10, 24)
	stw      r9, (r10, 28)
	fmfs     r6, fr24
	fmfs     r7, fr25
	fmfs     r8, fr26
	fmfs     r9, fr27
	stw      r6, (r10, 32)
	stw      r7, (r10, 36)
	stw      r8, (r10, 40)
	stw      r9, (r10, 44)
	fmfs     r6, fr28
	fmfs     r7, fr29
	fmfs     r8, fr30
	fmfs     r9, fr31
	stw      r6, (r10, 48)
	stw      r7, (r10, 52)
	stw      r8, (r10, 56)
	stw      r9, (r10, 60)
#elif defined(CONFIG_CPU_CSKYV2)
	/* Save FPU control regs task struct */
	mfcr     r7, cr<1, 2>
	mfcr     r6, cr<2, 2>
	stw      r7, (a3, THREAD_FCR)
	stw      r6, (a3, THREAD_FESR)
	/* Save FPU general regs task struct */
	fmfvrl   r6, vr0
	fmfvrh   r7, vr0
	fmfvrl   r8, vr1
	fmfvrh   r9, vr1
	stw      r6, (a3, THREAD_FPREG + 0)  /* In aviv2: stw can load longer */
	stw      r7, (a3, THREAD_FPREG + 4)
	stw      r8, (a3, THREAD_FPREG + 8)
	stw      r9, (a3, THREAD_FPREG + 12)
	fmfvrl   r6, vr2
	fmfvrh   r7, vr2
	fmfvrl   r8, vr3
	fmfvrh   r9, vr3
	stw      r6, (a3, THREAD_FPREG + 16)
	stw      r7, (a3, THREAD_FPREG + 20)
	stw      r8, (a3, THREAD_FPREG + 24)
	stw      r9, (a3, THREAD_FPREG + 28)
	fmfvrl   r6, vr4
	fmfvrh   r7, vr4
	fmfvrl   r8, vr5
	fmfvrh   r9, vr5
	stw      r6, (a3, THREAD_FPREG + 32)
	stw      r7, (a3, THREAD_FPREG + 36)
	stw      r8, (a3, THREAD_FPREG + 40)
	stw      r9, (a3, THREAD_FPREG + 44)
	fmfvrl   r6, vr6
	fmfvrh   r7, vr6
	fmfvrl   r8, vr7
	fmfvrh   r9, vr7
	stw      r6, (a3, THREAD_FPREG + 48)
	stw      r7, (a3, THREAD_FPREG + 52)
	stw      r8, (a3, THREAD_FPREG + 56)
	stw      r9, (a3, THREAD_FPREG + 60)
	fmfvrl   r6, vr8
	fmfvrh   r7, vr8
	fmfvrl   r8, vr9
	fmfvrh   r9, vr9
	stw      r6, (a3, THREAD_FPREG + 64)
	stw      r7, (a3, THREAD_FPREG + 68)
	stw      r8, (a3, THREAD_FPREG + 72)
	stw      r9, (a3, THREAD_FPREG + 76)
	fmfvrl   r6, vr10
	fmfvrh   r7, vr10
	fmfvrl   r8, vr11
	fmfvrh   r9, vr11
	stw      r6, (a3, THREAD_FPREG + 80)
	stw      r7, (a3, THREAD_FPREG + 84)
	stw      r8, (a3, THREAD_FPREG + 88)
	stw      r9, (a3, THREAD_FPREG + 92)
	fmfvrl   r6, vr12
	fmfvrh   r7, vr12
	fmfvrl   r8, vr13
	fmfvrh   r9, vr13
	stw      r6, (a3, THREAD_FPREG + 96)
	stw      r7, (a3, THREAD_FPREG + 100)
	stw      r8, (a3, THREAD_FPREG + 104)
	stw      r9, (a3, THREAD_FPREG + 108)
	fmfvrl   r6, vr14
	fmfvrh   r7, vr14
	fmfvrl   r8, vr15
	fmfvrh   r9, vr15
	stw      r6, (a3, THREAD_FPREG + 112)
	stw      r7, (a3, THREAD_FPREG + 116)
	stw      r8, (a3, THREAD_FPREG + 120)
	stw      r9, (a3, THREAD_FPREG + 124)
#endif
.endm

.macro  FPU_RESTORE_REGS
#if defined(CONFIG_CPU_CSKYV1)
	/* Save FPU control regs task struct */
	ldw      r6, (r5, THREAD_FCR)
	cpwcr    r6, cpcr1
	ldw      r6, (a3, THREAD_FSR)
	ldw      r7, (a3, THREAD_FESR)
	cpwcr    r6, cpcr2
	cpwcr    r7, cpcr4
	/* restore FPU general regs task struct */
	lrw      r10, THREAD_FPREG
	add      r10, a3
	ldw      r6, (r10, 0)
	ldw      r7, (r10, 4)
	ldw      r8, (r10, 8)
	ldw      r9, (r10, 12)
	fmts     r6, fr0
	fmts     r7, fr1
	fmts     r8, fr2
	fmts     r9, fr3
	ldw      r6, (r10, 16)
	ldw      r7, (r10, 20)
	ldw      r8, (r10, 24)
	ldw      r9, (r10, 28)
	fmts     r6, fr4
	fmts     r7, fr5
	fmts     r8, fr6
	fmts     r9, fr7
	ldw      r6, (r10, 32)
	ldw      r7, (r10, 36)
	ldw      r8, (r10, 40)
	ldw      r9, (r10, 44)
	fmts     r6, fr8
	fmts     r7, fr9
	fmts     r8, fr10
	fmts     r9, fr11
	ldw      r6, (r10, 48)
	ldw      r7, (r10, 52)
	ldw      r8, (r10, 56)
	ldw      r9, (r10, 60)
	fmts     r6, fr12
	fmts     r7, fr13
	fmts     r8, fr14
	fmts     r9, fr15
	movi     r11, 64
	add      r10, r11
	ldw      r6, (r10, 0)
	ldw      r7, (r10, 4)
	ldw      r8, (r10, 8)
	ldw      r9, (r10, 12)
	fmts     r6, fr16
	fmts     r7, fr17
	fmts     r8, fr18
	fmts     r9, fr19
	ldw      r6, (r10, 16)
	ldw      r7, (r10, 20)
	ldw      r8, (r10, 24)
	ldw      r9, (r10, 28)
	fmts     r6, fr20
	fmts     r7, fr21
	fmts     r8, fr22
	fmts     r9, fr23
	ldw      r6, (r10, 32)
	ldw      r7, (r10, 36)
	ldw      r8, (r10, 40)
	ldw      r9, (r10, 44)
	fmts     r6, fr24
	fmts     r7, fr25
	fmts     r8, fr26
	fmts     r9, fr27
	ldw      r6, (r10, 48)
	ldw      r7, (r10, 52)
	ldw      r8, (r10, 56)
	ldw      r9, (r10, 60)
	fmts     r6, fr28
	fmts     r7, fr29
	fmts     r8, fr30
	fmts     r9, fr31
#elif defined(CONFIG_CPU_CSKYV2)
	/* Save FPU control regs task struct */
	ldw      r6, (a3, THREAD_FCR)
	ldw      r7, (a3, THREAD_FESR)
	mtcr     r6, cr<1, 2>
	mtcr     r7, cr<2, 2>
	/* restore FPU general regs task struct */
	ldw      r6, (a3, THREAD_FPREG + 0)
	ldw      r7, (a3, THREAD_FPREG + 4)
	ldw      r8, (a3, THREAD_FPREG + 8)
	ldw      r9, (a3, THREAD_FPREG + 12)
	fmtvrl   vr0, r6
	fmtvrh   vr0, r7
	fmtvrl   vr1, r8
	fmtvrh   vr1, r9
	ldw      r6, (a3, THREAD_FPREG + 16)
	ldw      r7, (a3, THREAD_FPREG + 20)
	ldw      r8, (a3, THREAD_FPREG + 24)
	ldw      r9, (a3, THREAD_FPREG + 28)
	fmtvrl   vr2, r6
	fmtvrh   vr2, r7
	fmtvrl   vr3, r8
	fmtvrh   vr3, r9
	ldw      r6, (a3, THREAD_FPREG + 32)
	ldw      r7, (a3, THREAD_FPREG + 36)
	ldw      r8, (a3, THREAD_FPREG + 40)
	ldw      r9, (a3, THREAD_FPREG + 44)
	fmtvrl   vr4, r6
	fmtvrh   vr4, r7
	fmtvrl   vr5, r8
	fmtvrh   vr5, r9
	ldw      r6, (a3, THREAD_FPREG + 48)
	ldw      r7, (a3, THREAD_FPREG + 52)
	ldw      r8, (a3, THREAD_FPREG + 56)
	ldw      r9, (a3, THREAD_FPREG + 60)
	fmtvrl   vr6, r6
	fmtvrh   vr6, r7
	fmtvrl   vr7, r8
	fmtvrh   vr7, r9
	ldw      r6, (a3, THREAD_FPREG + 64)
	ldw      r7, (a3, THREAD_FPREG + 68)
	ldw      r8, (a3, THREAD_FPREG + 72)
	ldw      r9, (a3, THREAD_FPREG + 76)
	fmtvrl   vr8, r6
	fmtvrh   vr8, r7
	fmtvrl   vr9, r8
	fmtvrh   vr9, r9
	ldw      r6, (a3, THREAD_FPREG + 80)
	ldw      r7, (a3, THREAD_FPREG + 84)
	ldw      r8, (a3, THREAD_FPREG + 88)
	ldw      r9, (a3, THREAD_FPREG + 92)
	fmtvrl   vr10, r6
	fmtvrh   vr10, r7
	fmtvrl   vr11, r8
	fmtvrh   vr11, r9
	ldw      r6, (a3, THREAD_FPREG + 96)
	ldw      r7, (a3, THREAD_FPREG + 100)
	ldw      r8, (a3, THREAD_FPREG + 104)
	ldw      r9, (a3, THREAD_FPREG + 108)
	fmtvrl   vr12, r6
	fmtvrh   vr12, r7
	fmtvrl   vr13, r8
	fmtvrh   vr13, r9
	ldw      r6, (a3, THREAD_FPREG + 112)
	ldw      r7, (a3, THREAD_FPREG + 116)
	ldw      r8, (a3, THREAD_FPREG + 120)
	ldw      r9, (a3, THREAD_FPREG + 124)
	fmtvrl   vr14, r6
	fmtvrh   vr14, r7
	fmtvrl   vr15, r8
	fmtvrh   vr15, r9
#endif
.endm

#endif /* __ASSEMBLY__ */	

#endif /* __CSKY_FPU_H */
