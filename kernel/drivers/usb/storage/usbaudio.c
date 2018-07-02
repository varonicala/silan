/*
 * Driver for Apple device audio  
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>

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
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_def.h>
#include <silan_memory.h>
#include <silan_gpio.h>
#include <linux/dma-mapping.h>

#include <linux/sched.h>
#include <linux/version.h>

#include "chap9.h"
#include "usb.h"
#include "transport.h"
#include "protocol.h"
#include "debug.h"

#define UVC_TRACE_FRAME		(1 << 7)

#define UVC_URBS		5
#define MAX_WAIT_TIME		500

#define MIPS_DSP_SYSCHANGE_SIZE		(PAGE_SIZE)	
#define DSP_R_RINGBUFFER_SIZE 0x100000
#define DSP_W_RINGBUFFER_SIZE 0x100000

#define KEY_UP 					0x40
#define MSG_KEY_				0x0100
#define	KEY_PLAY				(MSG_KEY_|0x08|KEY_UP)
#define	KEY_NEXT				(MSG_KEY_|0x07|KEY_UP)
#define	KEY_PREV				(MSG_KEY_|0x06|KEY_UP)
#define	KEY_STOP    			(MSG_KEY_|0x05|KEY_UP)
#define	KEY_RM					(MSG_KEY_|0x04|KEY_UP)
#define	KEY_CH					(MSG_KEY_|0x03|KEY_UP)
#define KEY_VOLUP				(MSG_KEY_|0x02|KEY_UP)
#define KEY_VOLDN				(MSG_KEY_|0x01|KEY_UP)

#define IRVALUE_PLAY			0x45
#define IRVALUE_PREV			0x16
#define IRVALUE_NEXT			0x19
#define IRVALUE_STOP			0x17
#define IRVALUE_RM		    	0x16
#define IRVALUE_VOLUP		    0x15
#define IRVALUE_VOLDN		    0x14
#define IRVALUE_CH   			0x18

#define MSG_IR0_				0x0A00
#define MSG_USER_				0x0C00
#define	IRKEY_PLAY 				(MSG_IR0_|IRVALUE_PLAY)
#define	IRKEY_NEXT				(MSG_IR0_|IRVALUE_NEXT)
#define	IRKEY_PREV				(MSG_IR0_|IRVALUE_PREV)
#define	IRKEY_STOP 				(MSG_IR0_|IRVALUE_STOP)
#define	IRKEY_RM				(MSG_IR0_|IRVALUE_RM)
#define	IRKEY_CH				(MSG_IR0_|IRVALUE_CH)

#define IRKEY_VOLUP				(MSG_USER_|IRVALUE_VOLUP)
#define IRKEY_VOLDN				(MSG_USER_|IRVALUE_VOLDN)

#define	SwapCon16(val)				(((val) << 8) | ((unsigned short)(val) >> 8))
#define USBLP_CTL_TIMEOUT	5000			/* 5 seconds */
//#define I2C_DBG
#define USB_AUDIO_DEV_NAME "silan-usbaudio"
typedef struct usb_audio_drv_context_t {
	struct fasync_struct *async_queue;	
} usb_audio_drv_context_t;

#if 0
static spinlock_t s_usb_audio_lock = __SPIN_LOCK_UNLOCKED(s_usb_audio_lock);
static usb_audio_drv_context_t s_usb_audio_drv_context;
static u32 s_usb_audio_open_count;
#endif

struct usbaudio_ringbuffer {
	u8               *data;
	ssize_t           size;
	ssize_t           pread;
	ssize_t           pwrite;
	int               error;

	wait_queue_head_t queue;
	spinlock_t        lock;
};

typedef struct
{
	int filternum;
	struct dspdev_filter *filter;
	int initialized;
	int users;
	struct mutex mutex;
	struct list_head	queue;
}dspdev_info_t;

struct dspdev_filter
{
//	enum dspdev_state state;
	struct mutex rmutex;
	struct mutex wmutex;
	struct usbaudio_ringbuffer rbuffer;
	struct usbaudio_ringbuffer wbuffer;
	struct timer_list timer;
	dspdev_info_t *pdspdev_info;

	u32 taskid;
//	DSP_CODEC_TYPE type;
	struct list_head	queue;
	u32 rdma_addr;
	u32 wdma_addr;
	u32 frame_sync;
	u8* sysex;
	u8* bufstart;
	u32 update;
};

struct usb_dev_info {
	struct usb_interface *intf;
	struct usb_device *udev;
	int qdepth;
	unsigned cmd_pipe, status_pipe, data_in_pipe, data_out_pipe;
	unsigned bulkin_pipe,bulkout_pipe,iso_pipe,intin_pipe;
	unsigned use_streams:1;
	unsigned usb_sense_old:1;
	struct i2c_client *client;
	struct delayed_work work;
	struct urb *urb;
};

dspdev_info_t       usbaudio_dev_info;
struct usb_dev_info *devinfo;
struct i2c_client *usb_i2c;

unsigned char	s_buf[0x420];
DEVICE_REQUEST  *s_pStdReq;
unsigned char   s_DevAddr;

unsigned short	TransID = 0;
unsigned char to_char_data[192*8000];
int flag = 0;

#define UVC_MAX_PACKETS 10

struct uvc_streaming {
	struct urb * urb[UVC_URBS];
	char *urb_buffer[UVC_URBS];
	dma_addr_t urb_dma[UVC_URBS];
	unsigned int urb_size;
};

static int usb_i2c_i2c_read(struct i2c_client *client, u8 *reg, u8 *data,int len1,int len2)
{
    int ret;
    struct i2c_msg msgs[2] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = len1,
            .buf = reg,
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = len2,
            .buf = data,
        }

    };

    ret = i2c_transfer(client->adapter, msgs, 2);
 #ifdef I2C_DBG
    for(ret = 0; ret < len1; ret++)
        printk("reg[%d]: %x\n", ret, reg[ret]);
    for(ret = 0; ret < len2; ret++)
        printk("data[%d]: %x\n", ret, data[ret]);
#endif
   
    return ret;
}

static int usb_i2c_i2c_write(struct i2c_client *client, u8 *reg, u8 *data, int len1,int len2)
{
    int ret;
	u8 buff[24];	

	for(ret = 0; ret < len1; ret++)
		buff[ret] = reg[ret];
	for(ret = 0; ret < len2; ret++)
		buff[len1+ret] = data[ret];
    
	struct i2c_msg msgs[1] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = len1 +len2,
            .buf = buff,
        },
    };

    ret = i2c_transfer(client->adapter, msgs, 1);
#ifdef I2C_DBG
    for(ret = 0; ret < len1; ret++)
        printk("reg[%d]: %x\n", ret, reg[ret]);
    for(ret = 0; ret < len2; ret++)
        printk("data[%d]: %x\n", ret, data[ret]);
#endif

    return ret; 
}


/* USB Audio for apple device */
void CP(unsigned char * challenge, unsigned char * read_data)
{
	u8 buf1[2] = {0x20};
	u8 buf2[2] = {0x00, 0x14};

	usb_i2c_i2c_write(devinfo->client, buf1, buf2, 1, 2);
//	usb_i2c_i2c_read(devinfo->client, buf1, buf2, 1, 1);
	
	buf1[0] = 0x21;
	usb_i2c_i2c_write(devinfo->client, buf1,challenge,1,20);
	buf1[0] = 0x10;
	buf2[0] = 1;
	usb_i2c_i2c_write(devinfo->client, buf1,buf2,1,1);
	
	msleep(1000);
	buf1[0] = 0x10;
	usb_i2c_i2c_read(devinfo->client, buf1, read_data, 1, 1);
//	printk("$$$$$$ read_data[0]: %x\n", read_data[0]);
	if(read_data[0] == 0x10)
	{
		buf1[0]= 0x11;
		usb_i2c_i2c_read(devinfo->client, buf1,buf2,1,2);
	//	printk("SignatureLength is %x %x\n", buf2[0], buf2[1]);
		buf1[0] = 0x12;
		usb_i2c_i2c_read(devinfo->client, buf1,read_data,1,128);
	}
}

void cp_readcert(unsigned char * read_data)
{
	int i = 0;
	unsigned char reg[2];
	reg[0] = 0x31;
	
	for(i = 0;i < 8;i++, reg[0]++)
		usb_i2c_i2c_read(usb_i2c, reg, read_data+(i*0x80),0x1, 128);
}


unsigned char CheckSum(unsigned char * pData,unsigned short Length)
{
	 unsigned char sum=0;
	 unsigned short i;
	
	 for(i=0;i<=Length;i++)
	 	sum+= pData[i];
	 
	sum=0x100-sum;
	return sum;	
}

int StartIDPS(void)
{
	int actual_len;
	__u8 bmRequestType;
	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x09;//USB_REQUEST_SET_CONFIGURATION
	s_pStdReq->wValue					= 0x0205;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= 0x900;
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	TransID++;
	s_buf[0x8]=0x05;
	s_buf[0x9]=0x00;
	s_buf[0xa]=0x55;
	s_buf[0xb]=0x04;
	s_buf[0xc]=0x00;
	s_buf[0xd]=0x38;
	s_buf[0xe]=TransID>>8;
	s_buf[0xf]=TransID&0xff;
	s_buf[0x10]=CheckSum(s_buf+0xb,4);
	
//	if ( USBSTATUS_OK != usb_control_msg(s_DevAddr, s_buf, 9, false) )//startIDPS Packet
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
		usb_sndctrlpipe(devinfo->udev, 0),
		s_pStdReq->bRequest,	
		bmRequestType,
		s_pStdReq->wValue,
		s_pStdReq->wIndex,
		&s_buf[8],
		9,
		USBLP_CTL_TIMEOUT));


	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
		usb_rcvintpipe(devinfo->udev, 3),
		&s_buf[8],
		13,
		&actual_len,
		USBLP_CTL_TIMEOUT));

	return 1;
}

static int	RequestTransprotMaxPayLoadSize(void)
{
	int actual_len;
	__u8 bmRequestType;
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x09;//USB_REQUEST_SET_CONFIGURATION
	s_pStdReq->wValue					= 0x0205;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= 0x900;
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);	
	TransID++;
	s_buf[0x8]=0x05;
	s_buf[0x9]=0x00;
	s_buf[0xa]=0x55;
	s_buf[0xb]=0x04;
	s_buf[0xc]=0x00;
	s_buf[0xd]=0x11;
	s_buf[0xe]=TransID>>8;
	s_buf[0xf]=TransID&0xff;
	s_buf[0x10]=CheckSum(s_buf+0xb,4);

	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
		usb_sndctrlpipe(devinfo->udev, 0),
		s_pStdReq->bRequest,	
		bmRequestType,
		s_pStdReq->wValue,
		s_pStdReq->wIndex,
		&s_buf[8],
		9,
		USBLP_CTL_TIMEOUT));
	
	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
		usb_rcvintpipe(devinfo->udev, 3),
		&s_buf[8],
		11,
		&actual_len,
		USBLP_CTL_TIMEOUT));

	return 1;	
}

static int	GetiPodOptionsForLingo(void)
{
	int actual_len;
	unsigned char bmRequestType=0;
	
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x09;//USB_REQUEST_SET_CONFIGURATION
	s_pStdReq->wValue					= 0x0206;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= 0xa00;
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	TransID++;
	s_buf[0x8]=0x06;
	s_buf[0x9]=0x00;
	s_buf[0xa]=0x55;
	s_buf[0xb]=0x05;
	s_buf[0xc]=0x00;
	s_buf[0xd]=0x4b;
	s_buf[0xe]=TransID>>8;
	s_buf[0xf]=TransID&0xff;
	s_buf[0x10]= 0x0;
	s_buf[0x11]=CheckSum(s_buf+0xb,5);
	
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
		usb_sndctrlpipe(devinfo->udev, 0),
		s_pStdReq->bRequest,	
		bmRequestType,
		s_pStdReq->wValue,
		s_pStdReq->wIndex,
		&s_buf[8],
		10,
		USBLP_CTL_TIMEOUT));
		
	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
		usb_rcvintpipe(devinfo->udev, 3),
		&s_buf[8],
		18,
		&actual_len,
		USBLP_CTL_TIMEOUT));

	return 1;	

}
	  
static int SetFIDTokenValue(void)
{
		int actual_len;
		#define Count_start 8
		unsigned char bmRequestType;
		unsigned char * ipod_coach=s_buf+9;
		unsigned char numFIDTokenValues,count,j;
		unsigned char numLingoes[]={4,0x00,0x02,0x03,0x0A};
		unsigned char accCapsBitmask[]={0x00,0x00  ,0x00  ,0x00  ,0x00  ,0x00  ,0x00  ,0x10 };
		unsigned char AccessoryInfo_name[]={"CDMP3"};
		unsigned char AccessoryInfo_accessory_manufacturer[]={"SILAN"};
		unsigned char AccessoryInfo_accessory_firmware_version[3]={0,0,0};
		unsigned char AccessoryInfo_accessory_hardware_version[3]={0,0,0};
		unsigned char AccessoryInfo_accessory_model_number[]={"x0.0"};
		unsigned char AccessoryInfo_accessory_RFCertificationDeclaration[4]={0,0,0,0xb};
		unsigned char FIDTokenLen=0;
		
		TransID++;
		s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
		s_pStdReq->bmRequestType.Type		= 1;
		s_pStdReq->bmRequestType.DirIn		= FALSE;
		s_pStdReq->bRequest					= 0x09;
		s_pStdReq->wValue					= 0x0209;
		s_pStdReq->wIndex					= 0x0002;
		bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);		
	
	s_buf[0x8]=0x09;
	s_buf[0x9]=0x00;
	{	//IdentifyToken format
				
				*(ipod_coach + 1) = 0x55;
				*(ipod_coach + 3) = 0x00;
				*(ipod_coach + 4) = 0x39;
				*(ipod_coach + 5) = TransID>>8;
				*(ipod_coach + 6) = TransID;		
				numFIDTokenValues = 0;
				//Token name = Identify Token	
				 count = 0;
				*(ipod_coach + Count_start + count++) = 11 + numLingoes[0];//Length of this token-value field in bytes ,not include this byte
				*(ipod_coach + Count_start + count++) = 0;//InfoByteOne, first byte of Token ID
				*(ipod_coach + Count_start + count++) = 0;//InfoByteTwo, second byte of Token ID 
				*(ipod_coach + Count_start + count++) = numLingoes[0];//numLingoes :0x00 0x02 0x03 0x04

				for(j = 1;j <= numLingoes[0];j++)
				{
					*(ipod_coach + Count_start + count++) = numLingoes[j];
				}

				*(ipod_coach + Count_start + count++) = 0x00;//Device Options 0x.. nn nn nn
				*(ipod_coach + Count_start + count++) = 0x00;//Device Options 0xnn .. nn nn
				*(ipod_coach + Count_start + count++) = 0x00;//Device Options 0xnn nn .. nn
				*(ipod_coach + Count_start + count++) = 0x02;//Device Options 0xnn nn nn ..
					
				*(ipod_coach + Count_start + count++) = 0x00;//Device ID Authentication Coprocessor 2.0B 0x.. nn nn nn
				*(ipod_coach + Count_start + count++) = 0x00;//Device ID Authentication Coprocessor 2.0B 0xnn .. nn nn
				*(ipod_coach + Count_start + count++) = 0x02;//Device ID Authentication Coprocessor 2.0B 0xnn nn .. nn
				*(ipod_coach + Count_start + count++) = 0x00;//Device ID Authentication Coprocessor 2.0B 0xnn nn nn ..
				numFIDTokenValues++;
				
				//Token name = AccCaps [Accessory Capabilities]
				*(ipod_coach + Count_start + count++) = 10;//Length of this token-value field in bytes ,not include this byte
				*(ipod_coach + Count_start + count++) = 0;//InfoByteOne, first byte of Token ID
				*(ipod_coach + Count_start + count++) = 1;//InfoByteTwo, second byte of Token ID 

				for(j = 0;j < 8;j++)
				{
					*(ipod_coach + Count_start + count++) = accCapsBitmask[j];//Capabilities bits(63...56)
				}
				numFIDTokenValues++;
				
				//Token name = AccInfo Token
				j = 0;
				while(AccessoryInfo_name[j] != 0x00)
				{
					j++;
				}
				j++;
				*(ipod_coach + Count_start + count++) = j+3;//Length of this token-value field in bytes ,not include this byte
				*(ipod_coach + Count_start + count++) = 0;//InfoByteOne, first byte of Token ID
				*(ipod_coach + Count_start + count++) = 2;//InfoByteTwo, second byte of Token ID 
				*(ipod_coach + Count_start + count++) = 0x01;//acc info type [Accessory name]
					
				for (j = 0;AccessoryInfo_name[j] != 0x00; j++)
				{
					*(ipod_coach + Count_start + count++) = AccessoryInfo_name[j];
				}
				*(ipod_coach + Count_start + count++) = 0x00;//Null
				numFIDTokenValues++;

				*(ipod_coach + Count_start + count++) = 6;//Length of this token-value field in bytes ,not include this byte
				*(ipod_coach + Count_start + count++) = 0;//InfoByteOne, first byte of Token ID
				*(ipod_coach + Count_start + count++) = 2;//InfoByteTwo, second byte of Token ID 
				*(ipod_coach + Count_start + count++) = 4;//acc info type [Accessory fireware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_firmware_version[0];//acc info type [Accessory fireware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_firmware_version[1];//acc info type [Accessory fireware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_firmware_version[2];//acc info type [Accessory fireware version]				
				numFIDTokenValues++;

				*(ipod_coach + Count_start + count++) = 6;//Length of this token-value field in bytes ,not include this byte
				*(ipod_coach + Count_start + count++) = 0;//InfoByteOne, first byte of Token ID
				*(ipod_coach + Count_start + count++) = 2;//InfoByteTwo, second byte of Token ID 
				*(ipod_coach + Count_start + count++) = 5;//acc info type [Accessory hardware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_hardware_version[0];//acc info type [Accessory fireware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_hardware_version[1];//acc info type [Accessory fireware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_hardware_version[2];//acc info type [Accessory fireware version]				
				numFIDTokenValues++;

				j = 0;
				while(AccessoryInfo_accessory_manufacturer[j] != 0x00)
				{
					j++;
				}
				j++;
				*(ipod_coach + Count_start + count++) = j+3;//Length of this token-value field in bytes ,not include this byte
				*(ipod_coach + Count_start + count++) = 0;//InfoByteOne, first byte of Token ID
				*(ipod_coach + Count_start + count++) = 2;//InfoByteTwo, second byte of Token ID 
				*(ipod_coach + Count_start + count++) = 6;//acc info type [Accessory manufactuer]
					
				for (j = 0;AccessoryInfo_accessory_manufacturer[j] != 0x00; j++)
				{
					*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_manufacturer[j];
				}
				*(ipod_coach + Count_start + count++) = 0x00;//Null
				numFIDTokenValues++;

				j = 0;
				{
					while(AccessoryInfo_accessory_model_number[j] != 0x00)
					{
						j++;
					}
				}
				j++;
				*(ipod_coach + Count_start + count++) = j+3;//Length of this token-value field in bytes ,not include this byte
				*(ipod_coach + Count_start + count++) = 0;//InfoByteOne, first byte of Token ID
				*(ipod_coach + Count_start + count++) = 2;//InfoByteTwo, second byte of Token ID 
				*(ipod_coach + Count_start + count++) = 7;//acc info type [Accessory model number]
				
				{
					for (j = 0;AccessoryInfo_accessory_model_number[j] != 0x00; j++)
					{
						*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_model_number[j];
					}
			}
				*(ipod_coach + Count_start + count++) = 0x00;//Null			
				numFIDTokenValues++;

				*(ipod_coach + Count_start + count++) = 7;//Length of this token-value field in bytes ,not include this byte
				*(ipod_coach + Count_start + count++) = 0;//InfoByteOne, first byte of Token ID
				*(ipod_coach + Count_start + count++) = 2;//InfoByteTwo, second byte of Token ID 
				*(ipod_coach + Count_start + count++) = 0x0c;//acc info type [Accessory incoming]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_RFCertificationDeclaration[0];//acc info type [Accessory fireware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_RFCertificationDeclaration[1];//acc info type [Accessory fireware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_RFCertificationDeclaration[2];//acc info type [Accessory fireware version]
				*(ipod_coach + Count_start + count++) = AccessoryInfo_accessory_RFCertificationDeclaration[3];//acc info type [Accessory fireware version]
				numFIDTokenValues++;

				*(ipod_coach + 2) = count + 0x05;
				*(ipod_coach + 7) = numFIDTokenValues;
				*(ipod_coach + Count_start + count++) = CheckSum(s_buf+0xb,s_buf[0xb]);		
				
			}
			FIDTokenLen=(*(ipod_coach + 2))+7;
			
			if(FIDTokenLen>0x40)
			{
					s_pStdReq->wLength					= 0x4000;
					s_buf[0x9]=0x02;
					
				if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0x40,
					USBLP_CTL_TIMEOUT));
					
					FIDTokenLen-=0x40;
					s_buf[0x9]=0x01; 
					memcpy(s_buf+10,s_buf+0x48,FIDTokenLen);
			}
	
	s_pStdReq->wLength					=((unsigned short)FIDTokenLen)<<8;

	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
				usb_sndctrlpipe(devinfo->udev, 0),
				s_pStdReq->bRequest,	
				bmRequestType,
				s_pStdReq->wValue,
				s_pStdReq->wIndex,
				&s_buf[0x8],
				FIDTokenLen,
				USBLP_CTL_TIMEOUT));

	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					0x40,
					&actual_len,
					USBLP_CTL_TIMEOUT));

	return 1;
}

static int EndIDPS(void)
{
	int actual_len;
	__u8 bmRequestType;
	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x09;
	s_pStdReq->wValue					= 0x0206;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= 0xa00;
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	TransID++;
	s_buf[0x8]=0x09;
	s_buf[0x9]=0x02;
	s_buf[0xa]=0x55;
	s_buf[0xb]=0x05;
	s_buf[0xc]=0x00;
	s_buf[0xd]=0x3b;
	s_buf[0xe]=TransID>>8;
	s_buf[0xf]=TransID&0xff;
	s_buf[0x10]= 0x0;
	s_buf[0x11]=CheckSum(s_buf+0xb,5);
		
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xa,
					USBLP_CTL_TIMEOUT));
	
	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					13,
					&actual_len,
					USBLP_CTL_TIMEOUT));
	
	return 1;
}

static int Authentication(void)
{
	unsigned int actual_len;
	__u8 bmRequestType;
	unsigned char * pData=s_buf+0x16 ;
	unsigned char i=0;
	unsigned char nCheckSum;
	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					13,
					&actual_len,
					USBLP_CTL_TIMEOUT));
	
	TransID=(((unsigned short)s_buf[6+8])<<8)+s_buf[7+8];
	s_buf[0x10]=s_buf[6+8];
	s_buf[0x11]=s_buf[7+8];
//	printk("s_buf[14] %x,s_buf[15] %x\n",s_buf[14],s_buf[15]);

	cp_readcert(pData);//read accessory certificate lenth and data	 8*128byte
	
	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x09;
	s_pStdReq->wValue					= 0x0209;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= 0x4000;
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	s_buf[0x8]=0x09;
	s_buf[0x9]=0x02;
	s_buf[0xa]=0x55;
	s_buf[0xb]=0x00;
	s_buf[0xc]=0x01;
	s_buf[0xd]=0xfc;
	s_buf[0xe]=0;
	s_buf[0xf]=0x15;
	
	s_buf[0x12]=0x02;
	s_buf[0x13]=0;
	s_buf[0x14]=0;
	s_buf[0x15]=0x1;

	nCheckSum=CheckSum(s_buf+0xb,0x1fe);
	pData+=50;
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					64,
					USBLP_CTL_TIMEOUT));
	
	s_buf[0x9]=3;
	for(;i<7;i++)
	{	
		memcpy(s_buf+0xa,pData,62);
		pData+=62;
		if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					64,
					USBLP_CTL_TIMEOUT));
	}
			
	s_pStdReq->wValue					= 0x0208;
	s_pStdReq->wLength					= 0x1300;
	s_buf[0x8]=8;
	s_buf[0x9]=1;
		
	memcpy(s_buf+0xa,pData,16);
	pData+=16;

	s_buf[0x1a]=nCheckSum;
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					19,
					USBLP_CTL_TIMEOUT));
	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					13,
					&actual_len,
					USBLP_CTL_TIMEOUT));
	
	
	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x09;
	s_pStdReq->wValue					= 0x0209;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= 0x4000;
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	s_buf[0x8]=0x09;
	s_buf[0x9]=0x02;
	s_buf[0xa]=0x55;
	s_buf[0xb]=0x00;
	s_buf[0xc]=0x01;
	s_buf[0xd]=0xc5;
	s_buf[0xe]=0;
	s_buf[0xf]=0x15;
	s_buf[0x10]=TransID>>8;
	s_buf[0x11]=TransID&0xff;
	s_buf[0x12]=0x02;
	s_buf[0x13]=0;
	s_buf[0x14]=1;
	s_buf[0x15]=0x1;
	
	memcpy(s_buf+0x16,pData,0x1c5);
	pData=s_buf+0x16;
	
	nCheckSum=CheckSum(s_buf+0xb,0x1c7);
	pData+=50;
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					64,
					USBLP_CTL_TIMEOUT));
	
	s_buf[0x9]=3;
	for(i=0;i<6;i++)
	{	
			
		memcpy(s_buf+0xa,pData,62);
		pData+=62;
		if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					64,
					USBLP_CTL_TIMEOUT));
	}
			
	s_pStdReq->wLength					= 0x1a00;
	s_buf[0x9]=1;
		
	memcpy(s_buf+0xa,pData,24);
	pData+=23;

	s_buf[0x21]=nCheckSum;
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0x1a,
					USBLP_CTL_TIMEOUT));
	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					13,
					&actual_len,
					USBLP_CTL_TIMEOUT));
	
	return 1;
}

static int RetSampleRate(void)
{
		__u8 bmRequestType;
		int actual_len;
		if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					13,
					&actual_len,
					USBLP_CTL_TIMEOUT));
		TransID=(((unsigned short)s_buf[0xe])<<8)+s_buf[0xf];

		s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
		s_pStdReq->bmRequestType.Type		= 1;
		s_pStdReq->bmRequestType.DirIn		= FALSE;
		s_pStdReq->bRequest					= 0x09;
		s_pStdReq->wValue					= 0x0208;
		s_pStdReq->wIndex					= 0x0002;
		s_pStdReq->wLength					= 0x1500;
		bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);	
	{	
		unsigned char tempChar[0x15]=
		{0x08,0x00,
			0x55,
			0x10,
			0x0a,0x03,
			0x00,0x1c,
			0x00,0x00,0x7d,0x00,
			0x00,0x00,0xac,0x44,
			0x00,0x00,0xbb,0x80,
			0x1f };
		
		
		memcpy(s_buf+0x8,tempChar,0x15);
		s_buf[0xE]=TransID>>8;
		s_buf[0xf]=TransID&0xff;
		s_buf[0x1c]=CheckSum(s_buf+0xb,16);

		if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0x15,
					USBLP_CTL_TIMEOUT));
	}
	return 1;	

}

static int  AccAuthenticationSignature(void)
{
	__u8 bmRequestType;
	int actual_len;
	unsigned char tmp[64];
	unsigned char nCheckSum;
	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					64,
					&actual_len,
					USBLP_CTL_TIMEOUT));
	TransID=(((unsigned short)s_buf[14])<<8)+s_buf[15];

	CP(s_buf+16,s_buf+16);	
	
	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&tmp,
					13,
					&actual_len,
					USBLP_CTL_TIMEOUT));
	
	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x09;
	s_pStdReq->wValue					= 0x0209;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= 0x4000;
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	s_buf[0x8]=0x09;
	s_buf[0x9]=0x02;
	s_buf[0xa]=0x55;
	s_buf[0xb]=0x84;
	s_buf[0xc]=0x00;
	s_buf[0xd]=0x18;
	s_buf[0xe]=TransID>>8;
	s_buf[0xf]=TransID&0xff;

	nCheckSum=CheckSum(s_buf+0xb,0x84);
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					64,
					USBLP_CTL_TIMEOUT));

	s_buf[9]=0x03;
	memcpy(s_buf+0xa,s_buf+16+0x38,0x3e);
	
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev,
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					64,
					USBLP_CTL_TIMEOUT));
	  	s_pStdReq->wLength					= 0xd00;
		s_buf[9]=0x01;
		memcpy(s_buf+0xa,s_buf+16+0x38+0x3e,0xa);
		s_buf[0x8+0xd-1]=nCheckSum;
		
		if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0x0d,
					USBLP_CTL_TIMEOUT));

	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					13,
					&actual_len,
					USBLP_CTL_TIMEOUT));
	 TransID=(((unsigned short)s_buf[14])<<8)+s_buf[15];
	
	
	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x09;
	s_pStdReq->wValue					= 0x0206;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= 0xb00;
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	s_buf[0x8]=0x06;
	s_buf[0x9]=0x00;
	s_buf[0xa]=0x55;
	s_buf[0xb]=0x6;
	s_buf[0xc]=0xa;
	s_buf[0xd]=0x00;
	s_buf[0xe]=TransID>>8;
	s_buf[0xf]=TransID&0xff;
	s_buf[0x10]=0;
	s_buf[0x11]=4;
	s_buf[0x12]=CheckSum(s_buf+0xb,0x6);	
		
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xb,
					USBLP_CTL_TIMEOUT));
	
	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_ENDPOINT;
	s_pStdReq->bmRequestType.Type		= 1;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x01;
	s_pStdReq->wValue					= 0x0100;
	s_pStdReq->wIndex					= 0x0081;
	s_pStdReq->wLength					= 0x0300;

	s_buf[0x8]=0x44;
	s_buf[0x9]=0xac;
	s_buf[0xa]=0x00;
		
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					3,
					USBLP_CTL_TIMEOUT));

	if ( USBSTATUS_OK != usb_interrupt_msg(devinfo->udev, 
					usb_rcvintpipe(devinfo->udev, 3),
					&s_buf[0x8],
					13,
					&actual_len,
					USBLP_CTL_TIMEOUT));

	return 1;
}

static int USBAudioPlay(void)
{
		__u8 bmRequestType;
	   	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
		s_pStdReq->bmRequestType.Type		= 0;
		s_pStdReq->bmRequestType.DirIn		= FALSE;
		s_pStdReq->bRequest					= 0x0b;
		s_pStdReq->wValue					= 0x0001;
		s_pStdReq->wIndex					= 0x0001;
		s_pStdReq->wLength					= 0x0000;
	
		bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
		if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0,
					USBLP_CTL_TIMEOUT));
		
		s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
		s_pStdReq->bmRequestType.Type		= 1;
		s_pStdReq->bmRequestType.DirIn		= FALSE;
		s_pStdReq->bRequest					= 0x09;
		s_pStdReq->wValue					= 0x0207;
		s_pStdReq->wIndex					= 0x0002;
		s_pStdReq->wLength					= 0xd00;
		
		bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
		{	
		unsigned char tempChar[0xd]={0x07,0x00,0x55,0x8,0x02,0x00,0x00,0x1a,0x00,0x01,0x00,0x00,0xdb };
	
		memcpy(s_buf+0x8,tempChar,0xd);
		
		if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xd,
					USBLP_CTL_TIMEOUT));
	}
	
	return 1;
}

static int apple_audio_init(void)
{	
	__u8 bmRequestType;
	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_DEVICE;
	s_pStdReq->bmRequestType.Type		= USB_REQ_STD;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= USB_REQUEST_SET_CONFIGURATION;
	s_pStdReq->wValue					= 0x02;//ConfVal;
	s_pStdReq->wIndex					= 0;
	s_pStdReq->wLength					= 0;
	
	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	if ( 0 > usb_control_msg(devinfo->udev, 
		usb_sndctrlpipe(devinfo->udev, 0),
		s_pStdReq->bRequest,	
		bmRequestType,
		s_pStdReq->wValue,
		s_pStdReq->wIndex,
		&s_buf[8],
		0,
		USBLP_CTL_TIMEOUT));


	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
	s_pStdReq->bmRequestType.Type		= USB_REQ_STD;
	s_pStdReq->bmRequestType.DirIn		= TRUE;
	s_pStdReq->bRequest					= USB_REQUEST_GET_DESCRIPTOR;
	s_pStdReq->wValue					= 0x2200;
	s_pStdReq->wIndex					= 0x0002;
	s_pStdReq->wLength					= SwapCon16(0x60);

	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	
	if ( 0 > usb_control_msg(devinfo->udev, 
		usb_rcvctrlpipe(devinfo->udev, 0),
		s_pStdReq->bRequest,	
		bmRequestType,
		s_pStdReq->wValue,
		s_pStdReq->wIndex,
		&s_buf[8],
		0x60,
		USBLP_CTL_TIMEOUT));

	s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_DEVICE;
	s_pStdReq->bmRequestType.Type		= 2;
	s_pStdReq->bmRequestType.DirIn		= FALSE;
	s_pStdReq->bRequest					= 0x40;
	s_pStdReq->wValue					= 0x00;
	s_pStdReq->wIndex					= 0x01f4;
	s_pStdReq->wLength					= 0;

	bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
	if ( 0 > usb_control_msg(devinfo->udev, 
		usb_sndctrlpipe(devinfo->udev, 0),
		s_pStdReq->bRequest,	
		bmRequestType,
		s_pStdReq->wValue,
		s_pStdReq->wIndex,
		&s_buf[8],
		0,
		USBLP_CTL_TIMEOUT));


	if ( StartIDPS()==0 )
		goto Err;

	if ( RequestTransprotMaxPayLoadSize()==0 )
		goto Err;	
		
	if ( GetiPodOptionsForLingo()==0 )
		goto Err;
	 
	if ( SetFIDTokenValue()==0 )
		goto Err;
	if (  EndIDPS()==0 )
		goto Err;
	
	if(Authentication()==0)
		   goto Err;
	
	if(RetSampleRate()==0)
		goto Err;
	
	if(AccAuthenticationSignature()==0)
		goto Err;	

	 if(USBAudioPlay()==0)
		 goto Err;
//	 printk("end of %s \n", __func__);

		return TRUE;
Err:
 	return FALSE;

}

void DealMsgProc_USBAudio(int msg)
{
	__u8 bmRequestType;
	switch(msg)
	{
		case KEY_PLAY:
		case IRKEY_PLAY:
		{							
			unsigned char tempChar[0xd]={0x07,0x00,0x55,0x8,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //Command 0x04: AudioButtonStatus

			s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
			s_pStdReq->bmRequestType.Type		= 1;
			s_pStdReq->bmRequestType.DirIn		= FALSE;
			s_pStdReq->bRequest					= 0x09;
			s_pStdReq->wValue					= 0x0702;
			s_pStdReq->wIndex					= 0x200;
			s_pStdReq->wLength					= 0xd00;

			bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
			tempChar[7]=0x11;
			tempChar[8]=0x1;
			tempChar[9]=0x0;
			tempChar[0xc]=0xe4;
			memcpy(s_buf+0x8,tempChar,0xd);
			
			if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xd,
					USBLP_CTL_TIMEOUT));
			
			tempChar[7]=0x12;
			tempChar[8]=0x0;
			tempChar[9]=0x0;
			tempChar[0xc]=0xe4;
			memcpy(s_buf+0x8,tempChar,0xd);
			
			if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xd,
					USBLP_CTL_TIMEOUT));

		 }
			break;

		case KEY_NEXT:
		case IRKEY_NEXT:
	   	{

			unsigned char tempChar[0xd]={0x07,0x00,0x55,0x8,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //Command 0x04: AudioButtonStatus
			
			s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
			s_pStdReq->bmRequestType.Type		= 1;
			s_pStdReq->bmRequestType.DirIn		= FALSE;
			s_pStdReq->bRequest					= 0x09;
			s_pStdReq->wValue					= 0x0702;
			s_pStdReq->wIndex					= 0x200;
			s_pStdReq->wLength					= 0xd00;
			tempChar[7]=0x1d;
			tempChar[8]=0x8;
			tempChar[0xc]=0xd1;
			memcpy(s_buf+0x8,tempChar,0xd);
			
			bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
			if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xd,
					USBLP_CTL_TIMEOUT));
				
			msleep(200);
			tempChar[7]=0x1e;
			tempChar[8]=0x0;
			tempChar[0xc]=0xd8;
			memcpy(s_buf+0x8,tempChar,0xd);
			
			if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xd,
					USBLP_CTL_TIMEOUT));
			//Player_SendCmd(MPI_NEXT,0);
		}	
			break;

		case KEY_PREV:
		case IRKEY_PREV:
		{
			unsigned char tempChar[0xd]={0x07,0x00,0x55,0x8,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //Command 0x04: AudioButtonStatus
			
			s_pStdReq->bmRequestType.Recipient	= USB_RECIPIENT_INTERFACE;
			s_pStdReq->bmRequestType.Type		= 1;
			s_pStdReq->bmRequestType.DirIn		= FALSE;
			s_pStdReq->bRequest					= 0x09;
			s_pStdReq->wValue					= 0x0702;
			s_pStdReq->wIndex					= 0x200;
			s_pStdReq->wLength					= 0xd00;
			tempChar[7]=0x0d;
			tempChar[8]=0x10;
			tempChar[0xc]=0xd9;
			memcpy(s_buf+0x8,tempChar,0xd);
			
			bmRequestType = (s_pStdReq->bmRequestType.Recipient) | (s_pStdReq->bmRequestType.Type << 5)|(s_pStdReq->bmRequestType.DirIn<<7);
			if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xd,
					USBLP_CTL_TIMEOUT));
			
			msleep(200);
			tempChar[7]=0x0E;
			tempChar[8]=0x0;
			tempChar[0xc]=0xE8;
			memcpy(s_buf+0x8,tempChar,0xd);
			
			if ( USBSTATUS_OK != usb_control_msg(devinfo->udev, 
					usb_sndctrlpipe(devinfo->udev, 0),
					s_pStdReq->bRequest,	
					bmRequestType,
					s_pStdReq->wValue,
					s_pStdReq->wIndex,
					&s_buf[0x8],
					0xd,
					USBLP_CTL_TIMEOUT));
			}
			//Player_SendCmd(MPI_PREV,0);
			break;
		case KEY_VOLUP:
		case IRKEY_VOLUP:
			//Player_SendCmd(MPI_SETVOLUME,(UINT8)1);
			break;

		case KEY_VOLDN:
		case IRKEY_VOLDN:
			//Player_SendCmd(MPI_SETVOLUME,(UINT8)-1);
			break;

		case KEY_CH:
		//	ChangeDisk();
			break;

			break;
		default:
			break;
	}
}


static int usb_audio_open(struct inode *inode, struct file *filp)
{
	struct dspdev_filter *dspfilter;
	int i = 0;

	dspfilter = &usbaudio_dev_info.filter[i];
	dspfilter->wbuffer.pread = dspfilter->wbuffer.pwrite = 0;
	dspfilter->rbuffer.pread = dspfilter->rbuffer.pwrite = 0;
	dspfilter->wbuffer.error = dspfilter->rbuffer.error =0;
	filp->private_data = dspfilter;
	
	usbaudio_dev_info.users++;
	return 0;
}

void usbaudio_ringbuffer_flush(struct usbaudio_ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite;
	rbuf->error = 0;
}

ssize_t usbaudio_ringbuffer_avail(struct usbaudio_ringbuffer *rbuf)
{
	ssize_t avail;

	avail = rbuf->pwrite - rbuf->pread;
	if (avail < 0)
		avail += rbuf->size;
	return avail;
}

ssize_t usbaudio_ringbuffer_free(struct usbaudio_ringbuffer *rbuf)
{
	ssize_t free;

	free = rbuf->pread - rbuf->pwrite;
	if (free <= 0)
		free += rbuf->size;
	return free;
}

ssize_t usbaudio_ringbuffer_read_user(struct usbaudio_ringbuffer *rbuf, u8 __user *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pread+ len > rbuf->size) ? rbuf->size - rbuf->pread : 0;
	if (split > 0) {
		if (copy_to_user(buf, rbuf->data+rbuf->pread, split))
			return -EFAULT;
		buf += split;
		todo -= split;
		rbuf->pread = 0;
	}
	if (copy_to_user(buf, rbuf->data+rbuf->pread, todo))
		return -EFAULT;

	rbuf->pread = (rbuf->pread + todo) % rbuf->size;

	return len;
}

int usbaudio_ringbuffer_empty(struct usbaudio_ringbuffer *rbuf)
{
	return (rbuf->pread==rbuf->pwrite);
}

static ssize_t usbaudio_buffer_read(struct usbaudio_ringbuffer *src,int non_blocking, char __user *buf,
				      size_t count, loff_t *ppos)
{
	ssize_t avail;
	ssize_t ret = 0;
	long timeout = 0;
	if (!src->data)
		return 0;

	if (src->error) {
		ret = src->error;
		usbaudio_ringbuffer_flush(src);
		return ret;
	}

	timeout = msecs_to_jiffies(MAX_WAIT_TIME);
		if (non_blocking && usbaudio_ringbuffer_empty(src)) {
			ret = -EWOULDBLOCK;
			return ret;
		}
		ret = wait_event_interruptible_timeout(src->queue,
					       !usbaudio_ringbuffer_empty(src) ||
					       (src->error != 0),timeout);
		if (ret < 0)
		{
			printk("wait error\n");
			return ret;
		}
		if (src->error) {
			usbaudio_ringbuffer_flush(src);
			return 0;
		}

		avail = usbaudio_ringbuffer_avail(src);
	//	if((avail > src->size - 192*8) ||(avail < 4095))
	//		printk("avail is %x\n", avail);
		if (avail > count)
			avail = count;

		ret = usbaudio_ringbuffer_read_user(src, buf, avail);
		if (ret < 0)
			return ret;

		buf += ret;
	return ret;
}

static ssize_t usb_audio_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct dspdev_filter *dspfilter = file->private_data;
	int ret;
	dspfilter = &usbaudio_dev_info.filter[0];
	if (mutex_lock_interruptible(&dspfilter->wmutex))
		return -ERESTARTSYS;
	ret = usbaudio_buffer_read(&dspfilter->wbuffer,file->f_flags & O_NONBLOCK,buf, count, ppos);
	mutex_unlock(&dspfilter->wmutex);
	return ret;
}

static long usb_audio_ioctl( struct file *filp, u_int cmd, u_long argp)
{
	int ret = 0; 
	switch(cmd)
	{
		case 1:
			if(copy_to_user((unsigned char *)argp, &usbaudio_dev_info.filter[0].wbuffer,192*8000))
				return -EFAULT;
			break;
		default:
            break;
	}

	return ret;
}

struct file_operations usb_audio_fops = {
	.owner = THIS_MODULE,
	.open = usb_audio_open,
	.read = usb_audio_read,
	.unlocked_ioctl = usb_audio_ioctl,
};

static struct miscdevice misc = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = USB_AUDIO_DEV_NAME,
    .fops    = &usb_audio_fops,
};

void usbaudio_ringbuffer_init(struct usbaudio_ringbuffer *rbuf, void *data, size_t len)
{
	rbuf->pread=rbuf->pwrite=0;
	rbuf->data=data;
	rbuf->size=len;
	rbuf->error=0;

	init_waitqueue_head(&rbuf->queue);

	spin_lock_init(&(rbuf->lock));
}

static int silan_usbaudio_ringbuffer_init(void)
{
	int i,size;
	dma_addr_t ring_buffer_dma;
	u_char* ring_buffer,*rbuffer,*wbuffer;
	
	usbaudio_dev_info.users = 0;
	usbaudio_dev_info.filternum = 1;
	usbaudio_dev_info.filter = kmalloc(usbaudio_dev_info.filternum*sizeof(struct dspdev_filter), GFP_KERNEL);
	if (!usbaudio_dev_info.filter)
		return -ENOMEM;

	size = (MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*usbaudio_dev_info.filternum;
	ring_buffer_dma = prom_phy_mem_malloc(size, SILAN_DEV_USBHOST);
	if(!ring_buffer_dma)
		return -ENOMEM;

	ring_buffer = ioremap_nocache(ring_buffer_dma,size);
	
	for(i = 0; i < usbaudio_dev_info.filternum; i++)
	{
	//	usbaudio_dev_info.filter[i].state = 0;//DSPDEV_STATE_FREE;
		usbaudio_dev_info.filter[i].pdspdev_info = &usbaudio_dev_info;
		usbaudio_dev_info.filter[i].bufstart = ring_buffer;
		usbaudio_dev_info.filter[i].sysex = ring_buffer;

		mutex_init(&usbaudio_dev_info.filter[i].rmutex);
		mutex_init(&usbaudio_dev_info.filter[i].wmutex);

		rbuffer = ring_buffer+(MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*i+MIPS_DSP_SYSCHANGE_SIZE;
		wbuffer = ring_buffer+(MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*i+MIPS_DSP_SYSCHANGE_SIZE+DSP_R_RINGBUFFER_SIZE;
		
		usbaudio_ringbuffer_init(&usbaudio_dev_info.filter[i].rbuffer, rbuffer,DSP_R_RINGBUFFER_SIZE);
		usbaudio_ringbuffer_init(&usbaudio_dev_info.filter[i].wbuffer, wbuffer,DSP_W_RINGBUFFER_SIZE);
	
		init_timer(&usbaudio_dev_info.filter[i].timer);
	
		usbaudio_dev_info.filter[i].rdma_addr = ring_buffer_dma+(DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*i;
		usbaudio_dev_info.filter[i].wdma_addr = ring_buffer_dma+(DSP_R_RINGBUFFER_SIZE+DSP_W_RINGBUFFER_SIZE)*i+DSP_R_RINGBUFFER_SIZE;
	}

	return 0;
}


/*
 * Free transfer buffers.
 */
static void uvc_free_urb_buffers(struct uvc_streaming *stream)
{
	unsigned int i;

	for (i = 0; i < UVC_URBS; ++i) {
		if (stream->urb_buffer[i]) {
			usb_free_coherent(devinfo->udev, stream->urb_size,
				stream->urb_buffer[i], stream->urb_dma[i]);
			stream->urb_buffer[i] = NULL;
		}
	}

	stream->urb_size = 0;
}

/*
 * Uninitialize isochronous/bulk URBs and free transfer buffers.
 */
static void uvc_uninit_video(struct uvc_streaming *stream, int free_buffers)
{
	struct urb *urb;
	unsigned int i;

	for (i = 0; i < UVC_URBS; ++i) {
		urb = stream->urb[i];
		if (urb == NULL)
			continue;

		usb_kill_urb(urb);
		usb_free_urb(urb);
		stream->urb[i] = NULL;
	}

	if (free_buffers)
		uvc_free_urb_buffers(stream);
}


/*
 * Allocate transfer buffers. This function can be called with buffers
 * already allocated when resuming from suspend, in which case it will
 * return without touching the buffers.
 *
 * Limit the buffer size to UVC_MAX_PACKETS bulk/isochronous packets. If the
 * system is too low on memory try successively smaller numbers of packets
 * until allocation succeeds.
 *
 * Return the number of allocated packets on success or 0 when out of memory.
 */
static int uvc_alloc_urb_buffers(struct uvc_streaming *stream,
	unsigned int size, unsigned int psize, gfp_t gfp_flags)
{
	unsigned int npackets;
	unsigned int i;

	/* Buffers are already allocated, bail out. */
	if (stream->urb_size)
		return stream->urb_size / psize;

	/* Compute the number of packets. Bulk endpoints might transfer UVC
	 * payloads across multiple URBs.
	 */
	npackets = DIV_ROUND_UP(size, psize);
	if (npackets > UVC_MAX_PACKETS)
		npackets = UVC_MAX_PACKETS;

	/* Retry allocations until one succeed. */
	for (; npackets > 1; npackets /= 2) {
		for (i = 0; i < UVC_URBS; ++i) {
			stream->urb_size = psize * npackets;
			stream->urb_buffer[i] = usb_alloc_coherent(
				devinfo->udev, stream->urb_size,
				gfp_flags | __GFP_NOWARN, &stream->urb_dma[i]);
			if (!stream->urb_buffer[i]) {
				uvc_free_urb_buffers(stream);
				break;
			}
		}
		if (i == UVC_URBS) {
			printk("Allocated %u URB buffers "
				"of %ux%u bytes each.\n", UVC_URBS, npackets,
				psize);
			return npackets;
		}
	}

	printk("Failed to allocate URB buffers (%u bytes "
		"per packet).\n", psize);
	return 0;
}


static void uvc_video_complete(struct urb *urb)
{
	devinfo->urb = urb;
	schedule_delayed_work(&devinfo->work, 0);
}

static void delay_work_func(struct work_struct *work)
{
//	struct usb_dev_info *devinfo_local = container_of(work, struct usb_dev_info, work.work); 
	struct urb *urb = devinfo->urb;
//	struct uvc_streaming *stream = urb->context;
	int len,free;
	int ret,i=0;
	u8 *mem;
	struct usbaudio_ringbuffer *rbuf;
	rbuf = &usbaudio_dev_info.filter[0].wbuffer;

	len = 5*192*8;
	free = usbaudio_ringbuffer_free(rbuf);

	 if(free < len)
	{
	//	printk(" ### buffer overflow,free: %d len:%d\n",free,len);
		do{
			free = usbaudio_ringbuffer_free(rbuf);
			msleep(1);
	//		printk("free is %d\n", free);
		}while(free < len);
	}

	for (i = 0;i < urb->number_of_packets; ++i) {	
		len =  urb->iso_frame_desc[i].actual_length;
		mem = urb->transfer_buffer + urb->iso_frame_desc[i].offset;
	
		memcpy(rbuf->data + rbuf->pwrite, mem, len);
		rbuf->pwrite = (rbuf->pwrite + len) % rbuf->size;
	}	
		
	if((ret = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
		printk("Failed to resubmit video URB(%d). \n", ret);
	}
}


/*
 * Initialize isochronous URBs and allocate transfer buffers. The packet size
 * is given by the endpoint.
 */
static int uvc_init_audio_isoc(struct uvc_streaming *stream)
{
	struct urb *urb;
	unsigned int npackets, i, j;
	u16 psize;
	u32 size;

	psize = 192;
	size = 192*8;

	npackets = uvc_alloc_urb_buffers(stream, size, psize, GFP_KERNEL);
	if (npackets == 0)
		return -ENOMEM;
	
	size = npackets * psize;

	for (i = 0; i < UVC_URBS; ++i) {
		urb = usb_alloc_urb(npackets, GFP_KERNEL);
		if (urb == NULL) {
			uvc_uninit_video(stream, 1);
			return -ENOMEM;
		}

		urb->dev = devinfo->udev;
		urb->context = stream;
		urb->pipe = usb_rcvisocpipe(devinfo->udev,
				1);
		urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
		urb->interval = 4;
		urb->transfer_buffer = stream->urb_buffer[i];
		urb->transfer_dma = stream->urb_dma[i];
		urb->complete = uvc_video_complete;
		urb->number_of_packets = npackets;
		urb->transfer_buffer_length = size;

		for (j = 0; j < npackets; ++j) {
			urb->iso_frame_desc[j].offset = j * psize;
			urb->iso_frame_desc[j].length = psize;
		}

		stream->urb[i] = urb;
	}

	return 0;
}

static int usbaudio_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct uvc_streaming *stream;

	int err = 0,i=0 ;
	int ret=0;
	struct usb_device *udev = interface_to_usbdev(intf);
	stream = kmalloc(sizeof(struct uvc_streaming), GFP_KERNEL);
	devinfo = kmalloc(sizeof(struct usb_dev_info), GFP_KERNEL);
	if (!devinfo)
		return -ENOMEM;

	s_pStdReq = kmalloc(sizeof(struct _device_request), GFP_KERNEL); 
	devinfo->intf = intf;
	devinfo->udev = udev;

	devinfo->cmd_pipe = usb_sndctrlpipe(udev , 0);
	devinfo->status_pipe = usb_rcvctrlpipe(udev, 0);
	devinfo->bulkin_pipe = usb_rcvbulkpipe(udev, 2);
	devinfo->bulkout_pipe = usb_sndbulkpipe(udev,2);
	devinfo->iso_pipe = usb_rcvisocpipe(udev, 1);
	devinfo->intin_pipe = usb_rcvintpipe(udev, 3);
	devinfo->client = usb_i2c;

	INIT_DELAYED_WORK(&devinfo->work, delay_work_func);
	apple_audio_init();
	
	ret = silan_usbaudio_ringbuffer_init();
	if(ret)
	goto out;
	
	uvc_init_audio_isoc(stream);

	/* Submit the URBs. */
	for (i = 0; i < UVC_URBS; ++i) {
		ret = usb_submit_urb(stream->urb[i], GFP_KERNEL);
		if (ret < 0) {
			printk( "Failed to submit URB %u "
					"(%d).\n", i, ret);
			uvc_uninit_video(stream, 1);
			return ret;
		}
	}

	if (misc_register(&misc) < 0) {
		err = -EBUSY;
	}
	
	return 0;
out:
	return ret;
}

static int silan_usb_i2c_probe(struct i2c_client *client,
                const struct i2c_device_id *id)
{
	usb_i2c = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!usb_i2c)
		return -ENOMEM;

	usb_i2c = client;	
    return 0;
}

static const struct i2c_device_id silan_usb_i2c_id[] =
{
    { "silan_usb_i2c", 0 },
};

MODULE_DEVICE_TABLE(i2c, silan_usb_i2c_id);

static struct i2c_driver silan_usb_i2c_driver = 
{
    .driver = {
        .name    = "silan_usb_i2c",
        .owner    = THIS_MODULE,
    },
    .probe        = silan_usb_i2c_probe,
    .id_table    = silan_usb_i2c_id,
};

static void usbaudio_disconnect(struct usb_interface *intf)
{
	kfree(devinfo);
	kfree(s_pStdReq);
}

static struct usb_device_id usbaudio_ids[] = {
	{
		.match_flags = USB_DEVICE_ID_MATCH_DEVICE,
		.idVendor = 1452,
		.idProduct = 4768, 
	},
};

static struct usb_driver usbaudio_driver = {
	.name =		"apple-usbaudio",
	.probe =	usbaudio_probe,
	.disconnect = usbaudio_disconnect,
	.id_table = usbaudio_ids,
};

static int __init usbaudio_init(void)
{
	usb_register(&usbaudio_driver);
	return i2c_add_driver(&silan_usb_i2c_driver);
}

static void __exit usbaudio_exit(void)
{
	usb_deregister(&usbaudio_driver);
}

module_init(usbaudio_init);
module_exit(usbaudio_exit);

