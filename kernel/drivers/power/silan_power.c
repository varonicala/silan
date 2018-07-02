/*
 * Power supply driver for silan.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/vermagic.h>

static int battery_status		= POWER_SUPPLY_STATUS_DISCHARGING;
static int battery_health		= POWER_SUPPLY_HEALTH_GOOD;
static int battery_present		= 1; /* true */
static int battery_technology		= POWER_SUPPLY_TECHNOLOGY_LION;
static int battery_capacity		= 50;


static int silan_power_get_battery_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
	switch (psp) 
	{
		case POWER_SUPPLY_PROP_MODEL_NAME:
			val->strval = "Silan battery";
			break;
		case POWER_SUPPLY_PROP_MANUFACTURER:
			val->strval = "Linux";
			break;
		case POWER_SUPPLY_PROP_SERIAL_NUMBER:
			val->strval = UTS_RELEASE;
			break;
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = battery_status;
			break;
		case POWER_SUPPLY_PROP_CHARGE_TYPE:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = battery_health;
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = battery_present;
			break;
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = battery_technology;
			break;
		case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
		case POWER_SUPPLY_PROP_CHARGE_NOW:
			val->intval = battery_capacity;
			break;
		case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		case POWER_SUPPLY_PROP_CHARGE_FULL:
			val->intval = 100;
			break;
		case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
		case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
			val->intval = 3600;
			break;
		default:
			pr_info("%s: some properties deliberately report errors.\n",
					__func__);
			return -EINVAL;
	}

	return 0;
}

static enum power_supply_property silan_power_battery_props[] = 
{
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
};

static struct power_supply silan_power_supplies[] = {
	{
		.name = "silan_battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = silan_power_battery_props,
		.num_properties = ARRAY_SIZE(silan_power_battery_props),
		.get_property = silan_power_get_battery_property,
	}, 
};


static int __init silan_power_init(void)
{
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(silan_power_supplies); i++) 
	{
		ret = power_supply_register(NULL, &silan_power_supplies[i]);
		if (ret) 
		{
			pr_err("%s: failed to register %s\n", __func__,
				silan_power_supplies[i].name);
			goto failed;
		}
	}

	return 0;
failed:
	while (--i >= 0)
		power_supply_unregister(&silan_power_supplies[i]);
	return ret;
}
module_init(silan_power_init);

static void __exit silan_power_exit(void)
{
	int i;

	/* Let's see how we handle changes... */
	for (i = 0; i < ARRAY_SIZE(silan_power_supplies); i++)
		power_supply_changed(&silan_power_supplies[i]);
	pr_info("%s: 'changed' event sent, sleeping for 10 seconds...\n",
		__func__);
	ssleep(10);

	for (i = 0; i < ARRAY_SIZE(silan_power_supplies); i++)
		power_supply_unregister(&silan_power_supplies[i]);
}
module_exit(silan_power_exit);
