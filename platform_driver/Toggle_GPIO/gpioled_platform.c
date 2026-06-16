#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

#define GPIOLED_CNT			1		  	/* Device number count */
#define GPIOLED_NAME		"gpioled"	/* Device node name (/dev/gpioled) */
#define LEDOFF 				0			/* Turn off LED */
#define LEDON 				1			/* Turn on LED */

/* gpioled device structure */
struct gpioled_dev {
	dev_t devid;				/* Device number */
	struct cdev cdev;			/* cdev */
	struct class *class;		/* class */
	struct device *device;		/* device */
	int major;					/* Major device number */
	int minor;					/* Minor device number */
	int led_gpio;				/* GPIO number used by the LED */
};

static struct gpioled_dev gpioled;	/* Global device structure */

/* Open device */
static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpioled; /* Set private data */
	return 0;
}

/* Write data to device (control LED on/off) */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;
	struct gpioled_dev *dev = filp->private_data;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	ledstat = databuf[0];		/* Get the status value from userspace */

	/* Note: Depending on the hardware circuit design, some LEDs light up with low level, some with high level */
	if(ledstat == LEDON) {
		gpio_set_value(dev->led_gpio, 0);	/* Output low level, turn on LED */
	} else if(ledstat == LEDOFF) {
		gpio_set_value(dev->led_gpio, 1);	/* Output high level, turn off LED */
	}
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* File operations structure */
static struct file_operations gpioled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
	.release = led_release,
};

/*
 * @description	: platform driver probe function
 * When the device tree's compatible property matches the driver, the kernel executes this function
 */
static int gpioled_probe(struct platform_device *pdev)
{
	int ret = 0;
	printk("gpioled driver and device matched successfully!\r\n");

	/* 1. Get device tree node from platform_device */
	struct device_node *nd = pdev->dev.of_node;
	if (!nd) {
		dev_err(&pdev->dev, "Missing device tree node\n");
		return -EINVAL;
	}

	/* 2. Get the gpio property from the device tree, obtain the GPIO number used by the LED */
	gpioled.led_gpio = of_get_named_gpio(nd, "led-gpio", 0);
	if(gpioled.led_gpio < 0) {
		dev_err(&pdev->dev, "can't get led-gpio\n");
		return -EINVAL;
	}
	printk("led-gpio num = %d\r\n", gpioled.led_gpio);

	/* 3. Request GPIO pin (this was missing in your old code; requesting ensures exclusive resource access) */
	ret = gpio_request(gpioled.led_gpio, "led-gpio");
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request gpio\n");
		return ret;
	}

	/* 4. Set GPIO to output mode with default high level (LED off) */
	ret = gpio_direction_output(gpioled.led_gpio, 1);
	if(ret < 0) {
		dev_err(&pdev->dev, "can't set gpio direction!\r\n");
		goto free_gpio;
	}

	/* 5. Register character device driver */
	if (gpioled.major) {		/* Static major device number defined */
		gpioled.devid = MKDEV(gpioled.major, 0);
		ret = register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
	} else {					/* Not defined, let the system assign automatically */
		ret = alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
		gpioled.major = MAJOR(gpioled.devid);
		gpioled.minor = MINOR(gpioled.devid);
	}
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to allocate chrdev region\n");
		goto free_gpio;
	}
	printk("gpioled major=%d,minor=%d\r\n", gpioled.major, gpioled.minor);

	/* 6. Initialize and add cdev */
	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &gpioled_fops);
	ret = cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to add cdev\n");
		goto unregister_chrdev;
	}

	/* 7. Create class */
	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(gpioled.class)) {
		ret = PTR_ERR(gpioled.class);
		goto del_cdev;
	}

	/* 8. Create device node, this will create gpioled under /dev/ */
	gpioled.device = device_create(gpioled.class, &pdev->dev, gpioled.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(gpioled.device)) {
		ret = PTR_ERR(gpioled.device);
		goto destroy_class;
	}

	return 0;

	/* Error handling goto blocks to ensure proper resource cleanup on failure */
destroy_class:
	class_destroy(gpioled.class);
del_cdev:
	cdev_del(&gpioled.cdev);
unregister_chrdev:
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
free_gpio:
	gpio_free(gpioled.led_gpio);
	return ret;
}

/*
 * @description	: platform driver remove function
 * Called when unloading the driver module
 */
static int gpioled_remove(struct platform_device *pdev)
{
	printk("gpioled driver remove!\r\n");
	
	/* Turn off LED before exiting */
	gpio_set_value(gpioled.led_gpio, 1);

	/* Unregister character device and release resources */
	device_destroy(gpioled.class, gpioled.devid);
	class_destroy(gpioled.class);
	cdev_del(&gpioled.cdev);
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
	gpio_free(gpioled.led_gpio); /* Release GPIO */

	return 0;
}

/* Device tree match table: must match the compatible property in DTS exactly */
static const struct of_device_id led_of_match[] = {
	{ .compatible = "atkalpha-gpioled" }, /* Match your device tree content */
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, led_of_match);

/* Platform driver structure */
static struct platform_driver led_platform_driver = {
	.driver = {
		.name           = "imx6ul-led",			/* Driver name */
		.of_match_table = led_of_match,			/* Include device tree match table */
	},
	.probe      = gpioled_probe,
	.remove     = gpioled_remove,
};

/* Macro: automatically generate module_init and module_exit and register platform_driver */
module_platform_driver(led_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("Modern Linux Platform Driver for LED using Pinctrl and GPIO Subsystem");