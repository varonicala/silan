/**
 * COPYRIGHT (C) 2006 SIGINE, ALL RIGHTS RESERVED
 * $Abstract:	USB 1.1/2.0 Chapter 9 definitions
 * $Source: /SWCvs/SWProject/Muse/Platform/Src/Include/USB/Chap9.h,v $
 * $Author: silan\sunhongjun $
 * $Date: 2008/08/24 08:04:24 $
 * $Revision: 1.2 $ 
 * 
 * $Log: Chap9.h,v $
 * Revision 1.2  2008/08/24 08:04:24  silan\sunhongjun
 * 添加OS String Descriptor.
 *
 * Revision 1.1  2007/01/16 02:47:59  silan\qinjianxun
 * 文件迁移！位置有变化，从原来的Include --> Include\USB
 *
 */


#ifndef __CHAP9_H__
#define __CHAP9_H__


#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif

#ifndef	true
#define	true	1
#endif
#ifndef	false
#define	false	0
#endif

#undef  NULL
#define	NULL	0


#define	USBSTATUS_OK			0x01
#define	USBSTATUS_REMOVAL		0x02
#define	USBSTATUS_STALL			0x04
#define	USBSTATUS_RETRY_LIMIT	0x08
#define	USBSTATUS_TIMEOUT		0x10
#define	USBSTATUS_SHORTPACKET	0x80


//*************************************************************************
//						basic #defines
//*************************************************************************

// USB defined request codes
// see chapter 9 of the USB 1.1 specifcation for
// more information.

// These are the correct values based on the USB 1.1
// specification
#define	USB_MAX_STD_REQUEST					12

#define USB_REQUEST_GET_STATUS				0x00
#define USB_REQUEST_CLEAR_FEATURE			0x01

#define USB_REQUEST_SET_FEATURE				0x03

#define USB_REQUEST_SET_ADDRESS				0x05
#define USB_REQUEST_GET_DESCRIPTOR			0x06
#define USB_REQUEST_SET_DESCRIPTOR			0x07
#define USB_REQUEST_GET_CONFIGURATION		0x08
#define USB_REQUEST_SET_CONFIGURATION		0x09
#define USB_REQUEST_GET_INTERFACE			0x0A
#define USB_REQUEST_SET_INTERFACE			0x0B
#define USB_REQUEST_SYNC_FRAME				0x0C

// defined USB device classes
#define USB_DEVICE_CLASS_RESERVED			0x00
#define USB_DEVICE_CLASS_AUDIO				0x01
#define USB_DEVICE_CLASS_COMMUNICATIONS		0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE	0x03
#define USB_DEVICE_CLASS_MONITOR			0x04
#define USB_DEVICE_CLASS_PHYSICAL_INTERFACE 0x05
#define USB_DEVICE_CLASS_POWER				0x06
#define USB_DEVICE_CLASS_PRINTER			0x07
#define USB_DEVICE_CLASS_STORAGE			0x08
#define USB_DEVICE_CLASS_HUB				0x09
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC	0xFF


#define	USB_MAX_CLASS_MS_REQUEST			2

#define USB_CLASS_CODE_MASSSTORAGE_DEVICE	0x08

#define USB_SUBCLASS_CODE_RBC				0x01
#define USB_SUBCLASS_CODE_SFF8020I			0x02
#define USB_SUBCLASS_CODE_QIC157			0x03
#define USB_SUBCLASS_CODE_UFI				0x04
#define USB_SUBCLASS_CODE_SFF8070I			0x05
#define USB_SUBCLASS_CODE_SCSI				0x06

#define USB_PROTOCOL_CODE_CBI0				0x00
#define USB_PROTOCOL_CODE_CBI1				0x01
#define USB_PROTOCOL_CODE_BULK				0x50


// USB defined Feature selectors
#define USB_FEATURE_ENDPOINT_STALL			0x0000
#define USB_FEATURE_REMOTE_WAKEUP			0x0001
#define USB_FEATURE_POWER_D0				0x0002
#define USB_FEATURE_POWER_D1				0x0003
#define USB_FEATURE_POWER_D2				0x0004
#define USB_FEATURE_POWER_D3				0x0005


// values for the bits returned by the USB GET_STATUS command
#define USB_GETSTATUS_SELF_POWERED			0x01
#define USB_GETSTATUS_REMOTE_WAKEUP_ENABLED	0x02

#define USB_DEVICE_DESCRIPTOR_TYPE			0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE	0x02
#define USB_STRING_DESCRIPTOR_TYPE			0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE		0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE		0x05
#define USB_DEVQUALIFIER_DESCRIPTOR_TYPE	0x06

#define USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(d, i)	((unsigned short)((unsigned short)d<<8 | i))

// definitions for bits in the bmAttributes field of a 
// configuration descriptor.
#define USB_CONFIG_POWERED_MASK				0xc0

#define USB_CONFIG_BUS_POWERED				0x80
#define USB_CONFIG_SELF_POWERED				0x40
#define USB_CONFIG_REMOTE_WAKEUP			0x20


// Values for bmAttributes field of an
// endpoint descriptor
#define USB_ENDPOINT_TYPE_MASK				0x03

#define USB_ENDPOINT_TYPE_CONTROL			0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS		0x01
#define USB_ENDPOINT_TYPE_BULK				0x02
#define USB_ENDPOINT_TYPE_INTERRUPT			0x03

// Endpoint direction bit, stored in address
#define USB_ENDPOINT_DIRECTION_MASK			0x80
#define	USB_ENDPOINT_DIRECTION_IN			0x80
#define	USB_ENDPOINT_DIRECTION_OUT			0x00


#define MAXIMUM_USB_STRING_LENGTH			255
// String indices constant to a descriptor
#define STR_INDEX_LANGUAGE					0x00	
#define STR_INDEX_MANUFACTURER				0x01	
#define STR_INDEX_PRODUCT					0x02	
#define STR_INDEX_SERIALNUMBER				0x03
#define STR_INDEX_CONFIGURATION				0x04	
#define STR_INDEX_INTERFACE					0x05
#define STR_INDEX_OSDESCRIPTOR				0xEE


//*************************************************************************
//							Masks
//*************************************************************************
#define USB_RECIPIENT						(unsigned char)0x1F
#define USB_RECIPIENT_DEVICE				(unsigned char)0x00
#define USB_RECIPIENT_INTERFACE				(unsigned char)0x01
#define USB_RECIPIENT_ENDPOINT				(unsigned char)0x02

#define USB_REQUEST_TYPE_MASK				(unsigned char)0x60
#define USB_STANDARD_REQUEST				(unsigned char)0x00
#define USB_CLASS_REQUEST					(unsigned char)0x20
#define USB_VENDOR_REQUEST					(unsigned char)0x40
#define USB_REQ_STD							(unsigned char)0x00
#define USB_REQ_CLASS						(unsigned char)0x01
#define USB_REQ_VENDOR						(unsigned char)0x02

//#define USB_REQUEST_MASK					(unsigned char)0xFF

#define USB_DEVICE_ADDRESS_MASK				0x7F

/* GetStatus */
#define USB_DEVSTS_SELFPOWERED				0x01
#define USB_DEVSTS_REMOTEWAKEUP				0x02
#define USB_ENDPSTS_HALT					0x01


//*************************************************************************
//						USB Protocol Layer
//*************************************************************************

typedef struct _device_request
{
	
	struct {
		unsigned char	Recipient:	5;
		unsigned char	Type:		2;
		unsigned char	DirIn:		1;
	}bmRequestType;
	unsigned char	bRequest;
	unsigned short	wValue;
	unsigned short	wIndex;
	unsigned short	wLength;
} DEVICE_REQUEST;
typedef struct _USB_DEVICE_DESCRIPTOR {
    unsigned char	bLength;
    unsigned char	bDescriptorType;
    unsigned short	bcdUSB;
    unsigned char	bDeviceClass;
    unsigned char	bDeviceSubClass;
    unsigned char	bDeviceProtocol;
    unsigned char	bMaxPacketSize0;
    unsigned short	idVendor;
    unsigned short	idProduct;
    unsigned short	bcdDevice;
    unsigned char	iManufacturer;
    unsigned char	iProduct;
    unsigned char	iSerialNumber;
    unsigned char	bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;

typedef struct _USB_ENDPOINT_DESCRIPTOR {
    unsigned char	bLength;
    unsigned char	bDescriptorType;
    unsigned char	bEndpointAddress;
    unsigned char	bmAttributes;
    unsigned short	wMaxPacketSize;
    unsigned char	bInterval;
} USB_ENDPOINT_DESCRIPTOR, * PUSB_ENDPOINT_DESCRIPTOR;

//
// values for bmAttributes Field in
// USB_CONFIGURATION_DESCRIPTOR
//

#define BUS_POWERED							0x80
#define SELF_POWERED						0x40
#define REMOTE_WAKEUP						0x20

typedef struct _USB_CONFIGURATION_DESCRIPTOR {
    unsigned char	bLength;
    unsigned char	bDescriptorType;
    unsigned short	wTotalLength;
    unsigned char	bNumInterfaces;
    unsigned char	bConfigurationValue;
    unsigned char	iConfiguration;
    unsigned char	bmAttributes;
    unsigned char	MaxPower;
} USB_CONFIGURATION_DESCRIPTOR, * PUSB_CONFIGURATION_DESCRIPTOR;

typedef struct _USB_INTERFACE_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bInterfaceNumber;
    unsigned char bAlternateSetting;
    unsigned char bNumEndpoints;
    unsigned char bInterfaceClass;
    unsigned char bInterfaceSubClass;
    unsigned char bInterfaceProtocol;
    unsigned char iInterface;
} USB_INTERFACE_DESCRIPTOR, * PUSB_INTERFACE_DESCRIPTOR;

typedef struct _USB_DEVQUALIFIER_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClsass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize0;
    unsigned char bNumConfigurations;
    unsigned char bReserved;
} USB_DEVQUALIFIER_DESCRIPTOR, * PUSB_DEVQUALIFIER_DESCRIPTOR;


typedef struct _USB_STRING_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bString[1];
} USB_STRING_DESCRIPTOR, * PUSB_STRING_DESCRIPTOR;

typedef struct _USB_STRING_LANGUAGE_DESCRIPTOR {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned short ulanguageID;
} USB_STRING_LANGUAGE_DESCRIPTOR, * PUSB_STRING_LANGUAGE_DESCRIPTOR;

typedef struct _USB_STRING_INTERFACE_DESCRIPTOR {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  Interface[22];
} USB_STRING_INTERFACE_DESCRIPTOR, * PUSB_STRING_INTERFACE_DESCRIPTOR;

typedef struct _USB_STRING_CONFIGURATION_DESCRIPTOR {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  Configuration[16];
} USB_STRING_CONFIGURATION_DESCRIPTOR, * PUSB_STRING_CONFIGURATION_DESCRIPTOR;

typedef struct _USB_STRING_SERIALNUMBER_DESCRIPTOR {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  SerialNum[24];
} USB_STRING_SERIALNUMBER_DESCRIPTOR, * PUSB_STRING_SERIALNUMBER_DESCRIPTOR;

typedef struct _USB_STRING_PRODUCT_DESCRIPTOR {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  Product[30];
} USB_STRING_PRODUCT_DESCRIPTOR, * PUSB_STRING_PRODUCT_DESCRIPTOR;

typedef struct _USB_STRING_MANUFACTURER_DESCRIPTOR {
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  Manufacturer[24];
} USB_STRING_MANUFACTURER_DESCRIPTOR, * PUSB_STRING_MANUFACTURER_DESCRIPTOR;


// USB power descriptor added to core specification

#define USB_SUPPORT_D0_COMMAND      0x01
#define USB_SUPPORT_D1_COMMAND      0x02
#define USB_SUPPORT_D2_COMMAND      0x04
#define USB_SUPPORT_D3_COMMAND      0x08

#define USB_SUPPORT_D1_WAKEUP       0x10
#define USB_SUPPORT_D2_WAKEUP       0x20


typedef struct _USB_POWER_DESCRIPTOR {
    unsigned char	bLength;
    unsigned char	bDescriptorType;
    unsigned char	bCapabilitiesFlags;
    unsigned short	EventNotification;
    unsigned short	D1LatencyTime;
    unsigned short	D2LatencyTime;
    unsigned short	D3LatencyTime;
    unsigned char	PowerUnit;
    unsigned short	D0PowerConsumption;
    unsigned short	D1PowerConsumption;
    unsigned short	D2PowerConsumption;
} USB_POWER_DESCRIPTOR, * PUSB_POWER_DESCRIPTOR;


typedef struct _USB_COMMON_DESCRIPTOR {
    unsigned char bLength;
    unsigned char bDescriptorType;
} USB_COMMON_DESCRIPTOR, * PUSB_COMMON_DESCRIPTOR;


//
// Standard USB HUB definitions 
// See Chapter 11
typedef struct _USB_HUB_DESCRIPTOR {
    unsigned char		bDescriptorLength;		// Length of this descriptor
    unsigned char		bDescriptorType;		// Hub configuration type
    unsigned char		bNumberOfPorts;			// number of ports on this hub
    unsigned short		wHubCharacteristics;	// Hub Charateristics
    unsigned char		bPowerOnToPowerGood;	// port power on till power good in 2ms
    unsigned char		bHubControlCurrent;		// max current in mA
    // room for 255 ports power control and removable bitmask
    unsigned char        bRemoveAndPowerMask[64];
} USB_HUB_DESCRIPTOR, * PUSB_HUB_DESCRIPTOR;


typedef struct _OS_STRING_DESCRIPTOR {
	unsigned char		bDescriptorLength;
	unsigned char		bDescriptorType;
	unsigned char		qwSignature[14];
	unsigned char		bMS_VendorCode;
	unsigned char		bPad;
} OS_STRING_DESCRIPTOR, *POS_STRING_DESCRIPTOR;

#ifdef __cplusplus
extern "C" {
#endif

void	Chap9_Handler(void);	/**< Used by USB device model */


#ifdef __cplusplus
}
#endif


#endif //__CHAP9_H__
