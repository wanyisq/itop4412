#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio_keys.h>
#include <linux/of_irq.h>

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/of_address.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <asm/mach/map.h>
#include <asm/io.h>

#define DRIVER_NAME "user_key"
#define KEY_NUM	1
#define KEY0VALUE		0x01
#define INVAKEY			0xff
struct irq_keydest
{
	int gpio;
	int irqnum;
	unsigned char value;
	char name[10];
	irqreturn_t (*handler)(int, void *);
};

struct exynosirq_dev
{
	int major;
	int minor;
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *node;
	atomic_t keyvalue;
	atomic_t releasekey;
	struct timer_list timer;
	struct irq_keydest irq_keydest[KEY_NUM];
	unsigned char curkeynum;
	struct tasklet_struct tasklet;
};
struct exynosirq_dev exynosirq;

static ssize_t exynosirq_open(struct inode *node, struct file *filp)
{
	filp->private_data = &exynosirq;
	return 0;
}

static ssize_t exynosirq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct exynosirq_dev *dev = (struct exynosirq_dev *)filp->private_data;

	keyvalue = atomic_read(&dev->keyvalue);
	releasekey = atomic_read(&dev->releasekey);

	if(releasekey){
		if(keyvalue & 0x80){
			keyvalue &= 0x80;
			
			ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
		}else{
			goto data_error;
		}
		atomic_set(&dev->releasekey, 0);
	}else{
		goto data_error;
	}
	return 0;

data_error:
	return -EINVAL;

}
static struct file_operations exynosirq_fops = {
	.owner = THIS_MODULE,
	.open = exynosirq_open,
	.read = exynosirq_read,
};

static void test_tasklet_function(unsigned long data)
{
	struct exynosirq_dev *dev = (struct exynosirq_dev *)data;

	dev->curkeynum = 0;
	dev->timer.data = (volatile long)data;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
	printk("%s\r\n", __FUNCTION__);
}


static irqreturn_t key0_handler(int irq, void *dev_id)
{
	struct exynosirq_dev *dev = (struct exynosirq_dev *)dev_id;
	printk("%s\r\n", __FUNCTION__);

	tasklet_schedule(&dev->tasklet);
	return IRQ_RETVAL(IRQ_HANDLED);
}

void timer_function(unsigned long arg)
{
	unsigned char value;
	unsigned char num;
	struct irq_keydest *keydest;
	struct exynosirq_dev *dev = (struct exynosirq_dev *)arg;

	num  = dev->curkeynum;
	keydest = &dev->irq_keydest[num];
	value  = gpio_get_value(keydest->gpio);
	if(value == 1){
		atomic_set(&dev->keyvalue, keydest->value);
	}else{
		atomic_set(&dev->keyvalue, 0x80 | keydest->value);
		atomic_set(&dev->releasekey, 1);
	}

	printk("%s value %d, keyvalue %d, releasekey %d\r\n", __FUNCTION__, value, dev->keyvalue, dev->releasekey);
}

static int inter_probe(struct platform_device * pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int ret;

	printk("inter_probe\r");

	if(exynosirq.major){
		exynosirq.devid = MKDEV(exynosirq.major, 0);
		register_chrdev_region(exynosirq.devid, KEY_NUM, DRIVER_NAME);
	}else{
		alloc_chrdev_region(&exynosirq.devid, 0, KEY_NUM, DRIVER_NAME);
		exynosirq.major = MAJOR(exynosirq.devid);
		exynosirq.minor = MINOR(exynosirq.devid);
	}
	cdev_init(&exynosirq.cdev, &exynosirq_fops);
	cdev_add(&exynosirq.cdev, exynosirq.devid, KEY_NUM);
	exynosirq.class = class_create(THIS_MODULE, DRIVER_NAME);
	if(IS_ERR(exynosirq.class)){
		return PTR_ERR(exynosirq.class);
	}
	exynosirq.device = device_create(exynosirq.class, NULL, exynosirq.devid, NULL, DRIVER_NAME);
	if(IS_ERR(exynosirq.device)){
		return PTR_ERR(exynosirq.device);
	}

	atomic_set(&exynosirq.keyvalue, INVAKEY);
	atomic_set(&exynosirq.releasekey, 0);

	exynosirq.irq_keydest[0].irqnum = irq_of_parse_and_map(node, 0);
	printk("irq num is %d\r", exynosirq.irq_keydest[0].irqnum);
	
	tasklet_init(&exynosirq.tasklet, test_tasklet_function, (unsigned long *)&exynosirq);
	ret = request_irq(exynosirq.irq_keydest[0].irqnum, key0_handler, IRQ_TYPE_EDGE_FALLING, "user_init_key", &exynosirq);
	if (ret < 0) {
		printk("Request IRQ %d failed, %d\n", exynosirq.irq_keydest[0].irqnum,ret);
		return -1;
	}

	exynosirq.timer.function = timer_function;
	init_timer(&exynosirq.timer);

	printk("%s, %x\r\n", __FUNCTION__, timer_function);
	printk("inter_probe end\r");
	
    return 0;
}

static int inter_remove(struct platform_device * pdev)
{
	unsigned i = 0;
	del_timer_sync(&exynosirq.timer);

	for(i = 0; i < KEY_NUM; i++){
		free_irq(exynosirq.irq_keydest[i].irqnum, &exynosirq);
	}

	cdev_del(&exynosirq.cdev);
	unregister_chrdev_region(exynosirq.devid, KEY_NUM);
	device_destroy(exynosirq.class, exynosirq.devid);
	class_destroy(exynosirq.class);
	
	printk(KERN_ALERT "Goodbye, curel world, this is remove\n");
	
	return 0;
}

static const struct of_device_id of_inter_dt_match[] = {
	{.compatible = DRIVER_NAME},
	{},
};

MODULE_DEVICE_TABLE(of,of_inter_dt_match);

static struct platform_driver inter_driver = {
	.probe 	= inter_probe,
	.remove = inter_remove,
	.driver = {
		.name = DRIVER_NAME, //此属性用于传统的驱动与设备匹配
		.owner = THIS_MODULE,
		.of_match_table = of_inter_dt_match, //此属性用于设备树下驱动和设备的匹配
		},
};

static int inter_init(void)
{
	printk(KERN_ALERT "Hello, world\n");
	return platform_driver_register(&inter_driver);
	return 0;
}

static void inter_exit(void)
{
	printk(KERN_ALERT "Goodbye, curel world\n");
	platform_driver_unregister(&inter_driver);
}

module_init(inter_init);
module_exit(inter_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("wanyisq");
MODULE_DESCRIPTION("exynos4412_key_irq");