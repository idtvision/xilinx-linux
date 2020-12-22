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

/* Support hardware revision */
#define IDT_VDM_REV 89

/**
 * struct vdm_device - DMA device structure
 * @reg: I/O  mapped base address
 * @dev: Device Structure
 */
struct vdm_device {
	void __iomem *regs;
	struct device *dev;
};

static const struct of_device_id vdm_of_ids[] = {
	{ .compatible = "idt,vdm-1.00" },
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
	//struct device_node *node = pdev->dev.of_node;
	struct vdm_device *vdev;
	struct resource *io;
	unsigned rev;

	/* Allocate inst struct */
	vdev = devm_kzalloc(&pdev->dev, sizeof(*vdev), GFP_KERNEL);
	if (!vdev)	
		return -ENOMEM;
	vdev->dev = &pdev->dev;

	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	vdev->regs = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(vdev->regs))
		return PTR_ERR(vdev->regs);

	rev = ioread32(vdev->regs);
	/* Check hardware revision */
	if (rev != IDT_VDM_REV) {
		dev_err(&pdev->dev, "Wrong HW version %d; support version is %d\n", rev, IDT_VDM_REV);
		return -EFAULT;
	}
	printk(KERN_INFO"Loaded vdm driver ver=%d\n", rev);
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
	of_dma_controller_free(pdev->dev.of_node);
	printk(KERN_INFO"Removed vdm driver\n");
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
