// SPDX-License-Identifier: GPL-2.0
/*
 * ramdisk_noreq.c — RAM disk block driver using Make Request (no scheduler)
 *
 * Method 2: Bypasses the I/O scheduler and processes bio requests directly.
 *   blk_alloc_queue() — allocates a queue without an I/O scheduler
 *   blk_queue_make_request() — sets the direct bio handler
 *   ramdisk_make_request_fn() — processes bio directly
 *
 * This approach is simpler and more efficient for RAM/flash storage
 * where request merging/reordering provides no benefit.
 * Reference: drivers/block/zram/zram_drv.c
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
#define RAMDISK_NAME	"ramdisk_noreq"
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
	printk("ramdisk_noreq: open\n");
	return 0;
}

/* Release block device */
static void ramdisk_release(struct gendisk *disk, fmode_t mode)
{
	printk("ramdisk_noreq: release\n");
}

/* Get disk geometry */
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
 * Make request function: processes bio directly, bypassing the I/O scheduler.
 * Iterates over each segment (bvec) in the bio and copies data.
 */
static void ramdisk_make_request_fn(struct request_queue *q, struct bio *bio)
{
	int offset;
	struct bio_vec bvec;
	struct bvec_iter iter;
	unsigned long len = 0;

	offset = (bio->bi_iter.bi_sector) << 9;

	bio_for_each_segment(bvec, bio, iter) {
		char *ptr = page_address(bvec.bv_page) + bvec.bv_offset;
		len = bvec.bv_len;

		if (bio_data_dir(bio) == READ)
			memcpy(ptr, ramdisk.ramdiskbuf + offset, len);
		else if (bio_data_dir(bio) == WRITE)
			memcpy(ramdisk.ramdiskbuf + offset, ptr, len);
		offset += len;
	}
	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
}

static int __init ramdisk_init(void)
{
	int ret = 0;
	printk("ramdisk_noreq: init\n");

	/* 1. Allocate storage buffer */
	ramdisk.ramdiskbuf = kzalloc(RAMDISK_SIZE, GFP_KERNEL);
	if (!ramdisk.ramdiskbuf) {
		ret = -ENOMEM;
		goto ram_fail;
	}

	/* 2. Initialize spinlock */
	spin_lock_init(&ramdisk.lock);

	/* 3. Register block device */
	ramdisk.major = register_blkdev(0, RAMDISK_NAME);
	if (ramdisk.major < 0) {
		ret = ramdisk.major;
		goto register_blkdev_fail;
	}
	printk("ramdisk_noreq: major = %d\n", ramdisk.major);

	/* 4. Allocate gendisk */
	ramdisk.gendisk = alloc_disk(RADMISK_MINOR);
	if (!ramdisk.gendisk) {
		ret = -ENOMEM;
		goto gendisk_alloc_fail;
	}

	/* 5. Allocate request queue (NO I/O scheduler) */
	ramdisk.queue = blk_alloc_queue(GFP_KERNEL);
	if (!ramdisk.queue) {
		ret = -ENOMEM;
		goto blk_alloc_fail;
	}

	/* 6. Set make_request function (bypass I/O scheduler) */
	blk_queue_make_request(ramdisk.queue, ramdisk_make_request_fn);

	/* 7. Register disk */
	ramdisk.gendisk->major = ramdisk.major;
	ramdisk.gendisk->first_minor = 0;
	ramdisk.gendisk->fops = &ramdisk_fops;
	ramdisk.gendisk->private_data = &ramdisk;
	ramdisk.gendisk->queue = ramdisk.queue;
	sprintf(ramdisk.gendisk->disk_name, RAMDISK_NAME);
	set_capacity(ramdisk.gendisk, RAMDISK_SIZE / 512);
	add_disk(ramdisk.gendisk);

	return 0;

blk_alloc_fail:
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
	printk("ramdisk_noreq: exit\n");
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
MODULE_DESCRIPTION("RAM disk block driver with Make Request (method 2)");
