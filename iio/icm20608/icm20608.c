// SPDX-License-Identifier: GPL-2.0
/*
 * icm20608.c — ICM20608 SPI IMU driver using IIO (Industrial I/O)
 *
 * Demonstrates the Linux IIO framework for sensor drivers.
 * Provides standardized sysfs interface:
 *   cat /sys/bus/iio/devices/iio:deviceX/in_anglvel_x_raw
 *   cat /sys/bus/iio/devices/iio:deviceX/in_accel_x_raw
 *   cat /sys/bus/iio/devices/iio:deviceX/in_temp_raw
 *
 * Uses regmap + IIO for clean, bus-agnostic register access
 * with a standardized user-space interface.
 *
 * Platform: I.MX6ULL (ALIENTEK ATK-IMX6U)
 * Sensor:   ICM20608 @ SPI, CS0
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger.h>
#include "icm20608reg.h"

#define ICM20608_NAME	"icm20608_iio"

struct icm20608_dev {
	struct regmap *regmap;
	struct regulator *vddio;
	signed int gyro_x_adc, gyro_y_adc, gyro_z_adc;
	signed int accel_x_adc, accel_y_adc, accel_z_adc;
	signed int temp_adc;
};

enum inv_icm20608_scan {
	ICM20608_TIMESTAMP,
	ICM20608_GYRO_X,
	ICM20608_ACCEL_X,
	ICM20608_ACCEL_Y,
	ICM20608_ACCEL_Z,
	ICM20608_TEMP,
};

static int icm20608_read_reg(struct icm20608_dev *dev, u8 reg, int *val)
{
	unsigned int data;
	int ret = regmap_read(dev->regmap, reg, &data);
	*val = data;
	return ret;
}

static int icm20608_write_reg(struct icm20608_dev *dev, u8 reg, u8 val)
{
	return regmap_write(dev->regmap, reg, val);
}

/* Read all sensor data (gyro + accel + temp = 14 bytes) */
static int icm20608_readdata(struct icm20608_dev *dev)
{
	int ret, val;
	u8 data[14];
	int i;

	for (i = 0; i < 14; i++) {
		ret = icm20608_read_reg(dev, ICM20_ACCEL_XOUT_H + i, &val);
		if (ret < 0) return ret;
		data[i] = (u8)val;
	}

	dev->accel_x_adc = (signed short)((data[0] << 8) | data[1]);
	dev->accel_y_adc = (signed short)((data[2] << 8) | data[3]);
	dev->accel_z_adc = (signed short)((data[4] << 8) | data[5]);
	dev->temp_adc    = (signed short)((data[6] << 8) | data[7]);
	dev->gyro_x_adc  = (signed short)((data[8] << 8) | data[9]);
	dev->gyro_y_adc  = (signed short)((data[10] << 8) | data[11]);
	dev->gyro_z_adc  = (signed short)((data[12] << 8) | data[13]);

	return 0;
}

/* IIO read_raw callback */
static int icm20608_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int *val, int *val2, long mask)
{
	int ret;
	struct icm20608_dev *dev = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = icm20608_readdata(dev);
		if (ret < 0) return ret;

		switch (chan->channel2) {
		case IIO_MOD_X:
			*val = (chan->type == IIO_ANGL_VEL) ?
				dev->gyro_x_adc : dev->accel_x_adc;
			return IIO_VAL_INT;
		case IIO_MOD_Y:
			*val = (chan->type == IIO_ANGL_VEL) ?
				dev->gyro_y_adc : dev->accel_y_adc;
			return IIO_VAL_INT;
		case IIO_MOD_Z:
			*val = (chan->type == IIO_ANGL_VEL) ?
				dev->gyro_z_adc : dev->accel_z_adc;
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ANGL_VEL:
			*val = 0;
			*val2 = 598;  /* 59.8 dps/digit */
			return IIO_VAL_INT_PLUS_MICRO;
		case IIO_ACCEL:
			*val = 0;
			*val2 = 598;  /* 0.598 mg/digit */
			return IIO_VAL_INT_PLUS_MICRO;
		case IIO_TEMP:
			*val = 0;
			*val2 = 598;  /* 0.598 degC/digit */
			return IIO_VAL_INT_PLUS_MICRO;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_OFFSET:
		switch (chan->type) {
		case IIO_TEMP:
			*val = 25;
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	default:
		return -EINVAL;
	}
}

static int icm20608_write_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan,
			      int val, int val2, long mask)
{
	return 0;
}

static const struct iio_info icm20608_info = {
	.read_raw  = icm20608_read_raw,
	.write_raw = icm20608_write_raw,
};

static const struct iio_chan_spec icm20608_channels[] = {
	{
		.type = IIO_ANGL_VEL,
		.modified = 1,
		.channel2 = IIO_MOD_X,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = ICM20608_GYRO_X,
		.scan_type = { .sign = 's', .realbits = 16,
			       .storagebits = 16, .shift = 0,
			       .endianness = IIO_LE },
	}, {
		.type = IIO_ANGL_VEL,
		.modified = 1,
		.channel2 = IIO_MOD_Y,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = ICM20608_GYRO_X + 1,
		.scan_type = { .sign = 's', .realbits = 16,
			       .storagebits = 16, .shift = 0,
			       .endianness = IIO_LE },
	}, {
		.type = IIO_ANGL_VEL,
		.modified = 1,
		.channel2 = IIO_MOD_Z,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = ICM20608_GYRO_X + 2,
		.scan_type = { .sign = 's', .realbits = 16,
			       .storagebits = 16, .shift = 0,
			       .endianness = IIO_LE },
	}, {
		.type = IIO_ACCEL,
		.modified = 1,
		.channel2 = IIO_MOD_X,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = ICM20608_ACCEL_X,
		.scan_type = { .sign = 's', .realbits = 16,
			       .storagebits = 16, .shift = 0,
			       .endianness = IIO_LE },
	}, {
		.type = IIO_ACCEL,
		.modified = 1,
		.channel2 = IIO_MOD_Y,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = ICM20608_ACCEL_X + 1,
		.scan_type = { .sign = 's', .realbits = 16,
			       .storagebits = 16, .shift = 0,
			       .endianness = IIO_LE },
	}, {
		.type = IIO_ACCEL,
		.modified = 1,
		.channel2 = IIO_MOD_Z,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = ICM20608_ACCEL_X + 2,
		.scan_type = { .sign = 's', .realbits = 16,
			       .storagebits = 16, .shift = 0,
			       .endianness = IIO_LE },
	}, {
		.type = IIO_TEMP,
		.modified = 0,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				      BIT(IIO_CHAN_INFO_OFFSET),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = ICM20608_TEMP,
		.scan_type = { .sign = 's', .realbits = 16,
			       .storagebits = 16, .shift = 0,
			       .endianness = IIO_LE },
	},
	IIO_CHAN_SOFT_TIMESTAMP(7),
};

static int icm20608_probe(struct spi_device *spi)
{
	int ret;
	struct icm20608_dev *dev;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*dev));
	if (!indio_dev)
		return -ENOMEM;

	dev = iio_priv(indio_dev);

	/* Configure regmap with SPI read flag mask */
	static const struct regmap_config regmap_config = {
		.reg_bits = 8,
		.val_bits = 8,
		.read_flag_mask = 0x80,
	};

	dev->regmap = devm_regmap_init_spi(spi, &regmap_config);
	if (IS_ERR(dev->regmap))
		return PTR_ERR(dev->regmap);

	/* Init IIO device */
	indio_dev->info = &icm20608_info;
	indio_dev->name = ICM20608_NAME;
	indio_dev->channels = icm20608_channels;
	indio_dev->num_channels = ARRAY_SIZE(icm20608_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;

	/* Register IIO device */
	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&spi->dev, "iio_device_register failed\n");
		return ret;
	}

	spi->mode = SPI_MODE_0;
	spi_setup(spi);

	/* Init ICM20608 registers */
	icm20608_write_reg(dev, ICM20_PWR_MGMT_1, 0x80);
	mdelay(50);
	icm20608_write_reg(dev, ICM20_PWR_MGMT_1, 0x01);
	mdelay(50);

	dev_info(&spi->dev, "ICM20608 ID = %X\n",
		 icm20608_read_reg(dev, ICM20_WHO_AM_I));

	icm20608_write_reg(dev, ICM20_SMPLRT_DIV, 0x00);
	icm20608_write_reg(dev, ICM20_GYRO_CONFIG, 0x18);
	icm20608_write_reg(dev, ICM20_ACCEL_CONFIG, 0x18);
	icm20608_write_reg(dev, ICM20_CONFIG, 0x04);
	icm20608_write_reg(dev, ICM20_ACCEL_CONFIG2, 0x04);
	icm20608_write_reg(dev, ICM20_PWR_MGMT_2, 0x00);
	icm20608_write_reg(dev, ICM20_LP_MODE_CFG, 0x00);
	icm20608_write_reg(dev, ICM20_FIFO_EN, 0x00);

	dev_info(&spi->dev, "probe success\n");
	return 0;
}

static int icm20608_remove(struct spi_device *spi)
{
	return 0;
}

static const struct spi_device_id icm20608_id[] = {
	{"alientek,icm20608", 0},
	{}
};
MODULE_DEVICE_TABLE(spi, icm20608_id);

static const struct of_device_id icm20608_of_match[] = {
	{ .compatible = "alientek,icm20608" },
	{ }
};
MODULE_DEVICE_TABLE(of, icm20608_of_match);

static struct spi_driver icm20608_driver = {
	.probe  = icm20608_probe,
	.remove = icm20608_remove,
	.driver = {
		.name    = ICM20608_NAME,
		.of_match_table = icm20608_of_match,
	},
	.id_table = icm20608_id,
};

module_spi_driver(icm20608_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("ICM20608 IIO driver (SPI + regmap)");
