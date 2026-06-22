// SPDX-License-Identifier: GPL-2.0
/*
 * ramdisk_wreq.c — RAM disk block driver using Request Queue
 *
 * Method 1: Uses the full block I/O framework with an I/O scheduler.
 *   blk_init_queue() — creates a request queue with I/O scheduler
 *   ramdisk_request_fn() — processes requests from the queue
 *   ramdisk_transfer() — handles actual data transfer (memcpy)
 *
 * This approach is suitable for mechanical HDDs where request
 * merging and reordering improve performance due to seek times.
 * For RAM/flash, Method 2 (make_request) is more appropriate.
 *
 * Platform: I.MX6ULL
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define RAMDISK_SIZE	(2 * 1024 * 1024)	/* 2MB */
#define RAMDISK_NAME	"ramdisk_wreq"
#define RADMISK_MINOR	3			/* 3 partitions (p1, p2, p3) */

struct ramdisk_dev {
	int major;
	unsigned char *ramdiskbuf;	/* simulated storage in DDR */
	spinlock_t lock;
	struct gendisk *gendisk;
	struct request_queue *queue;
};

static struct ramdisk_dev ramdisk;

/* Open block device */
static int ramdisk_open(struct block_device *dev, fmode_t mode)
{
	printk("ramdisk_wreq: open\n");
	return 0;
}

/* Release block device */
static void ramdisk_release(struct gendisk *disk, fmode_t mode)
{
	printk("ramdisk_wreq: release\n");
}

/* Get disk geometry (heads/cylinders/sectors — legacy HDD concept) */
static int ramdisk_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
	geo->heads = 2;
	geo->cylinders = 32;
	geo->sectors = RAMDISK_SIZE / (2 * 32 * 512);
	return 0;
}

static struct block_device_operations ramdisk_fops = {
	.owner   = THIS_MODULE,
	.open    = ramdisk_open,
	.release = ramdisk_release,
	.getgeo  = ramdisk_getgeo,
};

/*
 * Transfer data for a single request.
 * blk_rq_pos() returns the sector number; shift left by 9 to get byte offset.
 */
static void ramdisk_transfer(struct request *req)
{
	unsigned long start = blk_rq_pos(req) << 9;
	unsigned long len   = blk_rq_cur_bytes(req);
	void *buffer = bio_data(req->bio);

	if (rq_data_dir(req) == READ)
		memcpy(buffer, ramdisk.ramdiskbuf + start, len);
	else if (rq_data_dir(req) == WRITE)
		memcpy(ramdisk.ramdiskbuf + start, buffer, len);
}

/*
 * Request handler: called by the I/O scheduler.
 * Fetches each request from the queue and processes it.
 */
static void ramdisk_request_fn(struct request_queue *q)
{
	int err = 0;
	struct request *req;

	req = blk_fetch_request(q);
	while (req) {
		ramdisk_transfer(req);
		if (!__blk_end_request_cur(req, err))
			req = blk_fetch_request(q);
	}
}

static int __init ramdisk_init(void)
{
	int ret = 0;
	printk("ramdisk_wreq: init\n");

	/* 1. Allocate storage buffer */
	ramdisk.ramdiskbuf = kzalloc(RAMDISK_SIZE, GFP_KERNEL);
	if (!ramdisk.ramdiskbuf) {
		ret = -ENOMEM;
		goto ram_fail;
	}

	/* 2. Initialize spinlock */
	spin_lock_init(&ramdisk.lock);

	/* 3. Register block device (kernel assigns major number) */
	ramdisk.major = register_blkdev(0, RAMDISK_NAME);
	if (ramdisk.major < 0) {
		ret = ramdisk.major;
		goto register_blkdev_fail;
	}
	printk("ramdisk_wreq: major = %d\n", ramdisk.major);

	/* 4. Allocate and initialize gendisk */
	ramdisk.gendisk = alloc_disk(RADMISK_MINOR);
	if (!ramdisk.gendisk) {
		ret = -ENOMEM;
		goto gendisk_alloc_fail;
	}

	/* 5. Allocate and initialize request queue (with I/O scheduler) */
	ramdisk.queue = blk_init_queue(ramdisk_request_fn, &ramdisk.lock);
	if (!ramdisk.queue) {
		ret = -ENOMEM;
		goto blk_init_fail;
	}

	/* 6. Register disk */
	ramdisk.gendisk->major = ramdisk.major;
	ramdisk.gendisk->first_minor = 0;
	ramdisk.gendisk->fops = &ramdisk_fops;
	ramdisk.gendisk->private_data = &ramdisk;
	ramdisk.gendisk->queue = ramdisk.queue;
	sprintf(ramdisk.gendisk->disk_name, RAMDISK_NAME);
	set_capacity(ramdisk.gendisk, RAMDISK_SIZE / 512);
	add_disk(ramdisk.gendisk);

	return 0;

blk_init_fail:
	put_disk(ramdisk.gendisk);
gendisk_alloc_fail:
	unregister_blkdev(ramdisk.major, RAMDISK_NAME);
register_blkdev_fail:
	kfree(ramdisk.ramdiskbuf);
ram_fail:
	return ret;
}

static void __exit ramdisk_exit(void)
{
	printk("ramdisk_wreq: exit\n");
	del_gendisk(ramdisk.gendisk);
	put_disk(ramdisk.gendisk);
	blk_cleanup_queue(ramdisk.queue);
	unregister_blkdev(ramdisk.major, RAMDISK_NAME);
	kfree(ramdisk.ramdiskbuf);
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lance Cheng");
MODULE_DESCRIPTION("RAM disk block driver with Request Queue (method 1)");
