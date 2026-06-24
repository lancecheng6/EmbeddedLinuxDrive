// SPDX-License-Identifier: GPL-2.0
/*
 * icm20608_regmap.c — ICM20608 SPI IMU driver with regmap API
 *
 * Modern style: spi_driver + of_match_table + misc_register
 * Uses regmap for simplified register access.
 *
 * Platform: I.MX6ULL
 * Sensor:   ICM20608 @ SPI, CS0
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/spi/spi.h>
#include <linux/miscdevice.h>
#include <linux/regmap.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "icm20608reg.h"

#define ICM20608_NAME	"icm20608_regmap"

struct icm20608_dev {
	struct miscdevice misc;
	struct regmap *regmap;
	signed int gyro_x_adc, gyro_y_adc, gyro_z_adc;
	signed int accel_x_adc, accel_y_adc, accel_z_adc;
	signed int temp_adc;
};

static struct icm20608_dev icm20608dev;

/* Read a single register via regmap */
static unsigned char icm20608_read_reg(u8 reg)
{
	unsigned int data;
	regmap_read(icm20608dev.regmap, reg, &data);
	return (u8)data;
}

/* Write a single register via regmap */
static void icm20608_write_reg(u8 reg, u8 value)
{
	regmap_write(icm20608dev.regmap, reg, value);
}

/* Read all sensor data */
static void icm20608_readdata(struct icm20608_dev *dev)
{
	/* Read 14 bytes starting from ACCEL_XOUT_H */
	int i;
	u8 data[14];

	for (i = 0; i < 14; i++)
		data[i] = icm20608_read_reg(ICM20_ACCEL_XOUT_H + i);

	dev->accel_x_adc = (signed short)((data[0] << 8) | data[1]);
	dev->accel_y_adc = (signed short)((data[2] << 8) | data[3]);
	dev->accel_z_adc = (signed short)((data[4] << 8) | data[5]);
	dev->temp_adc    = (signed short)((data[6] << 8) | data[7]);
	dev->gyro_x_adc  = (signed short)((data[8] << 8) | data[9]);
	dev->gyro_y_adc  = (signed short)((data[10] << 8) | data[11]);
	dev->gyro_z_adc  = (signed short)((data[12] << 8) | data[13]);
}

static int icm20608_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &icm20608dev;
	return 0;
}

static ssize_t icm20608_read(struct file *filp, char __user *buf,
			     size_t cnt, loff_t *off)
{
	signed int data[7];
	struct icm20608_dev *dev = (struct icm20608_dev *)filp->private_data;

	icm20608_readdata(dev);
	data[0] = dev->gyro_x_adc;
	data[1] = dev->gyro_y_adc;
	data[2] = dev->gyro_z_adc;
	data[3] = dev->accel_x_adc;
	data[4] = dev->accel_y_adc;
	data[5] = dev->accel_z_adc;
	data[6] = dev->temp_adc;

	if (copy_to_user(buf, data, sizeof(data)))
		return -EFAULT;
	return sizeof(data);
}

static int icm20608_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations icm20608_ops = {
	.owner   = THIS_MODULE,
	.open    = icm20608_open,
	.read    = icm20608_read,
	.release = icm20608_release,
};

/* Initialize ICM20608 registers */
static void icm20608_reginit(void)
{
	icm20608_write_reg(ICM20_PWR_MGMT_1, 0x80);
	mdelay(50);
	icm20608_write_reg(ICM20_PWR_MGMT_1, 0x01);
	mdelay(50);

	pr_info("ICM20608 ID = %#X\n", icm20608_read_reg(ICM20_WHO_AM_I));

	icm20608_write_reg(ICM20_SMPLRT_DIV, 0x00);
	icm20608_write_reg(ICM20_GYRO_CONFIG, 0x18);
	icm20608_write_reg(ICM20_ACCEL_CONFIG, 0x18);
	icm20608_write_reg(ICM20_CONFIG, 0x04);
	icm20608_write_reg(ICM20_ACCEL_CONFIG2, 0x04);
	icm20608_write_reg(ICM20_PWR_MGMT_2, 0x00);
	icm20608_write_reg(ICM20_LP_MODE_CFG, 0x00);
	icm20608_write_reg(ICM20_FIFO_EN, 0x00);
}

static int icm20608_probe(struct spi_device *spi)
{
	int ret;

	/* Configure regmap (SPI: read flag mask = 0x80 for MSB) */
	static const struct regmap_config regmap_config = {
		.reg_bits = 8,
		.val_bits = 8,
		.read_flag_mask = 0x80,
	};

	icm20608dev.regmap = devm_regmap_init_spi(spi, &regmap_config);
	if (IS_ERR(icm20608dev.regmap))
		return PTR_ERR(icm20608dev.regmap);

	/* Register MISC device */
	icm20608dev.misc.minor = MISC_DYNAMIC_MINOR;
	icm20608dev.misc.name = ICM20608_NAME;
	icm20608dev.misc.fops = &icm20608_ops;
	ret = misc_register(&icm20608dev.misc);
	if (ret < 0) {
		pr_err("icm20608_regmap: misc_register failed!\n");
		return ret;
	}

	spi->mode = SPI_MODE_0;
	spi_setup(spi);

	icm20608_reginit();

	dev_info(&spi->dev, "probe success (regmap)\n");
	return 0;
}

static int icm20608_remove(struct spi_device *spi)
{
	misc_deregister(&icm20608dev.misc);
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
	.probe    = icm20608_probe,
	.remove   = icm20608_remove,
	.driver   = {
		.name    = ICM20608_NAME,
		.of_match_table = icm20608_of_match,
	},
	.id_table = icm20608_id,
};

module_spi_driver(icm20608_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("ICM20608 SPI IMU driver with regmap");
