/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2013 C-SKY Microsystem Co.,Ltd 
 */

#ifndef __ASM_CSKY_VDSO_H
#define __ASM_CSKY_VDSO_H

struct csky_vdso {
	unsigned short signal_retcode[4];
	unsigned short rt_signal_retcode[4];
};

#ifndef CONFIG_MMU
extern struct csky_vdso *global_vdso;
#endif

#endif /* __ASM_CSKY_VDSO_H */
