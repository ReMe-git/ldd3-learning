#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h> //struct cdev
#include <linux/kernel.h> //container_of
#include <linux/slab.h> //kmalloc, kfree
#include <linux/uaccess.h> //copy_to_user, copy_from_user
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/rwsem.h>

#include "./scull.h"
#include "linux/kdev_t.h"
#define DEBUG
#include "./log.h"

MODULE_LICENSE("DualBSD/GPL");

static loff_t scull_llseek(struct file*, loff_t, int);
static ssize_t scull_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t scull_write(struct file*, const char __user*, size_t, loff_t*);
static long scull_unblocked_ioctl(struct file*, unsigned int, unsigned long);
static int scull_open(struct inode*, struct file*);
static int scull_release(struct inode*, struct file*);

static dev_t scull_devno; //scull device number
static int scull_major = SCULL_MAJOR; //scull major number
static int scull_minor = SCULL_MINOR; //scull minor number
static int scull_ndev = SCULL_NDEV; //scull devices number
static int scull_qset = SCULL_QSET; // scull qset array size
static int scull_quantum = SCULL_QUANTUM; // scull signal quantum size

module_param(scull_major, int, S_IRUGO); //get scull major value
module_param(scull_minor, int, S_IRUGO); //get scull minor value
module_param(scull_ndev, int, S_IRUGO); //get scull devices number
module_param(scull_quantum, int, S_IRUGO); // get scull qset size
module_param(scull_qset, int, S_IRUGO); //get scull quantum size

static struct file_operations scull_ops = {
	.owner = THIS_MODULE,
	.llseek = scull_llseek,
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.unlocked_ioctl = scull_unblocked_ioctl,
	.release = scull_release,
}; //scull file operations

struct scull_qset {
	void** data;
	struct scull_qset* next;
}; // scull qset

struct scull_dev {
	struct scull_qset* data;
	int quantum;
	int qset;
	unsigned long size;
	struct cdev cdev;
	struct rw_semaphore sem;
}; //scull device

struct scull_dev *scull_devices;

/**
 * STORE MODEL: one qset size = qset * quantum
 *	|data|
 *	  |
 *	  |--->|qset|--->|qset|--->...
 *	  	|
 *	  	|
 *	      |data[scull_qset]|
 *	       		|
 *	       		|--->|quantum[scull_quantum]|
 *	       		|--->|quantum[scull_quantum]|
 *	       		|--->|quantum[scull_quantum]|
 *	       		|...
 *	       		|--->|quantum[scull_quantum]|
 * */

static int
scull_trim(struct scull_dev *dev)
{
	struct scull_qset *n, *p;
	int qset = dev->qset, i;
	
	for (p = dev->data; p; p = n) {
		if (p->data) {
			for (i = 0; i < qset; i++)
				kfree(p->data[i]);
			kfree(p->data);
			p->data = NULL;
		} //free quantums
		n = p->next;
		kfree(p);
	} // free qsets
	dev->size = 0;
	dev->qset = scull_qset;
	dev->quantum = scull_quantum;
	dev->data = NULL; // set default value
	return 0;
} //scull_trim

static struct scull_qset *
scull_follow(struct scull_dev *dev, int index)
{
	struct scull_qset *dp = NULL;
	
	if (!dev->data) {
		dev->data = (struct scull_qset *)kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if (!dev->data) {
			WARNING_LOG("failed alloc memory for qset\n");
			goto fail;
		}
		dev->data->data = NULL;
		dev->data->next = NULL;
		DEBUG_LOG("alloc memory for first qset\n");
	} // if fist qset is nullptr alloc memory for it

	dp = dev->data;
	for (int i = 0; i < index; dp = dp->next) {
		if (!dp->next) {
			dp->next = (struct scull_qset *)kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if (!dp->next) {
				WARNING_LOG("failed alloc memory for qset\n");
				goto fail;
			}
			dp->next->data = NULL;
			dp->next->next = NULL;
			DEBUG_LOG("alloc memory for next qset\n");
		}
	} // find target qset pointer

fail:
	return dp; //if present return dp else return nullptr
} //scull_follow

static loff_t
scull_llseek(struct file* filp, loff_t f_pos, int offset)
{
	return 0;
} //llseek

static ssize_t 
scull_read(struct file* filp, char __user* buff, size_t count, loff_t* f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dp;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;
	
	down_read(&dev->sem);
	if (*f_pos > dev->size) //f_pos out of size
		goto out;
	if (*f_pos + count > dev->size) //read out of size
		count = dev->size - *f_pos;

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dp = scull_follow(dev, item);
	if (!dp || !dp->data || !dp->data[s_pos]) {
		WARNING_LOG("invalid data pointer\n");		
		goto out;
	} //invalid data pointer
	
	if (count > quantum - q_pos) //count of out quantum
		count = quantum - q_pos;

	if (copy_to_user(buff, dp->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	} //read data
	DEBUG_LOG("copy %d bytes to userspace\n", (int)count);
	*f_pos += count;
	retval = count;
out:
	up_read(&dev->sem);
	return retval;
} //read

static ssize_t
scull_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dp;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, rest, s_pos, q_pos;
	ssize_t retval = -ENOPARAM;
	
	down_write(&dev->sem);
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;
	dp = scull_follow(dev, item);
	if (dp == NULL)
		goto out;
	
	if (!dp->data) {
		dp->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dp->data)
			goto out;
		memset(dp->data, 0, qset * sizeof(char *));
	} //alloc memory if qset is nullptr

	if (!dp->data[s_pos]) {
		dp->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dp->data[s_pos])
			goto out;
	} //alloc memory if quantum is nullptr
	if (count > quantum - q_pos)
		count = quantum - q_pos;
	if (copy_from_user(dp->data[s_pos] + q_pos, buff, count)) {
		retval = -EFAULT;
		goto out;
	} //read data
	DEBUG_LOG("copy %d bytes from userspace\n", (int)count);
	*f_pos += count;
	retval = count;

	if (*f_pos > dev->size)
		dev->size = *f_pos;
out:
	up_write(&dev->sem);
	return retval;
} //write

static int
scull_open(struct inode* inode, struct file* filp)
{
	struct scull_dev *dev; // pointer of struct scull_dev
	
	dev = container_of(inode->i_cdev, struct scull_dev, cdev); //get the pointer of struct scull_dev
	filp->private_data = (void *)dev; //save for other methods
	
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		down_write(&dev->sem);
		scull_trim(dev);
		up_write(&dev->sem);
	} //if write only mode clean data
	return 0;
} // open


static long
scull_unblocked_ioctl(struct file* filp, unsigned int a, unsigned long b)
{
	return 0;
} // unblock_ioctl

static int
scull_release(struct inode* inode, struct file* filp)
{
	return 0;
} //release

static void 
scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err;
	dev_t devnum = MKDEV(scull_major, scull_minor + index);
	
	cdev_init(&dev->cdev, &scull_ops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devnum, 1);
	if (err < 0) {
		ERR_LOG("failed register scull%d, error %d\n", index, err);
	} //if failed print warning message

} //scull_setup_cdev

static int __init scull_init(void)
{
	int res, i; // varible storing return value
	char buffer[32]; //store device number

	if (scull_major) {
		scull_devno = MKDEV(scull_major, scull_minor);
		res = register_chrdev_region(scull_devno, scull_ndev, "scull");
	} //if give major number 
	else {
		res = alloc_chrdev_region(&scull_devno, scull_minor, scull_ndev, "scull");
		scull_major = MAJOR(scull_devno);
	} //alloc major number
	if (res < 0) {
		ERR_LOG("failed alloc device numbers, error %d\n", res);
		return res;
	}// failed register device number return err
	print_dev_t(buffer, scull_devno);

	scull_devices = kmalloc(sizeof(struct scull_dev) * scull_ndev, GFP_KERNEL);
	if (!scull_devices) {
		res = -ENOPARAM;
		goto fail;
	} //alloc memory for scull devices
	memset(scull_devices, 0, sizeof(struct scull_dev) * scull_ndev);

	for (i = 0; i < scull_ndev; i++) {
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
		scull_devices[i].size = 0;
		scull_devices[i].data = NULL;
		init_rwsem(&scull_devices[i].sem);
		scull_setup_cdev(&scull_devices[i], i);
	} //register char devices
	
	NORM_LOG("succeed install module, first device number is %s\n", buffer);
	return 0;

fail:
	return res;
} //scull init function

static void __exit scull_exit(void)
{
	for (int i = 0; i < scull_ndev; i++) {
		cdev_del(&scull_devices[i].cdev);
		scull_trim(&scull_devices[i]);
	}
	kfree(scull_devices);
	unregister_chrdev_region(scull_devno, scull_ndev);
	NORM_LOG("succeed remove module\n");
} //scull exit function

module_init(scull_init);
module_exit(scull_exit);
