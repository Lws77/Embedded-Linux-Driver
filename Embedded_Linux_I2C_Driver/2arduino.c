#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/i2c.h>
#include<linux/string.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/slab.h>
#include<linux/fs.h>
#include<linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lws<wslin0317@gmail.com>");

#define DRIVER_NAME "2arduino_i2c"
#define DRIVER_CLASS "2arduinoClass"
#define BUF_SIZE 64


static dev_t devNo;
static struct class *arduino_i2c_class=NULL; 
static char devBuf[BUF_SIZE];

struct arduino_i2c_cdev{
	char name[16];
    struct i2c_client *client;
    struct cdev cdev;
};

int i2cdev_open(struct inode *inode, struct file *file){
	struct arduino_i2c_cdev *i2c_cdev;
	i2c_cdev=container_of(inode->i_cdev, struct arduino_i2c_cdev, cdev);
	pr_info("%s: dev_name=%s\n", __func__, i2c_cdev->name);
    if (!i2c_cdev){
    	pr_err("Cannot extrace aduino structure from i_cdev.\n");
	return -EINVAL;
    }
    file->private_data=i2c_cdev->client;
	return 0;
}
/* Copy to user */
ssize_t i2cdev_read(struct file *file, char __user *usrBuf, size_t len, loff_t *loff){
	int i=0, ret=0;
	struct i2c_client *client;
	client=file->private_data;
	memset((void*)devBuf, 0, BUF_SIZE);
	/* read from i2c */
	devBuf[i]=(char)i2c_smbus_read_byte_data(client, 0);
	len=strlen(devBuf);
	ret=copy_to_user((void *)usrBuf, (void *)devBuf, len);
		
	return len;
}
/* Copy from user */
ssize_t i2cdev_write(struct file *file, const char __user *usrBuf, size_t len, loff_t *loff){
	int i=0, ret=0;
	struct i2c_client *client;
	client=file->private_data;
    
	ret=copy_from_user((void *)devBuf, (void *)usrBuf, len);
	for (i=0; i<len; i++) {
		/* write to i2c */
	    ret=i2c_smbus_write_byte(client, devBuf[i]);
    }
	return len;	
}

/* File operations mapping */
static const struct file_operations i2cdev_fops = {
	.owner		= THIS_MODULE,
	.open		= i2cdev_open,
	.read		= i2cdev_read,
	.write		= i2cdev_write,
};

/* Probe function */
static int my_probe(struct i2c_client *client, const struct i2c_device_id *id){
	int i=0, ret = 0;
	char *info = "Hello, Arduino!\n";
	struct arduino_i2c_cdev *i2c_cdev=NULL;
	struct device *device;
	
	pr_info("Dummy device is being probed.\n");
    
    for ( i=0; i < strlen(info); i++) {
    	i2c_smbus_write_byte(client, info[i]);
    }
	
/**
 *	@ Cdev Register 
 */
	/* Allocate a device */
    ret=alloc_chrdev_region(&devNo, 0, 1, DRIVER_NAME);
    if (ret<0) {
        pr_err ("%s: Failed in alloc_chrdev_reion for arduino.\n", __func__);
	goto out_alloc_chrdev;
    }
	/* Allocate memory */
	i2c_cdev=kzalloc(sizeof(struct arduino_i2c_cdev), GFP_KERNEL); 
	if(!i2c_cdev){
		pr_err ("%s: kzalloc() was Failed.\n", __func__);
		goto out_oom;
	}

	strcpy(i2c_cdev->name, DRIVER_NAME);
	
	/* Regisering device to kernel */
	ret=cdev_add(&(i2c_cdev->cdev), devNo, 1);
	if(ret){
		pr_err ("%s: cdev_add() was Failed.\n", __func__);
		goto out_cdev_add;
	}
	
	/* Initialize device file */
	cdev_init(&(i2c_cdev->cdev), &i2cdev_fops);
	i2c_cdev->cdev.owner = THIS_MODULE;
	
	/* Create device class */
	arduino_i2c_class=class_create(THIS_MODULE, DRIVER_CLASS);
	if(!arduino_i2c_class){
		pr_err ("%s: class_create() was Failed.\n", __func__);
		goto out_sysfs_class;
	}
	
	/* Create device file */
	device=device_create(arduino_i2c_class, NULL, devNo, NULL, DRIVER_NAME);
	if(!device){
		pr_err ("%s: device_create() was Failed.\n", __func__);
		goto out_device;
	}


	i2c_cdev->client=client;
	i2c_set_clientdata(client, i2c_cdev);
	
	return 0;
	
out_device:
    cdev_del(&i2c_cdev->cdev);
out_sysfs_class:
	class_destroy(arduino_i2c_class);
out_cdev_add:
    kfree(i2c_cdev);
out_oom:
    unregister_chrdev_region(devNo, 1);
out_alloc_chrdev:
	return ret; 
}

static int my_remove(struct i2c_client *client){
    struct arduino_i2c_cdev *i2c_cdev;
	pr_info("%s: Dummy device is removing.\n", __func__);
    i2c_cdev=i2c_get_clientdata(client);
    device_destroy(arduino_i2c_class, devNo);
    cdev_del(&(i2c_cdev->cdev));
    kfree(i2c_cdev);
    class_destroy(arduino_i2c_class);
    unregister_chrdev_region(devNo, 1);
	
    return 0;
}


static struct of_device_id my_id_tables [] = {
    { .compatible="arduino", },
    { }
};

MODULE_DEVICE_TABLE(of, my_id_tables);

static struct i2c_driver my_drv = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
    	.name = DRIVER_NAME,
	.owner = THIS_MODULE,
	.of_match_table = my_id_tables,
    },
};

module_i2c_driver(my_drv);
MODULE_LICENSE("GPL");




/**
	s32 i2c_smbus_read_byte(const struct i2c_client *client);
	s32 i2c_smbus_write_byte(const struct i2c_client *client, u8 value);
	
	#define module_i2c_driver(__i2c_driver) \
		module_driver(__i2c_driver, i2c_add_driver, \
				i2c_del_driver)
	@__i2c_driver: i2c_driver struct
	
	void i2c_set_clientdata(struct i2c_client *client, void *data){
		dev_set_drvdata(&client->dev, data);
	}
struct i2c_client {
	unsigned short flags;				div., see below		
#define I2C_CLIENT_PEC		0x04		 Use Packet Error Checking 
#define I2C_CLIENT_TEN		0x10		 we have a ten bit chip address 
					Must equal I2C_M_TEN below 
#define I2C_CLIENT_SLAVE	0x20		 we are the slave 
#define I2C_CLIENT_HOST_NOTIFY	0x40	 We want to use I2C host notify 
#define I2C_CLIENT_WAKE		0x80		 for board_info; true iff can wake 
#define I2C_CLIENT_SCCB		0x9000		 Use Omnivision SCCB protocol 
										 Must match I2C_M_STOP|IGNORE_NAK 

	unsigned short addr;			 chip address - NOTE: 7bit	
									 addresses are stored in the	
									 _LOWER_ 7 bits		
	char name[I2C_NAME_SIZE];
	struct i2c_adapter *adapter;	 the adapter we sit on	
	struct device dev;				 the device structure		
	int init_irq;			     	 irq set at initialization	
	int irq;			           	 irq issued by device		
	struct list_head detected;
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	i2c_slave_cb_t slave_cb;	 	 callback for slave mode	
#
	void *devres_group_id;		   	 ID of probe devres group	
};	



*/






/**
	struct file_operations{
	...
		loff_t (*llseek) (struct file *, loff_t, int);
		ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
		ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	...
		long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
		long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
		int (*mmap) (struct file *, struct vm_area_struct *);
		unsigned long mmap_supported_flags;
		int (*open) (struct inode *, struct file *);
		int (*release) (struct inode *, struct file *);
	...
	};

 */
 
 /**
	int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name)
	
	int cdev_add(struct cdev * p, dev_t dev, unsigned count);
	
	struct cdev * cdev_alloc(void);

	void cdev_init (struct cdev * cdev, const struct file_operations * fops);
	
	extern void class_destroy(struct class *cls);
	
	
	#define class_create(owner, name)({		 \
		static struct lock_class_key __key;	 \
		__class_create(owner, name, &__key); \
	})
	struct class * __must_check __class_create(struct module *owner, const char *name, struct lock_class_key *key);
 */
 
 
 /**
	copy_from_user(void *to, const void __user *from, unsigned long n)
	copy_to_user(void __user *to, const void *from, unsigned long n)

 */