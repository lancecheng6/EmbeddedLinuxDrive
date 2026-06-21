// SPDX-License-Identifier: GPL-2.0
/*
 * key_input_platform.c
 *
 * Ch58 input subsystem key driver — platform_driver version
 *
 * Uses the Linux standard input subsystem. Instead of creating a custom
 * /dev/ node, the kernel automatically creates /dev/input/eventX and
 * the application reads events using struct input_event.
 *
 * Application usage:
 *   struct input_event ev;
 *   read(fd, &ev, sizeof(ev));
 *   -> ev.type == EV_KEY, ev.code == KEY_0, ev.value == 1/0
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
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/input.h>

struct key_dev {
	int key_gpio;
	int irq_num;
	struct timer_list timer;
	struct input_dev *inputdev;
};

static struct key_dev keydev;

/* Timer callback: debounce + report key event to input subsystem */
static void key_timer_function(unsigned long arg)
{
	struct key_dev *dev = (struct key_dev *)arg;
	int value = gpio_get_value(dev->key_gpio);

	if (value == 0)
		input_report_key(dev->inputdev, KEY_0, 1);  /* pressed */
	else
		input_report_key(dev->inputdev, KEY_0, 0);  /* released */

	input_sync(dev->inputdev);  /* synchronize: event is complete */
}

/* Interrupt handler (top half) */
static irqreturn_t key_irq_handler(int irq, void *dev)
{
	mod_timer(&((struct key_dev *)dev)->timer,
		  jiffies + msecs_to_jiffies(10));
	return IRQ_HANDLED;
}

static int key_probe(struct platform_device *pdev)
{
	struct device_node *nd = pdev->dev.of_node;
	int ret;

	if (!nd) return -EINVAL;

	/* 1. Allocate input device */
	keydev.inputdev = input_allocate_device();
	if (!keydev.inputdev) {
		dev_err(&pdev->dev, "input_allocate_device failed!\n");
		return -ENOMEM;
	}

	/* 2. Set input device properties */
	keydev.inputdev->name = "key_input_platform";
	__set_bit(EV_KEY, keydev.inputdev->evbit);
	__set_bit(EV_REP, keydev.inputdev->evbit);
	__set_bit(KEY_0, keydev.inputdev->keybit);

	/* 3. Get GPIO */
	keydev.key_gpio = of_get_named_gpio(nd, "key-gpio", 0);
	if (keydev.key_gpio < 0) {
		ret = -EINVAL;
		goto fail_free;
	}

	ret = gpio_request(keydev.key_gpio, "key-input");
	if (ret < 0) goto fail_free;
	gpio_direction_input(keydev.key_gpio);

	/* 4. Get IRQ and request interrupt */
	keydev.irq_num = irq_of_parse_and_map(nd, 0);

	ret = request_irq(keydev.irq_num, key_irq_handler,
			  IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
			  "key-input", &keydev);
	if (ret < 0) {
		dev_err(&pdev->dev, "request_irq failed!\n");
		goto fail_gpio;
	}

	/* 5. Initialize debounce timer */
	init_timer(&keydev.timer);
	keydev.timer.function = key_timer_function;
	keydev.timer.data = (unsigned long)&keydev;

	/* 6. Register input device -> creates /dev/input/eventX */
	ret = input_register_device(keydev.inputdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "input_register_device failed!\n");
		goto fail_irq;
	}

	dev_info(&pdev->dev, "registered: /dev/input/eventX (KEY_0)\n");
	return 0;

fail_irq:
	free_irq(keydev.irq_num, &keydev);
	del_timer(&keydev.timer);
fail_gpio:
	gpio_free(keydev.key_gpio);
fail_free:
	input_free_device(keydev.inputdev);
	return ret;
}

static int key_remove(struct platform_device *pdev)
{
	del_timer(&keydev.timer);
	free_irq(keydev.irq_num, &keydev);
	input_unregister_device(keydev.inputdev);
	/* input_unregister_device internally calls input_free_device */
	gpio_free(keydev.key_gpio);
	return 0;
}

static const struct of_device_id key_of_match[] = {
	{ .compatible = "atkalpha-key" },
	{ }
};
MODULE_DEVICE_TABLE(of, key_of_match);

static struct platform_driver key_platform_driver = {
	.driver = {
		.name           = "key-input",
		.of_match_table = key_of_match,
	},
	.probe  = key_probe,
	.remove = key_remove,
};

module_platform_driver(key_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("Input subsystem key driver (platform_driver version)");
