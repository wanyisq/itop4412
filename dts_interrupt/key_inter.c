#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include <linux/miscdevice.h>
#include <linux/fs.h>

#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio_keys.h>
#include <linux/of_irq.h>

#define DRIVER_NAME "user_key"

int gpio_irq_num = 0;

static irqreturn_t eint_interrupt(int irq, void *dev_id) 
{
	printk("HOME KEY HIGH TO LOW!\n");
	
	return IRQ_HANDLED;
}

static int inter_probe(struct platform_device * pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int ret;

	printk("inter_probe\r");
	gpio_irq_num = irq_of_parse_and_map(node, 0);
	printk("irq num is %d\r", gpio_irq_num);
	
	ret = request_irq(gpio_irq_num, eint_interrupt,IRQ_TYPE_EDGE_FALLING, "user_init_key", pdev);
	if (ret < 0) {
			printk("Request IRQ %d failed, %d\n", gpio_irq_num,ret);
			return -1;
	}
    return 0;
}

static int inter_remove(struct platform_device * pdev)
{
	free_irq(gpio_irq_num, pdev);
	
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
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_inter_dt_match,
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