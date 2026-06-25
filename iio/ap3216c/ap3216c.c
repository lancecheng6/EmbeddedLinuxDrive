// SPDX-License-Identifier: GPL-2.0
/*
 * ap3216c.c — AP3216C I2C sensor driver using IIO (Industrial I/O)
 *
 * Demonstrates the Linux IIO framework for sensor drivers.
 * Instead of creating a custom /dev/ node with file_operations,
 * IIO provides a standardized interface via sysfs:
 *   cat /sys/bus/iio/devices/iio:deviceX/in_intensity_both_raw
 *
 * Uses regmap for I2C register access.
 *
 * Platform: I.MX6ULL
 * Sensor:   AP3216C @ I2C address 0x1e
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/unaligned/be_byteshift.h>
#include <linux/iio/trigger.h>
#include "ap3216creg.h"

#define AP3216C_NAME	"ap3216c_iio"

enum inv_icm20608_scan {
	AP3216C_ALS,
	AP3216C_PS,
	AP3216C_IR,
};

/* ALS resolution (lux * 1000000) */
static const int als_scale_ap3216c[] = {315000, 78800, 19700, 4900};

struct ap3216c_dev {
	struct i2c_client *client;
	struct regmap *regmap;
	struct regmap_config regmap_config;
};

/* Read a single register via regmap */
static int ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg, int *val)
{
	unsigned int data;
	int ret = regmap_read(dev->regmap, reg, &data);
	*val = (int)data;
	return ret;
}

static int ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 val)
{
	return regmap_write(dev->regmap, reg, val);
}

/* Read all sensor raw data */
static int ap3216c_readdata(struct ap3216c_dev *dev, u8 *buf)
{
	int i, val, ret;

	for (i = 0; i < 6; i++) {
		ret = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i, &val);
		if (ret < 0) return ret;
		buf[i] = (u8)val;
	}
	return 0;
}

/* IIO read_raw callback */
static int ap3216c_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long mask)
{
	int ret;
	u8 buf[6];
	struct ap3216c_dev *dev = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = ap3216c_readdata(dev, buf);
		if (ret < 0) return ret;

		switch (chan->channel2) {
		case IIO_MOD_LIGHT_BOTH:
			*val = ((int)buf[3] << 8) | buf[2];
			return IIO_VAL_INT;
		case IIO_MOD_LIGHT_IR:
			if (buf[0] & 0x80)
				*val = 0;
			else
				*val = ((int)buf[1] << 2) | (buf[0] & 0x03);
			return IIO_VAL_INT;
		case IIO_MOD_LIGHT_CLEAR:
			if (buf[4] & 0x40)
				*val = 0;
			else
				*val = ((int)(buf[5] & 0x3f) << 4) | (buf[4] & 0x0f);
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}

	case IIO_CHAN_INFO_SCALE:
		switch (chan->channel2) {
		case IIO_MOD_LIGHT_BOTH:
			ret = ap3216c_read_reg(dev, AP3216C_SYSTEMCONG, val);
			if (ret < 0) return ret;
			*val = als_scale_ap3216c[(*val) >> 2 & 3];
			*val2 = 1000000;
			return IIO_VAL_FRACTIONAL;
		default:
			return -EINVAL;
		}

	default:
		return -EINVAL;
	}
}

static const struct iio_info ap3216c_info = {
	.read_raw = ap3216c_read_raw,
};

static const struct iio_chan_spec ap3216c_channels[] = {
	{
		.type = IIO_INTENSITY,
		.channel2 = IIO_MOD_LIGHT_BOTH,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				     BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = AP3216C_ALS,
		.scan_type = {
			.sign = 'u',
			.realbits = 16,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		},
	}, {
		.type = IIO_INTENSITY,
		.channel2 = IIO_MOD_LIGHT_IR,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = AP3216C_PS,
		.scan_type = {
			.sign = 'u',
			.realbits = 10,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		},
	}, {
		.type = IIO_PROXIMITY,
		.channel2 = IIO_MOD_LIGHT_CLEAR,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.scan_index = AP3216C_IR,
		.scan_type = {
			.sign = 'u',
			.realbits = 10,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		},
	},
	IIO_CHAN_SOFT_TIMESTAMP(3),
};

static int ap3216c_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret;
	struct ap3216c_dev *dev;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*dev));
	if (!indio_dev)
		return -ENOMEM;

	dev = iio_priv(indio_dev);
	dev->client = client;

	/* Configure regmap */
	dev->regmap_config.reg_bits = 8;
	dev->regmap_config.val_bits = 8;
	dev->regmap = devm_regmap_init_i2c(client, &dev->regmap_config);
	if (IS_ERR(dev->regmap))
		return PTR_ERR(dev->regmap);

	/* Init IIO device */
	indio_dev->info = &ap3216c_info;
	indio_dev->name = AP3216C_NAME;
	indio_dev->channels = ap3216c_channels;
	indio_dev->num_channels = ARRAY_SIZE(ap3216c_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;

	/* Register IIO device */
	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "iio_device_register failed\n");
		return ret;
	}

	dev_info(&client->dev, "probe success\n");
	return 0;
}

static int ap3216c_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = dev_get_drvdata(&client->dev);
	iio_device_unregister(indio_dev);
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
		.owner = THIS_MODULE,
		.name  = AP3216C_NAME,
		.of_match_table = ap3216c_of_match,
	},
	.id_table = ap3216c_id,
};

module_i2c_driver(ap3216c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("AP3216C IIO driver (I2C + regmap)");
