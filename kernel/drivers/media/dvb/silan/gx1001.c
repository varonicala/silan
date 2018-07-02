#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/i2c.h>

#include "dvb_math.h"
#include "dvb_frontend.h"

#include "gx1001.h"

struct i2c_device 
{
    struct i2c_adapter *i2c_adap;
    u8 i2c_addr;
};

struct gx1001_state {
    struct dvb_frontend demod;
//    struct gx1001_config cfg;

    u8 i2c_addr;
    struct i2c_adapter *i2c_adap;

    /* for the I2C transfer */
    struct i2c_msg msg[2];
    u8 i2c_write_buffer[2];
    u8 i2c_read_buffer[1];
};

static u16 gx1001_read_word(struct gx1001_state *state, u8 reg)
{
    state->i2c_write_buffer[0] = reg & 0xff;

    memset(state->msg, 0, 2 * sizeof(struct i2c_msg));
    state->msg[0].addr = state->i2c_addr >> 1;
    state->msg[0].flags = 0;
    state->msg[0].buf = state->i2c_write_buffer;
    state->msg[0].len = 1;
    state->msg[1].addr = state->i2c_addr >> 1;
    state->msg[1].flags = I2C_M_RD;
    state->msg[1].buf = state->i2c_read_buffer;
    state->msg[1].len = 1;

    if (i2c_transfer(state->i2c_adap, state->msg, 2) != 2)
    {
        printk("i2c read error on %x, i2c_addr:    %x\n", reg, state->i2c_addr);
        return FAILURE;
    }

    return (state->i2c_read_buffer[0]);
}

static int gx1001_write_word(struct gx1001_state *state, u8 reg, u16 val)
{
    state->i2c_write_buffer[0] = reg & 0xff;
    state->i2c_write_buffer[1] = val & 0xff;

    memset(&state->msg[0], 0, sizeof(struct i2c_msg));
    state->msg[0].addr = state->i2c_addr >> 1;
    state->msg[0].flags = 0;
    state->msg[0].buf = state->i2c_write_buffer;
    state->msg[0].len = 2;

    return i2c_transfer(state->i2c_adap, state->msg, 1) != 1 ? FAILURE : SUCCESS;
}

static int gx1001_write_words(struct gx1001_state *state, u8 addr, u8 *data, u8 len)
{
    memset(&state->msg[0], 0, sizeof(struct i2c_msg));
    state->msg[0].addr = addr;
    state->msg[0].flags = 0;
    state->msg[0].buf = data;
    state->msg[0].len = len;

    return i2c_transfer(state->i2c_adap, state->msg, 1) != 1 ? FAILURE : SUCCESS;
}

static int gx1001_read_words(struct gx1001_state *state, u8 addr, u8 reg, u8 *data, u8 len)
{    
    u8 a[2];
    a[0] = reg;
    memset(state->msg, 0, 2 * sizeof(struct i2c_msg));
    state->msg[0].addr = addr;
    state->msg[0].flags = 0;
    state->msg[0].buf = a;
    state->msg[0].len = 1;
    state->msg[1].addr = addr;
    state->msg[1].flags = I2C_M_RD;
    state->msg[1].buf = data;
    state->msg[1].len = len;

    return i2c_transfer(state->i2c_adap, state->msg, 2) != 2 ? FAILURE : SUCCESS;
}

//set agc patameter
static int gx1001_set_agc_parameter(struct gx1001_state *state)
{
    int tmp = 0;
    
    gx1001_write_word(state, GX_AGC_STD, 23);

    tmp = gx1001_read_word(state, GX_MODE_AGC);
    if(tmp != FAILURE)
    {
        tmp &= 0xe6;
        tmp = tmp + 0x10;
        tmp = gx1001_write_word(state, GX_MODE_AGC, tmp);
    }

    return tmp;

}

//get the version of the chip
static int gx1001_get_version(struct gx1001_state *state)
{
    int tmp1 = 0;
    int tmp2 = 0;

    tmp1 = gx1001_read_word(state, GX_CHIP_IDD);
    tmp2 = gx1001_read_word(state, GX_CHIP_VERSION);
    if((tmp1 == 0x01) && ((tmp2 == 0x02) || (tmp2 == 0x82)))
    {
        return NEWONE;
    }
    else
    {
        return OLDONE;
    }
}

//set oscillator frequrancy
static int gx1001_set_osc_freq(struct gx1001_state *state) 
{
    int tmp = FAILURE;
    unsigned long temp = 0;
    unsigned long osc_freq_value = 0;

    if(NEWONE == gx1001_get_version(state))
    {
        osc_freq_value = GX_OSCILLATE_FREQ * (GX_PLL_M_VALUE+1)
                        / ((GX_PLL_N_VALUE+1) * (GX_PLL_L_VALUE+1)) / 2;
    }
    else
    {
        osc_freq_value = GX_OSCILLATE_FREQ;
    }
    
    temp = osc_freq_value * 250;

    tmp = gx1001_write_word(state, GX_FSAMPLE_H, ((u8)((temp>>16) & 0xff)));
    if(SUCCESS == tmp)
    {

        tmp = gx1001_write_word(state, GX_FSAMPLE_M, ((u8)((temp>>8) & 0xff)));
        if(SUCCESS == tmp)
        {
            tmp = gx1001_write_word(state, GX_FSAMPLE_H, ((u8)((temp) & 0xff)));
        }
    }

    return tmp;
}

//set TS output mode
static int gx1001_set_outputmode(struct gx1001_state *state, int mode)
{
    int temp = 0;
    int tmp = FAILURE;

    temp = gx1001_read_word(state, GX_OUT_FORMAT);

    if(temp != FAILURE)
    {
        temp &= 0xbf;
        if(mode)
        {
            temp += 0x40;
        }

        tmp = gx1001_write_word(state, GX_OUT_FORMAT, temp);
    }

    return tmp;
}

//set digital agc
static int gx1001_set_digital_agc(struct gx1001_state *state, int mode)
{
    int tmp;

    tmp = gx1001_read_word(state, GX_DIGITAL_AGC_ON);
    if(tmp == FAILURE)
    {
        return tmp;
    }
    
    if(mode == 0)
    {
        tmp &= 0x7f;
    }
    else
    {
        tmp |= 0x80;
    }

    if(gx1001_write_word(state, GX_DIGITAL_AGC_ON, tmp))
    {
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

//set SF mode
static int gx1001_set_SF(struct gx1001_state *state, int mode)
{
    int temp;

    temp = gx1001_read_word(state, GX_SF_CANCEL_CTRL);
    
    if(mode == 0)
    {
        temp &= 0xef;
    }
    else
    {
        temp |= 0x10;
    }

    if(gx1001_write_word(state, GX_SF_CANCEL_CTRL, temp))
    {
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

//set FM mode
static int gx1001_set_FM(struct gx1001_state *state, int mode)
{
    int temp;

    temp = gx1001_read_word(state, GX_FM_CANCEL_CTRL);
    
    if(mode == 0)
    {
        temp &= 0xef;
    }
    else
    {
        temp |= 0x10;
    }

    if(gx1001_write_word(state, GX_FM_CANCEL_CTRL, temp))
    {
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

//set the minimum of RF gac controller
static int gx1001_set_RF_min(struct gx1001_state *state, int value)
{
    if(gx1001_write_word(state, GX_RF_MIN, value))
    {
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

//set the maxmum of RF gac controller
static int gx1001_set_RF_max(struct gx1001_state *state, int value)
{
    if(gx1001_write_word(state, GX_RF_MAX, value))
    {
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

//set the minimum of RF gac controller
static int gx1001_set_IF_min(struct gx1001_state *state, int value)
{
    if(gx1001_write_word(state, GX_IF_MIN, value))
    {
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

//set the maxmum of RF gac controller
static int gx1001_set_IF_max(struct gx1001_state *state, int value)
{
    if(gx1001_write_word(state, GX_IF_MAX, value))
    {
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }
}

//set pll value
static int gx1001_set_pll_value(struct gx1001_state *state, int pll_m_value, int pll_n_value, int pll_l_value)
{
    if(SUCCESS == gx1001_write_word(state, GX_PLL_M, pll_m_value))
    {
        if(SUCCESS == gx1001_write_word(state, GX_PLL_M, ((pll_m_value<<4) & 0xf0) | (pll_l_value & 0x0f)))
        {
            return SUCCESS;
        }
        return FAILURE;
    }
    else
    {
        return FAILURE;
    }
}

//init the gx1001
static int gx1001_init_chip(struct gx1001_state *state)
{
    int tmp = FAILURE;

    tmp = gx1001_set_agc_parameter(state);

    if(SUCCESS == tmp)
    {
        tmp = gx1001_set_osc_freq(state);
    
        if(SUCCESS == tmp)
        {
            tmp = gx1001_set_outputmode(state, GX_TS_OUTPUT_MODE);
        }
    }

    if((NEWONE == gx1001_get_version(state)) && (SUCCESS == tmp))
    {
        tmp = gx1001_set_digital_agc(state, ENABLE);
        if(tmp == FAILURE)
        {
            return tmp;
        }

        tmp = gx1001_set_RF_min(state, GX_RF_AGC_MIN_VALUE);
        if(tmp == FAILURE)
        {
            return tmp;
        }
    
        tmp = gx1001_set_RF_max(state, GX_RF_AGC_MAX_VALUE);
        if(tmp == FAILURE)
        {
            return tmp;
        }
    
        tmp = gx1001_set_IF_min(state, GX_IF_AGC_MIN_VALUE);
        if(tmp == FAILURE)
        {
            return tmp;
        }

        tmp = gx1001_set_IF_max(state, GX_IF_AGC_MAX_VALUE);
        if(tmp == FAILURE)
        {
            return tmp;
        }
        
        tmp = gx1001_set_pll_value(state, GX_PLL_M_VALUE, GX_PLL_N_VALUE, GX_PLL_L_VALUE);
        if(tmp == FAILURE)
        {
            return tmp;
        }
    }

    return tmp;
}

//hot reset the chip
static int gx1001_hot_reset_chip(struct gx1001_state *state)
{
    int tmp = FAILURE;
    int temp;

    temp = gx1001_read_word(state, GX_MAN_PARA);
    if(temp != FAILURE)
    {
        temp |= 0x02;
        tmp = gx1001_write_word(state, GX_MAN_PARA, temp);
    }

    return tmp;
}

//cool reset the chip
static int gx1001_cool_reset_chip(struct gx1001_state *state)
{
    int tmp = FAILURE;
    int temp;

    temp = gx1001_read_word(state, GX_MAN_PARA);
    if(temp != FAILURE)
    {
        temp |= 0x08;
        tmp = gx1001_write_word(state, GX_MAN_PARA, temp);
    }

    return tmp;
}

//select QAM size (4 - 256), only for DVB
//0-2:  reserved
//  3:  16QAM
//  4:    32QAM
//  5:    64QAM
//  6:    128QAM
//  7:    256QAM
static int gx1001_select_dvb_qam_size(struct gx1001_state *state, int size)
{
    int temp = 0;
    int tmp = FAILURE;

    if((size > 7) || (size <= 2))
    {
        size = 5;
    }

    size <<= 5;

    temp = gx1001_read_word(state, GX_MODE_AGC);

    if(temp != FAILURE)
    {
        temp &= 0x1f;
        temp += size;
        tmp = gx1001_write_word(state, GX_MODE_AGC, temp);
    }

    return tmp;
}

//set symbol rate
//the range is from 450khz---9000khz
static int gx1001_set_symbol_rate(struct gx1001_state *state, unsigned long symbol_rate_value)
{
    int tmp = FAILURE;
    unsigned long temp = 0;

    temp = symbol_rate_value * 1000;

    tmp = gx1001_write_word(state, GX_SYMB_RATE_H, ((u8)((temp>>16) & 0xff)));
    if(SUCCESS == tmp)
    {
        tmp = gx1001_write_word(state, GX_SYMB_RATE_M, ((u8)((temp>>8) & 0xff)));
        if(SUCCESS == tmp)
        {
            tmp = gx1001_write_word(state, GX_SYMB_RATE_L, ((u8)(temp & 0xff)));
        }
    }
    
    return tmp;
}

//delay n ms
static void gx1001_delay_n_ms(unsigned int ms_value)
{
    msleep(ms_value);
}

//enable/disable the tuner repeater
//1:    on;   0:    off;
static int gx1001_set_tuner_repeater_enbale(struct gx1001_state *state, int onoff)
{
    int tmp = FAILURE;
    int read_temp;

    read_temp = gx1001_read_word(state, GX_MAN_PARA);

    if(read_temp != FAILURE)
    {
        if(onoff)
        {
            read_temp |= 0x40;
        }
        else
        {
            read_temp &= 0xbf;
        }

        tmp = gx1001_write_word(state, GX_MAN_PARA, read_temp);
    }
    
    return tmp;
}

//set RF frequency 
static int gx1001_set_RF_frequency(struct gx1001_state *state, unsigned long fvalue)
{
    int tmp = FAILURE;
    unsigned char data[6];
    unsigned char read_data[6];
    unsigned long freq;
    unsigned char addr;

    freq = (fvalue + GX_IF_FREQUENCY) * 10 / 625;
    addr = 0xc0 / 2;
    data[0] = (unsigned char)((freq>>8) & 0xff);
    data[1] = (unsigned char)(freq & 0xff);
    data[2] = 0x9B; /*62.5KHZ*/
    data[3] = 0x20;
    data[4] = 0xc6;

    if(fvalue < 125000)
    {
        data[3] |= 0x80;
    }
    else if((fvalue >= 125000) && (fvalue < 366000))
    {
        data[3] |= 0x82;
    }
    else if((fvalue >= 366000) && (fvalue < 622000))
    {
        data[3] |= 0x48;
    }
    else if((fvalue >= 622000) && (fvalue < 726000))
    {
        data[3] |= 0x88;
    }
    else if(fvalue >= 726000)
    {
        data[3] |= 0xc8;
    }

    if(SUCCESS == gx1001_set_tuner_repeater_enbale(state, 1))
    {
        gx1001_delay_n_ms(10);
        if(SUCCESS == gx1001_write_words(state, addr, data, 5))
        {
            gx1001_read_words(state, addr, data[0] ,read_data, 5);
            gx1001_delay_n_ms(10);
            tmp = gx1001_set_tuner_repeater_enbale(state, 0);
        }
    }
    
    if(SUCCESS == tmp)
    {
        gx1001_delay_n_ms(50);
        tmp = gx1001_hot_reset_chip(state);
    }

    return tmp;
}

//read equ ok
static int gx1001_read_equ_ok(struct gx1001_state *state)
{
    int read_temp = 0;

    read_temp = gx1001_read_word(state, GX_STATE_IND);
    if(read_temp != FAILURE)
    {
        if((read_temp & 0xe0) == 0xe0)
        {
            return SUCCESS;
        }
    }

    return FAILURE;
}

//read all ok
static int gx1001_read_all_ok(struct gx1001_state *state)
{
    int read_temp = 0;

    read_temp = gx1001_read_word(state, GX_STATE_IND);
    if(read_temp != FAILURE)
    {
        if((read_temp & 0xf1) == 0xf1)
        {
            return SUCCESS;
        }
    }

    return FAILURE;
}

//100logN  calculating function
static int gx1001_100log(int inumber_n)
{
    int ileftmovecount_m = 0;
    int ichangen_y = 0;
    int ibumay_x = 0;
    int ireturn_value = 0;
    long itemp = 0;
    long iresult = 0;
    long k = 0;

    ichangen_y = inumber_n;
    
    for(ileftmovecount_m = 0; ileftmovecount_m < 16; ileftmovecount_m++)
    {
        if((ichangen_y & 0x8000) == 0x8000)
            break;
        else
        {
            ichangen_y = inumber_n << ileftmovecount_m;
        }
    }

    ibumay_x = 0x10000 - ichangen_y;

    k = (long)ibumay_x * 10000 / 65536;

    itemp = k + (k*k) / 20000 + ((k*k/10000) * (k*33/100)) / 10000
            + ((k*k/100000) * (k*k*100000)) / 400;

    iresult = 48165 - (itemp * 10000 / 23025);

    k = iresult - 3010 * (ileftmovecount_m-1);

    ireturn_value = (k / 100);

    return ireturn_value;
}

//convert a integer to percentage ranging form 0% to 100%
static unsigned char gx1001_change_to_percent(int value, int low, int high)
{
    unsigned char temp = 0;
    
    if(value <= low)
    {
        return 0;
    }
    
    if(value >= high)
    {
        return 100;
    }

    temp = (unsigned char )((value-low) * 100 / (high-low));
    
    return temp;
}

//set spectrum invert
static int gx1001_set_spec_invert(struct gx1001_state *state, int spec_invert)
{
    int write_value = 0;
    unsigned int osc_freq_value = 0;
    unsigned int carrier_center = GX_IF_FREQUENCY;

    if(NEWONE == gx1001_get_version(state))
    {
        osc_freq_value = GX_OSCILLATE_FREQ * (GX_PLL_M_VALUE+1) 
                       / ((GX_PLL_N_VALUE+1) * (GX_PLL_L_VALUE+1)) / 2; 
    }
    else
    {
        osc_freq_value = GX_OSCILLATE_FREQ;
    }
    
    if(carrier_center < osc_freq_value)
    {
        if(spec_invert)
        {
            write_value = (int)(((osc_freq_value - carrier_center) * 1000) / 1024);
        }
        else
        {
            write_value = (int)((carrier_center * 1000) / 1024);
        }
    }
    else
    {
        if(spec_invert)
        {
            write_value = (int)(((2 * osc_freq_value - carrier_center) * 1000) / 1024);
        }
        else
        {
            write_value = (int)(((carrier_center - osc_freq_value) * 1000) / 1024);
        }
    }

    if(SUCCESS == gx1001_write_word(state, GX_DCO_CENTER_H, (((write_value >> 8) & 0xff))));
    {
        if(SUCCESS == gx1001_write_word(state, GX_DCO_CENTER_L, ((write_value & 0xff))))
        {
            return SUCCESS;
        }
    }

    return FAILURE;
}
//set chip wake up
//sleep_enable    1: sleep;    0: working
static int gx1001_set_sleep(struct gx1001_state *state, int sleep_enable)
{
    int temp1 = 0, temp2 = 0;
    int uctmp1 = FAILURE;
    int uctmp2 = FAILURE;

    if(NEWONE == gx1001_get_version(state))
    {
        temp1 = gx1001_read_word(state, GX_MAN_PARA);
        temp2 = gx1001_read_word(state, 0x14);
        if((temp1 != FAILURE) && (temp2 != FAILURE))
        {
            temp1 &= 0xfb;
            temp2 &= 0xcf;

            temp1 |= (0x04 & (sleep_enable << 2));
            temp2 |= (0x10 & (sleep_enable << 4));
            temp2 |= (0x20 & (sleep_enable << 5));

            uctmp1 = gx1001_write_word(state, GX_MAN_PARA, temp1);
            uctmp2 = gx1001_write_word(state, 0x14, temp2);

            if((SUCCESS == uctmp1) && (SUCCESS == uctmp2))
            {
                if(0 == sleep_enable)
                {
                    uctmp1 = gx1001_hot_reset_chip(state);
                }
            }
        }
        return (uctmp1 && uctmp2);
    }

    return FAILURE;
}


static int gx1001_sleep(struct dvb_frontend *fe)
{
    struct gx1001_state *state = fe->tuner_priv;

    return gx1001_set_sleep(state, 1);
}

static int gx1001_wakeup(struct dvb_frontend *fe)
{
    struct gx1001_state *state = fe->tuner_priv;
    
    return gx1001_set_sleep(state, 0);
}

//search signal with setted parameters
//symbol rate: range [450--9000]KHZ
//spec mode: 0-3
//qam_size: 0-2:reserved 3-7: 16*(2^n)QAM
//rf_freq: the RF frequency
//static int gx1001_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
static enum dvbfe_search gx1001_search_signal(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
    struct gx1001_state *state = fe->tuner_priv;
    int after_equ_ok_delay = 60;
    int spec_invert_enable = 0;
    int spec_invert_value  = 0;
    int symbol2_enable   = 0;
    
    int wait_ok_x_ms_temp  = 0;
    int wait_ok_sf_temp    = 0;
    int gx1001bflag        = 0;
    int wait_ok_x_ms       = 700;
    int symbol_rate2 = 0;

    p->frequency = p->frequency / 1000;

#if 0
    printk("p->frequency:    %d\np->u.qam.symbol_rate:    %d\np->u.qam.modulation:    %d\np->inversion:    %d\n\n",
            p->frequency, p->u.qam.symbol_rate, p->u.qam.modulation, p->inversion);
#endif

    gx1001_cool_reset_chip(state);
    if(FAILURE == gx1001_init_chip(state))
    {
        return FAILURE;
    }

    wait_ok_x_ms_temp = wait_ok_x_ms / 10;

    if(FAILURE == gx1001_select_dvb_qam_size(state, p->u.qam.modulation+2))
    {
        return FAILURE;
    }

    if(FAILURE == gx1001_set_symbol_rate(state, p->u.qam.symbol_rate))
    {
        return FAILURE;
    }

    if(FAILURE == gx1001_set_RF_frequency(state, p->frequency))
    {
        return FAILURE;
    }
    
    if(symbol_rate2 >= 4500) 
    {
        symbol2_enable = 1;
    }

    if(p->u.qam.symbol_rate < 2500) 
    {
        after_equ_ok_delay = 100;
    }

    if(NEWONE == gx1001_get_version(state))
    {
        gx1001bflag = 1;
        p->inversion = 0;
    }

SYMBOL2_SEARCH:
    switch(p->inversion)
    {
        case 3:
            {
                spec_invert_enable = 1;
                spec_invert_value  = 0;
            }
        case 1:
            {
                gx1001_set_spec_invert(state, 1);
            }
            break;
        case 2:
            {
                spec_invert_enable = 1;
                spec_invert_value  = 1;
            }
        default:
            {
                gx1001_set_spec_invert(state, 0);
            }
            break;
    }

SPEC_INVERT_SEARCH:
    if(FAILURE == gx1001_hot_reset_chip(state))
    {
//        return FAILURE;
    }

    wait_ok_x_ms_temp = wait_ok_x_ms / 10;

    while((FAILURE == gx1001_read_equ_ok(state)) && (wait_ok_x_ms_temp))
    {
        wait_ok_x_ms_temp--;
        gx1001_delay_n_ms(10);
    }

    if(0 == wait_ok_x_ms_temp)
    {
        if(gx1001bflag && sfenable)
        {
            gx1001_set_SF(state, ENABLE);
            gx1001_set_FM(state, ENABLE);
            gx1001_hot_reset_chip(state);
            wait_ok_sf_temp = 80;

            while((FAILURE == gx1001_read_all_ok(state)) && (wait_ok_sf_temp))
            {
                wait_ok_sf_temp--;
                gx1001_delay_n_ms(20);
            }

            if(SUCCESS == gx1001_read_all_ok(state))
            {
                return SUCCESS;
            }
        }
        else if(symbol2_enable)
        {
            symbol2_enable = 0;
            
            if(symbol_rate2 < 25000)
            {
                after_equ_ok_delay = 100;
            }
            else
            {
                after_equ_ok_delay = 60;
            }
            
            gx1001_set_symbol_rate(state, symbol_rate2);

            if(1 == gx1001bflag)
            {
                gx1001_set_SF(state, DISABLE);
                gx1001_set_FM(state, DISABLE);
            }
            goto SYMBOL2_SEARCH;
        }
        else
        {
            return FAILURE;
        }
    }

    gx1001_delay_n_ms(after_equ_ok_delay);

    if(SUCCESS == gx1001_read_all_ok(state))
    {
        if(gx1001bflag && fmenable)
        {
            gx1001_set_FM(state, ENABLE);
        }
        return SUCCESS;
    }
    else
    {
        if(spec_invert_enable)
        {
            spec_invert_enable = 0;

            if(FAILURE == gx1001_set_spec_invert(state, spec_invert_value))
            {
                return FAILURE;
            }
            else
            {
                goto SPEC_INVERT_SEARCH;
            }
        }
        else
        {
            return FAILURE;
        }
    }
}

//get error rate value
static int gx1001_read_ber(struct dvb_frontend *fe, u32 *ber)
{
    struct gx1001_state *state = fe->tuner_priv;
    int flag = 0;
    int e_value = 0;
    int return_value = 0;
    int temp = 0;
    u8 read_value[4];
    u8 error_count = 0;
    int i = 0;
    unsigned long divied = 53477376;   //(2^20*51);

    *ber = 0;

    if(gx1001_read_all_ok(state) == FAILURE)
    {
        *ber = 0;
        return 1;
    }

    for(i = 0; i < 4; i++)
    {
        flag = gx1001_read_word(state, GX_ERR_SUM_1 + i);
        if(FAILURE == flag)
        {
            *ber = 0;
            return 1;
        }
        else
        {
            read_value[i] = (u8)flag;
        }
    }

    read_value[3] &= 0x03;
    error_count = (u8)(read_value[0] + (read_value[1] << 8) 
                   + (read_value[2] << 16) + (read_value[3] << 24));

    for(i = 0; i < 20; i++)
    {
        temp = error_count / divied;
        if(temp)
        {
            return_value = error_count / (divied / 100);
            break;
        }
        else
        {
            e_value += 1;
            error_count *= 10;
        }
    }
    
    *ber = ((return_value << 8) & 0xff) + e_value;

    return 0;
}


static int gx1001_read_status(struct dvb_frontend* fe, fe_status_t* status)
{
    struct gx1001_state *state = fe->tuner_priv;
    int read_temp = 0;

    read_temp = gx1001_read_word(state, GX_STATE_IND);
    if(read_temp != FAILURE)
    {
        if((read_temp & 0xf1) == 0xf1)
        {
            *status = 0x01;
            return SUCCESS;
        }
    }

    return FAILURE;
}

//get the signal intensity expressed int percentage
//range:    [0, 100]
static int gx1001_read_signal_strength(struct dvb_frontend* fe, u16* strength)
{
    struct gx1001_state *state = fe->tuner_priv;
    unsigned int agc1_word = 300;
    unsigned int agc2_word = 300;
    unsigned int amp_value;
    unsigned int agc1_temp = 0;
    unsigned int agc2_temp = 0;

    int c0 = 95;
    int c1 = 0xb2;
    int c2 = 204;
    int c3 = 0x8c;
    int c4 = 179;
    int a1 = 20;
    int a2 = 0;
    int a3 = 20;
    int a4 = 0;

    int i = 0;
    
    while(i < 40)
    {

        agc1_temp = gx1001_read_word(state, GX_AGC1_CTRL);
        agc2_temp = gx1001_read_word(state, GX_AGC2_CTRL);
        
        if((agc1_temp > 0) && agc2_temp > 0)
        {
            if((((agc1_temp - agc1_word) < 5)||((agc1_temp - agc1_word) > -5 )) 
                && (((agc2_temp - agc2_word) < 5 ) || ((agc2_temp - agc2_word) > -5 )))
            {
                break;
            }
            
            agc1_word = agc1_temp;
            agc2_word = agc2_temp;
        }
        gx1001_delay_n_ms(10);
        i++;
    }

    if(i >= 40)
    {
        agc1_word = gx1001_read_word(state, GX_AGC1_CTRL);
        agc2_word = gx1001_read_word(state, GX_AGC2_CTRL);
    }
    
    if(agc1_word > 0xe4) 
    {
        agc1_word = 0xe4;
    }

    amp_value = c0 - ((agc1_word-c1) * (a1-a2)) / (c2-c1)
                   - ((agc2_word-c3) * (a3-a4)) / (c4-c3);
    
    *strength = gx1001_change_to_percent(amp_value, 0, 100);
    
    return SUCCESS;
}

//get the signal quality expressed in percentage
//range:    [0, 100]  (0 express SNR = 5 dB, 100 express SNR = 35 dB)
static int gx1001_read_snr(struct dvb_frontend* fe, u16* snr)
{
    struct gx1001_state *state = fe->tuner_priv;
    int s_n_value = 0;
    int read_temp = 0;
    int read_temp1 = 0;
    int read_temp2 = 0;

    if(gx1001_read_all_ok(state) == SUCCESS)
    {
        read_temp1 = (gx1001_read_word(state, GX_SUM_ERR_POW_L) & 0xff);
        read_temp2 = (gx1001_read_word(state, GX_SUM_ERR_POW_H) & 0xff);
        if((read_temp1 > 0) || (read_temp2 > 0))
        {
            read_temp = read_temp1 + (read_temp2 << 8);
            s_n_value = 493 - gx1001_100log(read_temp);
            *snr = gx1001_change_to_percent(s_n_value, 50, 350);
            return SUCCESS;
        }
    }
    
    return FAILURE;
}

static void gx1001_release(struct dvb_frontend *fe)
{
    struct gx1001_state *st = fe->tuner_priv;
    kfree(st);
}

static struct dvb_frontend_ops gx1001_ops;

struct dvb_frontend *gx1001_attach(struct i2c_adapter *i2c_adap, u8 i2c_addr)
{
    struct dvb_frontend *demod;
    struct gx1001_state *st;
    st = kzalloc(sizeof(struct gx1001_state), GFP_KERNEL);
    if (st == NULL)
        return NULL;

    st->i2c_adap = i2c_adap;
    st->i2c_addr = i2c_addr;

    demod = &st->demod;
    demod->tuner_priv = st;
    memcpy(&st->demod.ops, &gx1001_ops, sizeof(struct dvb_frontend_ops));

    return demod;
}
EXPORT_SYMBOL(gx1001_attach);

static enum dvbfe_algo gx1001_get_tuning_algo(struct dvb_frontend *fe)
{
    //return DVBFE_ALGO_SW;
    return DVBFE_ALGO_CUSTOM;
}

static struct dvb_frontend_ops gx1001_ops = {
    .info = {
         .name = "GX1001",
         .type = FE_QAM,
         .frequency_min = 44250000,
         .frequency_max = 867250000,
         .frequency_stepsize = 62500,
         .caps = FE_CAN_INVERSION_AUTO |
         FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
         FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
         FE_CAN_QPSK | FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
         FE_CAN_TRANSMISSION_MODE_AUTO | FE_CAN_GUARD_INTERVAL_AUTO | FE_CAN_RECOVER | FE_CAN_HIERARCHY_AUTO,
         },

    .release              = gx1001_release,

    .init                 = gx1001_wakeup,
    .sleep                = gx1001_sleep,

    .read_status          = gx1001_read_status,
    .read_ber             = gx1001_read_ber,
    .read_signal_strength = gx1001_read_signal_strength,
    .read_snr             = gx1001_read_snr,
    
    .search               = gx1001_search_signal,
    .get_frontend_algo    = gx1001_get_tuning_algo,
};

MODULE_DESCRIPTION("Driver for the GX1001");
MODULE_AUTHOR("Chen Jianneng");
MODULE_LICENSE("GPL");
