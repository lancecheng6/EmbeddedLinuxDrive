// SPDX-License-Identifier: GPL-2.0
/*
 * icm20608.c
 *
 * ICM20608 6-axis IMU (gyroscope + accelerometer) SPI driver.
 *
 * Uses the Linux SPI subsystem (spi_device, spi_message, spi_transfer)
 * and the miscdevice framework for simplified char device registration.
 *
 * Platform: I.MX6ULL (ALIENTEK ATK-IMX6U)
 * Sensor:   ICM20608 @ SPI bus, CS0
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "icm20608reg.h"

#define ICM20608_NAME	"icm20608_spi"

struct icm20608_dev {
	struct miscdevice misc;		/* MISC device                  */
	void *private_data;		/* SPI device                   */
	signed int gyro_x_adc;		/* gyroscope X axis raw         */
	signed int gyro_y_adc;		/* gyroscope Y axis raw         */
	signed int gyro_z_adc;		/* gyroscope Z axis raw         */
	signed int accel_x_adc;		/* accelerometer X axis raw     */
	signed int accel_y_adc;		/* accelerometer Y axis raw     */
	signed int accel_z_adc;		/* accelerometer Z axis raw     */
	signed int temp_adc;		/* temperature raw              */
};

static struct icm20608_dev icm20608dev;

/* Read multiple registers from ICM20608 via SPI */
static int icm20608_read_regs(struct icm20608_dev *dev, u8 reg, void *buf, int len)
{
	int ret = -1;
	unsigned char txdata[1];
	unsigned char *rxdata;
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = (struct spi_device *)dev->private_data;

	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	if (!t) return -ENOMEM;

	rxdata = kzalloc(sizeof(char) * len, GFP_KERNEL);
	if (!rxdata) { kfree(t); return -ENOMEM; }

	/* First byte = register address with bit8 set for read */
	txdata[0] = reg | 0x80;
	t->tx_buf = txdata;
	t->rx_buf = rxdata;
	t->len = len + 1;
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);
	if (!ret)
		memcpy(buf, rxdata + 1, len);

	kfree(rxdata);
	kfree(t);
	return ret;
}

/* Write multiple registers to ICM20608 via SPI */
static s32 icm20608_write_regs(struct icm20608_dev *dev, u8 reg, u8 *buf, u8 len)
{
	int ret = -1;
	unsigned char *txdata;
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = (struct spi_device *)dev->private_data;

	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	if (!t) return -ENOMEM;

	txdata = kzalloc(sizeof(char) + len, GFP_KERNEL);
	if (!txdata) { kfree(t); return -ENOMEM; }

	/* First byte = register address with bit8 cleared for write */
	*txdata = reg & ~0x80;
	memcpy(txdata + 1, buf, len);
	t->tx_buf = txdata;
	t->len = len + 1;
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);

	kfree(txdata);
	kfree(t);
	return ret;
}

/* Read a single register */
static unsigned char icm20608_read_onereg(struct icm20608_dev *dev, u8 reg)
{
	u8 data = 0;
	icm20608_read_regs(dev, reg, &data, 1);
	return data;
}

/* Write a single register */
static void icm20608_write_onereg(struct icm20608_dev *dev, u8 reg, u8 value)
{
	u8 buf = value;
	icm20608_write_regs(dev, reg, &buf, 1);
}

/* Read all sensor data (gyro + accel + temp) */
void icm20608_readdata(struct icm20608_dev *dev)
{
	unsigned char data[14] = { 0 };
	icm20608_read_regs(dev, ICM20_ACCEL_XOUT_H, data, 14);

	dev->accel_x_adc = (signed short)((data[0] << 8) | data[1]);
	dev->accel_y_adc = (signed short)((data[2] << 8) | data[3]);
	dev->accel_z_adc = (signed short)((data[4] << 8) | data[5]);
	dev->temp_adc    = (signed short)((data[6] << 8) | data[7]);
	dev->gyro_x_adc  = (signed short)((data[8] << 8) | data[9]);
	dev->gyro_y_adc  = (signed short)((data[10] << 8) | data[11]);
	dev->gyro_z_adc  = (signed short)((data[12] << 8) | data[13]);
}

/* Open device */
static int icm20608_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &icm20608dev;
	return 0;
}

/* Read sensor data: returns 7 signed ints (gyro3, accel3, temp) */
static ssize_t icm20608_read(struct file *filp, char __user *buf,
			     size_t cnt, loff_t *off)
{
	signed int data[7];
	long err = 0;
	struct icm20608_dev *dev = (struct icm20608_dev *)filp->private_data;

	icm20608_readdata(dev);
	data[0] = dev->gyro_x_adc;
	data[1] = dev->gyro_y_adc;
	data[2] = dev->gyro_z_adc;
	data[3] = dev->accel_x_adc;
	data[4] = dev->accel_y_adc;
	data[5] = dev->accel_z_adc;
	data[6] = dev->temp_adc;

	err = copy_to_user(buf, data, sizeof(data));
	return err ? -EFAULT : sizeof(data);
}

static int icm20608_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* File operations */
static const struct file_operations icm20608_ops = {
	.owner   = THIS_MODULE,
	.open    = icm20608_open,
	.read    = icm20608_read,
	.release = icm20608_release,
};

/* Initialize ICM20608 internal registers */
void icm20608_reginit(void)
{
	u8 value = 0;

	icm20608_write_onereg(&icm20608dev, ICM20_PWR_MGMT_1, 0x80);
	mdelay(50);
	icm20608_write_onereg(&icm20608dev, ICM20_PWR_MGMT_1, 0x01);
	mdelay(50);

	value = icm20608_read_onereg(&icm20608dev, ICM20_WHO_AM_I);
	printk("ICM20608 ID = %#X\r\n", value);

	icm20608_write_onereg(&icm20608dev, ICM20_SMPLRT_DIV, 0x00);
	icm20608_write_onereg(&icm20608dev, ICM20_GYRO_CONFIG, 0x18);
	icm20608_write_onereg(&icm20608dev, ICM20_ACCEL_CONFIG, 0x18);
	icm20608_write_onereg(&icm20608dev, ICM20_CONFIG, 0x04);
	icm20608_write_onereg(&icm20608dev, ICM20_ACCEL_CONFIG2, 0x04);
	icm20608_write_onereg(&icm20608dev, ICM20_PWR_MGMT_2, 0x00);
	icm20608_write_onereg(&icm20608dev, ICM20_LP_MODE_CFG, 0x00);
	icm20608_write_onereg(&icm20608dev, ICM20_FIFO_EN, 0x00);
}

/*
 * SPI probe: called when the driver matches a device tree node.
 * Initializes the SPI device, reads the sensor ID, and registers
 * the MISC device.
 */
static int icm20608_probe(struct spi_device *spi)
{
	int ret;

	/* Register MISC device (automatically creates /dev/icm20608) */
	icm20608dev.misc.minor = MISC_DYNAMIC_MINOR;
	icm20608dev.misc.name = ICM20608_NAME;
	icm20608dev.misc.fops = &icm20608_ops;
	ret = misc_register(&icm20608dev.misc);
	if (ret < 0) {
		printk("icm20608: misc_register failed!\n");
		return ret;
	}

	/* Configure SPI mode */
	spi->mode = SPI_MODE_0;
	spi_setup(spi);
	icm20608dev.private_data = spi;

	/* Initialize sensor registers */
	icm20608_reginit();

	printk("icm20608: probe success!\n");
	return 0;
}

/* SPI remove */
static int icm20608_remove(struct spi_device *spi)
{
	misc_deregister(&icm20608dev.misc);
	printk("icm20608: removed\n");
	return 0;
}

/* Legacy SPI device ID table */
static const struct spi_device_id icm20608_id[] = {
	{"alientek,icm20608", 0},
	{}
};

/* Device tree match table */
static const struct of_device_id icm20608_of_match[] = {
	{ .compatible = "alientek,icm20608" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, icm20608_of_match);

/* SPI driver structure */
static struct spi_driver icm20608_driver = {
	.probe    = icm20608_probe,
	.remove   = icm20608_remove,
	.driver   = {
		.owner   = THIS_MODULE,
		.name    = "icm20608_spi",
		.of_match_table = icm20608_of_match,
	},
	.id_table = icm20608_id,
};

module_spi_driver(icm20608_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("ICM20608 SPI IMU driver (gyroscope + accelerometer)");
