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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define DTSLED_CNT  1
#define DTSLED_NAME "dts_led"
#define LEDOFF  0
#define LEDON   1


struct led_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int gpio1;
    int gpio2;
    int major;
    int minor;
    struct device_node *nd;
};

struct led_dev dtsled;

void led_switch(unsigned int sta)
{
    printk("led switch %d\r\n", sta);
    gpio_set_value(dtsled.gpio2, sta);
}

static int led_open(struct inode *node, struct file *filep)
{
    filep->private_data = &dtsled;
    return 0;
}

static ssize_t led_read(struct file *filep, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    retvalue = copy_from_user(databuf, buf, cnt);
    if(retvalue < 0) {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];

    if(ledstat == LEDON) {
        led_switch(LEDON);
    } else if(ledstat == LEDOFF) {
        led_switch(LEDOFF);
    }
    return 0;
}

static int led_release(struct inode *node, struct file *filep)
{
    return 0;
}

static struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};



static int led_init(void)
{
    struct property *proper;
    struct device_node *nd, *nd2;

    dtsled.nd = of_find_node_by_path("/leds");
    if(dtsled.nd == NULL){
        printk("dtsled node not find\r\n");
        return -EINVAL;
    }

    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if(proper == NULL) {
        printk("compatible property find failed\r\n");
    } else {
        printk("compatible = %s\r\n", (char*)proper->value);
    }

    nd = of_get_next_child(dtsled.nd, NULL);
    nd2 = of_get_next_child(dtsled.nd, nd);


    dtsled.gpio1 = of_get_named_gpio(nd, "gpios", 0);
    if(dtsled.gpio1 < 0){
        printk("cant get gpio %d\r\n", 0);
    }

    dtsled.gpio2 = of_get_named_gpio(nd2, "gpios", 0);
    if(dtsled.gpio2 < 0){
        printk("cant get gpio %d\r\n", 1);
    }

    // if(!gpio_request(dtsled.gpio2, "user_led")){
    //     gpio_direction_output(dtsled.gpio2, 1);
    // }else
    //     printk("user led request failed\r\n");

    return 0;
}



static int __init keyirq_init(void)
{
    if(dtsled.major){
        dtsled.devid = MKDEV(dtsled.major, 0);
        register_chrdev_region(dtsled.devid, 1, DTSLED_NAME);
    }else{
        alloc_chrdev_region(&dtsled.devid, 0, 1, DTSLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }

    cdev_init(&dtsled.cdev, &dtsled_fops);
    cdev_add(&dtsled.cdev, dtsled.devid, 1);

    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
    if(IS_ERR(dtsled.class)){
        return PTR_ERR(dtsled.class);
    }

    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
    if(IS_ERR(dtsled.device)){
        return PTR_ERR(dtsled.device);
    }
    led_init();
    return 0;
}

static void __exit keyirq_exit(void)
{
    cdev_del(&dtsled.cdev);
    unregister_chrdev_region(dtsled.devid, 1);
    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);
}

module_init(keyirq_init);
module_exit(keyirq_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("wanyisq");
MODULE_DESCRIPTION("led_blink");