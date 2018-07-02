/*
 * driver/media/radio/radio-tef6621.c
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
#define DRIVER_NAME "tef6621"
#define DRIVER_CARD "ST TDA7703 AM/FM HIT"

#define DRIVER_AUTHOR "LiXinZhao <lxz@systech.com>"
#define DRIVER_DESC  "A driver for TDA7703 car-radio chip"

#define FREQ_MIN   87.5
#define FREQ_MAX   108
#define FREQ_MUL   16000


static int radio_nr = 1;
module_param(radio_nr, int, 0444);
MODULE_PARM_DESC(radio_nr, "Radio Nr");

struct tef6621_device {
	struct i2c_client		*client;
	struct video_device		*videodev;
	
	struct mutex			mutex;
	int 					users;
};

/*
 *	tef6621_ioctl - operate regs interface
 */
static long tef6621_ioctl(struct file *file, unsigned int cmd, void *buf)
{
	struct tef6621_device *radio = video_drvdata(file);
	int retval = 0;
	struct i2c_msg *msg = buf;
	
	msg->addr = radio->client->addr;
	msg->flags = cmd;

	retval = i2c_transfer(radio->client->adapter, msg, 1);
	if(retval != 1)
		return -EIO;

	return 0;
}


/*
 *	tef6621_vidioc_g_tuner   -  get tuner attributes
 */
/*
static int tef6621_vidioc_g_tuner(struct file *file, void *priv,
		struct v4l2_tuner *tuner)
{
	struct tef6621_device *radio = video_drvdata(file);
	int retval = 0;

	strcpy(tuner->name, "FM");
	tuner->type = V4L2_TUNER_RADIO;

	tuner->capability = V4L2_TUNER_CAP_LOW | V4L2_TUNER_CAP_STEREO;
	tuner->rangelow	= 87.5 * FREQ_MUL;
	tuner->rangehigh = 108 * FREQ_MUL;


}
*/


/*
 *	tef6621_vidioc_g_audio  - get audio attributes
 */
/*
static int tef6621_vidioc_g_audio(struct file *file, void *priv, 
		struct v4l2_audio *audio)
{
	audio->index = 0;
	strcpy(audio->name, "Radio");
	audio->capability = V4L2_AUDCAP_STEREO;
	audio->mode = 0;

	return 0;
}
*/
//????

static int tef6621_open(struct file *file)
{
	int minor = video_devdata(file)->minor;
	struct tef6621_device *radio = video_drvdata(file);

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

static int tef6621_close(struct file *file)
{
	struct tef6621_device *radio = video_drvdata(file);

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
static const struct v4l2_file_operations tef6621_fops = {
	.owner		=	THIS_MODULE,
	.open		=	tef6621_open,
	.release	=	tef6621_close,
	//.ioctl		=	video_ioctl2,
	.ioctl		=	tef6621_ioctl,
};


static const struct v4l2_ioctl_ops tef6621_ioctl_ops = {
/*	.vidioc_querycap	=	tef6621_vidioc_querycap,
	.vidioc_queryctrl	=	tef6621_vidioc_queryctrl,
	.vidioc_g_ctrl		=	tef6621_vidioc_g_ctrl,
	.vidioc_s_ctrl		=	tef6621_vidioc_s_ctrl,
	.vidioc_g_audio		=	tef6621_vidioc_g_audio,
	.vidioc_g_tuner		=	tef6621_vidioc_g_tuner,
	.vidioc_s_tuner		=	tef6621_vidioc_s_tuner,
	.vidioc_g_frequency	=	tef6621_vidioc_g_frequency,
	.vidioc_s_frequency	=	tef6621_vidioc_s_frequency,
	.vidioc_s_hw_freq_seek	=	tef6621_vidioc_s_hw_freq_seek,*/
};

static struct video_device tef6621_radio_template = {
	.name		=	DRIVER_NAME,
	.fops		=	&tef6621_fops,
	.ioctl_ops	=	&tef6621_ioctl_ops,
	.release	=	video_device_release,
};


static void tef6621_test(struct i2c_client *client)
{
	unsigned char buf[5];
	struct i2c_msg msgs[1] = {
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 5,
			.buf = buf,
		}
	};
	int ret;
	int i;

	ret = i2c_transfer(client->adapter, msgs, 1);
	
	for(i = 0; i < 5; i++)
		printk("%x\n", buf[i]);

return ;

}

/*
 *tef6621_i2c_probe - probe for the device
 */
static int __devinit tef6621_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct tef6621_device *radio;
	int retval = 0;

	radio = kzalloc(sizeof(struct tef6621_device), GFP_KERNEL);
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
	memcpy(radio->videodev, &tef6621_radio_template, 
			sizeof(tef6621_radio_template));
	video_set_drvdata(radio->videodev, radio);

	retval = video_register_device(radio->videodev, VFL_TYPE_RADIO, 
			radio_nr);
	if(retval) {
		printk("could not register video device\n");
		return retval;
	}
	i2c_set_clientdata(client, radio);
	printk("tef6621 probe success!!\n");

	tef6621_test(client);

	return 0;
}

static __devexit int tef6621_i2c_remove(struct i2c_client *client)
{
	struct tef6621_device *radio = i2c_get_clientdata(client);

	video_unregister_device(radio->videodev);
	kfree(radio);
	i2c_set_clientdata(client, NULL);

	return 0;
}


/*
 * i2c subsystem interface
 */
static const struct i2c_device_id tef6621_i2c_id[] = {
	{ "tef6621", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tef6621_i2c_id);

/*
 *tef6621_i2c_driver - i2c driver interface
 */
static struct i2c_driver tef6621_i2c_driver = {
	.driver = {
		.name	=	"tef6621",
		.owner  =   THIS_MODULE,
	},
	.probe		=	tef6621_i2c_probe,
	.remove		=	__devexit_p(tef6621_i2c_remove),
	.id_table	=	tef6621_i2c_id,
};



/*************************************************************
 * Module Interface
 ************************************************************/
/*
 *tef6621_i2c_init - module init
 */
static int __init tef6621_i2c_init(void)
{
printk("$$$$$$$$ %s $$$$$$$$\n", __func__);
	return i2c_add_driver(&tef6621_i2c_driver);
}

/*
 *tef6621_i2c_exit - module exit
 */
static void __exit tef6621_i2c_exit(void)
{
	i2c_del_driver(&tef6621_i2c_driver);
}


module_init(tef6621_i2c_init);
module_exit(tef6621_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);


