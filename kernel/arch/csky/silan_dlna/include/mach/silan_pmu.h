#ifndef _SILAN_PMU_H_
#define _SILAN_PMU_H_
#include <silan_resources.h>

#define SILAN_PMU_CFG           SILAN_PMU_BASE
#define SILAN_PMU_PLL_CFG_L     (SILAN_PMU_BASE | 0x04)
#define SILAN_PMU_PLL_CFG_H     (SILAN_PMU_BASE | 0x08)
#define SILAN_PMU_INTMASK       (SILAN_PMU_BASE | 0x0c)
#define SILAN_PMU_INTPOL        (SILAN_PMU_BASE | 0x10)
#define SILAN_PMU_INTCLR        (SILAN_PMU_BASE | 0x14) 
#define SILAN_PMU_STATUS        (SILAN_PMU_BASE | 0x20) 
#define SILAN_PMU_PLL_STATUS_L  (SILAN_PMU_BASE | 0x24) 
#define SILAN_PMU_PLL_STATUS_H  (SILAN_PMU_BASE | 0x28) 

#define SILAN_PMU_INT_SRC       14 // GPIO1_9
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
    CLOCK_SWITCH,
    CLOCK_GATE,
    PLL_UPDATE,
    PLL_POWERDOWN,
};

enum silan_pmu_clk_source
{
    SYS_PLL,
    OSC_12M
};

enum silan_pmu_cpu_ratio
{
    RATIO_1TO1,
    RATIO_2TO1,
};

struct pmu_cfg
{
    unsigned exit                             : 1;
    unsigned request                          : 1;
    unsigned bypass                           : 1;
    enum     silan_pmu_mode mode              : 2;
    unsigned sref_mode                        : 1;    
    unsigned sref_req                         : 1;
    unsigned sref_cnt_en                      : 1;
    unsigned clock_switch                     : 3;
    enum     silan_pmu_clk_source clk_source  : 1;
    unsigned cpu_clock_on                     : 1;
    unsigned sys_clock_on                     : 1;
    enum     silan_pmu_cpu_ratio clock_ratio  : 1;
    unsigned reversed                         : 17;
};

union silan_pmu_cfg
{
    unsigned int    data;
    struct pmu_cfg  reg;
};

struct pmu_pll_l
{
    unsigned pll_reset                 : 1;
    unsigned pll_pwd                   : 1;
    unsigned reversed0                 : 2;
    unsigned pll_dn                    : 6;
    unsigned pll_dm                    : 6;    
    unsigned pll_cfg_sel               : 1;
    unsigned reversed1                 : 11;
    unsigned pll_fast_cfg              : 4;
};

union silan_pmu_pll_l
{
    unsigned int      data;
    struct pmu_pll_l  reg;
};

#endif

