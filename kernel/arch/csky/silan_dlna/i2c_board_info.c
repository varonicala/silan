#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <sound/uda1380.h>

/*
 *	Register i2c board information into i2c bus,so driver can scan 
 *	i2c devices .There are three i2c controllers in suv soc,you can
 *	choose which bus to be registerred in ,each bus correspond with a 
 *	controller.
 *
 *	Specially,if you use silan_i2c_4_board_info to register,the device
 *	transfers data by gpio driver..
 */

static struct i2c_board_info __initdata silan_i2c_1_board_info[]= {
	
};

static struct i2c_board_info __initdata silan_i2c_2_board_info[]= {
};

static struct i2c_board_info __initdata silan_i2c_3_board_info[]= {
};

static int __init silan_i2c_board_init(void)
{
	i2c_register_board_info(0,silan_i2c_1_board_info,ARRAY_SIZE(silan_i2c_1_board_info));
	return 0; 
}

static void __exit silan_i2c_board_exit(void)
{
	
}

module_init(silan_i2c_board_init);
module_exit(silan_i2c_board_exit);


