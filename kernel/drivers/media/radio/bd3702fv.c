/*
 * driver/media/radio/radio-bd3702fv.c
 *
 * Driver for TDA7703 radio chip for linux 2.6.
 * This driver is for TDA7703 chip from ST,used in car-radio from systech
 * The I2C protocol is used for communicate with chip.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define DRIVER_VERSION "v0.01"
#define RADIO_VERSION  KERNEL_VERSION(0, 0, 1)
#define DRIVER_NAME "bd3702fv"
#define DRIVER_CARD "ST TDA7703 AM/FM HIT"

#define DRIVER_AUTHOR "LiXinZhao <lxz@systech.com>"
#define DRIVER_DESC  "A driver for TDA7703 car-radio chip"

#define FREQ_MIN   87.5
#define FREQ_MAX   108
#define FREQ_MUL   16000


static int radio_nr = 3;
module_param(radio_nr, int, 0444);
MODULE_PARM_DESC(radio_nr, "Radio Nr");

struct bd3702fv_device {
	struct i2c_client		*client;
	struct video_device		*videodev;
	
	struct mutex			mutex;
	int 					users;
};

/*
 *	bd3702fv_ioctl - operate regs interface
 */
static int bd3702fv_ioctl(struct file *file, unsigned int cmd, void *buf)
{
	struct bd3702fv_device *radio = video_drvdata(file);
	int retval = 0;
	struct i2c_msg *msg = buf;

	msg->addr = radio->client->addr;
	msg->flags = 0;

	retval = i2c_transfer(radio->client->adapter, msg, 1);
	if(retval != 1)
		return -EIO;

	return 0;
}

static int bd3702fv_open(struct file *file)
{
	int minor = video_devdata(file)->minor;
	struct bd3702fv_device *radio = video_drvdata(file);

	if(radio->videodev->minor != minor)
		return -ENODEV;

	mutex_lock(&radio->mutex);
	if(radio->users) {
		mutex_unlock(&radio->mutex);
		return -EBUSY;
	}
	radio->users++;
	mutex_unlock(&radio->mutex);
	file->private_data = radio;

	return 0;
}

static int bd3702fv_close(struct file *file)
{
	struct bd3702fv_device *radio = video_drvdata(file);

	if(!radio)
		return -ENODEV;

	mutex_lock(&radio->mutex);
	radio->users--;
	mutex_unlock(&radio->mutex);

	return 0;
}


/*
 * file system interface
 */
static const struct v4l2_file_operations bd3702fv_fops = {
	.owner		=	THIS_MODULE,
	.open		=	bd3702fv_open,
	.release	=	bd3702fv_close,
	//.ioctl		=	video_ioctl2,
	.ioctl		=	bd3702fv_ioctl,
};


static const struct v4l2_ioctl_ops bd3702fv_ioctl_ops = {
};

static struct video_device bd3702fv_radio_template = {
	.name		=	DRIVER_NAME,
	.fops		=	&bd3702fv_fops,
	.ioctl_ops	=	&bd3702fv_ioctl_ops,
	.release	=	video_device_release,
};


static void bd3702fv_test(struct i2c_client *client)
	
{
	unsigned char addr[3] = {0x02,0x01};
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = addr,
		}
	};

	int ret;
	
	ret = i2c_transfer(client->adapter, msgs, 1);

return ;

}

/*
 *bd3702fv_i2c_probe - probe for the device
 */
static int __devinit bd3702fv_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct bd3702fv_device *radio;
	int retval = 0;

	radio = kzalloc(sizeof(struct bd3702fv_device), GFP_KERNEL);
	if (!radio) {
		return -ENOMEM;
	}
	radio->users = 0;
	radio->client = client;

	mutex_init(&radio->mutex);
	
	radio->videodev = video_device_alloc();
	if (!radio->videodev) {
		return -ENOMEM;
	}
	memcpy(radio->videodev, &bd3702fv_radio_template, 
			sizeof(bd3702fv_radio_template));
	video_set_drvdata(radio->videodev, radio);

	retval = video_register_device(radio->videodev, VFL_TYPE_RADIO, 
			radio_nr);
	if(retval) {
		printk("could not register video device\n");
		return retval;
	}
	i2c_set_clientdata(client, radio);
	printk("bd3702fv probe success!!\n");
	
	bd3702fv_test(client);
	
	return 0;
}

static __devexit int bd3702fv_i2c_remove(struct i2c_client *client)
{
	struct bd3702fv_device *radio = i2c_get_clientdata(client);

	video_unregister_device(radio->videodev);
	kfree(radio);
	i2c_set_clientdata(client, NULL);

	return 0;
}


/*
 * i2c subsystem interface
 */
static const struct i2c_device_id bd3702fv_i2c_id[] = {
	{ "bd3702fv", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bd3702fv_i2c_id);

/*
 *bd3702fv_i2c_driver - i2c driver interface
 */
static struct i2c_driver bd3702fv_i2c_driver = {
	.driver = {
		.name	=	"bd3702fv",
		.owner  =   THIS_MODULE,
	},
	.probe		=	bd3702fv_i2c_probe,
	.remove		=	__devexit_p(bd3702fv_i2c_remove),
	.id_table	=	bd3702fv_i2c_id,
};



/*************************************************************
 * Module Interface
 ************************************************************/
/*
 *bd3702fv_i2c_init - module init
 */
static __init int bd3702fv_i2c_init(void)
{
printk("$$$$$$$$ %s $$$$$$$$\n", __func__);
	return i2c_add_driver(&bd3702fv_i2c_driver);
}

/*
 *bd3702fv_i2c_exit - module exit
 */
static __exit void bd3702fv_i2c_exit(void)
{
	i2c_del_driver(&bd3702fv_i2c_driver);
}


module_init(bd3702fv_i2c_init);
module_exit(bd3702fv_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);


