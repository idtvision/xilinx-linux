/**
 * Video Data Mover
 *
 * Microcoded data mover DMA
 *
 */

#include <linux/bitops.h>
#include <linux/dmapool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_dma.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io-64-nonatomic-lo-hi.h>

#include "../dmaengine.h"

static const struct of_device_id vdm_of_ids[] = {
	{ .compatible = "idt,vdm-1.00", .data = 0xdeadbeef },
	{}
};
MODULE_DEVICE_TABLE(of, vdm_of_ids);

/**
 * vdm_probe - Driver probe function
 * @pdev: Pointer to the platform_device structure
 *
 * Return: '0' on success and failure value on error
 */
static int vdm_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct device_node *child, *np = pdev->dev.of_node;
	printk(KERN_INFO"loaded vdm driver\n");
	return 0;
}

/**
 * vdm_remove - Driver remove function
 * @pdev: Pointer to the platform_device structure
 *
 * Return: Always '0'
 */
static int vdm_remove(struct platform_device *pdev)
{
	printk(KERN_INFO"removed vdm driver\n");
	return 0;
}

static struct platform_driver vdm_driver = {
	.driver = {
		.name = "vdm",
		.of_match_table = vdm_of_ids,
	},
	.probe = vdm_probe,
	.remove = vdm_remove,
};

module_platform_driver(vdm_driver);

MODULE_AUTHOR("Alex Ivanov");
MODULE_DESCRIPTION("Video Data Mover");
MODULE_LICENSE("GPL v2");
