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
#include <linux/debugfs.h>

#include "../dmaengine.h"

/* Support hardware revision */
#define IDT_VDM_REV 89

/**
 * struct vdm_device - DMA device structure
 * @reg: I/O  mapped base address
 * @dev: Device Structure
 * @debugfs_dir: Debug FS directory ptr
 */
struct vdm_device {
	void __iomem *regs;
	struct device *dev;
	struct dentry *debugfs_dir;
};

static const struct of_device_id vdm_of_ids[] = {
	{ .compatible = "idt,vdm-1.00" },
	{}
};
MODULE_DEVICE_TABLE(of, vdm_of_ids);

static void idt_debugfs_remove(struct platform_device *pdev)
{
	struct vdm_device *vdev = platform_get_drvdata(pdev);
        debugfs_remove_recursive(vdev->debugfs_dir);
        vdev->debugfs_dir = NULL;
}

static int idt_debugfs_create(struct platform_device *pdev)
{
        const char *devnode;
	int err;
        char debugfs_dir[32];
	struct vdm_device *vdev = platform_get_drvdata(pdev);

        err = of_property_read_string(pdev->dev.of_node, "devnode", &devnode);
        if (err) {
                dev_err(&pdev->dev, "devnode not in DT\n");
                return err;
        }
        snprintf(debugfs_dir, sizeof(debugfs_dir), "vdm-%s", devnode);

        vdev->debugfs_dir = debugfs_create_dir(debugfs_dir, NULL);
        if (vdev->debugfs_dir == NULL)
                return -ENOMEM;

#if 0
	TODO: find out how to use this create file thing
	if (!debugfs_create_file("streaming", 0644, priv->debugfs_dir, priv,
                        &blade_debugfs_streaming_fops))
                goto error;
#endif

        return 0;

//error:
        //idt_debugfs_remove(priv);

        //return -ENOMEM;
}

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
	int err;

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
	/* TODO: vdev is lost after this?; need to register it somewhere */
	/* Check hardware revision */
	if (rev != IDT_VDM_REV) {
		dev_err(&pdev->dev, "Wrong HW version %d; support version is %d\n", rev, IDT_VDM_REV);
		return -EFAULT;
	}

	platform_set_drvdata(pdev, vdev);

	err = idt_debugfs_create(pdev);
	if (err)
		return err;

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
	struct vdm_device *vdev = platform_get_drvdata(pdev);
	idt_debugfs_remove(pdev);
	printk(KERN_INFO"Removed vdm driver vdev=0x%p\n", vdev);
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
