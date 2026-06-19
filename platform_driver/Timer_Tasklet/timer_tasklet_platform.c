// SPDX-License-Identifier: GPL-2.0
/*
 * timer_tasklet_platform.c
 *
 * Demonstrates the complete "Timer -> Tasklet -> GPIO toggling" pipeline:
 *
 *   Kernel timer fires every 10 seconds (simulates an interrupt source)
 *       | tasklet_schedule()
 *       v
 *   Tasklet executes (when CPU is available)
 *       | gpio_set_value() toggles
 *       v
 *   LED blinks automatically — no userspace app required
 *
 * Top half (timer callback):     does only tasklet_schedule()
 * Bottom half (tasklet callback): does the actual GPIO work
 *
 * Platform: I.MX6ULL (ALIENTEK ATK-IMX6U)
 * Target:   Embedded Linux driver learning
 */

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
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define TASKLETLED_CNT      1
#define TASKLETLED_NAME     "tasklet-led"
#define LEDOFF              0
#define LEDON               1

/* Device private structure */
struct taskletled_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    int led_gpio;
    struct timer_list timer;        /* kernel timer (interrupt source simulator) */
    struct tasklet_struct tasklet;  /* tasklet (bottom half) */
    int led_status;                 /* current LED state */
};

static struct taskletled_dev taskletled;

/*
 * Tasklet callback function — bottom half
 *
 * This is where the real work happens.
 * After the timer fires, tasklet_schedule() queues this function,
 * and the CPU executes it when it has free cycles.
 * Since this runs outside hardirq context, more complex operations
 * are possible here (though we keep it simple).
 */
static void led_tasklet_function(unsigned long arg)
{
    struct taskletled_dev *dev = (struct taskletled_dev *)arg;

    /* Toggle LED state */
    dev->led_status = !dev->led_status;
    gpio_set_value(dev->led_gpio, dev->led_status);

    printk("tasklet: LED toggled to %s\n",
           dev->led_status ? "OFF" : "ON");
}

/*
 * Timer callback function — top half
 *
 * Called by the kernel timer every 10 seconds.
 * This runs in softirq context and must not sleep.
 * It does only one thing: schedule the tasklet and re-arm the timer.
 */
static void timer_callback(unsigned long arg)
{
    struct taskletled_dev *dev = (struct taskletled_dev *)arg;

    /* Schedule the tasklet — "please run this when you get a chance" */
    tasklet_schedule(&dev->tasklet);

    /* Re-arm the timer for another 10 seconds */
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10000));
}

/*
 * Open the device node
 */
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &taskletled;
    return 0;
}

/*
 * Write to the device — manual LED control
 *
 * Writing '1' turns the LED on and stops the timer.
 * Writing '0' turns the LED off and stops the timer.
 * (The timer can be restarted by reloading the module.)
 */
static ssize_t led_write(struct file *filp, const char __user *buf,
                         size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    struct taskletled_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0) return -EFAULT;

    if (databuf[0] == LEDON) {
        gpio_set_value(dev->led_gpio, 0);   /* active low: 0 = ON */
        dev->led_status = 1;
        del_timer(&dev->timer);             /* stop timer when manually controlled */
    } else if (databuf[0] == LEDOFF) {
        gpio_set_value(dev->led_gpio, 1);   /* 1 = OFF */
        dev->led_status = 0;
        del_timer(&dev->timer);             /* stop timer when manually controlled */
    }
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations taskletled_fops = {
    .owner      = THIS_MODULE,
    .open       = led_open,
    .write      = led_write,
    .release    = led_release,
};

/*
 * Platform driver probe
 *
 * Called when the kernel matches this driver's of_match_table
 * against a device tree node.
 */
static int taskletled_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct device_node *nd = pdev->dev.of_node;

    printk("tasklet-led probe!\r\n");

    if (!nd) {
        dev_err(&pdev->dev, "Missing device tree node\n");
        return -EINVAL;
    }

    taskletled.led_gpio = of_get_named_gpio(nd, "led-gpio", 0);
    if (taskletled.led_gpio < 0) return -EINVAL;
    printk("led-gpio num = %d\r\n", taskletled.led_gpio);

    ret = gpio_request(taskletled.led_gpio, "tasklet-led");
    if (ret < 0) return ret;

    ret = gpio_direction_output(taskletled.led_gpio, 1);
    if (ret < 0) goto free_gpio;
    taskletled.led_status = 1;  /* init: LED off */

    /* Register character device */
    if (taskletled.major) {
        taskletled.devid = MKDEV(taskletled.major, 0);
        ret = register_chrdev_region(taskletled.devid,
                                     TASKLETLED_CNT, TASKLETLED_NAME);
    } else {
        ret = alloc_chrdev_region(&taskletled.devid, 0,
                                  TASKLETLED_CNT, TASKLETLED_NAME);
        taskletled.major = MAJOR(taskletled.devid);
        taskletled.minor = MINOR(taskletled.devid);
    }
    if (ret < 0) goto free_gpio;

    cdev_init(&taskletled.cdev, &taskletled_fops);
    ret = cdev_add(&taskletled.cdev, taskletled.devid, TASKLETLED_CNT);
    if (ret < 0) goto unregister_chrdev;

    taskletled.class = class_create(THIS_MODULE, TASKLETLED_NAME);
    if (IS_ERR(taskletled.class)) {
        ret = PTR_ERR(taskletled.class);
        goto del_cdev;
    }

    taskletled.device = device_create(taskletled.class, &pdev->dev,
                                       taskletled.devid, NULL, TASKLETLED_NAME);
    if (IS_ERR(taskletled.device)) {
        ret = PTR_ERR(taskletled.device);
        goto destroy_class;
    }

    /* Initialize kernel timer (fires every 10 seconds) */
    init_timer(&taskletled.timer);
    taskletled.timer.function = timer_callback;
    taskletled.timer.data = (unsigned long)&taskletled;

    /* Initialize tasklet */
    tasklet_init(&taskletled.tasklet, led_tasklet_function,
                 (unsigned long)&taskletled);

    /* Start the timer */
    mod_timer(&taskletled.timer, jiffies + msecs_to_jiffies(10000));
    printk("Timer + tasklet started! LED will toggle every 10s\r\n");

    return 0;

destroy_class:   class_destroy(taskletled.class);
del_cdev:        cdev_del(&taskletled.cdev);
unregister_chrdev:
    unregister_chrdev_region(taskletled.devid, TASKLETLED_CNT);
free_gpio:       gpio_free(taskletled.led_gpio);
    return ret;
}

/*
 * Platform driver remove
 *
 * Stops the timer, kills the tasklet, and releases all resources.
 */
static int taskletled_remove(struct platform_device *pdev)
{
    printk("tasklet-led remove!\r\n");

    del_timer(&taskletled.timer);           /* stop the timer */
    tasklet_kill(&taskletled.tasklet);      /* ensure tasklet is gone */

    gpio_set_value(taskletled.led_gpio, 1); /* LED off */

    device_destroy(taskletled.class, taskletled.devid);
    class_destroy(taskletled.class);
    cdev_del(&taskletled.cdev);
    unregister_chrdev_region(taskletled.devid, TASKLETLED_CNT);
    gpio_free(taskletled.led_gpio);

    return 0;
}

/* Match table for device tree binding */
static const struct of_device_id taskletled_of_match[] = {
    { .compatible = "atkalpha-gpioled" },
    { }
};
MODULE_DEVICE_TABLE(of, taskletled_of_match);

static struct platform_driver taskletled_platform_driver = {
    .driver = {
        .name           = "tasklet-led",
        .of_match_table = taskletled_of_match,
    },
    .probe      = taskletled_probe,
    .remove     = taskletled_remove,
};

module_platform_driver(taskletled_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("Timer -> Tasklet -> GPIO toggling demo");
