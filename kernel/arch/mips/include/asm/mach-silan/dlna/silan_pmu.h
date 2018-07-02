#ifndef _SILAN_PMU_H_
#define _SILAN_PMU_H_
#include <silan_resources.h>

#define SILAN_PMU_CFG           SILAN_PMU_BASE
#define SILAN_PMU_INTMASK       (SILAN_PMU_BASE | 0x0c)
#define SILAN_PMU_INTPOL        (SILAN_PMU_BASE | 0x10)
#define SILAN_PMU_INTCLR        (SILAN_PMU_BASE | 0x14) 

#define SILAN_PMU_INT_SRC       3
#define SILAN_PMU_INT_POL_H     1
#define SILAN_PMU_INT_POL_L     0

#define SILAN_PMU_EXIT          1
#define SILAN_PMU_REQUEST       (1 << 1)
#define SILAN_PMU_BYPASS        (1 << 2)
#define SILAN_PMU_CLK_SWITCH    (0 << 3)
#define SILAN_PMU_CLK_GATE      (1 << 3)
#define SILAN_PMU_PLL_UPDATE    (2 << 3)
#define SILAN_PMU_PLL_PDOWN     (3 << 3)
#define SILAN_PMU_CPU_PLL       (0 << 8)
#define SILAN_PMU_SYS_PLL       (1 << 8)

enum silan_pmu_mode
{
    clock_switch,
    clock_gate,
    cpu_pll_update,
    cpu_pll_power_down,
};

enum silan_pmu_pll_source
{
    cpu_pll,
    sys_pll,
};

struct pmu_reg
{
    unsigned char exit                      : 1;
    unsigned char request                   : 1;
    unsigned char bypass                    : 1;
    enum silan_pmu_mode mode                : 2;
    unsigned                                : 3;
    unsigned char pmu_pll                   : 3;
    enum silan_pmu_pll_source pll_source    : 1;
    unsigned char silan_pmu_clock_on        : 1;
    unsigned int reserved                   : 19;
};


union silan_pmu_cfg
{
    unsigned int    data;
    struct pmu_reg  reg;
};

int silan_pmu_enter(union silan_pmu_cfg _cfg);
int silan_pmu_exit(void);
#endif

