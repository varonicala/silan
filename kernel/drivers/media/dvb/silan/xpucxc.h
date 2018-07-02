#ifndef _XPUCXC_H_
#define _XPUCXC_H_
#include <silan_resources.h>

#define XPU_DTV_MAX_CHANNEL 32
#define XPU_CXC_MAX_FIFO    32
#define XPU_CXC_MAX_BOX      8    

#define XPU_COMMAND_SYNC    0x55
#define XPU_RESPONSE_SYNC   0x56

#define SILAN_XPU_STATUS    (SILAN_XPU_CXC_BASE | 0x9040)

#define CXC_REG_BASE 0
#define CXC_XPU_INT_STATUS  (CXC_REG_BASE + 0x0)
#define CXC_XPU_INT_MASK    (CXC_REG_BASE + 0x4)
#define CXC_XPU_INT_SET     (CXC_REG_BASE + 0x8)
#define CXC_XPU_INT_CLR     (CXC_REG_BASE + 0xc)
#define CXC_HOST_INT_STATUS (CXC_REG_BASE + 0x10)
#define CXC_HOST_INT_MASK   (CXC_REG_BASE + 0x14)
#define CXC_HOST_INT_SET    (CXC_REG_BASE + 0x18)
#define CXC_HOST_INT_CLR    (CXC_REG_BASE + 0x1c)

#define CXC_MAIL_BOX(n)     (CXC_REG_BASE + 0x100 + 4*n)
#define CXC_MUTEX(n)        (CXC_REG_BASE + 0x20 + 4*n)

#define SEC_FILTER_BITS  4

typedef enum
{
    XPU_OPCODE_START = 0,    
    XPU_OPCODE_SET_PID,
    XPU_OPCODE_CLR_PID,
    XPU_OPCODE_CARESET,
    XPU_OPCODE_RESUME,
    XPU_OPCODE_SUSPEND,

    XPU_OPCODE_REV_SEC,
    XPU_OPCODE_REV_PES,
    XPU_OPCODE_END,
}XPU_OPCODE;

struct xpu_pidfilter
{
    u16 channel;
    u16 pid;
    u16 type;
    u8  filter[SEC_FILTER_BITS];
    u8  mask[SEC_FILTER_BITS];
    u8  mode[SEC_FILTER_BITS];
};

struct xpu_pidfilter_respond
{
    u16 channel;
    u16 pid;
    u32 err;
};

struct xpu_ca_status
{
    u32 err;
};

struct xpu_start_status
{
    u32 err;
};

struct xpu_rev_sec
{
    u16 channel;
    u16 pid;
    u32 start_index;
    u32 count;
    u32 err;
};

struct xpu_rev_sec_respond
{
    u16 channel;
    u16 pid;
    u32 count;
    u32 start_index;
    u8 *buf;    
    u32 len;
};

struct xpu_avc_command_frame 
{
    u8  sync;
    u8  size;
    u16 opcode;
    union
    {
        u8 operand[XPU_CXC_MAX_FIFO-4];
        struct xpu_pidfilter pid_filter;
        struct xpu_rev_sec rev_sec;
    }u;
};

struct xpu_avc_response_frame 
{
    u8 sync;
    u8 size;
    u16 opcode;
    union
    {
        u8 operand[XPU_CXC_MAX_FIFO-4];
        struct xpu_pidfilter_respond pid_filter_respond;
        struct xpu_rev_sec_respond rev_sec_respond;
        struct xpu_ca_status ca_status;
    }u;
};

#endif

