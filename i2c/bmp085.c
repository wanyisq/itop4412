#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "bmp085.h"

#define BMP085_CNT  1
#define BMP085_NAME "bmp085"

struct bmp085_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    void *private_data;
    unsigned short temp, press;
};

static struct bmp085_dev bmp085dev;

static int bmp085_read_regs(struct bmp085_dev *dev, unsigned int reg, void *val, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    //发送首地址
    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &reg;
    msg[0].len = 1;

    //读取数据
    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = len;

    ret = i2c_transfer(client->adapter, msg, 2);
    if(ret == 2){
        ret = 0;
    }else{
        printk("i2c rd failed %d, reg %d, len %d\r\n", ret, reg, len);
        ret = -EREMOTEIO;
    }
    return ret;
}

static int bmp085_write_regs(struct bmp085_dev *dev, u8 reg, u8 *buf, u8 len)
{
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    b[0] = reg;
    memcpy(&b[1], buf, len);

    //读取数据
    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = b;
    msg.len = len + 1;

    return i2c_transfer(client->adapter, &msg, 1);
}

static unsigned char bmp085_read_reg(struct bmp085_dev *dev, u8 reg)
{
    u8 data = 0;

    bmp085_read_regs(dev, reg, &data, 1);
    return data;
}

static void bmp085_write_reg(struct bmp085_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
    buf = data;
    bmp085_write_regs(dev, reg, &buf, 1);
}

s32 bmp085_read_data(struct bmp085_dev *dev)
{
    unsigned char i = 0;
    unsigned char buf[2];
    s32 temp = 0;

    bmp085_write_reg(dev, CONTROL_REG_ADDR, BMP_COVERT_TEMP);
    mdelay(5);
    
    for(i = 0; i < 2; i++)
    {
        buf[i] = bmp085_read_reg(&bmp085dev, BMP_TEMP_PRES_DATA_REG + i);
    }
    temp = (buf[0] << 8) + buf[1];
    printk("temp %d\r\n", temp);

    return temp;
}

static int bmp085_open(struct node *nd, struct file *filp)
{
    filp->private_data = &bmp085dev;
    printk("bmp085dev addr %d\r\n", &bmp085dev);

    unsigned char i = 0;
    unsigned char buf[6];

    for(i = 0; i < 6; i++)
    {
        buf[i] = bmp085_read_reg(&bmp085dev, BMP_AC1_ADDR + i);
        printk("buf[%d], %d\r\n", i, buf[i]);
    }
    return 0;
}

static ssize_t bmp085_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    s32 t;
    long err = 0;
    struct bmp085_dev *dev = (struct bmp085_dev *)filp->private_data;

    if(dev == NULL){
        printk("\r\n%s error\r\n", __FUNCTION__);
    }
    
    t = bmp085_read_data(dev);
    err = copy_to_user(buf, &t, sizeof(t));
    return 0;
}

static int bmp085_release(struct node *nd, struct file *filp)
{
    return 0;
}

static const struct file_operations bmp085_ops = {
    .owner = THIS_MODULE,
    .open = bmp085_open,
    .read = bmp085_read,
    .release = bmp085_release,
};

static int bmp085_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    if(bmp085dev.major){
        bmp085dev.devid = MKDEV(bmp085dev.major, 0);
        register_chrdev_region(bmp085dev.devid, BMP085_CNT, BMP085_NAME);
    }else{
        alloc_chrdev_region(&bmp085dev.devid, 0, BMP085_CNT, BMP085_NAME);
        bmp085dev.major = MAJOR(bmp085dev.devid);
    }

    cdev_init(&bmp085dev.cdev, &bmp085_ops);
    cdev_add(&bmp085dev.cdev, bmp085dev.devid, BMP085_CNT);
    bmp085dev.class = class_create(THIS_MODULE, BMP085_NAME);
    if(IS_ERR(bmp085dev.class)){
        return PTR_ERR(bmp085dev.class);
    }

    bmp085dev.device = device_create(bmp085dev.class, NULL, bmp085dev.devid, NULL, BMP085_NAME);
    if(IS_ERR(bmp085dev.device)){
        return PTR_ERR(bmp085dev.device);
    }

    bmp085dev.private_data = client;
    return 0;
}

static int bmp085_remove(struct i2c_client *client)
{
    cdev_del(&bmp085dev.cdev);
    unregister_chrdev_region(bmp085dev.devid, BMP085_CNT);
    device_destroy(bmp085dev.class, bmp085dev.devid);
    class_destroy(bmp085dev.class);
    return 0;
}

static const struct i2c_device_id bmp085_id[] = {
    {"bmp085", 0},
    {}
};

static const struct of_device_id bmp085_of_match[] = {
    {.compatible = "bmp085"},
    {}
};

static struct i2c_driver bmp085_driver = {
    .probe = bmp085_probe,
    .remove = bmp085_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "bmp085",
        .of_match_table = bmp085_of_match,
    },
    .id_table = bmp085_id,
};

static int __init bmp085_init(void)
{
    int ret = 0;

    ret = i2c_add_driver(&bmp085_driver);
    return ret;
}

static void __exit bmp085_exit(void)
{
    i2c_del_driver(&bmp085_driver);
}

module_init(bmp085_init);
module_exit(bmp085_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wanyisq");