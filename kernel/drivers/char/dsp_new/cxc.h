#ifndef _CXC_H
#define _CXC_H

#include "common.h"

extern int cxc_read_dsp2mips_fifo(struct mbcmd_st *cmd_st);
extern int cxc_write_mips2dsp_fifo(struct mbcmd *mcmd);
extern void cxc_set_mips2dsp_int(void);
extern void cxc_clr_dsp2mips_int(void);
extern void cxc_clr_mips2dsp_int(void);
extern void dsp_cxc_init(int base);

#endif

