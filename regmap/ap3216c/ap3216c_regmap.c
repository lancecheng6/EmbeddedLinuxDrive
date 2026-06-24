// SPDX-License-Identifier: GPL-2.0
/*
 * ap3216c_regmap.c — AP3216C I2C sensor driver with regmap API
 *
 * Modern style: i2c_driver + of_match_table + misc_register
 * Uses regmap for simplified register access.
 *
 * Sensor:   AP3216C @ I2C address 0x1e
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/regmap.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "ap3216creg.h"

#define AP3216C_NAME	"ap3216c_regmap"

struct ap3216c_dev {
	struct miscdevice misc;
	struct regmap *regmap;
	unsigned short ir, als, ps;
};

static struct ap3216c_dev ap3216cdev;

/* Read multiple registers via regmap */
static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg, void *val, int len)
{
	int ret;
	unsigned int data;
	int i;

	for (i = 0; i < len; i++) {
		ret = regmap_read(dev->regmap, reg + i, &data);
		if (ret < 0)
			return ret;
		((u8 *)val)[i] = (u8)data;
	}
	return 0;
}

/* Write multiple registers via regmap */
static int ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg, u8 *buf, u8 len)
{
	int ret;
	int i;

	for (i = 0; i < len; i++) {
		ret = regmap_write(dev->regmap, reg + i, buf[i]);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg)
{
	unsigned int data;
	regmap_read(dev->regmap, reg, &data);
	return (u8)data;
}

static int ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
	return regmap_write(dev->regmap, reg, data);
}

/* Read all sensor raw data */
static void ap3216c_readdata(struct ap3216c_dev *dev)
{
	unsigned char buf[6];
	int i;

	for (i = 0; i < 6; i++)
		buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);

	if (buf[0] & 0X80)
		dev->ir = 0;
	else
		dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03);

	dev->als = ((unsigned short)buf[3] << 8) | buf[2];

	if (buf[4] & 0x40)
		dev->ps = 0;
	else
		dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F);
}

static int ap3216c_open(struct inode *inode, struct file *filp)
{
	int ret;
	filp->private_data = &ap3216cdev;

	/* Reset sensor */
	ret = ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x04);
	if (ret < 0) return ret;
	mdelay(50);

	/* Enable ALS, PS+IR */
	ret = ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x03);
	if (ret < 0) return ret;

	return 0;
}

static ssize_t ap3216c_read(struct file *filp, char __user *buf,
			    size_t cnt, loff_t *off)
{
	short data[3];
	struct ap3216c_dev *dev = (struct ap3216c_dev *)filp->private_data;

	ap3216c_readdata(dev);

	data[0] = dev->ir;
	data[1] = dev->als;
	data[2] = dev->ps;

	if (copy_to_user(buf, data, sizeof(data)))
		return -EFAULT;
	return sizeof(data);
}

static int ap3216c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations ap3216c_ops = {
	.owner   = THIS_MODULE,
	.open    = ap3216c_open,
	.read    = ap3216c_read,
	.release = ap3216c_release,
};

static int ap3216c_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret;

	/* Configure regmap */
	static const struct regmap_config regmap_config = {
		.reg_bits = 8,
		.val_bits = 8,
	};

	ap3216cdev.regmap = devm_regmap_init_i2c(client, &regmap_config);
	if (IS_ERR(ap3216cdev.regmap))
		return PTR_ERR(ap3216cdev.regmap);

	/* Register MISC device */
	ap3216cdev.misc.minor = MISC_DYNAMIC_MINOR;
	ap3216cdev.misc.name = AP3216C_NAME;
	ap3216cdev.misc.fops = &ap3216c_ops;
	ret = misc_register(&ap3216cdev.misc);
	if (ret < 0) {
		pr_err("ap3216c_regmap: misc_register failed!\n");
		return ret;
	}

	dev_info(&client->dev, "probe success (regmap)\n");
	return 0;
}

static int ap3216c_remove(struct i2c_client *client)
{
	misc_deregister(&ap3216cdev.misc);
	return 0;
}

static const struct i2c_device_id ap3216c_id[] = {
	{"ap3216c", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ap3216c_id);

static const struct of_device_id ap3216c_of_match[] = {
	{ .compatible = "ap3216c" },
	{ }
};
MODULE_DEVICE_TABLE(of, ap3216c_of_match);

static struct i2c_driver ap3216c_driver = {
	.probe    = ap3216c_probe,
	.remove   = ap3216c_remove,
	.driver   = {
		.name    = AP3216C_NAME,
		.of_match_table = ap3216c_of_match,
	},
	.id_table = ap3216c_id,
};

module_i2c_driver(ap3216c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("AP3216C I2C sensor driver with regmap");
