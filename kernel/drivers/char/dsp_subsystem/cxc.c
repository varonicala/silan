#include <linux/kernel.h>
#include <linux/io.h>
#include "common.h"
#include "cxc.h"

#define CXC_REG_BASE 0
#define CXC_DSP_INT_STATUS  (CXC_REG_BASE + 0x0)
#define CXC_DSP_INT_MASK    (CXC_REG_BASE + 0x4)
#define CXC_DSP_INT_SET     (CXC_REG_BASE + 0x8)
#define CXC_DSP_INT_CLR     (CXC_REG_BASE + 0xc)
#define CXC_HOST_INT_STATUS (CXC_REG_BASE + 0x10)
#define CXC_HOST_INT_MASK   (CXC_REG_BASE + 0x14)
#define CXC_HOST_INT_SET    (CXC_REG_BASE + 0x18)
#define CXC_HOST_INT_CLR    (CXC_REG_BASE + 0x1c)

#define CXC_MUTEX(n)        (CXC_REG_BASE + 0x20 + 4*(n))
#define CXC_MAIL_BOX(n)     (CXC_REG_BASE + 0x100 + 4*(n))

static int cxc_base;

static unsigned int cxcreg_read(unsigned int reg)
{
    return __raw_readl((void __iomem *)(cxc_base + reg));
}

static void cxcreg_write(unsigned int val, unsigned int reg)
{
    __raw_writel(val, (void __iomem *)(cxc_base + reg));
}

int cxc_read_dsp2mips_fifo(struct mbcmd_st *cmd_st)
{
    int i, *pdata;
    int size = sizeof(struct mbcmd_st)/sizeof(int);

    pdata = (int *)cmd_st;
    if (!cxcreg_read(CXC_MUTEX(0)))
    {
        for (i = 0; i < size; i++)
        {
            *(pdata + i) = cxcreg_read(CXC_MAIL_BOX(i));
        }

        cxcreg_write(0, CXC_MUTEX(0));

        return DSP_ERROR_NONE;
    }

    return DSP_ERROR_CXC;
}

int cxc_write_mips2dsp_fifo(struct mbcmd *cmd)
{
    int i, *pdata;
    int size = sizeof(struct mbcmd)/sizeof(int);

    pdata = (int *)cmd;
    if (!cxcreg_read(CXC_MUTEX(0)))
    {
        for (i = 0; i < size; i++)
        {
            cxcreg_write(*(pdata + i), CXC_MAIL_BOX(i));
        }

        cxcreg_write(0, CXC_MUTEX(0));

        return DSP_ERROR_NONE;
    }

    return DSP_ERROR_CXC;
}

void cxc_set_mips2dsp_int(void)
{
    cxcreg_write(1, CXC_DSP_INT_MASK);
    cxcreg_write(1, CXC_DSP_INT_SET);
}

void cxc_clr_dsp2mips_int(void)
{
    cxcreg_write(0xffffffff, CXC_HOST_INT_CLR);
}

void cxc_clr_mips2dsp_int(void)
{
    cxcreg_write(0xffffffff, CXC_DSP_INT_CLR);
}

void dsp_cxc_init(int base)
{
    cxc_base = base;
}

