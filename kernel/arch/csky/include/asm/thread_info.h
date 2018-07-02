/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  (C) Copyright 2004, Li Chunqiang (chunqiang_li@c-sky.com)
 *  (C) Copyright 2009, Hu Junshan (junshan_hu@c-sky.com)
 *  (C) Copyright 2009, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
 *  
 */ 
  
#ifndef _ASM_CSKY_THREAD_INFO_H
#define _ASM_CSKY_THREAD_INFO_H

#ifndef __ASSEMBLY__

#include <asm/types.h>
#include <asm/page.h>
#include <asm/regdef.h>

struct thread_info {
	struct task_struct	  *task;	     /* main task structure */
	unsigned long		  flags;
	struct exec_domain	  *exec_domain;  /* execution domain */
	int					  preempt_count; /* 0 => preemptable, <0 => BUG */
	unsigned long         tp_value;      /* thread pointer */
	mm_segment_t          addr_limit;
	
	struct restart_block  restart_block;
	struct pt_regs		  *regs;
};

#define INIT_THREAD_INFO(tsk)				\
{											\
	.task		= &tsk,						\
	.exec_domain	= &default_exec_domain,	\
	.preempt_count  = INIT_PREEMPT_COUNT,	\
	.addr_limit     = KERNEL_DS,     		\
	.restart_block = {						\
		.fn = do_no_restart_syscall,		\
	},										\
}

/* THREAD_SIZE should be 8k, so handle differently for 4k and 8k machines */
#define THREAD_SIZE_ORDER (13 - PAGE_SHIFT)

#define init_thread_info	(init_thread_union.thread_info)
#define init_stack		(init_thread_union.stack)

static inline struct thread_info *current_thread_info(void)
{
    unsigned long sp;
    
    __asm__ __volatile__(
                        "mov %0, sp\n\t"
                        :"=r"(sp));

    return (struct thread_info *)(sp & ~(THREAD_SIZE - 1));
}

#endif /* !__ASSEMBLY__ */

#define PREEMPT_ACTIVE		0x4000000

/* entry.S relies on these definitions!
 * bits 0-5 are tested at every exception exit
 */
#define TIF_SIGPENDING		0	/* signal pending */
#define TIF_NOTIFY_RESUME	1       /* callback before returning to user */
#define TIF_NEED_RESCHED	2	/* rescheduling necessary */
#define TIF_SYSCALL_TRACE	5	/* syscall trace active */
#define TIF_DELAYED_TRACE	14	/* single step a syscall */
#define TIF_POLLING_NRFLAG	16	/* true if poll_idle() is polling TIF_NEED_RESCHED */
#define TIF_MEMDIE		18      /* is terminating due to OOM killer */
#define TIF_FREEZE		19	/* thread is freezing for suspend */
#define TIF_RESTORE_SIGMASK	20	/* restore signal mask in do_signal() */
#define TIF_SECCOMP		21	/* secure computing */

#define _TIF_SIGPENDING         (1 << TIF_SIGPENDING)
#define _TIF_NOTIFY_RESUME      (1 << TIF_NOTIFY_RESUME)
#define _TIF_NEED_RESCHED       (1 << TIF_NEED_RESCHED)
#define _TIF_SYSCALL_TRACE      (1 << TIF_SYSCALL_TRACE)
#define _TIF_DELAYED_TRACE	(1 << TIF_DELAYED_TRACE)
#define _TIF_POLLING_NRFLAG     (1 << TIF_POLLING_NRFLAG)
#define _TIF_MEMDIE		(1 << TIF_MEMDIE)
#define _TIF_FREEZE             (1 << TIF_FREEZE)
#define _TIF_RESTORE_SIGMASK    (1 << TIF_RESTORE_SIGMASK)
#define _TIF_SECCOMP            (1 << TIF_SECCOMP)

#endif	/* _ASM_CSKY_THREAD_INFO_H */
