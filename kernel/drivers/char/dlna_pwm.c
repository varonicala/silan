#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <mach/silan_resources.h>
#include <mach/silan_padmux.h>
#include <silan_irq.h>
#include <linux/pwm.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/miscdevice.h>

#define PWM_IOCTL_MAGIC        'e'
#define PWM_IOCTL_RESET         _IO(PWM_IOCTL_MAGIC, 1)
#define PWM_IOCTL_ENABLE        _IO(PWM_IOCTL_MAGIC, 2)
#define PWM_IOCTL_DISABLE       _IO(PWM_IOCTL_MAGIC, 5)
#define PWM_IOCTL_SET_DATA      _IO(PWM_IOCTL_MAGIC, 3)
#define PWM_IOCTL_CHANNEL_SEL   _IO(PWM_IOCTL_MAGIC, 4)
#define PWM_IOCTL_CLK_SEL       _IO(PWM_IOCTL_MAGIC, 6)
#define PWM_SEL_PWM0    0x01
#define PWM_SEL_PWM1    0x02
#define PWM_SEL_PWM2    0x04
#define PWM_SEL_PWM3    0x08
#define PWM_SEL_PWM4    0x10
#define PWM_SEL_PWM5    0x20
#define PWM_SEL_PWM6    0x40
#define PWM_SEL_PWM7    0x80
#define PWM_CLK1        (0x0157C000 * 80)
#define PWM_CLK2        (0x01770000 * 80)
#define PWM_CH1_EN      0x000000ff
#define PWM_CH2_EN      0x070000f1
#define PWM_CH_CLEAR    0xf8ffff00
#define EPS (1e-6)
#define CYCLE_MAX       1000
#define PSC_MAX         65536
#define FRE_TYPE        80000000


//#define REG32(addr)           (*(volatile unsigned int *)(addr))

static int pwm_clk = PWM_CLK1;

static u16 pwm_read (u32 offset)
{
	return readl (PWM_BASE + offset);
}

struct pwm_data_t
{
    int num;
    int channel;
    int enable;
    int clk;
    int h_time;
    int l_time;
};

static int pwm_write (u32 offset, u16 val)
{
    writel(val, PWM_BASE + offset);
    return 1;
}

int write_pwm_psc(u16 val)
{
    return pwm_write(PWM_PSC, val);
}

u16 read_pwm_psc(void)
{
    return pwm_read(PWM_PSC);
}

int write_pwm_data(u8 num, u16 val)
{
    switch(num)
    {
        case 0:
            return pwm_write(PWM_0D, val);
        case 1:
            return pwm_write(PWM_1D, val);
        case 2:
            return pwm_write(PWM_2D, val);
        case 3:
            return pwm_write(PWM_3D, val);
        case 4:
            return pwm_write(PWM_4D, val);
        case 5:
            return pwm_write(PWM_5D, val);
        case 6:
            return pwm_write(PWM_6D, val);
        case 7:
            return pwm_write(PWM_7D, val);
        default:
            return 0;
    }
}

u16 read_pwm_data(u8 num)
{
    switch(num)
    {
        case 0:
            return pwm_read(PWM_0D);
        case 1:
            return pwm_read(PWM_1D);
        case 2:
            return pwm_read(PWM_2D);
        case 3:
            return pwm_read(PWM_3D);
        case 4:
            return pwm_read(PWM_4D);
        case 5:
            return pwm_read(PWM_5D);
        case 6:
            return pwm_read(PWM_6D);
        case 7:
            return pwm_read(PWM_7D);
        default:
            return 0;
    }
}

int write_pwm_en(u8 num, u16 val)
{
    u16 pwm_con = pwm_read(PWM_CON);
    u8 tmp = pwm_con >> (num + PWM_0EN);
    if(tmp & 1)
        pwm_con = pwm_con - (1 << (num + PWM_0EN));
    else
        pwm_con = pwm_con + (1 << (num + PWM_0EN));
    return pwm_write(PWM_CON, pwm_con);
}

u16 read_pwm_en(u8 num)
{
    u16 pwm_con = pwm_read(PWM_CON);
    return (pwm_con >> (num + PWM_0EN)) & 1;
}

int write_pwm_readsel(u8 num, u16 val)
{
    u16 pwm_con = pwm_read(PWM_CON);
    u8 tmp = pwm_con >> (num + PWM_READSEL0);
    if(tmp & 1)
        pwm_con = pwm_con - (1 << (num + PWM_READSEL0));
    else
        pwm_con = pwm_con + (1 << (num + PWM_READSEL0));
    return pwm_write(PWM_CON, pwm_con);
}

u16 read_pwm_readsel(u8 num)
{
    u16 pwm_con = pwm_read(PWM_CON);
    return (pwm_con >> (num + PWM_READSEL0)) & 1;
}

int write_pwm_p(u8 num, u16 val)
{
    switch (num)
    {
        case 0:
            return pwm_write(PWM_P01, val);
        case 1:
            return pwm_write(PWM_P23, val);
        case 2:
            return pwm_write(PWM_P45, val);
        case 3:
            return pwm_write(PWM_P67, val);
        default:
            return 0;
    }
}

u16 read_pwm_p(u8 num)
{
    switch (num)
    {
        case 0:
            return read_pwm_p(PWM_P01);
        case 1:
            return read_pwm_p(PWM_P23);
        case 2:
            return read_pwm_p(PWM_P45);
        case 3:
            return read_pwm_p(PWM_P67);
        default:
            return 0;
    }
}

int pwm_reset(void)
{
    pwm_clk = PWM_CLK1;
    pwm_write(PWM_PSC, 0x0000);
    pwm_write(PWM_0D, 0x0000);
    pwm_write(PWM_1D, 0x0000);
    pwm_write(PWM_2D, 0x0000);
    pwm_write(PWM_3D, 0x0000);
    pwm_write(PWM_4D, 0x0000);
    pwm_write(PWM_5D, 0x0000);
    pwm_write(PWM_6D, 0x0000);
    pwm_write(PWM_7D, 0x0000);
    pwm_write(PWM_P01, 0x0fff);
    pwm_write(PWM_P23, 0x0fff);
    pwm_write(PWM_P45, 0x0fff);
    pwm_write(PWM_P67, 0x0fff);
    pwm_write(PWM_CON, 0x0000);
    return 1;
}
/*
static int get_total_cycles(int psc, int fre)
{
    return PWM_CLK / (fre * (psc + 1));
}

static int sqrt_cycle(int n)
{
    int ans, low, up;
    if(n < 0)
        return n;
    low = 0;
    up = n;
    ans = (low + up) / 2;
    while(up - low > 1)
    {
        if(ans*ans > n)
            up = ans;
        else
            low = ans;
        ans = (up + low)/2;
    }
    return ans;
}
*/
static unsigned int get_cycle(unsigned int clk, unsigned int fre)
{
    unsigned int res, i;
    for(i = 2; i < PSC_MAX + 1; i <<= 1)
    {
        if(i == PSC_MAX)
            i --;
        res = clk / i / fre;
        if(res < CYCLE_MAX)
            return res;
    }
    return 0;
}

int pwm_disable(void)
{
    //REG32(KSEG1(SILAN_CR_BASE+0x10)) &= ~(1 << 10);
    int val = readl(SILAN_CR_BASE + 0x10);
    writel(val&(~(1<<10)), SILAN_CR_BASE + 0x10);
#if 0
    REG32(KSEG1(SILAN_SYSTEM_CTL_REG9)) = 0x02800082;
#else
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM0, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM1, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM3, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM4, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM5, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM6, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM7, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM1_CH2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM2_CH2, PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM3_CH2, PAD_OFF);
#endif

    return 0;
}

int pwm_clk_sel(int clk)
{
    if(clk == 1)
        pwm_clk = PWM_CLK1;
    else
        pwm_clk = PWM_CLK2;
    return 1;
}

int pwm_channel_sel(int channel, int enable)
{
    
//    REG32(KSEG1(SILAN_CR_BASE+0x10)) |= 1 << 10;
    int val = readl(SILAN_CR_BASE + 0x10);
    writel(val|(1<<10), SILAN_CR_BASE + 0x10);
#if 0
    REG32(KSEG1(SILAN_SYSTEM_CTL_REG9)) = 0x02800082;
    unsigned long value = REG32(KSEG1(SILAN_PADMUX_CTRL2));
    value |= 0xff;
    REG32(KSEG1(SILAN_PADMUX_CTRL2)) = value;
    if(channel == 2)
    {
        unsigned long reg7 = REG32(KSEG1(SILAN_SYSTEM_CTL_REG7));
        reg7 = (reg7 & PWM_CH_CLEAR) | PWM_CH1_EN;
        REG32(KSEG1(SILAN_SYSTEM_CTL_REG7)) = reg7;
    }
    else if(channel == 1)
    {
        unsigned long reg7 = REG32(KSEG1(SILAN_SYSTEM_CTL_REG7));
        reg7 = (reg7 & PWM_CH_CLEAR) | PWM_CH2_EN;
        REG32(KSEG1(SILAN_SYSTEM_CTL_REG7)) = reg7;
    }
#else 
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM0, PWM_SEL_PWM0 & enable ? PAD_ON : PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM4, PWM_SEL_PWM4 & enable ? PAD_ON : PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM5, PWM_SEL_PWM5 & enable ? PAD_ON : PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM6, PWM_SEL_PWM6 & enable ? PAD_ON : PAD_OFF);
    silan_padmux2_ctrl(SILAN_PADMUX2_PWM7, PWM_SEL_PWM7 & enable ? PAD_ON : PAD_OFF);
    if(channel == 2)
    {
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM1, PWM_SEL_PWM1 & enable ? PAD_ON : PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM2, PWM_SEL_PWM2 & enable ? PAD_ON : PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM3, PWM_SEL_PWM3 & enable ? PAD_ON : PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM1_CH2, PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM2_CH2, PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM3_CH2, PAD_OFF);
    }
    else if(channel == 1)
    {
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM1, PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM2, PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM3, PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM1_CH2, PWM_SEL_PWM1 & enable ? PAD_ON : PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM2_CH2, PWM_SEL_PWM2 & enable ? PAD_ON : PAD_OFF);
        silan_padmux2_ctrl(SILAN_PADMUX2_PWM3_CH2, PWM_SEL_PWM3 & enable ? PAD_ON : PAD_OFF);
    }
#endif

    return 0;
}

static int dlna_pwm_open(struct inode *inode, struct file *fp)
{
    return 0;
}

static int dlna_pwm_release(struct inode *inode, struct file *fp)
{
    return 0;
}

static ssize_t dlna_pwm_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    return 0;
}

static long dlna_pwm_ioctl(struct file *filp, unsigned int cmd, unsigned long argp)
{
    int ret = 0;
    struct pwm_data_t pwm_data;
    unsigned int fre, cycle, h_cycle, psc;
    
    switch(cmd){
        case PWM_IOCTL_RESET:
            pwm_reset();
            break;
        case PWM_IOCTL_CHANNEL_SEL:
            if(copy_from_user(&pwm_data, (struct pwm_data_t *)argp, sizeof(pwm_data)))
                 return -EFAULT;
            pwm_channel_sel(pwm_data.channel, pwm_data.enable);
            break;
        case PWM_IOCTL_CLK_SEL:
            if(copy_from_user(&pwm_data, (struct pwm_data_t *)argp, sizeof(pwm_data)))
                 return -EFAULT;
            pwm_clk_sel(pwm_data.clk);
            break;
        case PWM_IOCTL_DISABLE:
            pwm_disable();
            break;
        case PWM_IOCTL_ENABLE:
            if(copy_from_user(&pwm_data, (struct pwm_data_t *)argp, sizeof(pwm_data)))
                 return -EFAULT;
            write_pwm_en(pwm_data.num, 1);
            break;
        case PWM_IOCTL_SET_DATA:
            if(copy_from_user(&pwm_data, (struct pwm_data_t *)argp, sizeof(pwm_data)))
                 return -EFAULT;
            fre = FRE_TYPE / (pwm_data.h_time + pwm_data.l_time);
            cycle = get_cycle(pwm_clk, fre);
            if(cycle == 0)
                break;
            psc = pwm_clk / fre / cycle - 1;
            h_cycle = cycle * pwm_data.h_time / (pwm_data.h_time + pwm_data.l_time);

            write_pwm_psc((int)psc);
            write_pwm_p(pwm_data.num / 2, (int)cycle);
            write_pwm_data(pwm_data.num, (int)h_cycle);
            break;
        default:
            break;
    }

    return ret;
}

static const struct file_operations dlna_pwm_ops = {
    .owner  = THIS_MODULE,
    .open   = dlna_pwm_open,
    .release= dlna_pwm_release,
    .read   = dlna_pwm_read,
    .unlocked_ioctl = dlna_pwm_ioctl,
};

static struct miscdevice pwmmisc = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "dlna_pwm",
    .fops   = &dlna_pwm_ops,
};

static __init int dlna_pwm_init(void)
{
    int err;
    if (misc_register(&pwmmisc) < 0){
        err = -EBUSY;
        return err;
    }
    
    return 0;
}

static __exit void dlna_pwm_exit(void)
{
    misc_deregister(&pwmmisc);
}

module_init(dlna_pwm_init);
module_exit(dlna_pwm_exit);

MODULE_AUTHOR("wumiaoxin@silan.com.cn");
MODULE_DESCRIPTION("DLNA PWM driver");
MODULE_LICENSE("GPL");

