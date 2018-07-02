/*
 * drivers/net/phy/dp83848.c
 *
 * Driver for DP83848 PHYs
 *
 * Author: Wang Zheng
 *
 * Copyright (c) 2010 SILAN micro electronic co. LTD.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/netdevice.h>

#define DP83848_PHY_ID						0x20005c90
#define LAN8710_PHY_ID						0x00070cf1

static int dp83848_config_init(struct phy_device *phydev)
{
	int value, err;

	/* Software Reset PHY */
	value = phy_read(phydev, MII_BMCR);
	if (value < 0)
		return value;

	value |= BMCR_RESET;
	err = phy_write(phydev, MII_BMCR, value);
	if (err < 0)
		return err;

	do {
		value = phy_read(phydev, MII_BMCR);
	} while (value & BMCR_RESET);

	return 0;
}

static struct phy_driver dp83848_drivers = {
	.phy_id = LAN8710_PHY_ID,
	.phy_id_mask = 0xfffffff0,
	#if 0 /* dp83848 */
	.name = "NatSemi DP83848",
	.features = PHY_BASIC_FEATURES | SUPPORTED_Pause | SUPPORTED_Asym_Pause,
	#else /* lan8710 */
	.name = "LAN8710",
	.features = PHY_BASIC_FEATURES | SUPPORTED_Pause | SUPPORTED_Asym_Pause,
	#endif
	.flags = PHY_POLL,
	.config_init = dp83848_config_init,
	.config_aneg = genphy_config_aneg,
	.read_status = genphy_read_status,
	.suspend = genphy_suspend,
	.resume = genphy_resume,	
	.driver = { 
		.owner = THIS_MODULE 
	},
};

static int __init dp83848_init(void)
{
	printk("Ethernet phy LAN8710 registered\n");
	return phy_driver_register(&dp83848_drivers);
}

static void __exit dp83848_exit(void)
{
	phy_driver_unregister(&dp83848_drivers);
}

MODULE_DESCRIPTION("DP83848 PHY driver");
MODULE_AUTHOR("Wang Zheng");
MODULE_LICENSE("GPL");

module_init(dp83848_init);
module_exit(dp83848_exit);
