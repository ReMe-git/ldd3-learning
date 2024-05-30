#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

MODULE_LICENSE("DualBSD/GPL");

static loff_t scull_llseek(struct file*, loff_t, int);
static ssize_t scull_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t scull_write(struct file*, const char __user*, size_t, loff_t*);
//static int scull_ioctl(struct inode*, struct file*, unsigned int, unsigned long); //新内核中ioctl被删除
static int scull_open(struct inode*, struct file*);
static int scull_release(struct inode*, struct file*);

static struct file_operations scull_ops = {
	.llseek = scull_llseek,
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.release = scull_release,
};

static loff_t
scull_llseek(struct file* filp, loff_t f_pos, int offset)
{
	return 0;
}

static ssize_t 
scull_read(struct file* filp, char __user* sp, size_t size, loff_t* offset)
{
	return 0;
}

static
ssize_t scull_write(struct file* filp, const char __user* sp, size_t size, loff_t* offset)
{
	return 0;
}

static
int scull_open(struct inode* np, struct file* filp)
{
	return 0;
}

static 
int scull_release(struct inode* np, struct file* filp)
{
	return 0;
}

static
int register_scull_devnum(dev_t major, dev_t minor)
{
	return 0;
}

static int __init scull_init(void)
{
	return 0;
}

static void __exit scull_exit(void)
{

}

module_init(scull_init);
module_exit(scull_exit);
