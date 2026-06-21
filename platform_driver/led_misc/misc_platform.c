// SPDX-License-Identifier: GPL-2.0
/*
 * misc_platform.c
 *
 * platform_driver + misc_register combined example
 *
 * platform_driver -> automatic DTB matching, probe/remove
 * misc_register   -> automatic /dev/ node creation (no cdev + class + device)
 *
 * Platform: I.MX6ULL (ALIENTEK ATK-IMX6U)
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define LEDOFF 0
#define LEDON  1

static int led_gpio;

/* ============ file_operations ============ */

static int led_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf,
			 size_t cnt, loff_t *offt)
{
	unsigned char databuf[1];

	if (copy_from_user(databuf, buf, cnt))
		return -EFAULT;

	if (databuf[0] == LEDON)
		gpio_set_value(led_gpio, 0);
	else
		gpio_set_value(led_gpio, 1);

	return cnt;
}

static struct file_operations led_fops = {
	.owner   = THIS_MODULE,
	.open    = led_open,
	.write   = led_write,
};

/* ============ MISC device ============ */

static struct miscdevice led_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "misc-led",		/* -> /dev/misc-led */
	.fops  = &led_fops,
};

/* ============ platform_driver ============ */

static int led_probe(struct platform_device *pdev)
{
	struct device_node *nd = pdev->dev.of_node;
	int ret;

	if (!nd) return -EINVAL;

	led_gpio = of_get_named_gpio(nd, "led-gpio", 0);
	if (led_gpio < 0) return -EINVAL;

	ret = gpio_request(led_gpio, "misc-led");
	if (ret < 0) return ret;

	gpio_direction_output(led_gpio, 1);

	ret = misc_register(&led_misc);
	if (ret < 0) {
		gpio_free(led_gpio);
		return ret;
	}

	dev_info(&pdev->dev, "misc-led registered (major=10, minor=dynamic)\n");
	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	misc_deregister(&led_misc);
	gpio_set_value(led_gpio, 1);
	gpio_free(led_gpio);
	return 0;
}

static const struct of_device_id led_of_match[] = {
	{ .compatible = "atkalpha-gpioled" },
	{ }
};
MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_driver = {
	.driver = {
		.name           = "misc-led",
		.of_match_table = led_of_match,
	},
	.probe  = led_probe,
	.remove = led_remove,
};

module_platform_driver(led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("platform_driver + misc_register demo");
