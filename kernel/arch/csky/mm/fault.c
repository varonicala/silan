/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Ye Yun (yun_ye@c-sky.com)
 */
#include <linux/signal.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/version.h>
#include <linux/vt_kern.h>
#include <linux/kernel.h>

#include <asm/hardirq.h>
#include <asm/mmu_context.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/traps.h>
#include <asm/page.h>

extern void die_if_kernel(char *, struct pt_regs *, long);

static inline int delay_slot(struct pt_regs *regs)
{
        return 0;
}

static inline unsigned long exception_epc(struct pt_regs *regs)
{
        if (!delay_slot(regs))
                return regs->pc;

        return regs->pc + 4;
}

int fixup_exception(struct pt_regs *regs)
{
        const struct exception_table_entry *fixup;

        fixup = search_exception_tables(exception_epc(regs));
        if (fixup) {
                regs->pc = fixup->nextinsn;

                return 1;
        }

        return 0;
}

/*
 * This routine handles page faults.  It determines the address,
 * and the problem, and then passes it off to one of the appropriate
 * routines.
 */
# ifdef CONFIG_MMU
asmlinkage void do_page_fault(struct pt_regs *regs, unsigned long write,
                              unsigned long address)
{
        struct vm_area_struct * vma = NULL;
        struct task_struct *tsk = current;
        struct mm_struct *mm = tsk->mm;
        siginfo_t info;
        int fault;

        info.si_code = SEGV_MAPERR;

        /*
         * We fault-in kernel-space virtual memory on-demand. The
         * 'reference' page table is init_mm.pgd.
         *
         * NOTE! We MUST NOT take any locks for this case. We may
         * be in an interrupt or a critical region, and should
         * only copy the information from the master page table,
         * nothing more.
         */
        if (unlikely(address >= VMALLOC_START && address <= VMALLOC_END))
                goto vmalloc_fault;
#ifdef MODULE_START
        if (unlikely(address >= MODULE_START && address < MODULE_END))
                goto vmalloc_fault;
#endif

        /*
         * If we're in an interrupt or have no user
         * context, we must not take the fault..
         */
        if (in_atomic() || !mm)
                goto bad_area_nosemaphore;

        down_read(&mm->mmap_sem);
        vma = find_vma(mm, address);
        if (!vma)
                goto bad_area;
        if (vma->vm_start <= address)
                goto good_area;
        if (!(vma->vm_flags & VM_GROWSDOWN))
                goto bad_area;
        if (expand_stack(vma, address))
                goto bad_area;
/*
 * Ok, we have a good vm_area for this memory access, so
 * we can handle it..
 */
good_area:
info.si_code = SEGV_ACCERR;

        if (write) {
                if (!(vma->vm_flags & VM_WRITE))
                        goto bad_area;
        } else {
                if (!(vma->vm_flags & (VM_READ | VM_WRITE | VM_EXEC)))
                        goto bad_area;
        }

        /*
         * If for any reason at all we couldn't handle the fault,
         * make sure we exit gracefully rather than endlessly redo
         * the fault.
         */
        fault = handle_mm_fault(mm, vma, address, write ? FAULT_FLAG_WRITE : 0);
        if (unlikely(fault & VM_FAULT_ERROR)) {
                if (fault & VM_FAULT_OOM)
                        goto out_of_memory;
                else if (fault & VM_FAULT_SIGBUS)
                        goto do_sigbus;
                BUG();
        }
        if (fault & VM_FAULT_MAJOR)
                tsk->maj_flt++;
        else
                tsk->min_flt++;

        up_read(&mm->mmap_sem);
        return;

/*
 * Something tried to access memory that isn't in our memory map..
 * Fix it, but check if it's kernel or user first..
 */
bad_area:
        up_read(&mm->mmap_sem);

bad_area_nosemaphore:
        /* User mode accesses just cause a SIGSEGV */
        if (user_mode(regs)) {
                tsk->thread.address = address;
                tsk->thread.error_code = write;
#if 0
                printk("do_page_fault() #2: sending SIGSEGV to %s for "
                       "invalid %s\n%0*lx (epc == %0*lx)\n",
                       tsk->comm,
                       write ? "write access to" : "read access from",
                       address,
                       (unsigned long) regs->pc);
#endif
                info.si_signo = SIGSEGV;
                info.si_errno = 0;
                /* info.si_code has been set above */
                info.si_addr = (void __user *) address;
                force_sig_info(SIGSEGV, &info, tsk);
                return;
        }

no_context:
        /* Are we prepared to handle this kernel fault?  */
        if (fixup_exception(regs)) {
                current->thread.baduaddr = address;
                return;
        }
        /*
         * Oops. The kernel tried to access some bad page. We'll have to
         * terminate things with extreme prejudice.
         */
        bust_spinlocks(1);

        printk(KERN_ALERT "Unable to handle kernel paging request at virtual "
               "address %08lx, epc == %08lx\n",
               address, regs->pc);
	die_if_kernel("Oops", regs, write);

out_of_memory:
        /*
         * We ran out of memory, call the OOM killer, and return the userspace
         * (which will retry the fault, or kill us if we got oom-killed).
         */
        pagefault_out_of_memory();
        return;

do_sigbus:
        up_read(&mm->mmap_sem);

        /* Kernel mode? Handle exceptions or die */
        if (!user_mode(regs))
                goto no_context;
        else
        /*
         * Send a sigbus, regardless of whether we were in kernel
         * or user mode.
         */
#if 0
                printk("do_page_fault() #3: sending SIGBUS to %s for "
                       "invalid %s\n%0*lx (epc == %0*lx, ra == %0*lx)\n",
                       tsk->comm,
                       write ? "write access to" : "read access from",
                       field, address,
                       field, (unsigned long) regs->cp0_epc,
                       field, (unsigned long) regs->regs[31]);
#endif
        tsk->thread.address = address;
        info.si_signo = SIGBUS;
        info.si_errno = 0;
        info.si_code = BUS_ADRERR;
        info.si_addr = (void __user *) address;
        force_sig_info(SIGBUS, &info, tsk);

        return;
vmalloc_fault:
        {
                /*
                 * Synchronize this task's top level page-table
                 * with the 'reference' page table.
                 *
                 * Do _not_ use "tsk" here. We might be inside
                 * an interrupt in the middle of a task switch..
                 */
                int offset = __pgd_offset(address);
                pgd_t *pgd, *pgd_k;
                pud_t *pud, *pud_k;
                pmd_t *pmd, *pmd_k;
                pte_t *pte_k;

#ifdef CONFIG_MMU_HARD_REFILL
                unsigned long pgd_base;
#ifdef CONFIG_CPU_MMU_V1
                __asm__ __volatile__("cpseti	cp15		\n\r"
                                     "cprcr	r6, cpcr29	\n\r"
                                     "bclri	r6, 0		\n\r"
				     "subu	r6, %1		\n\r"
                                     "bseti	r6, 31		\n\r"
                                     "mov	%0, r6		\n\r"
                                     :"=r"(pgd_base)
				     :"r"(PHYS_OFFSET)
                                     :"r6");
#else
                __asm__ __volatile__("mfcr  %0, cr<29, 15>\n\r"
                                     "bclri %0, 0         \n\r"
                                     "subu  %0, %1        \n\r"
                                     "bseti %0, 31        \n\r"
                                     :"=&r"(pgd_base)
				     :"r"(PHYS_OFFSET)
                                     :);
#endif	/* CONFIG_CPU_MMU_V1 */
                pgd = (pgd_t *)pgd_base + offset;
#else
                pgd = (pgd_t *) pgd_current[raw_smp_processor_id()] + offset;
#endif /* CONFIG_MMU_HARD_REFILL */
                pgd_k = init_mm.pgd + offset;

                if (!pgd_present(*pgd_k))
                        goto no_context;
                set_pgd(pgd, *pgd_k);

                pud = pud_offset(pgd, address);
                pud_k = pud_offset(pgd_k, address);
                if (!pud_present(*pud_k))
                        goto no_context;

                pmd = pmd_offset(pud, address);
                pmd_k = pmd_offset(pud_k, address);
                if (!pmd_present(*pmd_k))
                        goto no_context;
                set_pmd(pmd, *pmd_k);

                pte_k = pte_offset_kernel(pmd_k, address);
                if (!pte_present(*pte_k))
                        goto no_context;
                return;
        }
}
#else /*CONFIG_MMU*/

asmlinkage void do_page_fault(struct pt_regs *regs, unsigned long write,
                              unsigned long address)
{
}

#endif /*CONFIG_MMU*/

