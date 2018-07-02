#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/io.h>
#include <silan_resources.h>
#include <silan_regs.h>

#define SLINNER_RATES (SNDRV_PCM_RATE_8000_96000)

#define SLINNER_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_3LE )

#define AICR	   (0x01<<2)    //0x04
#define CR1        (0x02<<2)	//0x08
#define CR2        (0x03<<2)	//0x0c
#define CR3        (0x04<<2)	//0x10
#define CR4        (0x05<<2)	//0x14
#define CCR1       (0x06<<2)	//0x18
#define CCR2       (0x07<<2)	//0x1c
#define PMR1       (0x08<<2)	//0x20
#define PMR2       (0x09<<2)	//0x24
#define ICR        (0x0a<<2)	//0x28
#define IFR        (0x0b<<2)	//0x2c
#define GCR1       (0x0c<<2)	//0x30
#define GCR2       (0x0d<<2)	//0x34
#define GCR3       (0x0e<<2)	//0x38
#define GCR4       (0x0f<<2)	//0x3c
#define GCR5       (0x10<<2)	//0x40
#define GCR6       (0x11<<2)	//0x44
#define GCR7       (0x12<<2)	//0x48
#define GCR8       (0x13<<2)	//0x4c
#define GCR9       (0x14<<2)	//0x50
#define AGC1       (0x15<<2)	//0x54
#define AGC2       (0x16<<2)	//0x58
#define AGC3       (0x17<<2)	//0x5c
#define AGC4       (0x18<<2)	//0x60
#define AGC5       (0x19<<2)	//0x64
#define MIX1       (0x1a<<2)	//0x68
#define MIX2       (0x1b<<2)	//0x6c
#define	TR1        (0x1c<<2)	//0x70
#define	TR2        (0x1d<<2)	//0x74
#define	TR3        (0x1e<<2)	//0x78
#define	TR4        (0x1f<<2)	//0x7c

#define AICR_16_BIT  0x00
#define AICR_24_BIT  0x03

#define CODEC_MAX_DB     31
#define CODEC_MIC_MAX_DB 7

#define SILAN_INNER_CODEC_NAME  "silan-icodec"

#define INNER_CODEC_MAGIC 'i'

#define CODEC_IOCTL_MIC_BYPASS     _IO(INNER_CODEC_MAGIC, 1)
#define CODEC_IOCTL_LINEIN_BYPASS  _IO(INNER_CODEC_MAGIC, 2)
#define CODEC_IOCTL_MIXER_OUT      _IO(INNER_CODEC_MAGIC, 3)
#define CODEC_IOCTL_MIXER_IN       _IO(INNER_CODEC_MAGIC, 4)
#define CODEC_IOCTL_HP_GACL        _IO(INNER_CODEC_MAGIC, 5)
#define CODEC_IOCTL_HP_GACR        _IO(INNER_CODEC_MAGIC, 6)
#define CODEC_IOCTL_DAC_GACL       _IO(INNER_CODEC_MAGIC, 7)
#define CODEC_IOCTL_DAC_GACR       _IO(INNER_CODEC_MAGIC, 8)
#define CODEC_IOCTL_ADC_GACL       _IO(INNER_CODEC_MAGIC, 9)
#define CODEC_IOCTL_ADC_GACR       _IO(INNER_CODEC_MAGIC, 10)
#define CODEC_IOCTL_MIC1_GAC       _IO(INNER_CODEC_MAGIC, 11)
#define CODEC_IOCTL_MIC2_GAC       _IO(INNER_CODEC_MAGIC, 12)
#define CODEC_IOCTL_LINEIN_GACL    _IO(INNER_CODEC_MAGIC, 13)
#define CODEC_IOCTL_LINEIN_GACR    _IO(INNER_CODEC_MAGIC, 14)
#define CODEC_IOCTL_MIX_OUT_GAC    _IO(INNER_CODEC_MAGIC, 15)
#define CODEC_IOCTL_MIX_IN_GAC     _IO(INNER_CODEC_MAGIC, 16)
#define CODEC_IOCTL_MIC_ENABLE     _IO(INNER_CODEC_MAGIC, 17)
#define CODEC_IOCTL_LINEIN_ENABLE  _IO(INNER_CODEC_MAGIC, 18)
#define CODEC_IOCTL_LINEOUT_ENABLE _IO(INNER_CODEC_MAGIC, 19)
#define CODEC_IOCTL_PHOUT_ENABLE   _IO(INNER_CODEC_MAGIC, 20)
#define CODEC_IOCTL_DAC_MUTE       _IO(INNER_CODEC_MAGIC, 21)
#define CODEC_IOCTL_DAC_SAMPLE     _IO(INNER_CODEC_MAGIC, 22)
#define CODEC_IOCTL_DAC_BIT        _IO(INNER_CODEC_MAGIC, 23)
#define CODEC_IOCTL_LINEOUT_DAC    _IO(INNER_CODEC_MAGIC, 24)

#define CODEC_IRQ_SCMC             (1<<4)
#define CODEC_IRQ_RUP              (1<<3)
#define CODEC_IRQ_RDO              (1<<2)
#define CODEC_IRQ_GUP              (1<<1)
#define CODEC_IRQ_GDO              (1<<0)

#define CODEC_SW_REG               SILAN_SYS_REG6
#define CODEC_SW_RESETn            (1<<7)

typedef enum direciton
{
	GET,
	SET,
}DIRE;

struct codec_priv{
	DIRE dir; 
	int value;
	int arg;
	int flag;
}; 

struct delayed_work codec_work;

static const DECLARE_TLV_DB_SCALE(slinner_tlv, -31, 1, 1);

static const struct snd_kcontrol_new slinner_snd_controls[] = {
	SOC_SINGLE_TLV("Master Volume", MIX2, 0, 31, 0, slinner_tlv),
	SOC_SINGLE_TLV("Music Playback Volume", MIX2, 0, 255, 0, slinner_tlv),
	SOC_SINGLE_TLV("PCM Volume", MIX2, 0, 255, 0, slinner_tlv),
	//SOC_SINGLE_TLV("DAC Playback Volume", MIX2, 0, 255, 0, slinner_tlv),
	SOC_DOUBLE_S8_TLV("ADC Capture Volume", MIX1, -128, 48, slinner_tlv),
};

static const struct snd_soc_dapm_widget slinner_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "Playback", MIX2, 2, 0),
};

static const struct snd_soc_dapm_route slinner_intercon[] = {
/* Inputs */
	//{"DIN", NULL, "DAC"},

/* Outputs */
	//{"LOUT", NULL, "DAC"},
	//{"ROUT", NULL, "DAC"},
};

enum sample_rate{
	RATE_96000,
	RATE_48000,
	RATE_44100,
	RATE_32000,
	RATE_24000,
	RATE_22050,
	RATE_16000,
	RATE_12000,
	RATE_11025,
	RATE_8000,
};

enum enalbe
{
	DISABLE=0,
	ENABLE=1,
};

/* codec private data */
struct slinner_priv {
	struct miscdevice miscdev;
	void __iomem *base;
	void __iomem *crbase;
	enum snd_soc_control_type control_type;
	struct tasklet_struct tasklet;
	int icr_value;
}*slinner_def;

static void slinner_set_param(void __iomem *base, int value, int offset, int enable, int cnt)
{
	u32 val = 0;

	if(cnt < 0 || cnt >= 8){
		printk("The offset of the reg 0x%x if out of range\n", offset);
		return;
	}
	
	val = readl(base+offset);
	
	if(((val>>cnt) & 0x01) == enable)
		return;

	if(enable == ENABLE)
		val |= (value<<cnt);
	else if(enable == DISABLE)
		val &= ~(value<<cnt);

	//printk("reg: %x, value: %x\n", offset, val);
	writel(val, base+offset);
}

#ifdef EXTER
static int slinner_read_param(void __iomem *base, int offset, int cnt)
{
	u32 val = 0;
	
	if(cnt < 0 || cnt >= 8){
		printk("The offset of the reg 0x%x if out of range\n", offset);
		return -1;
	}
	
	val = readl(base+offset);

	return ((val>>cnt) & 0x01);
}
#endif

static void slinner_set_param1(void __iomem *base, int value, int offset, int len, int cnt)
{
	u32 val = 0, val1 = 0;
	u8 i = 0;
	
	if(cnt < 0 || cnt >= 8){
		printk("The offset of the reg 0x%x if out of range\n", offset);
		return;
	}
	
	val = readl(base+offset);
	for(i = 0; i < len; i++){
		val1 |= (1<<i);
	}

	//printk("1: reg: %x, value: %x, val1: %x\n", offset, val, val1);
	if(((val>>cnt)&val1) == value)
		return ;

	val &= ~(val1<<cnt);
	val |= (value<<cnt);

	//printk("2: reg: %x, value: %x, val1: %x\n", offset, val, val1);
	writel(val, base+offset);
}

static int slinner_get_param1(void __iomem *base, int offset, int len, int cnt)
{
	u32 val = 0, val1 = 0;
	u8 i = 0;
	
	if(cnt < 0 || cnt >= 8){
		printk("The offset of the reg 0x%x if out of range\n", offset);
		return -1;
	}
	
	val = readl(base+offset);
	for(i = 0; i < len; i++){
		val1 |= (1<<i);
	}
	
	//printk("1: reg: %x, value: %x, val1: %x, ret: %x\n", offset, val, val1, ((val>>cnt)&val1));
	
	return ((val>>cnt)&val1);
}

static void slinner_dac_enable(void __iomem *base)
{
	slinner_set_param1(base, 0x03, CR1, 2, 0);
}

static void slinner_linebypass_enable(void __iomem *base)
{
	slinner_set_param1(base, 0x02, CR1, 2, 0);
}

static void slinner_micbypass_enable(void __iomem *base)
{
	slinner_set_param1(base, 0x01, CR1, 2, 0);
}

static void slinner_hp_mute(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, CR1, enable, 5);
}

static void slinner_line_out(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR1, ENABLE, 7);
}

static void slinner_headphone_out(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR1, DISABLE, 7);
}

#ifdef EXTER
static void slinner_dac_mono(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR2, ENABLE, 7);
}
#endif

static void slinner_dac_stereo(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR2, DISABLE, 7);
}

static void slinner_dac_mute(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, CR2, enable, 5);
}

#ifdef EXTER
static void slinner_dock_mode(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR2, DISABLE, 1);
}
#endif

static void slinner_nomad_mode(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR2, ENABLE, 1);
}

#ifdef EXTER
static void slinner_dac_left_only(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, CR2, enable, 0);
}
#endif

static void slinner_adc_line_in(void __iomem *base)
{
	slinner_set_param1(base, 0x02, CR3, 2, 2);
}

static void slinner_adc_mic1_in(void __iomem *base)
{
	slinner_set_param1(base, 0x0, CR3, 2, 2);
}

#ifdef EXTER
static void slinner_adc_mic2_in(void __iomem *base)
{
	slinner_set_param1(base, 0x1, CR3, 2, 2);
}
#endif

static void slinner_mic_stereo(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR3, ENABLE, 1);
}

#ifdef EXTER
static void slinner_mic_mono(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR3, DISABLE, 1);
}
#endif

static void slinner_mic_single_mode(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR3, DISABLE, 0);
}

#ifdef EXTER
static void slinner_mic_diff_mode(void __iomem *base)
{
	slinner_set_param(base, 0x01, CR3, ENABLE, 0);
}
#endif

#ifdef EXTER
static void slinner_adc_hpf(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, CR4, enable, 7);
}

static void slinner_adc_left_only(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, CR4, enable, 0);
}
#endif

static void slinner_set_format(void __iomem *base, u8 value, int stream)
{
	u32 val = 0;
	val = readl(base+AICR);
	val |= 0xf;	
	if(stream == SNDRV_PCM_STREAM_PLAYBACK){
		val &= 0x3f;
		val |= (value<<6);
	}
	else{
		val &= 0xcf;	
		val |= (value<<4);
	}

	writel(val, base+AICR);
}

static int slinner_set_rate(void __iomem *base, u8 value, int stream)
{
	u32 val = 0;
	val = readl(base+CCR2);
	
	if(stream == SNDRV_PCM_STREAM_PLAYBACK){
		val &= 0x0f;
		val |= (value<<4);
	}
	else{
		val &= 0xf0;
		val |= (value);
	}
	//printk("-------- %x base: %x--------\n", val, base+CCR2);
	writel(val, base+CCR2);
	return 0;
}

static void slinner_pmr1(void __iomem *base, int enable, int cnt)
{
	slinner_set_param(base, 0x01, PMR1, enable, cnt);
}

static void slinner_sb(void __iomem *base, int enable)
{
	slinner_pmr1(base, enable, 7);
}

//sleep mode
static void slinner_sb_sleep(void __iomem *base, int enable)
{
	slinner_pmr1(base, enable, 6);
}

//complete aip(micbias, microphone and line inputs)
static void slinner_sb_aip(void __iomem *base, int enable)
{
	slinner_pmr1(base, !enable, 5);
}

//#ifdef EXTER
//stereo line input 
static void slinner_sb_line(void __iomem *base, int enable)
{
	slinner_pmr1(base, !enable, 4);
}
//#endif

//analog mic1 input 
static void slinner_sb_mic1(void __iomem *base, int enable)
{
	slinner_pmr1(base, !enable, 3);
}

//analog mic2 input 
static void slinner_sb_mic2(void __iomem *base, int enable)
{
	slinner_pmr1(base, !enable, 2);
}

//#ifdef EXTER
//analog line input(bypass) 
static void slinner_sb_bypass(void __iomem *base, int enable)
{
	slinner_pmr1(base, !enable, 1);
}
//#endif

//microphone biasing buffer
static void slinner_sb_micbias(void __iomem *base, int enable)
{
	slinner_pmr1(base, !enable, 0);
}

static void slinner_pmr2(void __iomem *base, int enable, int cnt)
{
	slinner_set_param(base, 0x01, PMR2, enable, cnt);
}

static void slinner_sb_adc(void __iomem *base, int enable)
{
	slinner_pmr2(base, !enable, 4);
}

//headphone output stage
static void slinner_sb_hp(void __iomem *base, int enable)
{
	slinner_pmr2(base, !enable, 3);
}

static void slinner_sb_dac(void __iomem *base, int enable)
{
	slinner_pmr2(base, !enable, 0);
}

static void slinner_icr(void __iomem *base, int enable, int cnt)
{
	slinner_set_param(base, 0x01, ICR, enable, cnt);
}

#ifdef EXTER
//Event on output jack plug detection
static void slinner_jack(void __iomem *base, int enable)
{
	slinner_icr(base, !enable, 5);
}
#endif

static void slinner_scmc(void __iomem *base, int enable)
{
	slinner_icr(base, !enable, 4);
}

#ifdef EXTER
static void slinner_rup(void __iomem *base, int enable)
{
	slinner_icr(base, !enable, 3);
}

static void slinner_rdo(void __iomem *base, int enable)
{
	slinner_icr(base, !enable, 2);
}

static void slinner_gup(void __iomem *base, int enable)
{
	slinner_icr(base, !enable, 1);
}

static void slinner_gdo(void __iomem *base, int enable)
{
	slinner_icr(base, !enable, 0);
}

static void slinner_ifr(void __iomem *base, int enable, int cnt)
{
	slinner_set_param(base, 0x01, IFR, enable, cnt);
}
#endif

#ifdef EXTER
//hp amplifier gain coupling
static void slinner_lrgo(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, GCR1, enable, 7);
}
#endif

//left channel hp amplifier gain programming value
static void slinner_hp_gol(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR1, 5, 0);
}

static int slinner_get_hp_gol(void __iomem *base)
{
	return slinner_get_param1(base, GCR1, 5, 0);
}

//right channel hp amplifier gain programming value
static void slinner_hp_gor(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR2, 5, 0);
}

static int slinner_get_hp_gor(void __iomem *base)
{
	return slinner_get_param1(base, GCR2, 5, 0);
}

//analog bypass gain coupling

#ifdef EXTER
static void slinner_lrgi(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, GCR3, 1, 7);
}
#endif

//left channel line in gain programming value
static void slinner_bypass_gil(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR3, 5, 0);
}

static int slinner_get_bypass_gil(void __iomem *base)
{
	return slinner_get_param1(base, GCR3, 5, 0);
}

//right channel line in gain programming value
static void slinner_bypass_gir(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR4, 5, 0);
}

static int slinner_get_bypass_gir(void __iomem *base)
{
	return slinner_get_param1(base, GCR4, 5, 0);
}

#ifdef EXTER
//dac digital gain coupling
static void slinner_lrgod(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, GCR5, enable, 7);
}
#endif

//left channel dac digital gain programming value
static void slinner_dac_godl(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR5, 5, 0);
}

static int slinner_get_dac_godl(void __iomem *base)
{
	return slinner_get_param1(base, GCR5, 5, 0);
}
//right channel dac digital gain programming value
static void slinner_dac_godr(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR6, 5, 0);
}

static int slinner_get_dac_godr(void __iomem *base)
{
	return slinner_get_param1(base, GCR6, 5, 0);
}

//microphone 1 boot stage gain programming value
static void slinner_mic_gim1(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR7, 3, 3);
}

static int slinner_get_mic_gim1(void __iomem *base)
{
	return slinner_get_param1(base, GCR7, 3, 3);
}

//microphone 2 boot stage gain programming value
static void slinner_mic_gim2(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR7, 3, 0);
}

static int slinner_get_mic_gim2(void __iomem *base)
{
	return slinner_get_param1(base, GCR7, 3, 0);
}

#ifdef EXTER
//adc digital gain coupling
static void slinner_lrgid(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, GCR8, enable, 7);
}
#endif

//left channel adc digital gain programming value
static void slinner_adc_gidl(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR8, 5, 0);
}

static int slinner_get_adc_gidl(void __iomem *base)
{
	return slinner_get_param1(base, GCR8, 5, 0);
}

//right channel adc digital gain programming value
static void slinner_adc_gidr(void __iomem *base, int value)
{
	slinner_set_param1(base, value, GCR9, 5, 0);
}

static int slinner_get_adc_gidr(void __iomem *base)
{
	return slinner_get_param1(base, GCR9, 5, 0);
}

#ifdef EXTER
//selection of the AGC system
static void slinner_agc_en(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, AGC1, enable, 7);
}
#endif

#ifdef EXTER
//target output level of the adc
static void slinner_target(void __iomem *base, int value)
{
	slinner_set_param1(base, value, AGC1, 4, 2);
}

//selection of the Noise Gate system
static void slinner_ng_en(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, AGC2, enable, 7);
}

//Noise Gate Threshold value
static void slinner_ng_thr(void __iomem *base, int value)
{
	slinner_set_param1(base, value, AGC2, 3, 4);
}

//Hold time before starting AGC adjustment to the target value
static void slinner_hold(void __iomem *base, int value)
{
	slinner_set_param1(base, value, AGC2, 4, 0);
}

//Attack Time-Gain Ramp Down
static void slinner_atk(void __iomem *base, int value)
{
	slinner_set_param1(base, value, AGC3, 4, 4);
}

//Decay Time - Gain Ramp up
static void slinner_dcy(void __iomem *base, int value)
{
	slinner_set_param1(base, value, AGC3, 4, 0);
}
#endif

#ifdef EXTER
//Maximum Gain Value to apply to the ADC path
static void slinner_agc_max(void __iomem *base, int value)
{
	slinner_set_param1(base, value, AGC4, 5, 0);
}

//Minimum Gain Value to apply to the ADC path
static void slinner_agc_min(void __iomem *base, int value)
{
	slinner_set_param1(base, value, AGC5, 5, 0);
}
#endif

//Mixer mode on ADC path
static void slinner_adc_mix_rec(void __iomem *base, int value)
{
	slinner_set_param1(base, value, MIX1, 2, 6);
}

//Mixer gain for input path
static void slinner_adc_gimix(void __iomem *base, int value)
{
	slinner_set_param1(base, value, MIX1, 5, 0);
}

static int slinner_get_adc_gimix(void __iomem *base)
{
	return slinner_get_param1(base, MIX1, 5, 0);
}

//Mixer mode on DAC path
static void slinner_dac_mix_rec(void __iomem *base, int value)
{
	slinner_set_param1(base, value, MIX2, 2, 6);
}

//Mixer gain for dac path
static void slinner_dac_gomix(void __iomem *base, int value)
{
	slinner_set_param1(base, value, MIX2, 5, 0);
}

static int slinner_get_dac_gomix(void __iomem *base)
{
	return slinner_get_param1(base, MIX2, 5, 0);
}

static void slinner_fast_on(void __iomem *base, int enable)
{
	slinner_set_param(base, 0x01, TR1, enable, 0);
}

static void slinner_codec_reset(void)
{
	int value;
        
	value = readl(CODEC_SW_REG);
    value &= ~CODEC_SW_RESETn;
    writel(value, CODEC_SW_REG);
    value |=  CODEC_SW_RESETn;
    writel(value, CODEC_SW_REG);
}

static void slinner_clk_init(void __iomem *crbase)
{
	u32 value;

	//enable mc_clk
	value = readl(crbase+0x08);
	value |= (0xF<<16);
	writel(value, crbase+0x08);	

	//use the parallel mode
	value = readl(crbase+0x3c);
	value &= ~0x1;
	writel(value, crbase+0x3c);	

	value = readl(crbase+0x10);
	value |= (1<<24);  //dac
	value &= ~(1<<25);
	
	value |= (1<<26);	//adc
	value &= ~(1<<27);
	writel(value, crbase+0x10);
}

static void slinner_irq_init(void __iomem *base)
{
	// Init codec irq mask as 0xe0
	writel(0x2f, base+ICR);
}

static void slinner_dac_init(void __iomem *base)
{
	int init_cnt = 3, time_out = 0;

	while (init_cnt--)
	{
#ifdef CONFIG_SILAN_INNER_CODEC_LINEOUT
	slinner_line_out(base);
#elif defined (CONFIG_SILAN_INNER_CODEC_HEADPHONE)
	slinner_headphone_out(base);
	slinner_nomad_mode(base);
#endif

	slinner_sb(base, DISABLE);
	
	msleep(500);
	
	slinner_sb_sleep(base, DISABLE);
	
	msleep(30);

	slinner_sb_dac(base, ENABLE);
    udelay(500);	
	slinner_hp_mute(base, DISABLE);
    mdelay(1);	
	
	slinner_sb_hp(base, ENABLE);

    time_out = 0;
    while (time_out < 1000)
    {
        if ((readl(base+IFR) & 0x08) != 0)
            break;
        msleep(1);
        time_out++;
    };
    if (time_out >= 1000)
    {
        slinner_codec_reset();
        printk("silan codec init failed step 1 %x\n", readl(base+IFR));
        continue;
    }
    writel(0x08, base+IFR);

    slinner_dac_mute(base, DISABLE);

    time_out = 0;
    while (time_out < 1000)
    {
        if ((readl(base+IFR) & 0x02) != 0)
            break;
        msleep(1);
        time_out++;
    };
    if (time_out >= 1000)
    {
        slinner_codec_reset();
        printk("silan codec init failed step 2 %x\n", readl(base+IFR));
        continue;
    }
    writel(0x02, base+IFR);

	slinner_hp_gol(base, 0x6);

	slinner_hp_gor(base, 0x6);
	
	slinner_dac_godl(base, 0x1);

	slinner_dac_godr(base, 0x1);
	
	slinner_scmc(base, DISABLE);
    // clear all intr 
	writel(0x5f, base + IFR);

	printk("slinner_dac_init done \n");
	break;
    }
}

static void slinner_linein_init(void __iomem *base)
{
	slinner_adc_line_in(base);

	slinner_sb_aip(base, ENABLE);

	slinner_sb_line(base, ENABLE);

	slinner_sb_bypass(base, ENABLE);
	
	slinner_sb_adc(base, ENABLE);
}

static void slinner_mic_init(void __iomem *base)
{
	slinner_mic_stereo(base);
	
	slinner_adc_mic1_in(base);

	slinner_mic_single_mode(base);

	slinner_sb_aip(base, ENABLE);
	
	slinner_sb_mic1(base, ENABLE);

	slinner_sb_mic2(base, ENABLE);

	slinner_sb_micbias(base, ENABLE);
	
	slinner_sb_adc(base, ENABLE);
}

static void slinner_init(void __iomem *base, void __iomem *crbase)
{
	slinner_clk_init(crbase);

	slinner_dac_init(base);

	slinner_irq_init(base);
}

static void set_sample_rate(void __iomem *base, int sample, int stream)
{
	u8 val = -1;
	switch (sample) {
		case 8000:
			val = RATE_8000;
			break;
		case 11025:
			val = RATE_11025;
			break;
		case 12000:
			val = RATE_12000;
			break;
		case 16000:
			val = RATE_16000;
			break;
		case 22050:
			val = RATE_22050;
			break;
		case 24000:
			val = RATE_24000;
			break;
		case 32000:
			val = RATE_32000;
			break;
		case 44100:
			val = RATE_44100;
			break;
		case 48000:
			val = RATE_48000;
			break;
		case 96000:
			val = RATE_96000;
			break;
		default:
			val = RATE_44100;
	}

	slinner_set_rate(base, val, stream);
}

static void set_bit_format(void __iomem *base, int format, int stream)
{
	u8 val = -1;

	switch (format) {
		case SNDRV_PCM_FORMAT_S16_LE:
			val = AICR_16_BIT;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
		case SNDRV_PCM_FORMAT_S24_3LE:
			val = AICR_24_BIT;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			break;
		case AICR_16_BIT:
			val = AICR_16_BIT;
			break;
		case AICR_24_BIT:
			val = AICR_24_BIT;
			break;
		default:
			val = AICR_16_BIT;
	}
	slinner_set_format(base, val, stream);
}

static int slinner_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct slinner_priv *slinner = snd_soc_codec_get_drvdata(codec);
	
	set_bit_format(slinner->base, params_format(params), substream->stream);

	set_sample_rate(slinner->base, params_rate(params), substream->stream);
	
	slinner_dac_enable(slinner->base);
	
	return 0;
}

static int slinner_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	return 0;
}
static int slinner_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	return 0;
}

#ifdef EXTER
static int slinner_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	return 0;
}
#endif

static int slinner_mute(struct snd_soc_dai *dai, int mute)
{
	//struct snd_soc_codec *codec = dai->codec;
	//struct slinner_priv *slinner = snd_soc_codec_get_drvdata(codec);

	//slinner_dac_mute(slinner->base, mute);
	//slinner_hp_mute(slinner->base, mute);

	return 0;
}

static struct snd_soc_dai_ops slinner_dai_ops = {
	.hw_params = slinner_hw_params,
	.set_fmt = slinner_set_dai_fmt,
	.digital_mute = slinner_mute,
	.set_sysclk = slinner_set_dai_sysclk,
};

static struct snd_soc_dai_driver slinner_dai = {
	.name = "silan-inner-dai",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = SLINNER_RATES,
		.formats = SLINNER_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SLINNER_RATES,
		.formats = SLINNER_FORMATS,
	},
	.ops = &slinner_dai_ops,
};

static int slinner_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	return 0;
}

static int slinner_resume(struct snd_soc_codec *codec)
{
	return 0;
}

static int slinner_resume_hp(void __iomem *base)
{
    int time_out;

    slinner_sb_hp(base, DISABLE);

    time_out = 0;
    while (time_out < 3000)
    {
        if ((readl(base+IFR) & 0x04) != 0)
            break;
        msleep(1);
        time_out++;
    };
    if (time_out >= 3000)
    {
        printk("silan codec output stage ramp down failed %x\n", readl(base+IFR));
        return -1;
    }
    writel(0x04, base+IFR);
    
    slinner_sb_hp(base, ENABLE);
    
    time_out = 0;
    while (time_out < 3000)
    {
        if ((readl(base+IFR) & 0x08) != 0)
            break;
        msleep(1);
        time_out++;
    };
    if (time_out >= 3000)
    {
        printk("silan codec output stage ramp up failed %x\n", readl(base+IFR));
        return -1;
    }
    writel(0x08, base+IFR);

    return 0;
}

static void codec_work_fn(struct work_struct *work)
{
	int value = slinner_def->icr_value;
    int time_out;

    if((value & CODEC_IRQ_SCMC) != 0) {
		printk("## %08x: codec output short detect! ##\n", value);
        time_out = 0;
        do {
            writel(value, slinner_def->base+IFR);
            msleep(1);
            value = readl(slinner_def->base+IFR);
            time_out ++;
            if (time_out > 3000)
                break;
        } while((value & CODEC_IRQ_SCMC) != 0);
        if (time_out > 3000) {
		    printk("## %08x: codec output short can't release! ##\n", value);
        } else {
            if(slinner_resume_hp(slinner_def->base) == -1)
                slinner_codec_reset();
            value  = readl(slinner_def->base+ICR);
            value &= ~CODEC_IRQ_SCMC;
	        writel(value, slinner_def->base+ICR);	
        }
	}
}

static irqreturn_t slinner_irq(int irq, void *dev)
{
	struct slinner_priv *slinner = (struct slinner_priv *)dev;
	int value = readl(slinner->base+IFR);
	
	// clear related interrupt
	writel(value, slinner->base+IFR);

	slinner->icr_value = value;

	if((value & CODEC_IRQ_SCMC) != 0) {
        // mask scmc intr, and unmask at codec_work_fn
        value  = readl(slinner->base+ICR);
        value |= CODEC_IRQ_SCMC;
	    writel(value, slinner->base+ICR);	
	}
	slinner_def = slinner;
	schedule_delayed_work(&codec_work, 0);
    
	return IRQ_HANDLED;
}

static int slinner_probe(struct snd_soc_codec *codec)
{
	snd_soc_add_controls(codec, slinner_snd_controls,
					ARRAY_SIZE(slinner_snd_controls));
	return 0;
}

/* power down chip */
static int slinner_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static inline unsigned int slinner_read(struct snd_soc_codec *codec, unsigned int reg)
{
	struct slinner_priv *slinner = snd_soc_codec_get_drvdata(codec);
    u32	val = 0;	
	//printk("##### %s %d reg: 0x%x\n", __func__, __LINE__, reg);
	
	switch(reg){
		case MIX2:
			val = readl(slinner->base+MIX2);
			val = 31-val;
			break;
		default:
			break;
	}

	return val;
}

static int slinner_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	//struct slinner_priv *slinner = snd_soc_codec_get_drvdata(codec);
	
	//printk("##### %s %d reg: 0x%x, value: 0x%x\n", __func__, __LINE__, reg, value);

	switch(reg){
		case MIX2:
			value = 31 - value;
			//slinner_dac_gomix(slinner->base+reg, value);
			break;
		default:
			break;
	}
	
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_slinner = {
	.probe	 = slinner_probe,
	.remove	 = slinner_remove,
	.read    = slinner_read,
	.write   = slinner_write,
	.dapm_widgets = slinner_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(slinner_dapm_widgets),
	.dapm_routes = slinner_intercon,
	.num_dapm_routes = ARRAY_SIZE(slinner_intercon),
	.suspend = slinner_suspend,
	.resume  = slinner_resume,
};

static int silan_inner_codec_open(struct inode *inode, struct file *fp)
{
//	struct slinner_priv *slinner = container_of(fp->private_data,
//					     struct slinner_priv, miscdev);
	return 0;
}

static int silan_inner_codec_release(struct inode *inode, struct file *fp)
{
    return 0;
}

static long silan_inner_codec_ioctl(struct file *filp, unsigned int cmd, unsigned long argp)
{
	struct slinner_priv *slinner = filp->private_data;
    int ret = 0;
	struct codec_priv param;

	if(argp != 0){
		if(copy_from_user(&param,(struct codec_priv *)argp, sizeof(param)))
			return -EFAULT;
		if(cmd != CODEC_IOCTL_MIC1_GAC && cmd != CODEC_IOCTL_MIC2_GAC)
			param.value = CODEC_MAX_DB - param.value;

		//printk("###### %s %d  val:%d  dir: %d ####\n", __func__, __LINE__, param.value, param.dir);
	}
	switch(cmd){
        case CODEC_IOCTL_MIC_ENABLE:
			slinner_mic_init(slinner->base);
            //printk("####### %d %s #######\n", __LINE__, __func__);
            break;
        case CODEC_IOCTL_LINEIN_ENABLE:
            //printk("####### %d %s #######\n", __LINE__, __func__);
            slinner_linein_init(slinner->base);
            break;
        case CODEC_IOCTL_LINEOUT_ENABLE:
            //printk("####### %d %s #######\n", __LINE__, __func__);
            slinner_dac_init(slinner->base);
            slinner_line_out(slinner->base);
            break;
        case CODEC_IOCTL_PHOUT_ENABLE:
            //printk("####### %d %s #######\n", __LINE__, __func__);
            slinner_dac_init(slinner->base);
            slinner_headphone_out(slinner->base);
            break;
		case CODEC_IOCTL_MIC_BYPASS:
			slinner_mic_init(slinner->base);
			
            slinner_micbypass_enable(slinner->base);
			
			slinner_hp_mute(slinner->base, DISABLE);

			slinner_dac_mute(slinner->base, DISABLE);
			break;
        case CODEC_IOCTL_MIXER_OUT:
			slinner_mic_init(slinner->base);
			
			slinner_dac_enable(slinner->base);
		
			slinner_dac_mix_rec(slinner->base, 0x01);

			slinner_hp_mute(slinner->base, DISABLE);

			slinner_dac_mute(slinner->base, DISABLE);
			break;
		case CODEC_IOCTL_LINEIN_BYPASS:
            slinner_linein_init(slinner->base);
		    
            slinner_linebypass_enable(slinner->base);

			slinner_hp_mute(slinner->base, DISABLE);

			slinner_dac_mute(slinner->base, DISABLE);

			break;
		case CODEC_IOCTL_MIXER_IN:
            slinner_mic_init(slinner->base);
			
			slinner_dac_enable(slinner->base);
		
			slinner_adc_mix_rec(slinner->base, 0x01);

			slinner_hp_mute(slinner->base, DISABLE);

			slinner_dac_mute(slinner->base, DISABLE);
			break;
		case CODEC_IOCTL_HP_GACL:
			if(param.dir == SET)
				slinner_hp_gol(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_hp_gol(slinner->base);	
			break;
		case CODEC_IOCTL_HP_GACR:
			if(param.dir == SET)
				slinner_hp_gor(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_hp_gor(slinner->base);	
			break;
		case CODEC_IOCTL_DAC_GACL:
			if(param.dir == SET)
				slinner_dac_godl(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_dac_godl(slinner->base);	
			break;
		case CODEC_IOCTL_DAC_GACR:
			if(param.dir == SET)
				slinner_dac_godr(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_dac_godr(slinner->base);	
			break;
		case CODEC_IOCTL_ADC_GACL:
			if(param.dir == SET)
				slinner_adc_gidl(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_adc_gidl(slinner->base);	
			break;
		case CODEC_IOCTL_ADC_GACR:
			if(param.dir == SET)
				slinner_adc_gidr(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_adc_gidr(slinner->base);	
			break;
		case CODEC_IOCTL_MIC1_GAC:
			if(param.dir == SET)
				slinner_mic_gim1(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MIC_MAX_DB - slinner_get_mic_gim1(slinner->base);	
			break;
		case CODEC_IOCTL_MIC2_GAC:
			if(param.dir == SET)
				slinner_mic_gim2(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MIC_MAX_DB - slinner_get_mic_gim2(slinner->base);	
			break;
		case CODEC_IOCTL_LINEIN_GACL:
			if(param.dir == SET)
				slinner_bypass_gil(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_bypass_gil(slinner->base);	
			break;
		case CODEC_IOCTL_LINEIN_GACR:
			if(param.dir == SET)
				slinner_bypass_gir(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_bypass_gir(slinner->base);	
			break;
		case CODEC_IOCTL_MIX_OUT_GAC:
			if(param.dir == SET)
				slinner_dac_gomix(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_dac_gomix(slinner->base);	
			break;
		case CODEC_IOCTL_MIX_IN_GAC:
			if(param.dir == SET)
				slinner_adc_gimix(slinner->base, param.value);
			else if(param.dir == GET)
				param.value = CODEC_MAX_DB - slinner_get_adc_gimix(slinner->base);
			break;
		case CODEC_IOCTL_DAC_MUTE:
			if(param.dir == SET){
				if(param.flag == DISABLE){
					slinner_dac_mute(slinner->base, DISABLE);
				}
				else if(param.flag == ENABLE){
					slinner_dac_mute(slinner->base, ENABLE);
				}
			}
			break;
		case CODEC_IOCTL_DAC_SAMPLE:
			set_sample_rate(slinner->base, param.arg, param.flag);	
			break;
		case CODEC_IOCTL_DAC_BIT:
			set_bit_format(slinner->base, param.arg, param.flag);	
			break;
        case CODEC_IOCTL_LINEOUT_DAC:
            slinner_dac_enable(slinner->base);
            break;
		default:
			break;
	}
	
	if(param.dir == GET){
		if(copy_to_user((struct codec_priv*)argp,&param,sizeof(param)))
			return -EFAULT;
	}

	return ret;
}

static const struct file_operations silan_inner_codec_ops = {
    .owner   = THIS_MODULE,
    .open    = silan_inner_codec_open,
    .release = silan_inner_codec_release,
    .unlocked_ioctl = silan_inner_codec_ioctl,
};

static int __devinit slinner_dev_probe(struct platform_device *pdev)
{
	struct slinner_priv *slinner;
	struct resource *res;
	int ret, irq;

	slinner = kzalloc(sizeof(struct slinner_priv), GFP_KERNEL);
	if(slinner == NULL)
		return -ENOMEM;
    
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(res == NULL){
        printk("Can't find silan inner codec resource\n");
        return -ENOENT;
    }

    slinner->base = ioremap(res->start, (res->end - res->start));
    if(slinner->base == NULL){
        printk("Can not silan inner codec io\n");
        return -ENXIO;
    }
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if(res == NULL){
        printk("Can't find cr resource\n");
        return -ENOENT;
    }

    slinner->crbase = ioremap(res->start, (res->end - res->start));
    if(slinner->base == NULL){
        printk("Can not cr io\n");
        return -ENXIO;
    }

    INIT_DELAYED_WORK(&codec_work, codec_work_fn);
	
    irq = platform_get_irq(pdev, 0);
    if(irq <= 0){
        printk("Can not find IRQ\n");
        return -ENXIO;
    }

	ret = request_irq(irq, slinner_irq, IRQF_DISABLED, "slinner", slinner);
	if (ret) {
		printk("%s failed to request interrupt %d\n", __func__, irq);
		return -1;
	}
	
	slinner_init(slinner->base, slinner->crbase);
	
	slinner->miscdev.minor = MISC_DYNAMIC_MINOR;
	slinner->miscdev.name = SILAN_INNER_CODEC_NAME;
	slinner->miscdev.fops = &silan_inner_codec_ops;
	
	dev_set_drvdata(&pdev->dev, slinner);

	//tasklet_init(&slinner->tasklet, slinner_tasklet, (unsigned long)slinner);
	
	if (misc_register(&slinner->miscdev) < 0){
        printk("Can not register the misc dev\n");
        return -EBUSY;
    }
	
	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_slinner, 
			&slinner_dai, 1);
}

static int __devexit slinner_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);

	return 0;
}

static struct platform_driver slinner_driver = {
	.driver = {
		.name = "silan-inner",
		.owner = THIS_MODULE,
	},
	.probe = slinner_dev_probe,
	.remove = slinner_dev_remove,
};

static int __init slinner_modinit(void)
{
	int ret = 0;
	ret = platform_driver_register(&slinner_driver);
	if(ret != 0) 
		printk(KERN_ERR "Failed to register slinner driver: %d\n", ret);
	return ret;
}
module_init(slinner_modinit);

static void __exit slinner_codec_exit(void)
{
	platform_driver_unregister(&slinner_driver);
}
module_exit(slinner_codec_exit);

MODULE_DESCRIPTION("ASoC Silan Inner Codec driver");
MODULE_AUTHOR("Chen Jianneng <chenjianneng@silan.com.cn>");
MODULE_LICENSE("GPL");
