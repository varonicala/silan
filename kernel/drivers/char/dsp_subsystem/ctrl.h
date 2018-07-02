#ifndef _CTRL_H
#define _CTRL_H

#include "core.h"

#define DSP_RING_BUF_SIZE           0x100000//0x200000
#define DSP_FIRMWARE_SIZE           0x300000//0x200000
#define DSP_FIRMWARE_NAME           "dsp_firmware.bin"

#define DSP_SW_REG                  SILAN_SYS_REG6
#define DSP_SW_RESETn               (1<<27)

extern int dsp_set_stall(int enable);
extern int dsp_ctrl_init(struct dsp_dev_info *dsp_info);

#endif

