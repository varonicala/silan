#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/rtc.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/bcd.h>
#include <linux/delay.h>

struct silan_rtc {
	struct rtc_device *rtc;
	unsigned char ctrl;
};

#define SILAN_RTC_DEUG 0

#define WR_RTC			0x0a
#define RD_RTC_DATA		0x3b
#define RTC_ADDR		0x0
#define SET_RTC_A		0x0c
#define ENABLE_RTC		0x03
#define SET_CLK_SRC		0x16
#define SRC_ENABLE      0x01


static struct i2c_driver silan_rtc_driver;

static int silan_rtc_src(struct i2c_client *client)
{
	unsigned char addr[2] = {SET_CLK_SRC, SRC_ENABLE};
	struct i2c_msg msgs[1] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = addr,
		}
	};
	int ret;

	ret = i2c_transfer(client->adapter, msgs, 1);

	return ret;
}

static int silan_rtc_enable(struct i2c_client *client)
{
	unsigned char addr[2] = { SET_RTC_A, ENABLE_RTC};
	struct i2c_msg msgs[1] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = addr,
		}
	};
	int ret;

	ret = i2c_transfer(client->adapter, msgs, 1);

	return ret; 
}

static int silan_rtc_get_datetime(struct i2c_client *client, struct rtc_time *dt)
{
	unsigned char buf[8], addr[2] = { RD_RTC_DATA, RTC_ADDR};
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = addr,
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 17,
			.buf = buf,
		}
	};
	int ret;

	memset(buf, 0, sizeof(buf));

	ret = i2c_transfer(client->adapter, msgs, 2);
	
	if (ret == 2)
	{

#ifdef SILAN_RTC_DEBUG
		for(i = 0; i < 17; i++)
			printk("buf[%d]: %d\n",i, buf[i]);
#endif

		dt->tm_year = (buf[5] > 0 ? 100 : 0) + bcd2bin(buf[6]);
		dt->tm_wday = bcd2bin(buf[3]);

		dt->tm_sec = bcd2bin(buf[0]);
		dt->tm_min = bcd2bin(buf[1]);
		dt->tm_hour = bcd2bin(buf[2]);
		dt->tm_mday = bcd2bin(buf[4]);
		dt->tm_mon = bcd2bin(buf[5]);
	}

#ifdef SILAN_RTC_DEBUG
	printk("$$$$$$ %d-%d-%d  %d:%d:%d\n", dt->tm_year, dt->tm_mon, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec);
#endif

	return ret >= 0 ? 0 : -EIO;
}

static int silan_rtc_set_datetime(struct i2c_client *client, struct rtc_time *dt, int datetoo)
{
	unsigned char buf[16];
	int ret, len = 5;

#ifdef SILAN_RTC_DEBUG
	printk("Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
	         dt->tm_mday, dt->tm_mon+1, dt->tm_year + 1900,
	         dt->tm_hour, dt->tm_min, dt->tm_sec);
#endif	

	buf[0] = WR_RTC;
	buf[1] = RTC_ADDR;
	buf[2] = bin2bcd(dt->tm_sec);
	buf[3] = bin2bcd(dt->tm_min);
	buf[4] = bin2bcd(dt->tm_hour);

	if (datetoo) 
	{
		len = 9;
		buf[5] = bin2bcd(dt->tm_wday);
		buf[6] = bin2bcd(dt->tm_mday); 
		buf[7] = bin2bcd(dt->tm_mon);
		buf[8] = bin2bcd(dt->tm_year-100);
	}

	ret = i2c_master_send(client, (char *)buf, len);
	if (ret != len)
		return -EIO;

	return ret == len ? 0 : -EIO;
}

static int silan_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct i2c_client *client = to_i2c_client(dev);
	int err;

	err = silan_rtc_src(client);
	if(err < 0)
	{
		printk("Set clk src error\n");
		return -1;
	}
	udelay(1000);

	err = silan_rtc_enable(client);
	if(err < 0)
	{
		printk("Enable silan_rtc error\n");
		return -1;
	}
	udelay(1000);

	err = silan_rtc_get_datetime(client, tm); 
	if(err != 0)
	{
		printk("Get_datetime error!\n");
		return -1;
	}
	return 0;
}

static int silan_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct i2c_client *client = to_i2c_client(dev);
	int ret;

	ret = silan_rtc_set_datetime(client, tm, 1);
	if(ret != 0)
	{
		printk("Set_datetime error!\n");
		return -1;
	}
	return ret;
}

static int silan_alarm_read_time(struct device *dev, struct rtc_wkalrm *tm)
{
	return 0;
}

static int silan_alarm_set_time(struct device *dev, struct rtc_wkalrm *tm)
{
	return 0;
}

static const struct rtc_class_ops silan_rtc_rtc_ops = {
	.read_time	= silan_rtc_read_time,
	.set_time   = silan_rtc_set_time,
	.read_alarm = silan_alarm_read_time,
	.set_alarm  = silan_alarm_set_time,
};

static int silan_rtc_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct silan_rtc *silan_rtc;
	int err;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	silan_rtc = kzalloc(sizeof(struct silan_rtc), GFP_KERNEL);
	if (!silan_rtc)
		return -ENOMEM;

	i2c_set_clientdata(client, silan_rtc);

	silan_rtc->rtc = rtc_device_register(silan_rtc_driver.driver.name,
			&client->dev, &silan_rtc_rtc_ops, THIS_MODULE);

	if (IS_ERR(silan_rtc->rtc)) {
		err = PTR_ERR(silan_rtc->rtc);
		goto exit_kfree;
	}

	return 0;

exit_kfree:
	kfree(silan_rtc);
	return err;
}

static int __devexit silan_rtc_remove(struct i2c_client *client)
{
	struct silan_rtc *silan_rtc = i2c_get_clientdata(client);

	if (silan_rtc->rtc)
		rtc_device_unregister(silan_rtc->rtc);
	kfree(silan_rtc);
	return 0;
}

static const struct i2c_device_id silan_rtc_id[] = 
{
	{ "silan_rtc", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, silan_rtc_id);

static struct i2c_driver silan_rtc_driver = 
{
	.driver = {
		.name	= "silan_rtc",
		.owner	= THIS_MODULE,
	},
	.probe		= silan_rtc_probe,
	.remove		= __devexit_p(silan_rtc_remove),
	.id_table	= silan_rtc_id,
};

static __init int silan_rtc_init(void)
{
	return i2c_add_driver(&silan_rtc_driver);
}

static __exit void silan_rtc_exit(void)
{
	i2c_del_driver(&silan_rtc_driver);
}

module_init(silan_rtc_init);
module_exit(silan_rtc_exit);

MODULE_AUTHOR("Chen Jianneng");
MODULE_DESCRIPTION("SILAN I2C RTC driver");
MODULE_LICENSE("GPL");
