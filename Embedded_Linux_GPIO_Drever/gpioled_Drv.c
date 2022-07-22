#include<linux/module.h>
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/of.h>
#include<linux/cdev.h>
#include<linux/fs.h>
#include<linux/slab.h>
#include<linux/device.h>
#include<linux/of_gpio.h>
#include<linux/gpio.h>
#include<linux/delay.h>
#include<linux/uaccess.h>
#include<linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lws<lws0317@gmail.con>");

#define DEV_NAME "gpioled_Dev"
#define DRIVER_CLASS "gpioled_class"
#define MAX_DEVNUM 1
#define BUF_SIZE 64

struct gpioled_dev{
	dev_t devId;
	struct class *class;
	struct device *device;
	struct cdev *cdev;
	int major;
	int minor;
	
	struct device_node *nd;
	int led_gpio;
	
	char devBuf[BUF_SIZE];
};
static struct gpioled_dev *gpioled;

static int gpioled_open(struct inode *inode, struct file *filp){
	filp->private_data=&gpioled;
	pr_info("%s: gpioled_Dev has been opened\n", __func__);
	return 0;
}

/**
 *	@ Copy to user 
 */
static ssize_t gpioled_read(struct file *filp, char __user *usrBuf, size_t len, loff_t *loff){
	int ret=0;
	ret=copy_to_user((void *)usrBuf, (void *)gpioled->devBuf, sizeof(gpioled->devBuf));
	if(ret<0){
		pr_info("%s: copy_to_user was failed, ret=%d\n", __func__, ret);
		return ret;
	}	
	return ret;
}

/**
 *	@ Copy from user 
 */
static ssize_t gpioled_write(struct file *filp, const char __user *usrBuf, size_t len, loff_t *loff){
	int ret=0;
	memset((void *)gpioled->devBuf, 0, sizeof(gpioled->devBuf));
	ret=copy_from_user((void *)gpioled->devBuf, (void *)usrBuf, len);
	if(ret<0){
		pr_info("%s: copy_from_user was failed, ret=%d\n", __func__, ret);
		return ret;
	}
	/* Setting the GPIO3A5 */
	switch (gpioled->devBuf[0]){
		case '1':
			pr_info("%s: 1\n", __func__);
			gpio_set_value(gpioled->led_gpio, 1);
			break;
		case '0':
			pr_info("%s: 0\n", __func__);
			gpio_set_value(gpioled->led_gpio, 0);
			break;
	}
	return sizeof(gpioled->devBuf);;
}

/**
 *	@ File operations mapping 
 */
static struct file_operations fops={
	.owner=THIS_MODULE,
	.open=gpioled_open,
	.read=gpioled_read,
	.write=gpioled_write,
};

static int __init initFunc(void){
	int ret=0;	
	gpioled=kmalloc(sizeof(gpioled), GFP_KERNEL);
	if(!gpioled){
		pr_err ("%s: kmalloc() was Failed.\n", __func__);
		return -1;
	}
/**
 *	@ Cdev Register 
 */
	/* Allocate a device */
	ret=alloc_chrdev_region(&gpioled->devId, 0, MAX_DEVNUM, DEV_NAME);
	if(ret<0){
		pr_err ("%s: cdev_add() was Failed.\n", __func__);
		goto out_alloc_chrdev;
	}
	
	/* allocate a cdev structure */
	gpioled->cdev=cdev_alloc();
	if(!gpioled->cdev){
		pr_err("%s: cdev_alloc() was Failed.\n", __func__);
		goto out_cdev_alloc;
	}
	
	/* Regisering device to kernel */
	ret=cdev_add(gpioled->cdev,gpioled->devId, MAX_DEVNUM);
	if(ret<0){
		pr_err ("%s: cdev_add() was Failed.\n", __func__);
		goto out_cdev_add;
	}
	
	/* Initialize device file */
	cdev_init(gpioled->cdev, &fops);
	
	gpioled->major=MAJOR(gpioled->devId);
	gpioled->minor=MINOR(gpioled->devId);

	pr_info("%s: major=%d, minor=%d\n", __func__, gpioled->major, gpioled->minor); 
	
	/* Create device class */
	gpioled->class=class_create(THIS_MODULE, DRIVER_CLASS);
	if(!gpioled->class){
		pr_err ("%s: class_create() was Failed.\n", __func__);
		goto out_class;
	}
	
	/* create device file */
	gpioled->device=device_create(gpioled->class, NULL, gpioled->devId, NULL, DEV_NAME);
	if(!gpioled->device){
		pr_err ("%s: device_create() was Failed.\n", __func__);
		goto out_device;
	}

/**
 *	@ GPIO Register 
 */		
	/* get the device node */
	gpioled->nd=of_find_node_by_path("/pinctrl/gpio@fe760000/gpio101_demo");
	if(!gpioled->nd){
		pr_info("%s: The gpioled node wasn't found!\n", __func__);
		ret=-EINVAL;
		goto out_of_find_node_by_path;
	}else{
		pr_info("%s: The gpioled node was found!\n", __func__);
	}

	/* Get a GPIO number to use with GPIO API */
	gpioled->led_gpio=of_get_named_gpio(gpioled->nd, "gpio101-gpios", 0);
	if(gpioled->led_gpio<0){
		pr_info("%s: can't get gpio101-gpios", __func__);
		ret=-EINVAL;
		goto out_of_get_named_gpio;
	}
	pr_info("%s: led-gpionum=%d\n", __func__, gpioled->led_gpio);

	/* request io */
	ret=gpio_request(gpioled->led_gpio, "gpio-led");
	if(ret<0){
		pr_info("%s: Failed to request gpio", __func__);
		goto out_gpio;
	}else{
		pr_info("%s: The gpio was gotten, ret=%d", __func__, ret);
	}
	
	/* gpio direction setting */
	ret=gpio_direction_output(gpioled->led_gpio, 1);
	if(ret<0){
		pr_info("%s: gpio_direction_output() was failed", __func__);
		goto out_gpio_direction_output;
	}

	return 0;
	
out_gpio_direction_output:
out_gpio:
	gpio_free(gpioled->led_gpio);
out_of_get_named_gpio:
out_of_find_node_by_path:
	device_destroy(gpioled->class, gpioled->devId);
out_device:
    class_destroy(gpioled->class);
out_class:
	cdev_del(gpioled->cdev);
out_cdev_add:
out_cdev_alloc:
    unregister_chrdev_region (gpioled->devId, MAX_DEVNUM);
out_alloc_chrdev:
	kfree(gpioled->cdev);
	return ret; 
}

static void __exit exitFunc(void){
	
	gpio_set_value(gpioled->led_gpio, 0);
	gpio_free(gpioled->led_gpio);
	
	device_destroy(gpioled->class, gpioled->devId);
	class_destroy(gpioled->class);
	cdev_del(gpioled->cdev);
	unregister_chrdev_region (gpioled->devId, MAX_DEVNUM);
	kfree(gpioled);
	
	pr_info("%s: module exit\n", __func__);
}

module_init(initFunc);
module_exit(exitFunc);



/** 
	int of_get_named_gpio(struct device_node *np, const char *propname, int index)
	static inline int of_get_named_gpio(struct device_node *np, const char *propname, int index)

 */
 
 
/**
	int gpio_request(unsigned gpio, const char *label);
	void gpio_free(unsigned gpio);

	static int gpio_direction_input(unsigned gpio)

	static int gpio_direction_output(unsigned gpio, int value)

	static int gpio_get_value(unsigned gpio)

	static void gpio_set_value(unsigned gpio, int value)

 */
 
 
 
 /**
 
	void *kmalloc(size_t size, gfp_t gfp); 
	
	int alloc_chrdev_region (dev_t * dev, unsigned baseminor, unsigned count, const char * name);

	void unregister_chrdev_region (dev_t from, unsigned count);

	int cdev_add (struct cdev * p, dev_t dev, unsigned count);

	void cdev_del (struct cdev * p);

	struct cdev * cdev_alloc (void);

	void cdev_init (struct cdev * cdev, const struct file_operations * fops);
	
	void kfree(void *p);

 */
 
 
 /**
	struct file_operations {
		struct module *owner;
		loff_t (*llseek) (struct file *, loff_t, int);
		ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
		ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
		ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
		ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
		int (*iopoll)(struct kiocb *kiocb, struct io_comp_batch *,
				unsigned int flags);
		int (*iterate) (struct file *, struct dir_context *);
		int (*iterate_shared) (struct file *, struct dir_context *);
		__poll_t (*poll) (struct file *, struct poll_table_struct *);
		long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
		long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
		int (*mmap) (struct file *, struct vm_area_struct *);
		unsigned long mmap_supported_flags;
		int (*open) (struct inode *, struct file *);
		int (*flush) (struct file *, fl_owner_t id);
		int (*release) (struct inode *, struct file *);
		int (*fsync) (struct file *, loff_t, loff_t, int datasync);
		int (*fasync) (int, struct file *, int);
		int (*lock) (struct file *, int, struct file_lock *);
		ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
		unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
		int (*check_flags)(int);
		int (*flock) (struct file *, int, struct file_lock *);
		ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
		ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
		int (*setlease)(struct file *, long, struct file_lock **, void **);
		long (*fallocate)(struct file *file, int mode, loff_t offset,
				  loff_t len);
		void (*show_fdinfo)(struct seq_file *m, struct file *f);
		unsigned (*mmap_capabilities)(struct file *);
		ssize_t (*copy_file_range)(struct file *, loff_t, struct file *,
				loff_t, size_t, unsigned int);
		loff_t (*remap_file_range)(struct file *file_in, loff_t pos_in,
					   struct file *file_out, loff_t pos_out,
					   loff_t len, unsigned int remap_flags);
		int (*fadvise)(struct file *, loff_t, loff_t, int);
	} __randomize_layout;

 */
 
 
 /**
	copy_from_user(void *to, const void __user *from, unsigned long n)
	copy_to_user(void __user *to, const void *from, unsigned long n)

 */