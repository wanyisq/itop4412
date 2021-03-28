#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define EXYNOS_IRQ_CNT  1
#define EXYNOS_IRQ_NAME "noblockio"
#define KEY_VALUE   0x01
#define INVAKEY 0xFF
#define KEY_NUM 1

struct key_irqdesc
{
    int gpio;
    int irqnum;
    unsigned char value;
    char name[10];
    irqreturn_t (*handler)(int, void *);
};

struct key_irq_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    atomic_t keyvalue;
    atomic_t releasekey;
    struct timer_list timer;
    struct key_irqdesc irqdest[KEY_NUM];
    unsigned char curkeynum;

    wait_queue_head_t r_wait;
};

struct key_irq_dev key_irq;

static irqreturn_t key_handler(int irq, void *dev_id)
{
    struct key_irq_dev *dev = (struct key_irq_dev *)dev_id;

    dev->curkeynum = 0;
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
    printk("key_handler\n");
    return IRQ_RETVAL(IRQ_HANDLED);
}

void timer_function(unsigned long arg)
{
    unsigned char value;
    unsigned char num;
    struct key_irqdesc *keydesc;
    struct key_irq_dev *dev = (struct key_irq_dev *)arg;

    num = dev->curkeynum;
    keydesc = &dev->irqdest[num];
    value = gpio_get_value(keydesc->gpio);
    if(value){
        atomic_set(&dev->keyvalue, keydesc->value);
    }else{
        atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
        atomic_set(&dev->releasekey, 1);
    }

    if(atomic_read(&dev->releasekey)){
        wake_up_interruptible(&dev->r_wait);
    }
}

static int keyio_init(void)
{
    unsigned char i = 0;
    char name[10];
    int ret = 0;

    key_irq.nd = of_find_node_by_path("/test_key");
    if(key_irq.nd == NULL){
        printk("key node not find\r\n");
        return -EINVAL;
    }

    for(i = 0; i < KEY_NUM; i++){
        key_irq.irqdest[i].gpio = of_get_named_gpio(key_irq.nd, "gpios", i);
        if(key_irq.irqdest[i].gpio < 0){
            printk("cant get gpio %d\r\n", i);
        }
    }

    for(i = 0; i < KEY_NUM; i++){
        memset(key_irq.irqdest[i].name, 0, sizeof(name));
        sprintf(key_irq.irqdest[i].name, "KEY%d", i);
        gpio_request(key_irq.irqdest[i].gpio, name);
        gpio_direction_input(key_irq.irqdest[i].gpio);
        key_irq.irqdest[i].irqnum = irq_of_parse_and_map(key_irq.nd, i);
        printk("key%d:gpio=%d, irqnum=%d\r\n",i,
                key_irq.irqdest[i].gpio,
                key_irq.irqdest[i].irqnum);
    }

    key_irq.irqdest[0].handler = key_handler;
    key_irq.irqdest[0].value = KEY_VALUE;

    for(i = 0; i < KEY_NUM; i++){
        ret = request_irq(key_irq.irqdest[i].irqnum,
                            key_irq.irqdest[i].handler,
                            IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                            key_irq.irqdest[i].name, &key_irq);
        if(ret < 0){
            printk("irq %d request failed %d\r\n", key_irq.irqdest[i].irqnum, ret);
            return -EFAULT;
        }
    }

    init_timer(&key_irq.timer);
    key_irq.timer.function = timer_function;

    init_waitqueue_head(&key_irq.r_wait);
    return 0;
}

static int key_open(struct inode *nd, struct file *filp)
{
    filp->private_data = &key_irq;
    return 0;
}

static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int ret = 0;
    unsigned char keyvalue = 0;
    unsigned char relesekey = 0;
    struct key_irq_dev *dev = (struct key_irq_dev *)filp->private_data;

    if(filp->f_flags & O_NONBLOCK){
        if(atomic_read(&dev->releasekey) == 0){
            return -EAGAIN;
        }
    }else{
        ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));
        if(ret){
            goto wait_error;
        }
    }
    
    keyvalue = atomic_read(&dev->keyvalue);
    relesekey = atomic_read(&dev->releasekey);
    if(relesekey){
        if(keyvalue & 0x80){
            keyvalue &= ~0x80;
            ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
        }else{
            goto data_error;
        }
        atomic_set(&dev->releasekey, 0);
        return 0;
    }else{
        goto data_error;
    }

wait_error:
    return ret;

data_error:
    return -EINVAL;
}

unsigned int key_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct key_irq_dev *dev = (struct key_irq_dev *)filp->private_data;

    poll_wait(filp, &dev->r_wait, wait);
    if(atomic_read(&dev->releasekey)){
        mask = POLLIN | POLLRDNORM;
    }

    return mask;
}

static struct file_operations keyirq_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .poll = key_poll,
};

static int __init keyirq_init(void)
{
    if(key_irq.major){
        key_irq.devid = MKDEV(key_irq.major, 0);
        register_chrdev_region(key_irq.devid, EXYNOS_IRQ_CNT, EXYNOS_IRQ_NAME);
    }else{
        alloc_chrdev_region(&key_irq.devid, 0, EXYNOS_IRQ_CNT, EXYNOS_IRQ_NAME);
        key_irq.major = MAJOR(key_irq.devid);
        key_irq.minor = MINOR(key_irq.devid);
    }

    cdev_init(&key_irq.cdev, &keyirq_fops);
    cdev_add(&key_irq.cdev, key_irq.devid, EXYNOS_IRQ_CNT);

    key_irq.class = class_create(THIS_MODULE, EXYNOS_IRQ_NAME);
    if(IS_ERR(key_irq.class)){
        return PTR_ERR(key_irq.class);
    }

    key_irq.device = device_create(key_irq.class, NULL, key_irq.devid, NULL, EXYNOS_IRQ_NAME);
    if(IS_ERR(key_irq.device)){
        return PTR_ERR(key_irq.device);
    }

    atomic_set(&key_irq.keyvalue, INVAKEY);
    atomic_set(&key_irq.releasekey, 0);
    keyio_init();
    return 0;
}

static void __exit keyirq_exit(void)
{
    unsigned int i = 0;

    del_timer_sync(&key_irq.timer);
    for(i = 0; i < KEY_NUM; i++){
        free_irq(key_irq.irqdest[i].irqnum, &key_irq);
    }
    cdev_del(&key_irq.cdev);
    unregister_chrdev_region(key_irq.devid, EXYNOS_IRQ_CNT);
    device_destroy(key_irq.class, key_irq.devid);
    class_destroy(key_irq.class);
}

module_init(keyirq_init);
module_exit(keyirq_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("wanyisq");
MODULE_DESCRIPTION("topeet4412_key_irq");