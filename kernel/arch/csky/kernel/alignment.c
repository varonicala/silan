/*
 *  arch/arm/kernel/alignment.c  - handle alignment exceptions for CSKY CPU. 
 *  
 *  Copyright (C) 2011, C-SKY Microsystems Co., Ltd. (www.c-sky.com) 
 *  Copyright (C) 2011, Hu Junshan (junshan_hu@c-sky.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#include <asm/unaligned.h>

extern void die_if_kernel(char *, struct pt_regs *, long);

#ifdef CONFIG_SOFT_HANDMISSALIGN

/* C-SKY CPU V2 32 bit instruction like 11'B in the highest two bit  */
#define IS_T32(hi16)  (((hi16) & 0xc000) == 0xc000 )

#define CODING_BITS(i)  (i & 0xf8000000)
#define LDST_TYPE(i)    (i & 0xf000)

static unsigned long ai_user;
static unsigned long ai_sys;
static unsigned long ai_skipped;
static unsigned long ai_half;
static unsigned long ai_word;
static unsigned long ai_qword;
static int ai_usermode;

#define UM_WARN		(1 << 0)
#define UM_FIXUP	(1 << 1)
#define UM_SIGNAL	(1 << 2)

#ifdef CONFIG_PROC_FS
static const char *usermode_action[] = {
	"ignored",
	"warn",
	"fixup",
	"fixup+warn",
	"signal",
	"signal+warn"
};

static int
proc_alignment_read(char *page, char **start, off_t off, int count, int *eof,
		    void *data)
{
	char *p = page;
	int len;

	p += sprintf(p, "User:\t\t%lu\n", ai_user);
	p += sprintf(p, "System:\t\t%lu\n", ai_sys);
	p += sprintf(p, "Skipped:\t%lu\n", ai_skipped);
	p += sprintf(p, "Half:\t\t%lu\n", ai_half);
	p += sprintf(p, "Word:\t\t%lu\n", ai_word);
	p += sprintf(p, "Qword:\t\t%lu\n", ai_qword);
	p += sprintf(p, "User faults:\t%i (%s)\n", ai_usermode,
			usermode_action[ai_usermode]);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int proc_alignment_write(struct file *file, const char __user *buffer,
				unsigned long count, void *data)
{
	char mode;

	if (count > 0) {
		if (get_user(mode, buffer))
			return -EFAULT;
		if (mode >= '0' && mode <= '5')
			ai_usermode = mode - '0';
	}
	return count;
}

#endif /* CONFIG_PROC_FS */

#ifdef  __cskyBE__ 
#define BE		1
#define FIRST_BYTE_16	"rotri	%1, 8\n"
#define FIRST_BYTE_32	"rotri	%1, 24\n"
#define NEXT_BYTE	"rotri  %1, 24\n"
#else
#define BE		0
#define FIRST_BYTE_16
#define FIRST_BYTE_32
#define NEXT_BYTE	"lsri   %1, 8\n"
#endif

#define __get8_unaligned_check(val,addr,err)		\
	__asm__(					\
 	"1:	ldb	%1, (%2)\n"			\
 	"	addi	%2, 1\n"			\
	"	br	3f\n"				\
	"2:	movi	%0, 1\n"			\
	"	br	3f\n"				\
	"	.section __ex_table,\"a\"\n"		\
	"	.align	2\n"				\
	"	.long	1b, 2b\n"			\
	"	.previous\n"				\
	"3:\n"						\
	: "=r" (err), "=r" (val), "=r" (addr)		\
	: "0" (err), "2" (addr))

#define get16_unaligned_check(val,addr)				\
	do {							\
		unsigned int err = 0, v, a = addr;		\
		__get8_unaligned_check(v,a,err);		\
		val =  v << ((BE) ? 8 : 0);			\
		__get8_unaligned_check(v,a,err);		\
		val |= v << ((BE) ? 0 : 8);			\
		if (err)					\
			goto fault;				\
	} while (0)

#define get32_unaligned_check(val,addr)				\
	do {							\
		unsigned int err = 0, v, a = addr;		\
		__get8_unaligned_check(v,a,err);		\
		val =  v << ((BE) ? 24 :  0);			\
		__get8_unaligned_check(v,a,err);		\
		val |= v << ((BE) ? 16 :  8);			\
		__get8_unaligned_check(v,a,err);		\
		val |= v << ((BE) ?  8 : 16);			\
		__get8_unaligned_check(v,a,err);		\
		val |= v << ((BE) ?  0 : 24);			\
		if (err)					\
			goto fault;				\
	} while (0)

#define put16_unaligned_check(val,addr)				\
	do {							\
		unsigned int err = 0, v = val, a = addr;	\
		__asm__( FIRST_BYTE_16				\
	 	"1:	stb	%1, (%2)\n"			\
	 	"	addi	%2, 1\n"			\
			NEXT_BYTE				\
		"2:	stb	%1, (%2)\n"			\
		"	br	4f\n"				\
		"3:	movi	%0, 1\n"			\
		"	br	4f\n"				\
		"	.section __ex_table,\"a\"\n"		\
		"	.align	2\n"				\
		"	.long	1b, 3b\n"			\
		"	.long	2b, 3b\n"			\
		"	.previous\n"				\
		"4:\n"						\
		: "=r" (err), "=r" (v), "=r" (a)		\
		: "0" (err), "1" (v), "2" (a));			\
		if (err)					\
			goto fault;				\
	} while (0)

#define put32_unaligned_check(val,addr)				\
	do {							\
		unsigned int err = 0, v = val, a = addr;	\
		__asm__( FIRST_BYTE_32				\
	 	"1:	stb	%1, (%2)\n"			\
	 	"	addi	%2, 1\n"			\
			NEXT_BYTE				\
	 	"2:	stb	%1, (%2)\n"			\
	 	"	addi	%2, 1\n"			\
			NEXT_BYTE				\
	 	"3:	stb	%1, (%2)\n"			\
	 	"	addi	%2, 1\n"			\
			NEXT_BYTE				\
		"4:	stb	%1, (%2)\n"			\
		"	br	6f\n"				\
		"5:	movi	%0, 1\n"			\
		"	br	6f\n"				\
		"	.section __ex_table,\"a\"\n"		\
		"	.align	2\n"				\
		"	.long	1b, 5b\n"			\
		"	.long	2b, 5b\n"			\
		"	.long	3b, 5b\n"			\
		"	.long	4b, 5b\n"			\
		"	.previous\n"				\
		"6:\n"						\
		: "=r" (err), "=r" (v), "=r" (a)		\
		: "0" (err), "1" (v), "2" (a));			\
		if (err)					\
			goto fault;				\
	} while (0)

static int alignment_handle_ldhsth(unsigned long addr, unsigned long instr, struct pt_regs *regs)
{
	unsigned int regz = 0; 

	ai_half += 1;

#ifndef __CSKYABIV2__	/* abiv1 */
	regz = (instr >> 8) & 0xf;
	if (instr & 0x1000)  // store
	{
		long dataregz;
		if(regz == 0) {
			if(user_mode(regs)) {
				__asm__ __volatile__("mfcr %0, ss1 \n\r" 
					     :"=r"(dataregz));
			}
			else {
				dataregz = sizeof(struct pt_regs) + 
					((long)regs);
			}			
		} else if(regz == 1){
			dataregz = regs->regs[9];
		} else if(regz == 15){
			dataregz = regs->r15;
		} else
		{
			dataregz = *((long *)regs + (regz + 1));
		}

		put16_unaligned_check(dataregz, addr);
	}
	else
	{
		unsigned short val;
		long * datapt = NULL;
		get16_unaligned_check(val, addr);
		if(regz == 0) {
			printk("warring: ld dest reg is sp!\n");
			goto fault;  // SP need not handle
		} else if(regz == 1){
			regs->regs[9] = (long)val;
		} else if(regz == 15){
			regs->r15 = (long)val;
		} else
		{
			datapt = (long *)regs + (regz + 1);
			*datapt = (long)val;
		}
	}
#else	/* abiv2 */
	if (instr & 0xc0000000) {  // 32bit instruction
		regz = (instr >> 21) & 0x1f;
		if (instr & 0x04000000)  // store
		{
			long dataregz;
			if(regz < 14) {
				dataregz = *((long *)regs + (regz + 3));
			}
			else if(regz == 14){
				if(user_mode(regs)) {
					__asm__ __volatile__("mfcr %0, cr<14, 1>\n\r"
                                           :"=r"(dataregz));
				}
				else {
					dataregz = sizeof(struct pt_regs) +
						  ((long)regs);
				}
			}
			else{
				dataregz = *((long *)regs + (regz + 2));
			}
			put16_unaligned_check(dataregz, addr);
		}
		else
		{
			unsigned short val;
			long * datapt = NULL; 
			get16_unaligned_check(val, addr);
			if(regz < 14) {
				datapt = (long *)regs + (regz + 3);
				*datapt = (long)val;
			}
			else if(regz < 14) {
				printk("warring: ld dest reg is sp!\n");
				goto fault;  // dest SP need not handle!	
			}
			else {
				datapt = (long *)regs + (regz + 2);
				*datapt = (long)val;
			}
		}
	}
	else {    //16bit
		regz = (instr >> 5) & 0x7;
		if (instr & 0x2000)  // store
		{
			long dataregz = *((long *)regs + (regz + 3));
			put16_unaligned_check(dataregz, addr);
		}
		else
		{
			unsigned short val;
			long * datapt = (long *)regs + (regz + 3);
			get16_unaligned_check(val, addr);
			*datapt = (long)val;
		}
	}
#endif
	return 1;

fault:
	return 0;
}

static int alignment_handle_ldwstw(unsigned long addr, unsigned long instr, struct pt_regs *regs)
{
	unsigned int regz = 0;

	ai_word += 1;
	
#ifndef __CSKYABIV2__	/* abiv1 */
	regz = (instr >> 8) & 0xf;
	if (instr & 0x1000)  // store
	{
		long dataregz;
		if(regz == 0) {
			if(user_mode(regs)) {
				__asm__ __volatile__("mfcr %0, ss1 \n\r" 
					     :"=r"(dataregz));
			}
			else {
				dataregz = sizeof(struct pt_regs) + 
					((long)regs);
			}			
		} else if(regz == 1){
			dataregz = regs->regs[9];
		} else if(regz == 15){
			dataregz = regs->r15;
		} else
		{
			dataregz = *((long *)regs + (regz + 1));
		}
	
		put32_unaligned_check(dataregz, addr);
	}
	else
	{
		long val;
		long * datapt = NULL;
		get32_unaligned_check(val, addr);
		if(regz == 0) {
			printk("warring: ld dest reg is sp!\n");
			goto fault;  // SP need not handle;
		} else if(regz == 1){
			regs->regs[9] = val;
		} else if(regz == 1){
			regs->r15 = val;
		} else
		{
			datapt = (long *)regs + (regz + 1);
			*datapt = val;
		}
	}
#else	/* abiv2 */
	if (instr & 0xc0000000) {  // 32bit instruction
                regz = (instr >> 21) & 0x1f;
		if (instr & 0x04000000)  // store
		{
			long dataregz;
			if(regz < 14) {
				dataregz = *((long *)regs + (regz + 3));
			}
			else if(regz == 14){
				if(user_mode(regs)) {
					__asm__ __volatile__("mfcr %0, cr<14, 1>\n\r"
                                           :"=r"(dataregz));
				}
				else {
					dataregz = sizeof(struct pt_regs) +
						  ((long)regs);
				}
			}
			else{
				dataregz = *((long *)regs + (regz + 2));
			}
			put32_unaligned_check(dataregz, addr);
		}
		else
		{
			long val;
			long * datapt = NULL; 
			get32_unaligned_check(val, addr);
			if(regz < 14) {
				datapt = (long *)regs + (regz + 3);
				*datapt = val;
			}
			else if(regz < 14) {
				printk("warring: ld dest reg is sp!\n");
				goto fault;  // dest SP need not handle!	
			}
			else {
				datapt = (long *)regs + (regz + 2);
				*datapt = val;
			}
		}
	}
	else {
		regz = (instr >> 5) & 0x7;
		if (instr & 0x2000)  // store
		{
			long dataregz = *((long *)regs + (regz + 3));
			put32_unaligned_check(dataregz, addr);
		}
		else
		{
			long val;
			long * datapt = (long *)regs + (regz + 3);
			get32_unaligned_check(val, addr);
			*datapt = val;
		}
	}
#endif
	return 1; 
	 
fault:
	return 0;
}


// NOTE: only ck800 has 32 bit instruction and only ldhs but no sths.
static int alignment_handle_ldhs(unsigned long addr, unsigned long instr, struct pt_regs *regs)
{
	short val;
	int * datapt = NULL;
	unsigned int regz = (instr >> 21) & 0x1f;
//	int destval;

	ai_half += 1;

	get16_unaligned_check(val, addr);
//	if(val & 0x8000) {
//		destval = (0xffff << 16) | ((long)val);
//	}
//	else 
//		destval = (long)val;

	if(regz < 14) {
	        datapt = (int *)regs + (regz + 3);
	        *datapt = (int)val;
	        //*datapt = destval;
	}
	else if(regz < 14) {
	        printk("warring: ld dest reg is sp!\n");
	        goto fault;  // dest SP need not handle!        
	}
	else {
	        datapt = (int *)regs + (regz + 2);
	        *datapt = (int)val;
	        //*datapt = destval;
	}
	return 1;
fault:
	return 0;
}

// NOTE: only ck800 has 32 bit instruction.
static int alignment_handle_lddstd(unsigned long addr, unsigned long instr, struct pt_regs *regs)
{
	unsigned int regz = (instr >> 21) & 0x1f;

	ai_word += 2;

	if (instr & 0x04000000)  // store
	{
		long dataregz, dataregz_in1;
		if(regz < 13) {
			dataregz = *((long *)regs + (regz + 3));
			dataregz_in1 = *((long *)regs + (regz + 4));
		}
		else if(regz == 13){
			dataregz = *((long *)regs + (regz + 3));
			if(user_mode(regs)) {
				__asm__ __volatile__("mfcr %0, cr<14, 1>\n\r"
                                   :"=r"(dataregz_in1));
			}
			else {
				dataregz_in1 = sizeof(struct pt_regs) +
					  ((long)regs);
			}
		}
		else if(regz == 14) {
			if(user_mode(regs)) {
				__asm__ __volatile__("mfcr %0, cr<14, 1>\n\r"
                                   :"=r"(dataregz));
			}
			else {
				dataregz = sizeof(struct pt_regs) +
					  ((long)regs);
			}
			dataregz_in1 = *((long *)regs + (regz + 3));
		}
		else{
			dataregz = *((long *)regs + (regz + 2));
			dataregz_in1 = *((long *)regs + (regz + 3));
		}
		put32_unaligned_check(dataregz, addr);
		put32_unaligned_check(dataregz_in1, (addr + 4));
	}
	else
	{
		long val1, val2;
		long * datapt = NULL; 
		get32_unaligned_check(val1, addr);
		get32_unaligned_check(val2, (addr + 4));
		if(regz < 13) {
			datapt = (long *)regs + (regz + 3);
			*datapt = val1;
			datapt = (long *)regs + (regz + 4);
			*datapt = val2;
		}
		else if(regz > 15){
			datapt = (long *)regs + (regz + 2);
			*datapt = val1;
			datapt = (long *)regs + (regz + 3);
			*datapt = val2;
		}
		else {
			printk("warring: ld dest reg is sp!\n");
			goto fault;  // dest SP need not handle!	
		}
	}
	return 1;
fault:
	return 0;
}

#ifndef __CSKYABIV2__	/* abiv1 */
static int alignment_handle_ldqstq(unsigned long addr, unsigned long instr, struct pt_regs *regs)
{
	ai_qword += 1;

#ifndef __CSKYABIV2__
	if (instr & 0x1000)  // store
	{
		long dataregz = regs->a2;

		put32_unaligned_check(dataregz, addr);
		addr += 1;
		dataregz = regs->a3;
		put32_unaligned_check(dataregz, addr);
		addr += 1;
		dataregz = regs->regs[0];
		put32_unaligned_check(dataregz, addr);
		addr += 1;
		dataregz = regs->regs[1];
		put32_unaligned_check(dataregz, addr);
	}
	else
	{
		long val;
		get32_unaligned_check(val, addr);
		regs->a2 = val;
		addr += 1;
		get32_unaligned_check(val, addr);
		regs->a3 = val;
		addr += 1;
		get32_unaligned_check(val, addr);
		regs->regs[0] = val;
		addr += 1;
		get32_unaligned_check(val, addr);
		regs->regs[1] = val;
	}
	return 1; 
	 
fault:
#endif
	return 0;
}
#endif /* __CSKYABIV2__ */

asmlinkage void alignment_c(struct pt_regs *regs)
{
	int err;
	unsigned long instr = 0, instrptr, srcaddr = 0;
	unsigned int fault;
	u16 tinstr = 0;
	int (*handler)(unsigned long addr, unsigned long inst, struct pt_regs *regs) = NULL;
	int isize = 2;
	mm_segment_t fs;
//printk("....alignment!\n");

	instrptr = instruction_pointer(regs);

	fs = get_fs();
	set_fs(KERNEL_DS);
	fault = __get_user(tinstr, (u16 *)(instrptr & ~1));
	instr = (unsigned long)tinstr;
#ifdef __CSKYABIV2__
	if (!fault) {
		if (IS_T32(tinstr)) {
			u16 tinst2 = 0;
			fault = __get_user(tinst2, (u16 *)(instrptr+2));
			instr = (tinstr << 16) | tinst2;	
			isize = 4;
		}
	}
#endif
	set_fs(fs);
	if (fault) {
		goto bad_or_fault;
	}

	if (user_mode(regs))
		goto user;
	
	ai_sys += 1;

fixup:	
	regs->pc += isize;

#ifndef __CSKYABIV2__	/* abiv1 */
	if(tinstr & 0x8000)   // ld/st
	{
		int imm4 = (tinstr >> 4) & 0xf;
		int regx = tinstr & 0xf;
		if(regx == 0) {
			__asm__ __volatile__("mfcr %0, ss1 \n\r" 
				                 :"=r"(srcaddr));
		} else if(regx == 1){
			srcaddr = (unsigned long )regs->regs[9];
		} else if(regx == 15){
			srcaddr = (unsigned long )regs->r15;
		} else
		{
			srcaddr = *((int*)regs + (regx + 1));
		}
		
		if(tinstr & 0x4000)   // ldh/sth
		{
			srcaddr += imm4 << 1;
			handler = alignment_handle_ldhsth;
		}
		else    // ldw/stw
		{
			srcaddr += imm4 << 2;
			handler = alignment_handle_ldwstw;
		}
				
	} 
	else if ((tinstr & 0x60) == 0x40){ // ldq/stq
		int regx = tinstr & 0xf;
		if(regx == 0) {
			__asm__ __volatile__("mfcr %0, ss1 \n\r" 
				                 :"=r"(srcaddr));
		} else if(regx == 1){
			srcaddr = (unsigned long )regs->regs[9];
		} else
		{
			srcaddr = *((int *)regs + (regx + 1));
		}
		handler = alignment_handle_ldqstq;
	}
	else {
		// FIXME: sourse reg of ldm/stm is r0(stack pointer). It may
		//     lead to unrecover exception if r0 unalign. So ignore it.
		printk("warnning: ldm/stm alignment.\n");
		goto bad_or_fault;  
	}
#else	/* abiv2 */
	if(2 == isize ) {
		if(instr & 0x8000)
		{
			int imm5 = instr & 0x1f;
			int regx = (instr >> 8) & 0x7;
			if(instr & 0x1000)   // ldw/stw
			{
				if(instr & 0x0800)  // stw/ldw rz,(sp, disp)
				{
					imm5 += (((instr >> 8) & 0x7) << 5);
					/*
					 * In super mode, it may lead to 
					 * unrecover exception if r0 unalign.
					 * So ignore it.
					*/
					__asm__ __volatile__("mfcr %0, cr<14, 1>\n\r"
                                                 :"=r"(srcaddr));
					srcaddr += (imm5 << 2);
				} else    //  stw/ldw rz,(rx, disp)
				{
					srcaddr = *((int*)regs + (regx + 3));
					srcaddr += (imm5 << 2);
				}	
				handler = alignment_handle_ldwstw;	
			} else     // ldh/sth
			{
				srcaddr = *((int*)regs + (regx + 3));
				srcaddr += (imm5 << 1);
				handler = alignment_handle_ldhsth;
			}
		}
		else  // push/pop
		{
			printk("warnning: push/pop alignment.\n");
		}
	} 
	else {
		int offset = 0, regx =0;
		switch (CODING_BITS(instr)) {
		case 0xD8000000:        // base load/store instruction.
			offset = (instr & 0xfff);
			regx = (instr >> 16) & 0x1f;	
			if(regx < 14) {
				srcaddr = *((int*)regs + (regx + 3));
			}
			else if (regx == 14) { // ignore super mode.
				__asm__ __volatile__("mfcr %0, cr<14, 1>\n\r"
                                         :"=r"(srcaddr));	
			} else {
				srcaddr = *((int*)regs + (regx + 2));
			}
			switch (LDST_TYPE(instr)) {
			case 0x1000:    // ldh/sth
				srcaddr += (offset << 1);
				handler = alignment_handle_ldhsth;
				break;
			case 0x2000:    // ldw/stw
				srcaddr += (offset << 2);
				handler = alignment_handle_ldwstw;
				break;
			case 0x3000:    // ldd/std
				srcaddr += (offset << 2);
				handler = alignment_handle_lddstd;
				break;
			case 0x5000:    // ldhs
				srcaddr += (offset << 1);
				handler = alignment_handle_ldhs;
				break;
			}
			break;
		case 0xD0000000:        // ldr/str instruction.
			break;
		case 0xC0000000:        // lrs/srs instruction.
			break;
		case 0x0:               // push/pop instruction.???!!!
			break;
		default:
                	// FIXME: stq/stq is pseudoinstruction of stm/stm and now ignore.
                	goto bad_or_fault;
		}
	}
#endif
	
	if (!handler)
		goto bad_or_fault;

	err = handler(srcaddr, instr, regs);
	if (!err)
	{
		regs->pc -=2;
		goto bad_or_fault;
	}

	return;

bad_or_fault:
	ai_skipped += 1;
	if(fixup_exception(regs)) {
	    return;
	}
	die_if_kernel("Alignment trap: not handle this instruction", regs, 0);
	return;

user:
	ai_user += 1;

	if (ai_usermode & UM_WARN)
		printk("Alignment trap: %s(pid=%d) PC=0x%x Ins=0x%x\n",
			current->comm, current->pid, 
			(unsigned int)regs->pc, (unsigned int)instr);

	if (ai_usermode & UM_FIXUP)
		goto fixup;

	if (ai_usermode & UM_SIGNAL)
		force_sig(SIGBUS, current);

	return;
}

/*
 * This needs to be done after sysctl_init, otherwise sys/ will be
 * overwritten.  Actually, this shouldn't be in sys/ at all since
 * it isn't a sysctl, and it doesn't contain sysctl information.
 * We now locate it in /proc/cpu/alignment instead.
 */
static int __init alignment_init(void)
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *res;

	res = proc_mkdir("cpu", NULL);
	if (!res)
		return -ENOMEM;

	res = create_proc_entry("alignment", S_IWUSR | S_IRUGO, res);
	if (!res)
		return -ENOMEM;

	res->read_proc = proc_alignment_read;
	res->write_proc = proc_alignment_write;
#endif

	ai_usermode = UM_FIXUP;

	return 0;
}

fs_initcall(alignment_init);

#else /* !CONFIG_SOFT_HANDMISSALIGN */

asmlinkage void alignment_c(struct pt_regs *regs)
{
	asm("bkpt");
	int sig;
	siginfo_t info;

	sig = SIGBUS;
	info.si_code = BUS_ADRALN;
	info.si_signo = sig;
	info.si_errno = 0;
	info.si_addr = (void *)regs->pc;
	if (user_mode(regs)){
		force_sig_info(sig, &info, current);
        return;
	}
	
	if(fixup_exception(regs)) {
	    return;
	}
	die_if_kernel("Kernel mode Alignment exception", regs, 0);
	return;
}

#endif /* CONFIG_SOFT_HANDMISSALIGN */
