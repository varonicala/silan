/*
 * linux/arch/csky/kernel/signal.c
 *
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * 1997-12-01  Modified for POSIX.1b signals by Andreas Schwab
 *
 * mathemu support by Roman Zippel
 * (Note: fpstate in the signal context is completly ignored for the emulator
 *         and the internal floating point format is put on stack)
 *
 * ++roman (07/09/96): implemented signal stacks (specially for tosemu on
 * Atari :-) Current limitation: Only one sigstack can be active at one time.
 * If a second signal with SA_ONSTACK set arrives while working on a sigstack,
 * SA_ONSTACK is ignored. This behaviour avoids lots of trouble with nested
 * signal handlers!
 *
 * Copyright (C) 2009 Hangzhou C-SKY Microsystems co.,ltd. 
 */

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/syscalls.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>
#include <linux/stddef.h>
#include <linux/highuid.h>
#include <linux/personality.h>
#include <linux/tty.h>
#include <linux/binfmts.h>
#include <linux/tracehook.h>
#include <linux/freezer.h>

#include <asm/setup.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/traps.h>
#include <asm/ucontext.h>
#include <asm/regdef.h>
#include <asm/vdso.h>

#define _BLOCKABLE (~(sigmask(SIGKILL) | sigmask(SIGSTOP)))


asmlinkage void do_notify_resume(unsigned int thread_flags, 
	struct pt_regs *regs, int syscall);

/*
 * Atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int sys_sigsuspend(int restart, unsigned long oldmask,
		old_sigset_t mask)
{
	mask &= _BLOCKABLE;
	spin_lock_irq(&current->sighand->siglock);
	current->saved_sigmask = current->blocked;
	siginitset(&current->blocked, mask);
	recalc_sigpending();
	spin_unlock_irq(&current->sighand->siglock);
	
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	set_restore_sigmask();
	return -ERESTARTNOHAND;
}

asmlinkage int sys_sigaction(int sig, const struct old_sigaction *act,
	      struct old_sigaction *oact)
{
	struct k_sigaction new_ka, old_ka;
	int ret;

	if (act) {
		old_sigset_t mask;
		if (verify_area(VERIFY_READ, act, sizeof(*act)) ||
		    __get_user(new_ka.sa.sa_handler, &act->sa_handler) ||
		    __get_user(new_ka.sa.sa_restorer, &act->sa_restorer))
			return -EFAULT;
		__get_user(new_ka.sa.sa_flags, &act->sa_flags);
		__get_user(mask, &act->sa_mask);
		siginitset(&new_ka.sa.sa_mask, mask);
	}

	ret = do_sigaction(sig, act ? &new_ka : NULL, oact ? &old_ka : NULL);

	if (!ret && oact) {
		if (verify_area(VERIFY_WRITE, oact, sizeof(*oact)) ||
		    __put_user(old_ka.sa.sa_handler, &oact->sa_handler) ||
		    __put_user(old_ka.sa.sa_restorer, &oact->sa_restorer))
			return -EFAULT;
		__put_user(old_ka.sa.sa_flags, &oact->sa_flags);
		__put_user(old_ka.sa.sa_mask.sig[0], &oact->sa_mask);
	}

	return ret;
}

asmlinkage int
sys_sigaltstack(const stack_t *uss, stack_t *uoss)
{
	return do_sigaltstack(uss, uoss, rdusp());
}

struct sigframe
{
	int sig;
	int code;
	struct sigcontext *psc;
	unsigned long extramask[_NSIG_WORDS-1];
	struct sigcontext sc;
};

struct rt_sigframe
{
	int sig;
	struct siginfo *pinfo;
	void *puc;
	struct siginfo info;
	struct ucontext uc;
};

typedef struct fpregset {
	int f_fcr;
	int f_fsr;		/* Nothing in CPU_CSKYV2 */
	int f_fesr;
	int f_feinst1;		/* Nothing in CPU_CSKYV2 */
	int f_feinst2;		/* Nothing in CPU_CSKYV2 */
	int f_fpregs[32];
} fpregset_t;

static inline int restore_fpu_state(struct sigcontext *sc)
{
	int err = 0;
#ifdef CONFIG_CPU_HAS_FPU
	fpregset_t fpregs;
	unsigned long flg;
	unsigned long tmp1, tmp2, tmp3, tmp4;
	unsigned long fctl0, fctl1, fctl2;
	int * fpgr;

	if (__copy_from_user(&fpregs, &sc->sc_fcr, sizeof(fpregs)))
	{
		err = 1;
		goto out;
	}
	
	local_irq_save(flg);
	fctl0 = fpregs.f_fcr;
	fctl1 = fpregs.f_fsr;
	fctl2 = fpregs.f_fesr;
	fpgr = &(fpregs.f_fpregs[0]);
#if defined(CONFIG_CPU_CSKYV1)
	__asm__ __volatile__("cpseti 1\n\r"
	                     "mov    r7, %0    \n\r"
	                     "cpwcr  r7, cpcr1 \n\r"
	                     "mov    r7, %1    \n\r"
	                     "cpwcr  r7, cpcr2 \n\r"
	                     "mov    r7, %2    \n\r"
	                     "cpwcr  r7, cpcr4 \n\r"
	                     : :"r"(fctl0), "r"(fctl1), "r"(fctl2)
	                     : "r7" );

	__asm__ __volatile__("cpseti 1\n\r"
	                     LDW_FPU_REGS(0, 4, 8, 12)
	                     FMTS_FPU_REGS(fr0, fr1, fr2, fr3)
	                     LDW_FPU_REGS(16, 20, 24, 28)
	                     FMTS_FPU_REGS(fr4, fr5, fr6, fr7)
	                     LDW_FPU_REGS(32, 36, 40, 44)
	                     FMTS_FPU_REGS(fr8, fr9, fr10, fr11)
	                     LDW_FPU_REGS(48, 52, 56, 60)
	                     FMTS_FPU_REGS(fr12, fr13, fr14, fr15)
	                     "addi   %4,32 \n\r"
	                     "addi   %4,32 \n\r"
	                     LDW_FPU_REGS(0, 4, 8, 12)
	                     FMTS_FPU_REGS(fr16, fr17, fr18, fr19)
	                     LDW_FPU_REGS(16, 20, 24, 28)
	                     FMTS_FPU_REGS(fr20, fr21, fr22, fr23)
	                     LDW_FPU_REGS(32, 36, 40, 44)
	                     FMTS_FPU_REGS(fr24, fr25, fr26, fr27)
	                     LDW_FPU_REGS(48, 52, 56, 60)
	                     FMTS_FPU_REGS(fr28, fr29, fr30, fr31)
	                     :"=r"(tmp1), "=r"(tmp2),"=r"(tmp3),"=r"(tmp4),
	                      "+r"(fpgr));
#elif defined(CONFIG_CPU_CSKYV2)
	__asm__ __volatile__("mtcr   %0, cr<1, 2> \n\r"
	                     "mtcr   %1, cr<2, 2> \n\r" 
	                     : :"r"(fctl0), "r"(fctl2));

	__asm__ __volatile__(LDW_FPU_REGS(0, 4, 8, 12)
	                     FMTVR_FPU_REGS(vr0, vr1)
	                     LDW_FPU_REGS(16, 20, 24, 28)
	                     FMTVR_FPU_REGS(vr2, vr3)
	                     LDW_FPU_REGS(32, 36, 40, 44)
	                     FMTVR_FPU_REGS(vr4, vr5)
	                     LDW_FPU_REGS(48, 52, 56, 60)
	                     FMTVR_FPU_REGS(vr6, vr7)
	                     "addi   %4, 32 \n\r"
	                     "addi   %4, 32 \n\r"
	                     LDW_FPU_REGS(0, 4, 8, 12)
	                     FMTVR_FPU_REGS(vr8, vr9)
	                     LDW_FPU_REGS(16, 20, 24, 28)
	                     FMTVR_FPU_REGS(vr10, vr11)
	                     LDW_FPU_REGS(32, 36, 40, 44)
	                     FMTVR_FPU_REGS(vr12, vr13)
	                     LDW_FPU_REGS(48, 52, 56, 60)
	                     FMTVR_FPU_REGS(vr14, vr15)
	                     :"=a"(tmp1), "=a"(tmp2), "=a"(tmp3), "=a"(tmp4),
	                      "+a"(fpgr));
#endif
	local_irq_restore(flg);
out:
#endif
	return err;
}

static inline int
restore_sigframe(struct pt_regs *regs, struct sigcontext *usc, int *pr2)
{
	int err = 0;
 	int i = 0;	
	unsigned long usp;

	/* Always make any pending restarted system calls return -EINTR */
        current_thread_info()->restart_block.fn = do_no_restart_syscall;
         
	/* restore passed registers */
	err |= __get_user(regs->a0, &usc->sc_a0);
        err |= __get_user(regs->a1, &usc->sc_a1);
        err |= __get_user(regs->a2, &usc->sc_a2);
        err |= __get_user(regs->a3, &usc->sc_a3);
        for(i = 0; i < 10; i++)
        {
        	err |= __get_user(regs->regs[i], &usc->sc_regs[i]);
        }
	err |= __get_user(regs->r15, &usc->sc_r15);
#if defined(CONFIG_CPU_CSKYV2)
	for(i = 0; i < 16; i++)
        {
                err |= __get_user(regs->exregs[i], &usc->sc_exregs[i]);
        }
	err |= __get_user(regs->rhi, &usc->sc_rhi);
	err |= __get_user(regs->rlo, &usc->sc_rlo);
#endif
	err |= __get_user(regs->sr, &usc->sc_sr);
	err |= __get_user(regs->pc, &usc->sc_pc);
	err |= __get_user(usp, &usc->sc_usp);
	wrusp(usp);
	err |= restore_fpu_state(usc);	
	*pr2 = regs->a0;
	return err;
}

asmlinkage int do_sigreturn(struct pt_regs *regs)
{
	unsigned long usp = rdusp();
	struct sigframe *frame = (struct sigframe *)usp;
	sigset_t set;
	int a0;

	if (verify_area(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__get_user(set.sig[0], &frame->sc.sc_mask) ||
	    (_NSIG_WORDS > 1 &&
	     __copy_from_user(&set.sig[1], &frame->extramask,
			      sizeof(frame->extramask))))
		goto badframe;

	sigdelsetmask(&set, ~_BLOCKABLE);
	spin_lock_irq(&current->sighand->siglock);
	current->blocked = set;
	recalc_sigpending( );
	spin_unlock_irq(&current->sighand->siglock);

	if (restore_sigframe(regs, &frame->sc, &a0))
		goto badframe;
	return a0;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

asmlinkage int do_rt_sigreturn(struct pt_regs *regs)
{
	unsigned long usp = rdusp();
	struct rt_sigframe *frame = (struct rt_sigframe *)usp;
	sigset_t set;
	int a0;

	if (verify_area(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	sigdelsetmask(&set, ~_BLOCKABLE);
	spin_lock_irq(&current->sighand->siglock);
	current->blocked = set;
	recalc_sigpending( );
	spin_unlock_irq(&current->sighand->siglock);
	
	if (restore_sigframe(regs, &frame->uc.uc_mcontext, &a0))
		goto badframe;
	
	if (do_sigaltstack(&frame->uc.uc_stack, NULL, usp) == -EFAULT)
		goto badframe;

	return a0;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

/*
 * Set up a signal frame.
 */

static inline int save_fpu_state(struct sigcontext *sc, struct pt_regs *regs)
{
	int err = 0;
#ifdef CONFIG_CPU_HAS_FPU
	fpregset_t fpregs;
	unsigned long flg;
	unsigned long tmp1, tmp2, tmp3, tmp4;
	int * fpgr;

	local_irq_save(flg);
	fpgr = &(fpregs.f_fpregs[0]);
#if defined(CONFIG_CPU_CSKYV1)
	__asm__ __volatile__("cpseti 1\n\r"
	                     "cprcr  r7, cpcr1 \n\r"
	                     "mov    %0, r7    \n\r"
	                     "cprcr  r7, cpcr2 \n\r"
	                     "mov    %1, r7    \n\r"
	                     "cprcr  r7, cpcr4 \n\r"
	                     "mov    %2, r7    \n\r"
	                     "btsti  r7, 30    \n\r"
	                     "cprcr  r7, cpcr5 \n\r"
	                     "mov    %3, r7    \n\r"
	                     "bf     1f        \n\r"
	                     "cprcr  r7, cpcr6 \n\r"
	                     "mov    %4, r7    \n\r"
	                     "1:               \n\r"
	                     : "=a"(fpregs.f_fcr), "=a"(fpregs.f_fsr),
	                       "=a"(fpregs.f_fesr), "=a"(fpregs.f_feinst1),
	                       "=a"(fpregs.f_feinst2)
	                     :: "r7");

	__asm__ __volatile__("cpseti 1\n\r"
	                     FMFS_FPU_REGS(fr0, fr1, fr2, fr3)
	                     STW_FPU_REGS(0, 4, 8, 12)
	                     FMFS_FPU_REGS(fr4, fr5, fr6, fr7)
	                     STW_FPU_REGS(16, 20, 24, 28)
	                     FMFS_FPU_REGS(fr8, fr9, fr10, fr11)
	                     STW_FPU_REGS(32, 36, 40, 44)
	                     FMFS_FPU_REGS(fr12, fr13, fr14, fr15)
	                     STW_FPU_REGS(48, 52, 56, 60)
	                     "addi   %4,32 \n\r" 
	                     "addi   %4,32 \n\r"
	                     FMFS_FPU_REGS(fr16, fr17, fr18, fr19)
	                     STW_FPU_REGS(0, 4, 8, 12)
	                     FMFS_FPU_REGS(fr20, fr21, fr22, fr23)
	                     STW_FPU_REGS(16, 20, 24, 28)
	                     FMFS_FPU_REGS(fr24, fr25, fr26, fr27)
	                     STW_FPU_REGS(32, 36, 40, 44)
	                     FMFS_FPU_REGS(fr28, fr29, fr30, fr31)
	                     STW_FPU_REGS(48, 52, 56, 60)
	                     :"=a"(tmp1), "=a"(tmp2),"=a"(tmp3),"=a"(tmp4),
	                       "+a"(fpgr));
#elif defined(CONFIG_CPU_CSKYV2)
	__asm__ __volatile__("mfcr    %0, cr<1, 2> \n\r"
	                     "mfcr    %1, cr<2, 2> \n\r"
	                     : "=r"(fpregs.f_fcr), "=r"(fpregs.f_fesr));

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
	                       "+a"(fpgr));
#endif
	local_irq_restore(flg);

	err |= copy_to_user(&sc->sc_fcr, &fpregs, sizeof(fpregs));
#endif
	return err;
}

static inline int setup_sigframe(struct sigcontext *sc, struct pt_regs *regs,
			     unsigned long mask)
{
	int err = 0;
	int i = 0;
	
	err |= __put_user(mask, &sc->sc_mask);
	err |= __put_user(rdusp(), &sc->sc_usp);
	err |= __put_user(regs->a0, &sc->sc_a0);
	err |= __put_user(regs->a1, &sc->sc_a1);
	err |= __put_user(regs->a2, &sc->sc_a2);
	err |= __put_user(regs->a3, &sc->sc_a3);
	for(i = 0; i < 10; i++)
	{
		err |= __put_user(regs->regs[i], &sc->sc_regs[i]);
	}
	err |= __put_user(regs->r15, &sc->sc_r15);
#if defined(CONFIG_CPU_CSKYV2)
 	for(i = 0; i < 16; i++)
	{	
		err |= __put_user(regs->exregs[i], &sc->sc_exregs[i]);
	}
	err |= __put_user(regs->rhi, &sc->sc_rhi);
	err |= __put_user(regs->rlo, &sc->sc_rlo);
#endif
	err |= __put_user(regs->sr, &sc->sc_sr);
	err |= __put_user(regs->pc, &sc->sc_pc);
	err |= save_fpu_state(sc, regs);

	return err;
}

static inline void *
get_sigframe(struct k_sigaction *ka, struct pt_regs *regs, size_t frame_size)
{
	unsigned long usp;

	/* Default to using normal stack.  */
	usp = rdusp();

	/* This is the X/Open sanctioned signal stack switching.  */
	if ((ka->sa.sa_flags & SA_ONSTACK) && !sas_ss_flags(usp)) {
		if (!on_sig_stack(usp))
			usp = current->sas_ss_sp + current->sas_ss_size;
	}
	return (void *)((usp - frame_size) & -8UL);
}

static int setup_frame (int sig, struct k_sigaction *ka,
			 sigset_t *set, struct pt_regs *regs)
{
	struct sigframe *frame;
	int err = 0;
#ifdef CONFIG_MMU
        struct csky_vdso *vdso = current->mm->context.vdso;
#else
        struct csky_vdso *vdso = global_vdso;
#endif

	frame = get_sigframe(ka, regs, sizeof(*frame));
	if (!frame)
		return 1;

	err |= __put_user((current_thread_info()->exec_domain
			   && current_thread_info()->exec_domain->signal_invmap
			   && sig < 32
			   ? current_thread_info()->exec_domain->signal_invmap[sig]
			   : sig),
			  &frame->sig);

	err |= __put_user(&frame->sc, &frame->psc);

	if (_NSIG_WORDS > 1)
		err |= copy_to_user(frame->extramask, &set->sig[1],
				    sizeof(frame->extramask));

	err |= setup_sigframe(&frame->sc, regs, set->sig[0]);

	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	wrusp ((unsigned long) frame);
	regs->pc = (unsigned long) ka->sa.sa_handler;
	regs->r15 = (unsigned long)vdso->signal_retcode;
	regs->a0 = sig;
	return err;

give_sigsegv:
	if (sig == SIGSEGV)
		ka->sa.sa_handler = SIG_DFL;
	force_sig(SIGSEGV, current);

	return err;
}

static int setup_rt_frame (int sig, struct k_sigaction *ka, siginfo_t *info,
			    sigset_t *set, struct pt_regs *regs)
{
	struct rt_sigframe *frame;
	int err = 0;
#ifdef CONFIG_MMU
        struct csky_vdso *vdso = current->mm->context.vdso;
#else
        struct csky_vdso *vdso = global_vdso;
#endif

	frame = get_sigframe(ka, regs, sizeof(*frame));
	if (!frame)
		return 1;

	err |= __put_user((current_thread_info()->exec_domain
			   && current_thread_info()->exec_domain->signal_invmap
			   && sig < 32
			   ? current_thread_info()->exec_domain->signal_invmap[sig]
			   : sig),
			  &frame->sig);
	err |= __put_user(&frame->info, &frame->pinfo);
	err |= __put_user(&frame->uc, &frame->puc);
	err |= copy_siginfo_to_user(&frame->info, info);

	/* Create the ucontext.  */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(0, &frame->uc.uc_link);
	err |= __put_user((void *)current->sas_ss_sp,
			  &frame->uc.uc_stack.ss_sp);
	err |= __put_user(sas_ss_flags(rdusp()),
			  &frame->uc.uc_stack.ss_flags);
	err |= __put_user(current->sas_ss_size, &frame->uc.uc_stack.ss_size);
	err |= setup_sigframe(&frame->uc.uc_mcontext, regs, 0);
	err |= copy_to_user (&frame->uc.uc_sigmask, set, sizeof(*set));

	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	wrusp ((unsigned long) frame);
	regs->pc = (unsigned long) ka->sa.sa_handler;
	regs->r15 = (unsigned long)vdso->rt_signal_retcode;

adjust_stack:
	regs->a0 = sig; /* first arg is signo */
	regs->a1 = (unsigned long)(&(frame->info)); /* second arg is (siginfo_t*) */
	regs->a2 = (unsigned long)(&(frame->uc));/* third arg pointer to ucontext */
	return err;

give_sigsegv:
	if (sig == SIGSEGV)
		ka->sa.sa_handler = SIG_DFL;
	force_sig(SIGSEGV, current);
	goto adjust_stack;
}

/*
 * OK, we're invoking a handler
 */
static int
handle_signal(int sig, struct k_sigaction *ka, siginfo_t *info,
	      sigset_t *oldset, struct pt_regs *regs)
{
	struct task_struct *tsk = current;
	int ret;

	/* set up the stack frame */
	if (ka->sa.sa_flags & SA_SIGINFO)
		ret = setup_rt_frame(sig, ka, info, oldset, regs);
	else
		ret = setup_frame(sig, ka, oldset, regs);

	if (ret != 0) {
		force_sigsegv(sig, tsk);
		return ret;
	}

	/* Block the signal if we were successful. */
	spin_lock_irq(&current->sighand->siglock);
	sigorsets(&current->blocked, &current->blocked, &ka->sa.sa_mask);
	if (!(ka->sa.sa_flags & SA_NODEFER))
		sigaddset(&current->blocked, sig);
	recalc_sigpending();
	spin_unlock_irq(&current->sighand->siglock);

	return 0;
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals
 * that the kernel can handle, and then we build all the user-level signal
 * handling stack-frames in one go after that.
 */
static void do_signal(struct pt_regs *regs, int syscall)
{
	unsigned int retval = 0, continue_addr = 0, restart_addr = 0;
	struct k_sigaction ka;
	siginfo_t info;
	int signr;

	/*
	 * We want the common case to go fast, which
	 * is why we may in certain cases get here from
	 * kernel mode. Just return without doing anything
	 * if so.
	 */
	if (!user_mode(regs))
		return;

	current->thread.esp0 = (unsigned long)regs;

	/*
     * If we were from a system call, check for system call restarting...
     */
    if (syscall) {
        continue_addr = regs->pc;
#if defined(CONFIG_CPU_CSKYV1)
        restart_addr = continue_addr - 2;
#else
        restart_addr = continue_addr - 4;
#endif
        retval = regs->a0;

		/*
         * Prepare for system call restart.  We do this here so that a
         * debugger will see the already changed.
         */
        switch (retval) {
        case -ERESTARTNOHAND:
        case -ERESTARTSYS:
        case -ERESTARTNOINTR:
            regs->a0 = regs->orig_a0;
            regs->pc = restart_addr;
            break;
        case -ERESTART_RESTARTBLOCK:
            regs->a0 = -EINTR;
            break;
        }
    }

	if (try_to_freeze())
        goto no_signal;

	/*
     * Get the signal to deliver.  When running under ptrace, at this
     * point the debugger may change all our registers ...
     */
	signr = get_signal_to_deliver(&info, &ka, regs, NULL);
	if (signr > 0) {
		sigset_t *oldset;

		/*
         * Depending on the signal settings we may need to revert the
         * decision to restart the system call.  But skip this if a
         * debugger has chosen to restart at a different PC.
         */
        if (regs->pc == restart_addr) {
            if (retval == -ERESTARTNOHAND
                || (retval == -ERESTARTSYS
                && !(ka.sa.sa_flags & SA_RESTART))) {
                regs->a0 = -EINTR;
                regs->pc = continue_addr;
            }
        }

        if (test_thread_flag(TIF_RESTORE_SIGMASK))
            oldset = &current->saved_sigmask;
        else
            oldset = &current->blocked;
		/* Whee!  Actually deliver the signal.  */
		if (handle_signal(signr, &ka, &info, oldset, regs) == 0) {
			/*
			 * A signal was successfully delivered; the saved
			 * sigmask will have been stored in the signal frame,
			 * and will be restored by sigreturn, so we can simply
			 * clear the TIF_RESTORE_SIGMASK flag.
			 */
			if (test_thread_flag(TIF_RESTORE_SIGMASK))
				clear_thread_flag(TIF_RESTORE_SIGMASK);
		}
		return;
	}

 no_signal:
    if (syscall) {
		/*
         * Handle restarting a different system call.  As above,
         * if a debugger has chosen to restart at a different PC,
         * ignore the restart.
         */
        if (retval == -ERESTART_RESTARTBLOCK
            && regs->pc == continue_addr) {
#if defined(CONFIG_CPU_CSKYV1)
			regs->regs[9] = __NR_restart_syscall;
			regs->pc -= 2;
#else
			regs->regs[3] = __NR_restart_syscall;
			regs->pc -= 4;
#endif
        }

		/* If there's no signal to deliver, we just put the saved sigmask
         * back.
         */
        if (test_thread_flag(TIF_RESTORE_SIGMASK)) {
            clear_thread_flag(TIF_RESTORE_SIGMASK);
            sigprocmask(SIG_SETMASK, &current->saved_sigmask, NULL);
        }
    }
}

asmlinkage void
do_notify_resume(unsigned int thread_flags, struct pt_regs *regs, int syscall)
{
	if (thread_flags & _TIF_SIGPENDING)
		do_signal(regs, syscall);

	if (thread_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
		if (current->replacement_session_keyring)
			key_replace_session_keyring();
	}
}
