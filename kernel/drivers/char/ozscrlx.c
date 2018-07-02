#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/firmware.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/mach-silan/suv2/silan_generic.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/time.h>
#include <linux/kgdb.h>
#include <silan_resources.h>
#include <silan_irq.h>
#include <silan_def.h>
#include <silan_gpio.h>
#include <silan_reset.h>
#include <silan_padmux.h>
#include <linux/gpio.h>

#include "ozscrlx.h"

//#define PCMCIA_DEBUG 

#include "sci.h"
#define SCI_TIMEOUT 5*HZ
#define SCI_PHY_BASE_ADDR 0x1fbca000

#define SMART_CARD_IRQ    4*32+24
#define GPIO_BASE          96

struct platform_device *p_dev;

struct clk *s_sci_clk;

unsigned long sci_clk;   //100M .etc  from LSP clk 

struct delayed_work sci_work;

#define ATR_SIZE        0x40    /* TS + 32 + SW + PROLOGUE + EPILOGUE... */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#define device                          class_device
#define device_create(a, b, c, d, e...) class_device_create(a, b, c, NULL, ##e)
#define device_destroy(a, b)            class_device_destroy(a, b)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define device_create(a, b, c, d, e...) device_create_drvdata(a, b, c, d, ##e)
#endif

#undef MODULE_NAME
#define MODULE_NAME    "SILAN_SCI "

/* Never heard of more than one on the same box, so 4 should be more than enough */
#define MAX_O2SCR_DEV 4

u16 Fi[] = { 372, 372, 558, 744, 1116, 1488, 1860, 0xFFFF, 0xFFFF,
             512, 768, 1024, 1536, 2048, 0xFFFF, 0xFFFF
};

u16 Di[] = { 0xFFFF, 1, 2, 4, 8, 16, 32, 0xFFFF, 12, 20, 0xFFFF,
             0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
};

/* Correct Table */
static u16 crctab[256] = {
0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};


static unsigned int CmdResetReader(READER_EXTENSION *pRdrExt,
                   BOOLEAN WarmReset, u8 *pATR, unsigned long *pATRLength);

static unsigned int CmdDeactivate(READER_EXTENSION *pRdrExt);
static unsigned int CBTransmit(READER_EXTENSION *pRdrExt);

//static char version[] = "silan SmartCardBus Reader (for kernel >= 2.6.17)";
/*
 * The probe() and detach() entry points are used to create and destroy
 * "instances" of the driver, where each instance represents everything
 * needed to manage one actual PCMCIA card.
 */

static struct platform_device *p_dev_list[MAX_O2SCR_DEV] = {NULL, NULL, NULL, NULL};

/* card-reader class */
static struct class *o2scr_class;
static int o2scr_major;

/*
 * Private data for OZSCR reader. Need to provide a dev_node_t structure for the device.
 * FIX: Possibly needs to be extended to support PRG encryption and/or
 *    downloading of additional drivers to the OZSCR.
 */
typedef struct ozscr_dev_t {
    int minor;        /* minor id of associated char dev  */
    unsigned int  sci_base_addr;    /* Base of mapped attribute memory  */
    READER_EXTENSION *o2scr_reader;    /* the internals of the card reader */
} ozscr_drv_t;

#ifdef PCMCIA_DEBUG
// XXX should we use dev_dbg()?
#define dprintk(x, args...) do {                    \
        printk(KERN_DEBUG MODULE_NAME "%s: " x, __FUNCTION__, ##args);    \
    } while (0)
#else
#define dprintk(x, args...) do { } while (0)
#endif

static int initial_sci(READER_EXTENSION *o2scr_reader)
{
    READER_EXTENSION *pRdrExt;
    int tmp = 0;
    
    pRdrExt = o2scr_reader;
    //The control register0 is set to derect convention, even parity
    sl_w16(SCICR0, 0x0);
    //receive mode, timeouts initially disabled
    sl_w16(SCICR1,0x0b);
    //enable all interrupts
    sl_w16(SCIIMSC, 0xffff);

    sl_w16(SCISTABLE,0x96);

    sl_w16(SCIATIME,0xafc8);
 
    sl_w16(SCIDTIME,0x12c0);

    sl_w16(SCIATRSTIME,0x9c40);

    sl_w16(SCIATRDTIME,0x4b00);

    sl_w16(SCICHTIMEMS,0x0);
    
    sl_w16(SCICHTIMELS,0x2580);

    sl_w16(SCITIDE,0x40);

    tmp = (sci_clk/2000000-1); //sci_clk: HZ
    
    sl_w16(SCICLKICC, 24);
    
    sl_w16(SCIBAUD,0x173);

    tmp = sci_clk/1000000;

    sl_w16(SCIVALUE,50);

    sl_w16(SCIBLKTIMEMS, 0x0);
    sl_w16(SCIBLKTIMELS, 0x2000);

    //SCIRETRY
    sl_w16(SCIRETRY,0x33);

    if(sl_r16(SCIRIS) & 0x1)
    {
        printk("a card detected\n");
    }
    else
    {
        printk("please insert a card\n");
    }

    return 0;
}

/*Read All DATA in FIFO_OUT*/
static unsigned int ReadFIFO(READER_EXTENSION *pRdrExt, u8 *pATR, unsigned long *pATRLength)
{
    unsigned long fifoLength;
    int rxfifo_data;
    u16 tmp; 
    //receive mode
    tmp = sl_r16(SCICR1);
    tmp &= 0xfffb;
    sl_w16(SCICR1, tmp);

    fifoLength = 0;
    dprintk("start to receive response data from smart card!\n");        
#if 1
    while((sl_r16(SCIRIS)>>8 & 0x1) != 0x1 && (sl_r16(SCIRIS)>>7 & 0x1) != 0x1)    
    {    
        //whether we put this check out of while up
        if((sl_r16(SCIRIS)>>4 & 0x1) == 1)
        {
            printk("transmit error occur!\n");
            sl_w16(SCIICR, 0x1<<4);
            sl_w16(SCITXCOUNT, 0x1);
            *pATRLength = 0;
            return STATUS_UNSUCCESSFUL;
        }
        
        if(sl_r16(SCIRXCOUNT) != 0)
        {
            rxfifo_data = sl_r16(SCIDATA);
            dprintk("%x\n", rxfifo_data);
            //check receive data parity error ,bit 9 is the check bit
            if(rxfifo_data>>8 & 0x1)
            {
                printk("receive parity error happen\n");
                *pATRLength = 0;
                return STATUS_UNSUCCESSFUL;
            }
            *(pATR + fifoLength) = (unsigned char)rxfifo_data;
            fifoLength++;

        }
    }
#endif

    *pATRLength = fifoLength;
    dprintk("*pATRLength is %lu in ReadFIFO\n",*pATRLength);
    if((sl_r16(SCIRIS)>>8 & 0x1) == 0x1)
    {   
        dprintk("response data receive over!\n");
        sl_w16(SCIICR,0x1 << 8);
    }

    if((sl_r16(SCIRIS)>>7 & 0x1) == 0x1)
    {
        printk("block timeout!\n");
        sl_w16(SCIICR, 0x1<<7);
        return STATUS_UNSUCCESSFUL;
    }

#ifdef PCMCIA_DEBUG
    u16 i = 0;
    printk("\n\n FIFO read =");
    for (i = 0; i < *pATRLength; i++)
        printk(" [%02X]", pATR[i]);
    printk("\n\n\n");
    msleep(100);
#endif
    return STATUS_SUCCESS;
}

/*open channel on OZSCR*/
static int ozscr_open(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);
    struct platform_device *p_dev = p_dev_list[minor];

    //to check the card is detacted 
    if (p_dev == NULL )
        return -ENODEV;

    return 0;        
}

/*close channel on OZSCR*/
static int ozscr_close(struct inode *inode, struct file *file)
{   
    int minor = iminor(inode);
    struct platform_device *p_dev = p_dev_list[minor];

    if (p_dev == NULL )
        return -ENODEV;

    return 0;
}

static long ozscr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ozscr_dev_t *dev;
    READER_EXTENSION *pRdrExt;
    int ret = 0;
    unsigned int size = _IOC_SIZE(cmd);    /* size for data transfers  */
    unsigned long ATRLength;
    struct ozscr_apdu apdu;
    u8 ATRBuffer[ATR_SIZE];    /* TLVList[16]; */

#if 1
    if ((cmd & IOC_IN) && (!access_ok(VERIFY_READ, (char *)arg, size)))
            return -EFAULT;
    if ((cmd & IOC_OUT) && (!access_ok(VERIFY_WRITE, (char *)arg, size)))
            return -EFAULT;
#endif
    p_dev = p_dev_list[iminor(file->f_dentry->d_inode)];
    if (p_dev == NULL )
            return -ENODEV;

    //dev = (struct ozscr_dev_t *)p_dev->dev->driver_data;
    dev = platform_get_drvdata(p_dev);
    pRdrExt = dev->o2scr_reader;

    switch (cmd) 
    {
        case OZSCR_RESET:    /* Reset CT */
                dprintk("OZSCR_RESET\n");
                pRdrExt->sci_base_addr = dev->sci_base_addr;    
                ret = initial_sci(pRdrExt); 
                break;
        case OZSCR_OPEN:    /* Request ICC */
                dprintk("OZSCR_OPEN\n");
                ATRLength = ATR_SIZE;

                pRdrExt->sci_base_addr = dev->sci_base_addr; 

                pRdrExt->m_SCard.AvailableProtocol = 0;
                pRdrExt->m_SCard.RqstProtocol = 0;
                pRdrExt->m_SCard.WireProtocol = 0;

                ret = CmdResetReader(pRdrExt, FALSE, ATRBuffer, &ATRLength);

                apdu.LengthOut = ATRLength;

#ifdef PCMCIA_DEBUG
                printk(KERN_DEBUG "Open finished, ATR buffer = ");
                for (ATRLength = 0; ATRLength < apdu.LengthOut; ATRLength++)
                    printk(" [%02X] ", ATRBuffer[ATRLength]);
                printk("\n");
#endif
                memcpy(apdu.DataOut, ATRBuffer, ATRLength);
                apdu.Status = pRdrExt->m_SCard.WireProtocol;
                if (copy_to_user((struct ozscr_apdu *)arg, &apdu, sizeof(struct ozscr_apdu)))
                    return -EFAULT;
                break;
        case OZSCR_PROTOCOL:
                dprintk("OZSCR_PROTOCOL\n");
                if (copy_from_user(&apdu, (struct ozscr_apdu *)arg, sizeof(struct ozscr_apdu)))
                    return -EFAULT;

                pRdrExt->m_SCard.WireProtocol = apdu.Status;
                break;
        case OZSCR_CLOSE:    /* Eject ICC */
                dprintk("OZSCR_CLOSE\n");
                pRdrExt->sci_base_addr = dev->sci_base_addr;
                //sl_w16(SCIICR,0xffff);
                //ret = CmdDeactivate(pRdrExt);
                break;
        case OZSCR_SELECT:
                dprintk("OZSCR_SELECT test clk\n");

                /* set protocol */
                if (copy_from_user(&apdu, (struct ozscr_apdu *)arg, sizeof(struct ozscr_apdu)))
                    return -EFAULT;
                ATRLength = ATR_SIZE;

                pRdrExt->sci_base_addr = dev->sci_base_addr;
                pRdrExt->m_SCard.RqstProtocol = apdu.DataIn[6];

                ret = CmdResetReader(pRdrExt, FALSE, ATRBuffer, &ATRLength);
                apdu.LengthOut = ATRLength;
                memcpy(apdu.DataOut, ATRBuffer, ATRLength);
                if (copy_to_user((struct ozscr_apdu *)arg, &apdu, sizeof(struct ozscr_apdu)))
                    return -EFAULT;
                break;
        case OZSCR_STATUS:    
                dprintk("OZSCR_STATUS\n");

                pRdrExt->sci_base_addr = dev->sci_base_addr;
                //indicate a card inserted;
                if(sl_r16(SCIRIS) & 0x1)
                    ret = 1;
                else ret = -1;

                break;
        case OZSCR_CMD:
                dprintk("OZSCR_CMD protocol=%d\n", pRdrExt->m_SCard.Protocol);

                if (copy_from_user(&apdu, (struct ozscr_apdu *)arg, sizeof(struct ozscr_apdu)))
                    return -EFAULT;

                pRdrExt->sci_base_addr = dev->sci_base_addr;
                dprintk("In data transfer: apdu.length = %hd\n", (u16) apdu.LengthIn);
#ifdef PCMCIA_DEBUG
                printk(KERN_DEBUG "APDU datain = ");
                for (ATRLength = 0; ATRLength < apdu.LengthIn; ATRLength++)
                    printk("[%02X] ", apdu.DataIn[ATRLength]);
                printk("\n");
                printk(KERN_DEBUG "%s\n", apdu.DataIn + 5);
#endif
                pRdrExt->SmartcardRequest.Buffer = apdu.DataIn;
                pRdrExt->SmartcardRequest.BufferLength = apdu.LengthIn;
                pRdrExt->SmartcardReply.Buffer = apdu.DataOut;

                pRdrExt->SmartcardReply.BufferLength = apdu.LengthOut;

                ret = CBTransmit(pRdrExt);
                apdu.LengthOut = pRdrExt->SmartcardReply.BufferLength;
#ifdef PCMCIA_DEBUG
                printk(KERN_DEBUG "Dump FIFO (a.L=%2hX) ", (u16) apdu.LengthOut);
                printk(" (r.L=%2hX) ", (u16) pRdrExt->SmartcardReply.BufferLength);
                printk("\n");
#endif
                if (copy_to_user((struct ozscr_apdu *)arg, &apdu, sizeof(struct ozscr_apdu)))
                    return -EFAULT;
                break;
        default:
                ret = -EINVAL;
                break;
        }

    return ret;
}

static void sci_work_fn(struct work_struct *work)
{
    if(gpio_get_value(SMART_CARD_IRQ) == 1)
    {
        printk("a cart is detected\n");
    }
    else
    {
        printk("a card is removed\n");
    }

}

static irqreturn_t silan_sci_irq(int irq, void *dev_id)
{
    schedule_delayed_work(&sci_work, 0);

    return IRQ_HANDLED; 
}

/*
 *  ozscr_probe() creates an "instance" of the driver, allocating
 *  local data structures for one device.  The device is registered
 *  with Card Services.
 *
 *  The platform_device structure is initialized, but we don't actually
 *  configure the card at this point -- we wait until we receive a
 *  card insertion event.
 */
static int ozscr_probe(struct platform_device *p_dev)
{
    struct ozscr_dev_t *dev;
    int ret = -ENOMEM;
    int minor;
    
    void __iomem *sci_addr;
    
    /* Allocate space for private device-specific data */
    dev = kzalloc(sizeof(struct ozscr_dev_t), GFP_KERNEL);
    if (dev == NULL) 
    {
        dprintk("allocate ozscr_dev_t fail\n");
        goto ErrHandle;
    }

    /* Allocate space for private device-specific data */
    dev->o2scr_reader = kzalloc(sizeof(READER_EXTENSION), GFP_KERNEL);
    if (dev->o2scr_reader == NULL) 
    {
        dprintk("allocate READER_EXTENSION fail\n");
        goto ErrHandle;
    }    

    //give the physical address   bly  todo
    //we need get SCI_PHY_BASE_ADDR from p_dev platform_device;

    sci_addr = ioremap((unsigned long)SCI_PHY_BASE_ADDR, (unsigned long)0x80);
    dev->sci_base_addr = (unsigned long )sci_addr;
    
    if(!dev->sci_base_addr) 
    {
        ret = -ENOMEM;
        goto ErrHandle;
    }

    /* Find an available minor for the device */
    for ( minor = 0; minor < MAX_O2SCR_DEV; minor++)
        if (p_dev_list[minor] == NULL)
            break;
    if (minor >= MAX_O2SCR_DEV) 
    {
        printk(KERN_WARNING "No more minor available for the new o2scr device.\n");
        //goto error_return;  //todo bly
    }
    p_dev_list[minor] = p_dev;

    dev->minor = minor;

    dprintk("o2scr_major is %d\n", o2scr_major);

    // it's just so redundant... we could merge those fields together
    //todo  give the address from pdev to here  and iomap  bly

    dev->o2scr_reader->sci_base_addr = dev->sci_base_addr;
    platform_set_drvdata(p_dev,dev);
    /*
     * Only one device handled for now, but let's number it in case
     * one day we can support more.
     */
    device_create(o2scr_class, NULL, MKDEV(o2scr_major, dev->minor), NULL, "o2scr%d", dev->minor);
    //initial configure of sci
#if 1
    s_sci_clk = clk_get(&p_dev->dev,"sci");
    if (s_sci_clk && IS_ERR(s_sci_clk)) 
    {
        printk( "fail to get sci clock controller\n");
        ret = -ENOENT;
        goto ErrHandle;
    }
    sci_clk = clk_get_rate(s_sci_clk);
#endif
    clk_enable(s_sci_clk);
    initial_sci(dev->o2scr_reader);
    
    ret = request_irq(GPIO_BASE+SMART_CARD_IRQ, silan_sci_irq, IRQF_DISABLED, "smart card", dev);
    if (ret != 0) 
    {
        printk("Cannot Claim IRQ\n");
        goto ErrHandle;
    }

    INIT_DELAYED_WORK(&sci_work, sci_work_fn);
    
    silan_padmux_ctrl(13, 1);  //sci pad on 
    
    return 0;

ErrHandle:
    platform_set_drvdata(p_dev, NULL);
    /* Free the allocated memory space */
    if(dev->sci_base_addr)
        iounmap((void __iomem *)dev->sci_base_addr);
    if (dev)
        kfree(dev->o2scr_reader);
    kfree(dev);
    //return ret;
    return -1;
}

/*
 * After a card is removed, ozscr_release() will unregister the
 * device, and release the PCMCIA configuration.
 */
static void ozscr_release(struct platform_device *p_dev)
{
/*
 * If the device is still in use we may not release resources
 * right now but we must ensure that no further actions are
 * done on the device. The resources we still hold will be
 * eventually freed through ozscr_detach.
 */
    //platform_disable_device(p_dev);  //todo

    return;
}

/*
 *  This deletes a driver "instance".  The device is de-registered
 *  with Card Services.  If it has been released, all local data
 *  structures are freed.
 */
static int ozscr_detach(struct platform_device *p_dev)
{
    //struct ozscr_dev_t *dev = (struct ozscr_dev_t *)p_dev->dev->driver_data;
    struct ozscr_dev_t *dev =platform_get_drvdata(p_dev);
    int i;

    flush_scheduled_work();

    silan_padmux_ctrl(13, 0);  //sci pad off
    ozscr_release(p_dev);

    /* clean up behind us */
    if (dev) 
    {        /* it should never be NULL but this is for safety */
        device_destroy(o2scr_class, MKDEV(o2scr_major, dev->minor));
        kfree(dev->o2scr_reader);
        iounmap((void __iomem *)dev->sci_base_addr);
        kfree(dev);

    }

    /* remove from the list of devices */
    for (i = 0; i < MAX_O2SCR_DEV; i++) 
    {
        if (p_dev == p_dev_list[i]) 
        {
            kfree(p_dev);
            //p_dev_list[i] = NULL;
            p_dev = NULL;
            break;
        }
    }

    return 0;
}

static ssize_t ozscr_read(struct file *file,
              char *pucRxBuffer, size_t count, loff_t *loc)
{
    return 1;
}

static ssize_t ozscr_write(struct file *file,
               const char *pucTxBuffer, size_t count, loff_t *loc)
{
    return 1;
}

static struct file_operations ozscr_chr_fops = 
{
    .owner          = THIS_MODULE,
    .read           = ozscr_read,
    .unlocked_ioctl = ozscr_ioctl,
    .write          = ozscr_write,
    .open           = ozscr_open,
    .release        = ozscr_close,
};

static struct platform_driver ozscrlx_driver = 
{    
    //.probe= ozscr_probe,
    .remove = ozscr_detach,
    .driver = {
        .name = "ozscrlx_cs",
        .owner = THIS_MODULE,
    },
};

/* Entry/Ending point of the driver*/
static int __init init_ozscrlx(void)
{
     o2scr_class = class_create(THIS_MODULE, "ozscrlx");
     if (!o2scr_class)
             return -1;

     /* Register new character device with kernel */
     o2scr_major = register_chrdev(0, "ozscrlx", &ozscr_chr_fops);
     if (!o2scr_major) 
     {
         dprintk("Grab device fail#%d\n", o2scr_major);
         class_destroy(o2scr_class);
         return -1;
     }

     if (platform_driver_register(&ozscrlx_driver) < 0) 
     {
         unregister_chrdev(o2scr_major, "ozscrlx");
         class_destroy(o2scr_class);
         return -1;
     }
     p_dev=kzalloc(sizeof(struct platform_device),GFP_KERNEL);
     ozscr_probe(p_dev);
     return 0;
}

static void __exit exit_ozscrlx(void)
{
    int i;

    /* free all the registered devices */
    for (i = 0; i < MAX_O2SCR_DEV; i++)
    {
        if (p_dev_list[i])
        {
            ozscr_detach(p_dev_list[i]);
        }
    }
#if 0
    if(p_dev)
    {
            kfree(p_dev);
    }
#endif
    platform_driver_unregister(&ozscrlx_driver);
    unregister_chrdev(o2scr_major, "ozscrlx");
    class_destroy(o2scr_class);
    
    return;
}

/*Read All DATA in FIFO_OUT*/
static unsigned int ReadATR(READER_EXTENSION *pRdrExt, u8 *pATR, unsigned long *pATRLength)
{
    unsigned long fifoLength;
    unsigned short rxfifo_data;
    unsigned int is_atr_read_successful=1;
    unsigned int count = 3; //try 3 times to get ATR
    unsigned short tmp;
    
    fifoLength = 0;
    while(count--)
    {
        //during all atr receive time, no atrstout and chtout    
        //jump the while when atr start timeout, it is normal
#if 1
        while( (((sl_r16(SCIRIS)>>5) & 0x1) != 1) && (((sl_r16(SCIRIS)>>8) & 0x1) != 0x1) )
        {
            //check fifo overrun todo
            if(sl_r16(SCIRXCOUNT) != 0)
            {
                rxfifo_data = sl_r16(SCIDATA);
                //check receive data parity error
                if(rxfifo_data>>8&0x1)
                {
                    printk("receive parity error happen\n");
                    fifoLength = 0;
                    {
                        printk("WarmReset\n");
                        sl_w16(SCICR2, 0x4);
                        tmp = sl_r16(SCICR1);
                        tmp &= 0xfffb;
                        sl_w16(SCICR1, tmp);
                    }
                    msleep(1);
                    continue;

                }
                *(pATR+fifoLength)= (unsigned char) rxfifo_data;
                pRdrExt->m_SCard.ATR[fifoLength] = *(pATR + fifoLength);    
                fifoLength++;
            }
        }
#endif
        pRdrExt->m_SCard.Atr_len = fifoLength;
        if( (sl_r16(SCIRIS)>>5 & 0x1) == 0x1)
        {    
            printk("atr start timeout!\n");
            sl_w16(SCIICR,0x1 << 5);
            is_atr_read_successful=0;
            *pATRLength =0; 
            return STATUS_UNSUCCESSFUL;
        }
        if((sl_r16(SCIRIS)>>8 & 0x1) == 0x1)
        {   
            sl_w16(SCIICR,0x1 <<8);
          //  printk("character timeout!\n");
        }
        //we get the atr
        is_atr_read_successful=1;
        *pATRLength = fifoLength;

        break;
    }    
    if(is_atr_read_successful)
    {
#ifdef PCMCIA_DEBUG
        u16 i = 0;
        printk("ATR data receive over!\n");
        printk("\n\nATR read =");
        for (i = 0; i < *pATRLength; i++)
            printk(" [%02X]", pRdrExt->m_SCard.ATR[i]);
        printk("\n\n\n");

        for(i = 4; i < *pATRLength; i++)
            printk("%c", pRdrExt->m_SCard.ATR[i]);
        printk("\n\n");
#endif
        return STATUS_SUCCESS;
    }
    else
    {
        dprintk("we failed to get the atr");
        return STATUS_UNSUCCESSFUL;
    }
}


/* Valid for 0<= x <=3 */
static int atr_find_tx1(int x, u8 *buffer, u8 *tx1)
{
    int offset = 1;

    if (!(buffer[0] & (0x10 << x)))
        return -1;

    while (x--)
    {   
        if (buffer[0] & (0x10 << x))  
            offset++;
    }

    *tx1 = buffer[offset];
    return 0;
}

/* Works only for n >= 2 */
static int atr_find_txn(int x, int n, u8 *buffer, u8 *txn)
{
    int i = 1, j = 1;

    for (i = 1; i < n; i++) 
    {
        if (!(buffer[j] & 0x80))
            return -1;

        j += hweight8(buffer[j] & 0xF0);
    };

    return atr_find_tx1(x, &buffer[j], txn);
}

static u8 ATRFindProtocol(u8 *buffer)
{
    u8 protocol = 0;

#ifdef PCMCIA_DEBUG
    int j;
    printk(KERN_DEBUG "Initial data in buffer = ");
    for (j = 0; j < 0x20; j++)
        printk("[%02x] ", buffer[j]);
    printk("\n");
#endif

    atr_find_tx1(3, buffer, &protocol);
    protocol &= 0x01;

    dprintk("Result protocol = %02x\n", protocol);
    return protocol;
}

static inline int ATRFindTA1(u8 *buffer, u8 *ta1)
{
    return atr_find_tx1(0, &buffer[1], ta1);
}

static inline int ATRFindTB1(u8 *buffer, u8 *tb1)
{
    return atr_find_tx1(1, &buffer[1], tb1);
}

static inline int ATRFindTC1(u8 *buffer, u8 *tc1)
{
    return atr_find_tx1(2, &buffer[1], tc1);
}

static inline int ATRFindTA2(u8 *buffer, u8 *ta2)
{
    return atr_find_txn(0, 2, buffer, ta2);
}

static inline int ATRFindTB2(u8 *buffer, u8 *tb2)
{
    return atr_find_txn(1, 2, buffer, tb2);
}

static inline int ATRFindTC2(u8 *buffer, u8 *tc2)
{
    return atr_find_txn(2, 2, buffer, tc2);
}

/* This could have been TA3, but this one is special, it's the IFSC */
static u8 ATRFindIFSC(u8 *buffer)
{
    int ret;
    u8 ifsc = 0;

    ret = atr_find_txn(0, 3, buffer, &ifsc);
    if (ret || ifsc < 5)
        return 32;    /* default IFSC */

    return ifsc;
}

static inline int ATRFindTB3(u8 *buffer, u8 *tb3)
{
    return atr_find_txn(1, 3, buffer, tb3);
}

static inline int ATRFindTC3(u8 *buffer, u8 *tc3)
{
    return atr_find_txn(2, 3, buffer, tc3);
}

static short InitATRParam(READER_EXTENSION *pRdrExt)
{
    u8 tmp;

    pRdrExt->m_SCard.Protocol = ATRFindProtocol(pRdrExt->m_SCard.ATR + 1);

    if (pRdrExt->m_SCard.RqstProtocol == 0x01)
        pRdrExt->m_SCard.Protocol = 0x00;
    else if (pRdrExt->m_SCard.RqstProtocol == 0x02)
        pRdrExt->m_SCard.Protocol = 0x01;

    pRdrExt->m_SCard.EdcType = 0;

    pRdrExt->m_SCard.AtrTA1 = 0xFFFF;
    pRdrExt->m_SCard.TA1 = 0x11;
    if (!ATRFindTA1(pRdrExt->m_SCard.ATR, &tmp))
        pRdrExt->m_SCard.AtrTA1 = tmp;

    pRdrExt->m_SCard.TB1 = 0xFFFF;
    if (!ATRFindTB1(pRdrExt->m_SCard.ATR, &tmp))
        pRdrExt->m_SCard.TB1 = tmp;

    pRdrExt->m_SCard.TC1 = 0xFFFF;
    if (!ATRFindTC1(pRdrExt->m_SCard.ATR, &tmp))
        pRdrExt->m_SCard.TC1 = tmp;

    if (!ATRFindTA2(pRdrExt->m_SCard.ATR, &tmp)) 
    {
        pRdrExt->m_SCard.TA2 = tmp;
        pRdrExt->m_SCard.Protocol = tmp & 0x01;    /*Only supp T=0/1 protocol */
    }

    if (!ATRFindTB2(pRdrExt->m_SCard.ATR, &tmp))
        pRdrExt->m_SCard.TB2 = tmp;

    pRdrExt->m_SCard.TC2 = 0xFFFF;
    if (!ATRFindTC2(pRdrExt->m_SCard.ATR, &tmp))
        pRdrExt->m_SCard.TC2 = tmp;

    pRdrExt->T1.ifsc = ATRFindIFSC(pRdrExt->m_SCard.ATR);
    pRdrExt->T1.ns = 0;
    pRdrExt->T1.nr = 0;
    pRdrExt->T1.nad = 0;

    if (!ATRFindTB3(pRdrExt->m_SCard.ATR, &tmp))
        pRdrExt->m_SCard.TB3 = tmp;

    pRdrExt->m_SCard.TC3 = 0xFFFF;
    if (!ATRFindTC3(pRdrExt->m_SCard.ATR, &tmp)) 
    {
        pRdrExt->m_SCard.TC3 = tmp;

        if (tmp & 0x40) 
        {
            if (tmp & 0x01)
                pRdrExt->T1.rc = SC_T1_CHECKSUM_CRC;
            else
                pRdrExt->T1.rc = SC_T1_CHECKSUM_LRC;
        }
    }

    pRdrExt->m_SCard.PowerOn = TRUE;
    pRdrExt->m_SCard.WakeUp = TRUE;

    return 0;
}

static unsigned int AsyT1Ini(READER_EXTENSION *pRdrExt)
{
    unsigned long cwt, BwtT1;
    u16 Cgt;
    u16 F, D;
    u8 ta1;

    unsigned short tmp;
    //direct or inverse direction
    if(pRdrExt->m_SCard.ATR[0]== 0x3f)
    {     
        tmp=sl_r16(SCICR0);
        tmp|=0x1;
        sl_w16(SCICR0, tmp);
    }
    else
    {
        tmp=sl_r16(SCICR0);
        tmp&=0xfffe;
        sl_w16(SCICR0, tmp);
    }

    ta1 = (u8) (pRdrExt->m_SCard.TA1 & 0xff);
    if (pRdrExt->m_SCard.TA1 != 0xffff) 
    {
        F = (ta1 & 0xf0) >> 4;
        D = ta1 & 0x000f;
        F = Fi[F];
        D = Di[D];

        if (F == 0xffff || D == 0xffff) 
        {
            F = 372;
            D = 1;
        }
    } 
    else 
    {
        F = 372;
        D = 1;
    }

    Cgt = 0;
    if (pRdrExt->m_SCard.TC1 != 0xffff)
        Cgt = pRdrExt->m_SCard.TC1 & 0x00ff;
    sl_w16(SCICHGUARD,Cgt + 1);

    sl_w16(SCIBAUD,(F-D)/D);

    cwt = 13;

    BwtT1 = 4;
    if (pRdrExt->m_SCard.TB3 != 0xffff) 
    {
        u8 tb3 = (u8) (pRdrExt->m_SCard.TB3 & 0xff);
        cwt = tb3 & 0x0f;

        BwtT1 = ((pRdrExt->m_SCard.TB3 & 0xf0) >> 4);

        if (BwtT1 > 9)
            return GE_II_ATR_PARM;
    }
    pRdrExt->m_SCard.Bwi = (u16) BwtT1;
    cwt = 11 + (1 << cwt);
    BwtT1 = ((1 << BwtT1) * 960 * 372) / F;
    BwtT1 = BwtT1 * D + 11;

    cwt = cwt - 11;  
    sl_w16(SCICHTIMEMS,cwt / 0x10000);
    sl_w16(SCICHTIMELS,cwt % 0x10000);


    sl_w16(SCIBLKTIMEMS,BwtT1 / 0x10000);
    sl_w16(SCIBLKTIMELS,BwtT1 % 0x10000);
    pRdrExt->T1.bwt = BwtT1;

    return 0;
}
static unsigned int AsyT0Ini(READER_EXTENSION *pRdrExt)
{
    unsigned long cwt;
    u16 Cgt = 0;
    u16 F, D;
    u8 ta1;

    unsigned short tmp;

    if(pRdrExt->m_SCard.ATR[0]== 0x3f)
    {     
        tmp = sl_r16(SCICR0);
        tmp |= 0x1;
        sl_w16(SCICR0, tmp);
    }
    else
    {
        tmp = sl_r16(SCICR0);
        tmp &= 0xfffe;
        sl_w16(SCICR0, tmp);
    }

    //rx tx parity error handshake enable 
    tmp = sl_r16(SCICR0);
    tmp |= ((0x1<<3) | (0x1<<5));
    sl_w16(SCICR0, tmp);

    ta1 = (u8) (pRdrExt->m_SCard.TA1 & 0xff);
    if (pRdrExt->m_SCard.TA1 != 0xffff) 
    {
        F = (ta1 & 0xf0) >> 4;
        D = ta1 & 0x000f;
        F = Fi[F];
        D = Di[D];

        if (F == 0) 
        {
            F = 372;
            D = 1;
        }
    } 
    else 
    {
        F = 372;
        D = 1;
    }

    if (pRdrExt->m_SCard.TC1 != 0xffff)
        Cgt = pRdrExt->m_SCard.TC1 & 0x00ff;
   
    sl_w16(SCICHGUARD,(Cgt + 1));
    
    sl_w16(SCIBAUD, (F-D)/D);

    //working waiting time
    if (pRdrExt->m_SCard.TC2 == 0xffff)
        cwt = 10;
    else
        cwt = pRdrExt->m_SCard.TC2;

    if (pRdrExt->m_SCard.TA1 != 0xffff)
        cwt = (((cwt * D * F * 10) / F) + 5) / 10;

    cwt *= 960;
    cwt = cwt - 12;  
    sl_w16(SCICHTIMEMS, cwt / 0x10000);
    sl_w16(SCICHTIMELS, cwt % 0x10000);
    
    return 0;
}

/*Single subroutine for Cold/Hot Reset and Get ATR back*/
static unsigned int CmdResetReader(READER_EXTENSION *pRdrExt,
                   BOOLEAN WarmReset, u8 *pATR, unsigned long *pATRLength)
{
   unsigned int ret = STATUS_IO_DEVICE_ERROR;
   unsigned long delay = 0;

   u8 protocol;
   u16 tmp;
   dprintk("original protocol before reset: pRdrExt->m_SCard.Protocol = %d\n",
            pRdrExt->m_SCard.Protocol);
   /*Clear FIFO before reset the card  todo bly*/

   if (WarmReset) 
   {
        dprintk("WarmReset\n");
        sl_w16(SCICR2, 0x4);
        tmp = sl_r16(SCICR1);
        tmp &= 0xfffb;
        sl_w16(SCICR1, tmp);
   } 
   else 
   {
        /*Cold Reset */
        CmdDeactivate(pRdrExt);
        //active sequence startup
        tmp = 0x1;
        sl_w16(SCICR2, 0x1);//trigger the startup
        //The SCI notifies the host that the activation sequence is complete
        delay = jiffies + SCI_TIMEOUT;
        while((((sl_r16(SCIRIS)>>2) & 0x1) != 1 ) && time_before(jiffies, delay));
        sl_w16(SCIICR, 0x1 << 2);
   }

   ret = ReadATR(pRdrExt, pATR, pATRLength);
   if (ret != STATUS_SUCCESS)
   {
       *pATRLength = 0;
   }
   else
   {
       InitATRParam(pRdrExt);

       protocol = pRdrExt->m_SCard.Protocol & 0x01;
       if (protocol) 
       {
           dprintk("AsyT1Ini() is called\n");
           AsyT1Ini(pRdrExt);
       } 
       else 
       {
           dprintk("AsyT0Ini() is called\n");
           AsyT0Ini(pRdrExt);
       }
   }

   return ret;
}

static unsigned int CmdDeactivate(READER_EXTENSION *pRdrExt)
{
    unsigned long delay = 0; 
    sl_w16(SCICR2, 0x2);
    
    //WAIT until deactive done
    delay = jiffies + SCI_TIMEOUT;
    while((((sl_r16(SCIRIS)>>3) & 0x1) != 1) && time_before(jiffies, delay));

    //clear 
    sl_w16(SCIICR, 0x1 << 3);
 
    return 0;
}

static unsigned int PscrWriteDirect(READER_EXTENSION *pRdrExt, u8 *pData,
                    unsigned long DataLen, unsigned long *pNBytes)
{
    unsigned int ret;
    unsigned short tmp;
    int Idx = 0;


    if (pNBytes)
        *pNBytes = 0;
    //mode=1, transmit data can write to txfifo only mode =1
    tmp = sl_r16(SCICR1);
    tmp |= (0x1 << 2);
    sl_w16(SCICR1, tmp);
    //flush the txfifo  
    sl_w16(SCITXCOUNT, 0x1);
    //write the command data to txfifo

    //here we need to check whether the data we successfully write 

    /*Write Data into FIFO */  
    for (Idx = 0; Idx < DataLen; Idx++)
        sl_w8(SCIDATA, pData[Idx]);
    *pNBytes = DataLen;  
    ret = STATUS_SUCCESS;

#ifdef PCMCIA_DEBUG
    printk(KERN_DEBUG "WriteFIFO :");
    for (Idx = 0; Idx < DataLen; Idx++)
            printk(" [%02X]", pData[Idx]);
    printk("\n");
#endif
    //change to the receive mode 
    tmp = sl_r16(SCICR1);
    tmp &= 0xfffb;
    sl_w16(SCICR1, tmp);

    return ret;
}

/*finishes the callback RDF_TRANSMIT for the T0 protocol*/
static unsigned int CBT0Transmit(READER_EXTENSION *pRdrExt)
{
    u8 *pRequest = pRdrExt->SmartcardRequest.Buffer;
    u8 *pReply = pRdrExt->SmartcardReply.Buffer;
    unsigned long IOBytes = 0;
    unsigned long RequestLength = pRdrExt->SmartcardRequest.BufferLength;
    unsigned int ret;
    u16 T0ExpctedReturnLen;


    /* Transmit pRdrExt->SmartcardRequest.Buffer to smart card */
    pRdrExt->SmartcardReply.BufferLength = 0;
    dprintk("CBT0 :: RequestLength = %ld\n", RequestLength);
    /* Since H/W may not STOP the command automaticaly. Check Return Byte */
    if (RequestLength >= 5)
        T0ExpctedReturnLen = pRequest[4];
    else
        T0ExpctedReturnLen = 0;

    if (T0ExpctedReturnLen == 0)
        T0ExpctedReturnLen = 256;

    T0ExpctedReturnLen += 2;    /*SW1 and SW2 */
    ret = PscrWriteDirect(pRdrExt, pRequest, RequestLength, &IOBytes);
    
#if 0
    if (RequestLength > 5) 
    {
        ret = PscrWriteDirect(pRdrExt, pRequest, RequestLength, &IOBytes);
        dprintk("RequestLength > 5 : code 048\n");
    } 
    else 
    {
        ret = PscrWriteDirectWithFIFOLevel(pRdrExt, pRequest,
                         RequestLength, &IOBytes, T0ExpctedReturnLen);
        dprintk("RequestLength <= 5 : code 049\n");
    }
#endif
    
    if (ret == STATUS_SUCCESS) 
    {
        IOBytes = MAX_T1_BLOCK_SIZE;
        ret = ReadFIFO(pRdrExt, pReply, &IOBytes);
        if (ret == STATUS_SUCCESS)
            pRdrExt->SmartcardReply.BufferLength = IOBytes;
        else
            dprintk("ReadFIFO() fail CBT0 : code 051\n");
    } 
    else
        dprintk("Fail CBT0 : code 050\n");

    dprintk("Function Complete\n");
    return ret;
}

/* Returns LRC of data */
static u8 scT1Lrc(u8 *data, int datalen)
{
    u8 lrc = 0x00;
    int i;

    for (i = 0; i < datalen; i++)
        lrc ^= data[i];
    return lrc;
}

/* Calculates CRC of data */
static void scT1Crc(u8 *data, int datalen, u8 *crc)
{
    int i;
    u16 tmpcrc = 0xFFFF;

    for (i = 0; i < datalen; i++)
        tmpcrc = ((tmpcrc >> 8) & 0xFF) ^ crctab[(tmpcrc ^ *data++) & 0xFF];
    crc[0] = (tmpcrc >> 8) & 0xFF;
    crc[1] = tmpcrc & 0xFF;
}

/* Checks RC. */
static BOOLEAN scT1CheckRc(READER_EXTENSION *pRdrExt, u8 *data,
               unsigned long *datalen)
{
    u8 cmp[2];

    switch (pRdrExt->T1.rc) 
    {
        case SC_T1_CHECKSUM_LRC:
        /* Check LEN. */
            if ((data[2] + 3 + 1) > *datalen) 
            {
                dprintk("(data[2]+3+1) != *datalen: code 040\n");
                return FALSE;
            }

            if (data[data[2] + 3] == scT1Lrc(data, data[2] + 3))
                return TRUE;
            break;
        case SC_T1_CHECKSUM_CRC:
        /* Check LEN. */
            if ((data[2] + 3 + 2) > *datalen) 
            {
                dprintk("data[2]+3+2) != *datalen: code 043\n");
                return FALSE;
            }
            scT1Crc(data, data[2] + 3, cmp);
            if (memcmp(data + data[2] + 3, cmp, 2) == 0)
                return TRUE;
            break;
        default:
            break;
    }

    dprintk("scT1CheckRc failed: 046\n");
    return FALSE;
}

/* Appends RC */
static unsigned int scT1AppendRc(READER_EXTENSION *pRdrExt, u8 *data, unsigned long *datalen)
{
    switch (pRdrExt->T1.rc) 
    {
        case SC_T1_CHECKSUM_LRC:
            data[*datalen] = scT1Lrc(data, *datalen);
            *datalen += 1;
            dprintk("SC_T1_CHECKSUM_LRC: code 038\n");
        case SC_T1_CHECKSUM_CRC:
            scT1Crc(data, *datalen, data + *datalen);
            *datalen += 2;
            dprintk("SC_T1_CHECKSUM_CRC: code 039\n");
            return SC_EXIT_OK;
        default:
            dprintk("scT1AppendRc() failed: code 037\n");
            return SC_EXIT_BAD_PARAM;
    }

    return SC_EXIT_OK;
}

/* Builds S-Block */
static unsigned int scT1SBlock(READER_EXTENSION *pRdrExt, int type, int dir,
                   int param, u8 *block, unsigned long *pLen)
{
    unsigned int ret;

    block[0] = pRdrExt->T1.nad;
    switch (type) 
    {
        case SC_T1_S_RESYNCH:
                block[2] = 0x00;
                *pLen = 3;
                break;
        case SC_T1_S_IFS:
                block[2] = 0x01;
                block[3] = (u8) param;
                *pLen = 4;
                break;
        case SC_T1_S_ABORT:
                block[2] = 0x00;
                *pLen = 3;
                break;
        case SC_T1_S_WTX:
                block[2] = 0x00;
                block[3] = (u8) param;
                *pLen = 4;
                break;
        default:
                printk(KERN_WARNING MODULE_NAME "default in scT1SBlock()\n");
                return SC_EXIT_BAD_PARAM;
    }

    if (dir == SC_T1_S_REQUEST)
        block[1] = 0xC0 | (u8) type;
    else
        block[1] = 0xE0 | (u8) type;

    ret = scT1AppendRc(pRdrExt, block, pLen);
    if (ret)
        printk(KERN_WARNING MODULE_NAME "Call scT1AppendRc() failed in scT1SBlock()\n");

    return ret;
}

/* Builds I-Block */
static unsigned int scT1IBlock(READER_EXTENSION *pRdrExt, BOOLEAN more,
                   u8 *data, unsigned long *datalen, u8 *block, unsigned long *blocklen)
{
    block[0] = pRdrExt->T1.nad;
    block[1] = 0x00;

    if (pRdrExt->T1.ns)
        block[1] |= 0x40;

    if (more)
        block[1] |= 0x20;

    if (*datalen > pRdrExt->T1.ifsc)
        return SC_EXIT_BAD_PARAM;

    block[2] = (u8) *datalen;
    memcpy(block + 3, data, *datalen);

    *blocklen = (*datalen) + 3;
    return scT1AppendRc(pRdrExt, block, blocklen);
}

/* Builds R-Block */
static unsigned int scT1RBlock(READER_EXTENSION *pRdrExt, int type, u8 *block,
                   unsigned long *len)
{
    unsigned int ret;

    block[0] = pRdrExt->T1.nad;
    block[2] = 0x00;

    if ((type != SC_T1_R_OK) && (type != SC_T1_R_EDC_ERROR) && (type != SC_T1_R_OTHER_ERROR)) 
    {
        dprintk("SC_EXIT_BAD_PARAM: code 035\n");
        return SC_EXIT_BAD_PARAM;
    }

    if (pRdrExt->T1.nr)
        block[1] = 0x90 | (u8) type;
    else
        block[1] = 0x80 | (u8) type;

    *len = 3;
    if ((ret = scT1AppendRc(pRdrExt, block, len)))
        return ret;

    return SC_EXIT_OK;
}

/* Returns N(R) or N(S) from R/I-Block. */
static u8 scT1GetN(u8 *block)
{
    /* R-Block */
    if ((block[1] & 0xC0) == 0x80)
        return ((block[1] >> 4) & 0x01);

    /* I-Block */
    if ((block[1] & 0x80) == 0x00)
        return ((block[1] >> 6) & 0x01);

    return 0;
}

/* Change IFSD. */
static unsigned int scT1ChangeIFSD(READER_EXTENSION *pRdrExt, u8 ifsd)
{
    unsigned long blocklen;
    unsigned long rblocklen = pRdrExt->SmartcardReply.BufferLength;
    unsigned int ret, errors = 0;
    BOOLEAN success = FALSE;
    u8 block[SC_T1_MAX_SBLKLEN];
    u8 *rblock = pRdrExt->SmartcardReply.Buffer;

    ret = scT1SBlock(pRdrExt, SC_T1_S_IFS, SC_T1_S_REQUEST, ifsd, block, &blocklen);
    if (ret) 
    {
        printk(KERN_WARNING MODULE_NAME "Call scTiSBlock() failed in scT1ChangeIFSD()\n");
        return ret;
    }

    while (!success) 
    {
        ret = PscrWriteDirect(pRdrExt, block, blocklen, &rblocklen);
        if (ret) 
        {
            printk(KERN_WARNING MODULE_NAME "Call PscrWriteDirect() failed in scT1ChangeIFSD()\n");
            return SC_EXIT_IO_ERROR;
        }

        //msleep(10); 

        rblocklen = MAX_T1_BLOCK_SIZE;
        ret = ReadFIFO(pRdrExt, rblock, &rblocklen);
        if (ret == STATUS_SUCCESS) 
        {
            if ((rblock[1] == 0xE1) && scT1CheckRc(pRdrExt, rblock, &rblocklen)) 
            {
                pRdrExt->T1.ifsreq = TRUE;
                pRdrExt->T1.ifsd = rblock[3];
                success = TRUE;
            } 
            else 
            {
                printk(KERN_WARNING MODULE_NAME "rblocklen==blocklen) && (rblock[1]==0xE1) && "
                       "scT1CheckRc( pRdrExt, rblock, &rblocklen ) " "failed in scT1ChangeIFSD()\n");
                printk(KERN_WARNING " values:\n rblocklen = %ld "
                       ": blocklen = %ld\n rblock[1] = %x\n",rblocklen, blocklen, rblock[1]);
                errors++;
            }
        } 
        else
            printk(KERN_WARNING MODULE_NAME "Call ReadFIFO() failed in scT1ChangeIFSD()\n");

        if (errors > 2) 
        {
            pRdrExt->T1.ifsreq = TRUE;
            success = TRUE;
        }
    }

    return SC_EXIT_OK;
}

static unsigned int t1_send_rblock(READER_EXTENSION *pRdrExt, int type,
                   u8 *block, unsigned long *blocklen, unsigned long *pNBytes)
{
    unsigned int ret;
    /* Create R-Block. */
    ret = scT1RBlock(pRdrExt, type, block, blocklen);
    if (ret) 
    {
        printk(KERN_WARNING "scT1RBlock() failed\n");
        return ret;
    }

    /* Send R-Block. */
    ret = PscrWriteDirect(pRdrExt, block, *blocklen, pNBytes);
    if (ret) 
    {
        dprintk("PscrWriteDirect() failed\n");
        return SC_EXIT_IO_ERROR;
    }
    return SC_EXIT_OK;
}

static unsigned int t1_send_sblock(READER_EXTENSION *pRdrExt, int type,
                   int dir, int param, u8 *block, unsigned long *blocklen,
                   unsigned long *pNBytes)
{
    unsigned int ret;
    /* Create S-Block. */
    ret = scT1SBlock(pRdrExt, type, dir, param, block, blocklen);
    if (ret) 
    {
        printk(KERN_WARNING "scT1SBlock() failed\n");
        return ret;
    }

    /* Send S-Block. */
    ret = PscrWriteDirect(pRdrExt, block, *blocklen, pNBytes);
    if (ret) 
    {
        printk(KERN_WARNING "PscrWriteDirect() failed\n");
        return SC_EXIT_IO_ERROR;
    }
    return SC_EXIT_OK;
}

static unsigned int t1_send_iblock(READER_EXTENSION *pRdrExt,
                   BOOLEAN *more, unsigned long *sendptr, u8 *block, unsigned long *blocklen,
                   unsigned long *rblocklen)
{
    unsigned long sendlen;
    unsigned long RequestLength = pRdrExt->SmartcardRequest.BufferLength;
    unsigned int ret;
    u8 *pRequest = pRdrExt->SmartcardRequest.Buffer;

    /* Make next I-Block. */
    sendlen = min(RequestLength - *sendptr, (unsigned long)pRdrExt->T1.ifsc);

    if (sendlen == (RequestLength - *sendptr))
        *more = FALSE;
    ret = scT1IBlock(pRdrExt, *more, pRequest + *sendptr, &sendlen, block, blocklen);
    if (ret) 
    {
        printk(KERN_WARNING "scT1IBlock() failed: code 015\n");
        return ret;
    }
    *sendptr += sendlen;

    /* Send I-Block. */
    dprintk("PscrWriteDirect: 007\n");
    ret = PscrWriteDirect(pRdrExt, block, *blocklen, rblocklen);
    if (ret) 
    {
        printk(KERN_WARNING "PscrWriteDirect() failed: code 016\n");
        return SC_EXIT_IO_ERROR;
    }
    return SC_EXIT_OK;
}

static unsigned int cb_t1_transmit_timeout(READER_EXTENSION *pRdrExt,
                       u8 *rblock, unsigned long *rblocklen, u8 *block2, unsigned long *block2len,
                       int *timeout_err_cntr, int *timecntr)
{
    unsigned int ret;

    /* Timeout handling. */
    (*timeout_err_cntr)++;
    if (*timeout_err_cntr >= 2) 
    {
        if (*timecntr >= 2) 
        {
            dprintk("time_out_cntr >= 2: quit!");
            return SC_EXIT_IO_ERROR;
        }

        /* send S-Block: resync request. */
        dprintk("S-Block: send resync Request\n");
        ret = t1_send_sblock(pRdrExt, SC_T1_S_RESYNCH, SC_T1_S_REQUEST,
                     0, block2, block2len, rblocklen);
        if (ret)
            return ret;

        *timeout_err_cntr = 0;
        (*timecntr)++;
    } 
    else 
    {
        /* indicate SC_T1_R_OTHER_ERROR */
        printk(KERN_WARNING MODULE_NAME "scT1RBlock::SC_T1_R_OTHER_ERROR in ReadFIFO error\n");
        ret = t1_send_rblock(pRdrExt, SC_T1_R_OTHER_ERROR, block2, block2len, rblocklen);
        if (ret)
            return ret;
    }

    return SC_EXIT_OK;
}

static unsigned int cb_t1_transmit_rc_error(READER_EXTENSION *pRdrExt,
                        u8 *rblock, unsigned long *rblocklen, u8 *block2, unsigned long *block2len,
                        int *parity_err_cntr, int *errcntr)
{
    unsigned int ret;
    if (pRdrExt->T1.rc == SC_T1_CHECKSUM_LRC) 
    {
        if ((rblock[2] + 3 + 1) != *rblocklen)
            return t1_send_rblock(pRdrExt, SC_T1_R_OTHER_ERROR,
                          block2, block2len, rblocklen);
    } 
    else if (pRdrExt->T1.rc == SC_T1_CHECKSUM_CRC) 
    {
        if ((rblock[2] + 3 + 2) != *rblocklen)
            return t1_send_rblock(pRdrExt, SC_T1_R_OTHER_ERROR,
                          block2, block2len, rblocklen);
    }

    (*parity_err_cntr)++;
    if (*parity_err_cntr >= 3) 
    {
        if (*errcntr >= 1) 
        {
            printk(KERN_WARNING "CBT1Transmit() :: errcntr >= 1. Quit !\n");
            return SC_EXIT_IO_ERROR;
        }

        /* send S-Block: resync request. */
        printk(KERN_WARNING MODULE_NAME "S-Block: send resync Request: parity error\n");
        ret = t1_send_sblock(pRdrExt, SC_T1_S_RESYNCH, SC_T1_S_REQUEST, 0, block2, block2len, rblocklen);
        if (ret)
            return ret;
        *parity_err_cntr = 0;
        (*errcntr)++;
    } 
    else 
    {
        printk(KERN_WARNING "pRdrExt->T1.rc = 0x%x\n", pRdrExt->T1.rc);
        printk(KERN_WARNING MODULE_NAME "scT1RBlock::SC_T1_R_EDC_ERROR in Wrong length or RC error\n ");
        ret = t1_send_rblock(pRdrExt, SC_T1_R_EDC_ERROR, block2, block2len, rblocklen);
        if (ret)
            return ret;
    }

    return SC_EXIT_OK;
}

static unsigned int cb_t1_transmit_rblock(READER_EXTENSION *pRdrExt,
                      BOOLEAN lastiicc, BOOLEAN *more, unsigned long *sendptr, u8 *block,
                      unsigned long *blocklen, u8 *rblock, unsigned long *rblocklen, u8 *block2,
                      unsigned long *block2len, int *other_err_cntr, int *rerrcntr, int *quit_cntr)
{
    unsigned int ret;
    
    if (lastiicc) 
    {
        (*quit_cntr)++;
        if (*quit_cntr >= 3)
            return SC_EXIT_IO_ERROR;
        /* Card is sending I-Blocks, so send R-Block. */
        dprintk("scT1RBlock::SC_T1_R_OK in R-Block\n");

        ret = t1_send_rblock(pRdrExt, SC_T1_R_OK, block2, block2len,
                   rblocklen);
        if (ret)
            return ret;

    } 
    else 
    {
        if (scT1GetN(rblock) == pRdrExt->T1.ns) 
        {
        resend_RBlock:
            (*other_err_cntr)++;
            if (*other_err_cntr >= 3) 
            {
                /* other_err_cntr >= 3, the third time. quit and return SC_EXIT_IO_ERROR */
                if (*rerrcntr >= 2) 
                {
                    printk(KERN_WARNING MODULE_NAME "CBT1Transmit() :: rerrcntr >= 2. Quit !\n");
                    return SC_EXIT_IO_ERROR;
                }

                /* send S-Block: resync request. */
                dprintk("S-Block: send resync Request\n");
                ret = t1_send_sblock(pRdrExt, SC_T1_S_RESYNCH, SC_T1_S_REQUEST, 0, block2,
                           block2len, rblocklen);
                if (ret)
                    return ret;
                *other_err_cntr = 0;
                (*rerrcntr)++;
            } 
            else 
            {
                /* N(R) is old N(S), so resend I-Block. */
                dprintk("PscrWriteDirect: 006\n");
                ret = PscrWriteDirect(pRdrExt, block, *blocklen, rblocklen);
                if (ret) 
                {
                    printk(KERN_WARNING MODULE_NAME "PscrWriteDirect() failed: code 012\n");
                    return SC_EXIT_IO_ERROR;
                }
            }
        } 
        else 
        {
            /* N(R) is next N(S), so make next I-Block and send it. */
            /* Check if data available. */
            if (!(*more)) 
            {
                printk(KERN_WARNING MODULE_NAME "SC_EXIT_PROTOCOL_ERROR: code 013\n");
                (*quit_cntr)++;
                if (*quit_cntr > 3)
                    return SC_EXIT_PROTOCOL_ERROR;
                else
                    goto resend_RBlock;
            }
            /* Change N(S) to new value. */
            pRdrExt->T1.ns ^= 1;

            ret = t1_send_iblock(pRdrExt, more, sendptr, block, blocklen, rblocklen);
            if (ret)
                return ret;
        }
    }
    return SC_EXIT_OK;
}

static unsigned int cb_t1_transmit_iblock(READER_EXTENSION *pRdrExt,
                      u8 *rblock, unsigned long *rblocklen, u8 *block2, unsigned long *block2len,
                      int *ib_other_err_cntr, int *ierrcntr, u8 *rsp, unsigned long *rsplen)
{
    unsigned int ret;

    if (scT1GetN(rblock) != pRdrExt->T1.nr) 
    {
        (*ib_other_err_cntr)++;
        if (*ib_other_err_cntr >= 3) 
        {
            /* ib_other_err_cntr >= 3, the third time. quit and return SC_EXIT_IO_ERROR */
            if (*ierrcntr >= 2) 
            {
                printk(KERN_WARNING MODULE_NAME "CBT1Transmit() :: ierrcntr >= 2. Quit !\n");
                return SC_EXIT_IO_ERROR;
            }

            /* send S-Block: resync request. */
            dprintk("S-Block: send resync Request: i-block\n");
            ret = t1_send_sblock(pRdrExt, SC_T1_S_RESYNCH, SC_T1_S_REQUEST, 0, block2, block2len, rblocklen);
            if (ret)
                return ret;

            *ib_other_err_cntr = 0;
            (*ierrcntr)++;
        } 
        else 
        {
            /* Card is sending wrong I-Block, so send R-Block. */
            printk(KERN_WARNING MODULE_NAME "scT1RBlock::SC_T1_R_OTHER_ERROR in I-Block\n");
            ret = t1_send_rblock(pRdrExt, SC_T1_R_OTHER_ERROR, block2, block2len, rblocklen);
            if (ret)
                return ret;
        }
        return SC_EXIT_RETRY;
    }

    /* Copy data. */
    if (rblock[2] > (SC_GENERAL_SHORT_DATA_SIZE + 2 - *rsplen)) 
    {
        dprintk("rblock[2]>(SC_GENERAL_SHORT_DATA_SIZE+2-rsplen): code 019\n");
        return SC_EXIT_PROTOCOL_ERROR;
    }
    memcpy(&rsp[*rsplen], &rblock[3], rblock[2]);
    *rsplen += rblock[2];
    /* Change N(R) to new value. */
    pRdrExt->T1.nr ^= 1;

    if (rblock[1] & 0x20) 
    {
        /* More data available. */
        /* Send R-Block. */
        dprintk("scT1RBlock::SC_T1_R_OK in copy data\n");
        ret = t1_send_rblock(pRdrExt, SC_T1_R_OK, block2, block2len,rblocklen);
        if (ret)
            return ret;
    } 
    else 
    {
        u8 *pReply = pRdrExt->SmartcardReply.Buffer;
        /* Last block. */
        if (*rsplen < 2) 
        {
            printk(KERN_WARNING "rsplen<2: code 022\n");
            return SC_EXIT_BAD_SW;
        }

        if ((pRdrExt->T1.cse == SC_APDU_CASE_2_SHORT) ||
            (pRdrExt->T1.cse == SC_APDU_CASE_4_SHORT)) 
        {
            unsigned long cpylen;
            /* Copy response and SW. */
            cpylen = min(*rsplen - 2, (unsigned long)SC_GENERAL_SHORT_DATA_SIZE);   
            memcpy(pReply, rsp, cpylen);
            memcpy(pReply + cpylen, &rsp[*rsplen - 2], 2);
            pRdrExt->SmartcardReply.BufferLength = cpylen + 2;
        } 
        else 
        {
            /* Copy only SW. */
            memcpy(pReply, &rsp[*rsplen - 2], 2);
            pRdrExt->SmartcardReply.BufferLength = 2;
        }
        dprintk("SC_EXIT_OK: code 023(Jordan) return correctly. ");
        return SC_EXIT_OK;
    }
    return SC_EXIT_RETRY;
}

static unsigned int cb_t1_transmit_sblock_ifs(READER_EXTENSION *pRdrExt,
                          u8 *rblock, unsigned long *rblocklen,
                          u8 *block2, unsigned long *block2len)
{
    unsigned int ret;
    
    ret = scT1SBlock(pRdrExt, SC_T1_S_IFS, SC_T1_S_RESPONSE, rblock[3],
               block2, block2len);
    if (ret) 
    {
        printk(KERN_WARNING "scT1SBlock() failed: code 024\n");
        return ret;
    }

    dprintk("PscrWriteDirect: 012\n");
    memset(block2, 0, 5);
    *block2len = 4;
    block2[0] = rblock[0];
    block2[1] = 0xE1;
    block2[2] = rblock[2];
    block2[3] = rblock[3];
    scT1AppendRc(pRdrExt, block2, block2len);

    ret = PscrWriteDirect(pRdrExt, block2, *block2len, rblocklen);

    if (ret) 
    {
        printk(KERN_WARNING MODULE_NAME "PscrWriteDirect() failed\n");
        return SC_EXIT_IO_ERROR;
    }
    pRdrExt->T1.ifsc = rblock[3];
    //msleep(10);        
    return SC_EXIT_OK;
}

static unsigned int cb_t1_transmit_sblock_abort(READER_EXTENSION *pRdrExt,
                        u8 *rblock, unsigned long *rblocklen,
                        u8 *block2, unsigned long *block2len)
{
    unsigned int ret;
    
    ret = scT1SBlock(pRdrExt, SC_T1_S_ABORT, SC_T1_S_RESPONSE, 0x00, block2,
               block2len);
    if (ret) 
    {
        printk(KERN_WARNING MODULE_NAME "scT1SBlock() failed\n");
        return ret;
    }

    dprintk("PscrWriteDirect: 011\n");
    ret = PscrWriteDirect(pRdrExt, block2, *block2len, rblocklen);
    if (ret != *block2len)    
    {
        printk(KERN_WARNING MODULE_NAME "PscrWriteDirect() failed\n");
        return SC_EXIT_IO_ERROR;
    }
    /* Wait BWT. todo bly*/
    msleep(10);        //XXX 3s, quite long
    ret = ReadFIFO(pRdrExt, rblock, rblocklen);
    if (ret) 
    {
        printk(KERN_WARNING MODULE_NAME "ReadFIFO() failed\n");
        return SC_EXIT_IO_ERROR;
    }
    printk(KERN_WARNING MODULE_NAME "SC_EXIT_UNKNOWN_ERROR\n");
    return SC_EXIT_UNKNOWN_ERROR;
}

static unsigned int cb_t1_transmit_sblock_wtx(READER_EXTENSION *pRdrExt,
                          u8 *rblock, unsigned long *rblocklen,
                          u8 *block2, unsigned long *block2len)
{
    unsigned int ret;
    /* define a new variable for WTX */
    u8 ucRqstWTX = rblock[3];
    unsigned long ulWTX = (pRdrExt->T1.bwt) * ucRqstWTX;
      //pRdrExt->T1.bwt=(pRdrExt->T1.bwt) * ucRqstWTX  //we need do this???
    dprintk("/* S-Block WTX Request */\n");
    dprintk("rblock[3] = %x\n", rblock[3]);
    dprintk("pRdrExt->T1.bwt = 0x%lx\n", pRdrExt->T1.bwt);
    dprintk("ulWTX = 0x%lx :: ucRqstWTX = %x\n", ulWTX, ucRqstWTX);
    //silan
    sl_w16(SCIBLKTIMEMS,(u16)(ulWTX / 0x10000));
    sl_w16(SCIBLKTIMELS,(u16)(ulWTX % 0x10000));

    #if 0
    writew((u16) (ulWTX / 0x10000), (pRdrExt->membase) + BWT_MSB);
    writew((u16) (ulWTX % 0x10000), (pRdrExt->membase) + BWT_LSB);
    #endif
    dprintk("PscrWriteDirect: 012\n");
    memset(block2, 0, 5);

    *block2len = 5;
    block2[0] = rblock[0];
    block2[1] = rblock[1] + 0x20;
    block2[2] = rblock[2];
    block2[3] = rblock[3];
    block2[4] = rblock[4] + 0x20;
    ret = PscrWriteDirect(pRdrExt, block2, *block2len, rblocklen);
    if (ret != SC_EXIT_OK) 
    {
        printk(KERN_WARNING MODULE_NAME "PscrWriteDirect() failed\n");
        return SC_EXIT_IO_ERROR;
    }

    return SC_EXIT_OK;
}

static unsigned int cb_t1_transmit_sblock_resync(READER_EXTENSION *pRdrExt,
                         BOOLEAN *more, unsigned long *sendptr,
                         u8 *block, unsigned long *blocklen, unsigned long *rblocklen)
{
    /* resend I-Block */
    pRdrExt->T1.ns = 0;
    pRdrExt->T1.nr = 0;
    sendptr = 0;
    return t1_send_iblock(pRdrExt, more, sendptr, block, blocklen, rblocklen);
}

static unsigned int CBT1Transmit(READER_EXTENSION *pRdrExt)
{
#ifdef PCMCIA_DEBUG
    int i;
#endif
    unsigned int ret;
    unsigned long sendptr = 0;    /* Points to begining of unsent data. */

    u8 block[SC_T1_MAX_BLKLEN];
    unsigned long blocklen = 0;
    u8 block2[SC_T1_MAX_BLKLEN];
    unsigned long block2len;
    u8 *rblock = pRdrExt->SmartcardReply.Buffer;
    unsigned long rblocklen;
    u8 rsp[SC_GENERAL_SHORT_DATA_SIZE + 3];
    unsigned long rsplen = 0;

    BOOLEAN more = TRUE;    /* More data to send. */
    BOOLEAN lastiicc = FALSE;    /* It's ICCs turn to send I-Blocks. */

    /* two counters per kind of error */
    int timeout_err_cntr = 0;
    int timecntr = 0;

    int parity_err_cntr = 0;
    int errcntr = 0;

    int other_err_cntr = 0;
    int rerrcntr = 0;

    int ib_other_err_cntr = 0;
    int ierrcntr = 0;
    int quit_cntr = 0;

    dprintk("pRdrExt = %x\n", sizeof(READER_EXTENSION));

    pRdrExt->SmartcardReply.BufferLength = 0;

#ifdef PCMCIA_DEBUG
    printk(KERN_DEBUG " @@@ Request Data from ifdtest: Length = %ld\n",
           pRdrExt->SmartcardRequest.BufferLength);
    for (i = 0; i < pRdrExt->SmartcardRequest.BufferLength; i++)
        printk("[%02x]", pRdrExt->SmartcardRequest.Buffer[i]);
    printk("\n");
#endif
    pRdrExt->T1.ifsreq = FALSE;

#if 0
    // XXX is this supposed to change pRdrExt->T1.ifsreq ?!
    writew((u16) (pRdrExt->T1.bwt / 0x10000), (pRdrExt->membase) + BWT_MSB);
    writew((u16) (pRdrExt->T1.bwt % 0x10000), (pRdrExt->membase) + BWT_LSB);
#endif
    /* Change IFSD if not already changed. */
    if (!pRdrExt->T1.ifsreq) 
    {
        ret = scT1ChangeIFSD(pRdrExt, 0xFE);
        if (ret)
            return ret;
    }

    ret = t1_send_iblock(pRdrExt, &more, &sendptr, block, &blocklen, &rblocklen);
    if (ret)
        return ret;
    dprintk("Finish IFSD Write IBlock\n");

    while (TRUE) 
    {
        rblocklen = 270;
//        ret = ReadFIFO(pRdrExt, rblock, &rblocklen);

        if ((ret != SC_EXIT_OK) || (rblocklen == 0)) 
        {
            ret = cb_t1_transmit_timeout(pRdrExt, rblock, &rblocklen,
                           block2, &block2len, &timeout_err_cntr, &timecntr);
            if (ret)
                return ret;
            continue;
        }

        /* Wrong length or RC error. */
        if (!scT1CheckRc(pRdrExt, rblock, &rblocklen)) 
        {
            ret = cb_t1_transmit_rc_error(pRdrExt, rblock, &rblocklen,
                            block2, &block2len, &parity_err_cntr, &errcntr);
            if (ret)
                return ret;
            continue;
        }

        /* R-Block */
        if ((rblock[1] & 0xC0) == 0x80) 
        {
            ret = cb_t1_transmit_rblock(pRdrExt, lastiicc, &more,
                          &sendptr, block, &blocklen, rblock, &rblocklen, block2,
                          &block2len, &other_err_cntr, &rerrcntr, &quit_cntr);
            if (ret)
                return ret;
            continue;
        }
        /* Reset rerrcntr, because when it is here it had not received an
         * R-Block.
         *///XXX not sure where it is reset?

        /* I-Block */
        if (!(rblock[1] & 0x80)) 
        {
            if (!lastiicc)
                pRdrExt->T1.ns ^= 1;    /* Change N(S) to new value. */
            lastiicc = TRUE;
            ret = cb_t1_transmit_iblock(pRdrExt, rblock, &rblocklen,
                            block2, &block2len, &ib_other_err_cntr,&ierrcntr, rsp, &rsplen);
            if (ret == SC_EXIT_RETRY)
                continue;
            return ret;    /* can be an error or an OK status */
        }

        switch (rblock[1]) 
        {
            case 0xC1:    /* S-Block IFS Request */
                    ret = cb_t1_transmit_sblock_ifs(pRdrExt, rblock,
                              &rblocklen, block2, &block2len);
                    if (ret)
                        return ret;
                    break;
            case 0xC2:    /* S-Block ABORT Request */
                    return cb_t1_transmit_sblock_abort(pRdrExt, rblock,
                               &rblocklen, block2, &block2len);
                    break;    /* it will always return an error */
            case 0xC3:    /* S-Block ABORT Request */
                    ret = cb_t1_transmit_sblock_wtx(pRdrExt, rblock,
                              &rblocklen, block2, &block2len);
                    if (ret)
                        return ret;
                    break;
            case 0xE0:    /* S-Block resync response */
                    ret = cb_t1_transmit_sblock_resync(pRdrExt, &more,
                             &sendptr, block, &blocklen, &rblocklen);
                    if (ret)
                        return ret;
                    break;
            default:
            /* nothing special */
            break;
        }
    }

    /* Ooops! Should never be here. It always exit from inside the while */
    return SC_EXIT_UNKNOWN_ERROR;
}

static unsigned int VerifyByte(u8 control, u8 address, u8 data, u8 WireProtocol)
{
    BOOLEAN ackData = 0;
    unsigned int ret = STATUS_UNSUCCESSFUL;
    int i;

   // man_send_command(control, address, data, WireProtocol);
    msleep(1);

    for (i = 0; i < 2; i++)
    {
        msleep(1);
       // man_high(CLK);
        msleep(1);
       // ackData = man_is_read_io();
       // man_low(CLK);
        if (!ackData && (i == 1))
            ret = STATUS_SUCCESS;
    }

    return ret;
}

static unsigned int VerifyCounter(u8 control, u8 address, u8 data, u8 WireProtocol)
{
    BOOLEAN ackData = 0;
    unsigned int ret = STATUS_UNSUCCESSFUL;
    int i;

    //man_send_command(control, address, data, WireProtocol);
    msleep(1);
    for (i = 0; i < 103; i++)
    {
        msleep(1);
       // man_high(CLK);
       // ackData = man_is_read_io(); 
        msleep(1);
       // man_low(CLK);
        if (ackData && (i == 100))
            ret = STATUS_SUCCESS;
    }

    return ret;
}

static unsigned int VerifyData(READER_EXTENSION *pRdrExt)
{
    u8 data = 0, trials = 0;
    u8 *pRequest = pRdrExt->SmartcardRequest.Buffer;
    u8 *pReply = pRdrExt->SmartcardReply.Buffer;
    unsigned int ret = STATUS_UNSUCCESSFUL;

    if (pRdrExt->m_SCard.WireProtocol == 2)
        goto out;

    //man_readb(1021, pRdrExt->m_SCard.WireProtocol, &data, 1);
    trials = hweight8(data);
    if (trials) 
    {
        data = (u8) (0xFF00 >> (trials - 1));    /* truncated on purpose */
        /* Use to be:
         * data = (trials == 8) ? 0xFE :
         (trials == 7) ? 0xFC :
         (trials == 6) ? 0xF8 :
         (trials == 5) ? 0xF0 :
         (trials == 4) ? 0xE0 :
         (trials == 3) ? 0xC0 :
         (trials == 2) ? 0x80 : 0x00; */

        ret = VerifyCounter(0xF2, 0xFD, data, pRdrExt->m_SCard.WireProtocol);
        msleep(1);

        if (ret == STATUS_SUCCESS) 
        {
            /* enter first PSC-code byte */
            ret = VerifyByte(0xCD, 0xFE, pRequest[5],pRdrExt->m_SCard.WireProtocol);
            msleep(1);

            if (ret == STATUS_SUCCESS)
            {
                /* enter second PSC-code byte */
                ret = VerifyByte(0xCD, 0xFF, pRequest[6], pRdrExt->m_SCard.WireProtocol);
                msleep(1);
            }
        }

        ret = VerifyCounter(0xF3, 0xFD, 0xFF, pRdrExt->m_SCard.WireProtocol);
    }

out:
    pRdrExt->SmartcardReply.BufferLength = 2;
    if (ret == STATUS_SUCCESS) 
    {
        pReply[0] = 0x90;
        pReply[1] = 0x00;
    } 
    else 
    {
        pReply[0] = 0x63;
        trials = (trials == 0) ? 0 : trials - 1;
        pReply[1] = 0xC0 | trials;
    }

    return SC_EXIT_OK;
}

static unsigned int ChangeVerifyData(READER_EXTENSION *pRdrExt)
{
    u8 *pRequest = pRdrExt->SmartcardRequest.Buffer;
    u8 *pReply = pRdrExt->SmartcardReply.Buffer;
    unsigned int ret = STATUS_UNSUCCESSFUL;

    if (pRdrExt->m_SCard.WireProtocol == 2)
        goto out;

    /* update first PSC-code byte */
    ret = VerifyCounter(0xF3, 0xFE, pRequest[8], pRdrExt->m_SCard.WireProtocol);
    msleep(1);

    if (ret == STATUS_SUCCESS) 
    {
        /* update second PSC-code byte */
        ret = VerifyCounter(0xF3, 0xFF, pRequest[9], pRdrExt->m_SCard.WireProtocol);

        msleep(1);
    }

out:
    pRdrExt->SmartcardReply.BufferLength = 2;
    if (ret == STATUS_SUCCESS) 
    {
        pReply[0] = 0x90;
        pReply[1] = 0x00;
    } 
    else 
    {
        pReply[0] = 0x63;
        pReply[1] = 0xC0;
    }

    return SC_EXIT_OK;
}

static unsigned int UpdateBinary(READER_EXTENSION *pRdrExt)
{
    u8 *pRequest = pRdrExt->SmartcardRequest.Buffer;
    u8 *pReply = pRdrExt->SmartcardReply.Buffer;
    unsigned int ret = STATUS_UNSUCCESSFUL;
    u8 i;
    //u16 address = (pRequest[2] << 8) | pRequest[3];

    pRdrExt->SmartcardReply.BufferLength = 2;

    for (i = 0; i < pRequest[4]; i++)
    {
       // ret = man_writeb(address + i, pRequest[5 + i], pRdrExt->m_SCard.WireProtocol);
        if (ret != STATUS_SUCCESS)
            break;
    }

    if (ret == STATUS_SUCCESS) 
    {
        pReply[0] = 0x90;
        pReply[1] = 0x00;
    } 
    else 
    {
        pReply[0] = 0x62;
        pReply[1] = 0x00;
    }

    return SC_EXIT_OK;
}

static unsigned int ReadBinary(READER_EXTENSION *pRdrExt)
{
    u8 *pRequest = pRdrExt->SmartcardRequest.Buffer;
    u8 *pReply = pRdrExt->SmartcardReply.Buffer;
    unsigned long ReplyLength = pRequest[4] + 2;
    unsigned int ret;
    //u16 address = (pRequest[2] << 8) | pRequest[3];

    //man_readb(address, pRdrExt->m_SCard.WireProtocol, pReply, pRequest[4]);

    pRdrExt->SmartcardReply.BufferLength = ReplyLength;

    ret = STATUS_SUCCESS;    
    if (ret == STATUS_SUCCESS) 
    {
        pReply[ReplyLength - 2] = 0x90;
        pReply[ReplyLength - 1] = 0x00;
    } 
    else 
    {
        pReply[ReplyLength - 2] = 0x62;
        pReply[ReplyLength - 1] = 0x81;
    }

    return SC_EXIT_OK;
}

static unsigned int SelectFile(READER_EXTENSION *pRdrExt)
{
    u8 *pRequest = pRdrExt->SmartcardRequest.Buffer;
    u8 *pReply = pRdrExt->SmartcardReply.Buffer;
    pRdrExt->SmartcardReply.BufferLength = 2;

    if ((pRequest[5] == 0x3F) && (pRequest[6] == 0x00)) 
    {
        pReply[0] = 0x90;
        pReply[1] = 0x00;
    } 
    else 
    {
        pReply[0] = 0x6A;
        pReply[1] = 0x82;
    }

    return SC_EXIT_OK;
}

static unsigned int BadCommand(READER_EXTENSION *pRdrExt)
{
    u8 *pReply = pRdrExt->SmartcardReply.Buffer;
    pRdrExt->SmartcardReply.BufferLength = 2;
    pReply[0] = 0x6E;
    pReply[1] = 0x00;

    return SC_EXIT_OK;
}

/*finishes the callback RDF_TRANSMIT for the RAW protocol*/
static unsigned int CBRawTransmit(READER_EXTENSION *pRdrExt)
{
    unsigned int ret;
    u8 *pRequest = pRdrExt->SmartcardRequest.Buffer;

    /* Transmit pRdrExt->SmartcardRequest.Buffer to smart card */
    pRdrExt->SmartcardReply.BufferLength = 0;

    /* Interindustry Commands */
    switch (pRequest[1]) {
    case 0xA4:
        ret = SelectFile(pRdrExt);
        dprintk("SelectFile\n");
        break;
    case 0xB0:
        ret = ReadBinary(pRdrExt);
        dprintk("ReadBinary\n");
        break;
    case 0xD6:
        ret = UpdateBinary(pRdrExt);
        dprintk("UpdateBinary\n");
        break;
    case 0x20:
        ret = VerifyData(pRdrExt);
        dprintk("VerifyData\n");
        break;
    case 0x24:
        ret = ChangeVerifyData(pRdrExt);
        dprintk("ChangeVerifyData\n");
        break;
    default:
        ret = BadCommand(pRdrExt);
    }

    dprintk("Function Complete\n");

    return ret;
}

/*callback handler for SMCLIB RDF_TRANSMIT*/
static unsigned int CBTransmit(READER_EXTENSION *pRdrExt)
{
    unsigned int ret;
    u8 protocol = pRdrExt->m_SCard.Protocol & 0x03;
    u16 apdulen;

    if ((protocol == SCARD_PROTOCOL_T0) || (protocol == SCARD_PROTOCOL_T1)) 
    {
        apdulen = pRdrExt->SmartcardRequest.BufferLength;
        if (apdulen < 5)
            pRdrExt->T1.cse = SC_APDU_CASE_1;
        else if (apdulen == 5)
            pRdrExt->T1.cse = SC_APDU_CASE_2_SHORT;
        else if ((apdulen - 5) == pRdrExt->SmartcardRequest.Buffer[4])
            pRdrExt->T1.cse = SC_APDU_CASE_3_SHORT;
        else
            pRdrExt->T1.cse = SC_APDU_CASE_4_SHORT;
    }

    switch (protocol) 
    {
        case SCARD_PROTOCOL_T0:
                ret = CBT0Transmit(pRdrExt);
                break;
        case SCARD_PROTOCOL_T1:
                ret = CBT1Transmit(pRdrExt);
                break;
        case SCARD_PROTOCOL_RAW:
                ret = CBRawTransmit(pRdrExt);    
                break;
        default:
            ret = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    return ret;
}

module_init(init_ozscrlx);
module_exit(exit_ozscrlx);
MODULE_LICENSE("GPL");
