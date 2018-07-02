#ifndef __SILAN_DEINT_H__
#define __SILAN_DEINT_H__

#include <linux/kernel.h>
#include "ex_deinterlace.h"

struct deint_info
{
    EXDITREG __iomem *deint_reg; 
    dma_addr_t  dma_addr;
    dma_addr_t  map_addr;
    dma_addr_t  y_base_addr; 
    dma_addr_t  mv_base_addr; 
    dma_addr_t  y_out_base_addr; 
    unsigned int is_done;
    unsigned int odd_flag;
	unsigned int mv_used;
};

typedef enum
{
    DEINT_CFG,
    DEINT_Y_UPDATE,
    DEINT_START,
    DEINT_CHECK_DONE,
    DEINT_Y_OUTPUT,
}DEINT_CTL;
#endif
