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

#include "vdm_controller_regs.h"

#define IDT_VDM_REV VDM_CONTROLLER_REVISION

/* Status Register */
#define IDT_VDM_REG_STATUS	0x0004
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

static int vdm_debugfs_streaming_show(void *data, u64 *val)
{
        struct vdm_device *vdev = data;
        //mutex_lock(&vdev->streaming_lock);
	*val = 0 == (STATUS_STATE_IDLE_BIT_MASK & ioread32(vdev->regs + STATUS_OFFSET));

        //mutex_unlock(&vdev->streaming_lock);

        return 0;
}

static int vdm_debugfs_streaming_write(void *data, u64 val)
{
        int err = 0;
        struct vdm_device *vdev = data;
        bool enable = (val != 0);

        //dev_info(&client->dev, "%s: %s sensor\n",
                        //__func__, (enable ? "enabling" : "disabling"));

        //mutex_lock(&priv->streaming_lock);
        if (enable) {
		// TODO: load the program (version will depend on camera vs. mipi dma)
		iowrite32(CONTROL_RUN_BIT_MASK, vdev->regs + CONTROL_OFFSET);
	} else {
		iowrite32(CONTROL_RESET_BIT_MASK, vdev->regs + CONTROL_OFFSET);
	}
        //mutex_unlock(&priv->streaming_lock);

        return err;
}

DEFINE_SIMPLE_ATTRIBUTE(vdm_debugfs_streaming_fops,
        vdm_debugfs_streaming_show,
        vdm_debugfs_streaming_write,
        "%lld\n");

static ssize_t
vdm_debugfs_status_read(struct file *file, char __user *user_buf,
                  size_t count, loff_t *ppos)
{
	char *buff;
	int desc = 0;
	//int j;
	ssize_t ret;
	struct vdm_device *vdev = file->private_data;
	unsigned status_reg;

	buff = kmalloc(1024, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;
	status_reg = ioread32(vdev->regs + STATUS_OFFSET);
	desc += sprintf(buff + desc,
			"reg=0x%08X\n"
			"fifo ready=%d\n"
			"fifo empty=%d\n"
			"state_idle=%d\n"
			"state_post_trigger=%d\n"
			"tag=%d\n"
			"internal error=%d\n"
			"dec error=%d\n"
			"slave error=%d\n"
			"ok=%d\n"
			"integrity error=%d\n"
			"virt_regs=%p\n",
			status_reg,
			(status_reg & STATUS_FIFO_READY_BIT_MASK)
				>> STATUS_FIFO_READY_BIT_OFFSET,
			(status_reg & STATUS_FIFO_EMPTY_BIT_MASK)
				>> STATUS_FIFO_EMPTY_BIT_OFFSET,
			(status_reg & STATUS_STATE_IDLE_BIT_MASK)
				>> STATUS_STATE_IDLE_BIT_OFFSET ,
			(status_reg & STATUS_STATE_POST_TRIGGER_BIT_MASK)
				>> STATUS_STATE_POST_TRIGGER_BIT_OFFSET,
			(status_reg & STATUS_TAG_BIT_MASK)
				>> STATUS_TAG_BIT_OFFSET,
			(status_reg & STATUS_INTERNAL_ERROR_BIT_MASK)
				>> STATUS_INTERNAL_ERROR_BIT_OFFSET,
			(status_reg & STATUS_DEC_ERROR_BIT_MASK)
				>> STATUS_DEC_ERROR_BIT_OFFSET,
			(status_reg & STATUS_SLAVE_ERROR_BIT_MASK)
				>> STATUS_SLAVE_ERROR_BIT_OFFSET,
			(status_reg & STATUS_OK_BIT_MASK)
				>> STATUS_OK_BIT_OFFSET,
			(status_reg & STATUS_INTEGRITY_ERROR_BIT_MASK)
				>> STATUS_INTEGRITY_ERROR_BIT_OFFSET,
			vdev->regs);
	ret = simple_read_from_buffer(user_buf, count, ppos, buff, desc);
	kfree(buff);
	return ret;
}

static const struct file_operations vdm_debugfs_status_fpos = {
    .read = vdm_debugfs_status_read,
    .open = simple_open,
    .llseek = default_llseek,
};


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

	if (!debugfs_create_file("streaming", 0644, vdev->debugfs_dir, vdev,
		&vdm_debugfs_streaming_fops))
			goto error;
	if (!debugfs_create_file("status", 0600, vdev->debugfs_dir, vdev,
		&vdm_debugfs_status_fpos))
			goto error;

        return 0;

error:
        idt_debugfs_remove(pdev);
        return -ENOMEM;
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
	if (rev != VDM_CONTROLLER_REVISION) {
		dev_err(&pdev->dev, "Wrong HW version %d; support version is %d\n",
			rev, VDM_CONTROLLER_REVISION);
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
