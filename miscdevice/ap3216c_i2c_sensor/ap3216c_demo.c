// SPDX-License-Identifier: GPL-2.0
/*
 * ap3216c_demo.c
 *
 * AP3216C ambient light + proximity + IR sensor I2C driver.
 *
 * Uses the Linux I2C subsystem (i2c_client, i2c_msg, i2c_transfer)
 * and the miscdevice framework for simplified char device registration.
 *
 * Platform: I.MX6ULL
 * Sensor:   AP3216C @ I2C address 0x1e
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "ap3216creg.h"

#define AP3216C_NAME	"ap3216c_demo"

struct ap3216c_dev {
	struct miscdevice misc;		/* MISC device                  */
	struct device_node *nd;		/* device tree node             */
	void *private_data;		/* I2C client                   */
	unsigned short ir, als, ps;	/* IR, ambient light, proximity */
};

static struct ap3216c_dev ap3216cdev;

/*
 * Read multiple registers from AP3216C via I2C (using i2c_msg + i2c_transfer)
 */
static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg, void *val, int len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client *)dev->private_data;

	/* msg[0]: send the register address to read from */
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = &reg;
	msg[0].len = 1;

	/* msg[1]: read data */
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = val;
	msg[1].len = len;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret == 2) {
		ret = 0;
	} else {
		printk(KERN_ERR "ap3216c_demo: i2c rd failed=%d reg=%02x len=%d\n", ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}

/*
 * Write multiple registers to AP3216C via I2C
 */
static s32 ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg, u8 *buf, u8 len)
{
	u8 b[256];
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->private_data;

	b[0] = reg;
	memcpy(&b[1], buf, len);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.buf = b;
	msg.len = len + 1;

	return i2c_transfer(client->adapter, &msg, 1);
}

/* Read a single register */
static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg)
{
	u8 data = 0;
	ap3216c_read_regs(dev, reg, &data, 1);
	return data;
}

/* Write a single register */
static int ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
	return ap3216c_write_regs(dev, reg, &data, 1);
}

/*
 * Read all sensor raw data from AP3216C
 * Note: when ALS and IR+PS are both enabled, read interval must be > 112.5ms
 */
static void ap3216c_readdata(struct ap3216c_dev *dev)
{
	unsigned char i = 0;
	unsigned char buf[6];

	for (i = 0; i < 6; i++)
		buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);

	printk(KERN_DEBUG "ap3216c_demo: raw buf = %02x %02x %02x %02x %02x %02x\n",
	       buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	/* IR data: if IR_OF bit is set, data is invalid */
	if (buf[0] & 0X80)
		dev->ir = 0;
	else
		dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03);

	/* ALS data */
	dev->als = ((unsigned short)buf[3] << 8) | buf[2];

	/* PS data: if IR_OF bit is set, data is invalid */
	if (buf[4] & 0x40)
		dev->ps = 0;
	else
		dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F);
}

/* Open device: reset and enable the sensor */
static int ap3216c_open(struct inode *inode, struct file *filp)
{
	int ret;
	filp->private_data = &ap3216cdev;

	/* Reset AP3216C */
	ret = ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x04);
	printk(KERN_DEBUG "ap3216c_demo: open reset ret=%d\n", ret);
	mdelay(50);		/* reset requires at least 10ms */

	/* Enable ALS, PS+IR */
	ret = ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0X03);
	printk(KERN_DEBUG "ap3216c_demo: open enable ret=%d\n", ret);

	return 0;
}

/* Read sensor data: returns three signed shorts (ir, als, ps) */
static ssize_t ap3216c_read(struct file *filp, char __user *buf,
			    size_t cnt, loff_t *off)
{
	short data[3];
	long err = 0;
	struct ap3216c_dev *dev = (struct ap3216c_dev *)filp->private_data;

	ap3216c_readdata(dev);

	data[0] = dev->ir;
	data[1] = dev->als;
	data[2] = dev->ps;

	err = copy_to_user(buf, data, sizeof(data));
	printk(KERN_DEBUG "ap3216c_demo: read copy_to_user err=%ld, "
	       "ir=%d als=%d ps=%d\n", err, data[0], data[1], data[2]);
	return (err == 0) ? sizeof(data) : -EFAULT;
}

/* Release device */
static int ap3216c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* File operations */
static const struct file_operations ap3216c_ops = {
	.owner   = THIS_MODULE,
	.open    = ap3216c_open,
	.read    = ap3216c_read,
	.release = ap3216c_release,
};

/*
 * I2C probe: called when the driver matches a device tree node
 */
static int ap3216c_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret;

	printk(KERN_INFO "ap3216c_demo: probe called, addr=0x%02x\n",
	       client->addr);

	/* Register MISC device (automatically creates /dev/ap3216c_demo) */
	ap3216cdev.misc.minor = MISC_DYNAMIC_MINOR;
	ap3216cdev.misc.name = AP3216C_NAME;
	ap3216cdev.misc.fops = &ap3216c_ops;
	ret = misc_register(&ap3216cdev.misc);
	if (ret < 0) {
		printk(KERN_ERR "ap3216c_demo: misc_register failed!\n");
		return ret;
	}

	ap3216cdev.private_data = client;

	/* Test I2C communication: read the system configuration register */
	{
		u8 syscfg = ap3216c_read_reg(&ap3216cdev, AP3216C_SYSTEMCONG);
		printk(KERN_INFO "ap3216c_demo: SYSTEMCONG = 0x%02x\n", syscfg);
	}

	printk(KERN_INFO "ap3216c_demo: probe success!\n");
	return 0;
}

/*
 * I2C remove: called when the driver is unloaded
 */
static int ap3216c_remove(struct i2c_client *client)
{
	misc_deregister(&ap3216cdev.misc);
	return 0;
}

/* Legacy I2C device ID table */
static const struct i2c_device_id ap3216c_id[] = {
	{"ap3216c_demo", 0},
	{}
};

/* Device tree match table */
static const struct of_device_id ap3216c_of_match[] = {
	{ .compatible = "ap3216c" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, ap3216c_of_match);

/* I2C driver structure */
static struct i2c_driver ap3216c_driver = {
	.probe    = ap3216c_probe,
	.remove   = ap3216c_remove,
	.driver   = {
		.owner   = THIS_MODULE,
		.name    = "ap3216c_demo",
		.of_match_table = ap3216c_of_match,
	},
	.id_table = ap3216c_id,
};

static int __init ap3216c_init(void)
{
	return i2c_add_driver(&ap3216c_driver);
}

static void __exit ap3216c_exit(void)
{
	i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance");
MODULE_DESCRIPTION("AP3216C I2C sensor driver (ALS + PS + IR)");
