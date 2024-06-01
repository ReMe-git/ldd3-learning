#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h> //struct cdev
#include <linux/kernel.h> //container_of
#include <linux/slab.h> //kmalloc, kfree
#include <linux/uaccess.h> //copy_to_user, copy_from_user

#include "./scull.h"
#include "linux/container_of.h"
#include "linux/kdev_t.h"
#include "linux/moduleparam.h"
#include "linux/printk.h"
#include "linux/stat.h"

MODULE_LICENSE("DualBSD/GPL");

static loff_t scull_llseek(struct file*, loff_t, int);
static ssize_t scull_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t scull_write(struct file*, const char __user*, size_t, loff_t*);
static long scull_unblocked_ioctl(struct file*, unsigned int, unsigned long);
static int scull_open(struct inode*, struct file*);
static int scull_release(struct inode*, struct file*);

static dev_t scull_devm; //scull device number
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
}; //scull device

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

static loff_t
scull_llseek(struct file* filp, loff_t f_pos, int offset)
{
	return 0;
} //llseek

static ssize_t 
scull_read(struct file* filp, char __user* buff, size_t count, loff_t* offp)
{
	return 0;
} //read

static ssize_t
scull_write(struct file* filp, const char __user* buff, size_t count, loff_t* offp)
{
	return 0;
} //write

static int
scull_open(struct inode* inode, struct file* filp)
{
	struct scull_dev *dev; // pointer of struct scull_dev
	
	dev = container_of(inode->i_cdev, struct scull_dev, cdev); //get the pointer of struct scull_dev
	inode->i_private = (void *)dev; //save for other methods
	
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		scull_trim(dev);
	} //if write only mode clean data
	return 0;
} // open


static long
scull_unblocked_ioctl(struct file* filp, unsigned int a, unsigned long b)
{
	return 0;
} // unblock_ioctl

static int
scull_release(struct inode* np, struct file* filp)
{
	return 0;
} //release

static void 
register_scull_chrdev(struct scull_dev *dev, int index)
{
	int err;
	dev_t devnum = MKDEV(scull_major, scull_minor + index);

	cdev_init(&dev->cdev, &scull_ops);
	err = cdev_add(&dev->cdev, devnum, 1);
	if (err < 0) //if failed print warning message
		printk(KERN_NOTICE, "register_scull_chrdev: \
				failed register scull%d, error %d", index, err);

} //register_scull_chrdev

static int __init scull_init(void)
{
	int res; // varible storing return value
	
	if (scull_major) {
		scull_devm = MKDEV(scull_major, scull_minor);
		res = register_chrdev_region(scull_devm, scull_ndev, "scull");
	} //if give major number 
	else {
		res = alloc_chrdev_region(&scull_devm, scull_minor, scull_ndev, "scull");
	} //alloc major number
	if (res < 0)
		printk(KERN_ERR, "scull_init: failed alloc device numbers, error %d", res);
		return -1;
	// failed register device number return -1

	return 0;
} //scull init function

static void __exit scull_exit(void)
{
	unregister_chrdev_region(scull_devm, scull_ndev);
} //scull exit function

module_init(scull_init);
module_exit(scull_exit);
