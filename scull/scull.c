#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include "./scull.h"

MODULE_LICENSE("DualBSD/GPL");

static loff_t scull_llseek(struct file*, loff_t, int);
static ssize_t scull_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t scull_write(struct file*, const char __user*, size_t, loff_t*);
//static int scull_ioctl(struct inode*, struct file*, unsigned int, unsigned long); //新内核中ioctl被删除
static long scull_unblocked_ioctl(struct file*, unsigned int, unsigned long);
static int scull_open(struct inode*, struct file*);
static int scull_release(struct inode*, struct file*);

static struct file_operations scull_ops = {
	.llseek = scull_llseek,
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.unlocked_ioctl = scull_unblocked_ioctl,
	.release = scull_release,
}; //scull file operations

static dev_t scull_dev; //scull device number
static int scull_major = SCULL_MAJOR; //scull major number
static int scull_minor = SCULL_MINOR; //scull minor number
static int scull_ndev = SCULL_NDEV; //scull devices number

static loff_t
scull_llseek(struct file* filp, loff_t f_pos, int offset)
{
	return 0;
} //llseek

static ssize_t 
scull_read(struct file* filp, char __user* sp, size_t size, loff_t* offset)
{
	return 0;
} //read

static
ssize_t scull_write(struct file* filp, const char __user* sp, size_t size, loff_t* offset)
{
	return 0;
} //write

static
int scull_open(struct inode* np, struct file* filp)
{
	return 0;
} // open


static
 long scull_unblocked_ioctl(struct file* filp, unsigned int a, unsigned long b)
{
	return 0;
} // unblock_ioctl

static 
int scull_release(struct inode* np, struct file* filp)
{
	return 0;
} //release

static
int register_scull_devnum(void)
{
	int res;

	if (scull_major) {
		scull_dev = MKDEV(scull_major, scull_minor);
		res = register_chrdev_region(scull_dev, scull_ndev, "scull");
	} //if give major number 
	else {
		res = alloc_chrdev_region(&scull_dev, scull_minor, scull_ndev, "scull");
	} //alloc major number

	return res;
} //register scull device number

static int __init scull_init(void)
{
	return 0;
} //scull init function

static void __exit scull_exit(void)
{

} //scull exit function

module_init(scull_init);
module_exit(scull_exit);
