#ifndef _CORE_H
#define _CORE_H

#define FIRMWARE_NAME_LEN 64

struct dsp_dev_info
{
    int init;
    int open;
    int irq;
    int cxc_base;
    int dsp_base;
    int area0_dma;
    int area0_cpu;
    int area0_len;
    int area1_dma;
    int area1_cpu;
    int area1_len;
    char firmware_name[FIRMWARE_NAME_LEN];
};

#endif

