/**
 * Video Data Mover
 *
 * Microcoded data mover DMA
 *
 */
#include <linux/cdev.h>
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
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/debugfs.h>

#include "vdm_controller_regs.h"
#include "vdm_controller_ops.h"
#include "vdm_kern_api.h"

#define IDT_VDM_REV VDM_CONTROLLER_REVISION

#define DRIVER_NAME "idt_vdm"
#define DRIVER_VERSION  "1.0"
#define DRIVER_MAX_DEV  BIT(MINORBITS)


static struct class *vdm_class;
static atomic_t vdm_ndevs = ATOMIC_INIT(0);
static dev_t vdm_devt;

typedef enum VDM_TYPE {
	VDM_CAM,
	VDM_MIPI,
	VDM_MIPI_PLAYBACK,
	VDM_OTHER
} VDM_TYPE;

/**
 * struct vdm_device - DMA device structure
 * @regs: I/O  mapped base address of the VDM regs
 * @regs1: I/O  mapped base address of the aux regs block
 * @dev: Device Structure
 * @debugfs_dir: Debug FS directory ptr
 * @vdm_cdev: Charachter device handle
 * @open_count: Count of char device being opened
 * @vdm_id: ID of the VDM instance
 * @irq: IRQ number
 * @waitq: IRQ wait queue
 * @line_irq: line irq happened
 * @node: name of this instance from the DT
 */
struct vdm_device {
	void __iomem *regs;
	void __iomem *regs1;
	struct device *dev;
	struct dentry *debugfs_dir;
	struct cdev vdm_cdev;
	atomic_t open_count;
	s32 vdm_id;
	int irq;
	wait_queue_head_t waitq;
	bool line_irq;
	unsigned irq_cnt;
	char node[32];
	VDM_TYPE vdm_type;
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

static const unsigned consts_idx = 0x40; /* constants begin at 0x40 * 4 */


// @op: 0 (main), 1(calibration, not on MIPI node)
// @cal_offs: start writing to the DDR with this offset
static void load_program(struct vdm_device *vdev, unsigned op, unsigned cal_offs) {
	static const unsigned dma_addr = consts_idx + 0;
	static const unsigned dma_size = consts_idx + 1;
	static const unsigned buf_siz = consts_idx + 2;
	static const unsigned irq_val = consts_idx + 3;
	static const unsigned zero_val = consts_idx + 4;
	static const unsigned cal_addr = consts_idx + 5;
	static const unsigned cal_size = consts_idx + 6;
	static const unsigned cal_start_addr = consts_idx + 7;
	static const unsigned cnt = consts_idx + 8;
	//static const unsigned dma_buff_addr = consts_idx + 3;
	//static const unsigned buf_size = 1*1024*1024*1024U;
	static const unsigned line_size = 3840;
	static const unsigned frame_size = 2160 * line_size;
	//static const unsigned n_frames = buf_size / frame_size;
	//static const unsigned last_frame_addr = frame_size * n_frames;
	static const unsigned ram = PROGRAM_OFFSET;
	unsigned prog = ram;
	unsigned label0, label1;
        bool mipi_prog = vdev->vdm_type == VDM_MIPI;
        bool mipi_playback_prog = vdev->vdm_type == VDM_MIPI_PLAYBACK;
	unsigned tx_size = frame_size; // full UHD frame
	unsigned cal_tx_size = frame_size*20/8; // full UHD frame 20 bits per pixel
	static const unsigned cal_base_addr = 512*1024*1024;
	int i;

	/* Load data */
	#define dload(d, idx) \
		iowrite32(d, vdev->regs + ram + 4*idx);
	dload(0, dma_addr);
	dload(tx_size, dma_size);
	dload(1, irq_val);
	dload(0, zero_val);
	dload(64, buf_siz);
	/* Load instructions */
	#define nexti(inst) \
		iowrite32(inst, \
				vdev->regs + prog); \
       		prog += 4
	#define lbl() ((prog - ram)/4)
	if (mipi_prog) {
		//dload(3, irq_val); // disable calibration
		//dload(2, zero_val); // siable cal
		dload(cal_base_addr + 0x38, cal_start_addr); // +0x38 to skip the header
		dload(cal_tx_size/4, cal_size);
		label0 = lbl();
		nexti(zero(dma_addr));
		for (i = 0; i < 4; i++) {
			// send frame
			nexti(in0(dma_addr)); 
			nexti(dma(0, dma_addr, dma_size));
			// send calibration (in 4 chunks)
			nexti(assign_mem(cal_addr, cal_start_addr));
			nexti(dma(1, cal_addr, cal_size));
			nexti(add_mem(cal_addr, cal_size));
			nexti(dma(1, cal_addr, cal_size));
			nexti(add_mem(cal_addr, cal_size));
			nexti(dma(1, cal_addr, cal_size));
			nexti(add_mem(cal_addr, cal_size));
			nexti(dma(1, cal_addr, cal_size));
			//nexti(add_mem(dma_addr, dma_size));
		}

		//nexti(add_mem(dma_addr, dma_size));
		for (i = 0; i < 3; i++)
			nexti(out0(irq_val)); // send an IRQ (3 clocks wide)
		nexti(out0(zero_val)); // turn off IRQ line
		nexti(br_imm(label0));
        	dev_info(vdev->dev, "load live mipi program\n");
		return;
	} else if (mipi_playback_prog) {
		dload(cal_base_addr + 0x38, cal_start_addr); // +0x38 to skip the header
		dload(cal_tx_size/4, cal_size);
		label0 = lbl();
		nexti(zero(dma_addr));
		nexti(zero(cnt));
		{ // repeat buf_siz times
			label1 = lbl();
			nexti(dma(0, dma_addr, dma_size));
			// send calibration (in 4 chunks)
			nexti(assign_mem(cal_addr, cal_start_addr));
			nexti(dma(1, cal_addr, cal_size));
			nexti(add_mem(cal_addr, cal_size));
			nexti(dma(1, cal_addr, cal_size));
			nexti(add_mem(cal_addr, cal_size));
			nexti(dma(1, cal_addr, cal_size));
			nexti(add_mem(cal_addr, cal_size));
			nexti(dma(1, cal_addr, cal_size));
			nexti(add_mem(dma_addr, dma_size));
			nexti(add_imm(cnt, 1));
			nexti(brl(cnt, buf_siz, label1));
		}

		for (i = 0; i < 3; i++)
			nexti(out0(irq_val)); // send an IRQ (3 clocks wide)
		nexti(out0(zero_val)); // turn off IRQ line
		nexti(br_imm(label0));
        	dev_info(vdev->dev, "load mipi playback program\n");
		return;
	} else if (op == 1) {
		dload(cal_base_addr + cal_offs, cal_addr);
		// calibration comes in 8MB chunks
		// here one chunk is loaded, the program is hung
		// 4 times use 2 MB here to fit into 23 bits data mover limit
		dload(2*1024*1024, cal_size);
		nexti(dma(0, cal_addr, cal_size));
		nexti(add_mem(cal_addr, cal_size));
		nexti(dma(0, cal_addr, cal_size));
		nexti(add_mem(cal_addr, cal_size));
		nexti(dma(0, cal_addr, cal_size));
		nexti(add_mem(cal_addr, cal_size));
		nexti(dma(0, cal_addr, cal_size));
		nexti(add_mem(cal_addr, cal_size));
		nexti(hang());
        	dev_info(vdev->dev, "load cal program\n");
		return;
	} else {
		label0 = lbl();
		nexti(zero(dma_addr));
		nexti(zero(cnt));

		{ // repeat buf_siz times
			label1 = lbl();
			nexti(dma(0, dma_addr, dma_size));
			nexti(out1(dma_addr));
			nexti(add_mem(dma_addr, dma_size));
			nexti(add_imm(cnt, 1));
			nexti(brl(cnt, buf_siz, label1));
		}

		for (i = 0; i < 3; i++)
			nexti(out0(irq_val)); // send an IRQ
		nexti(out0(zero_val)); // turn off IRQ line
		nexti(br_imm(label0));
        	dev_info(vdev->dev, "load cam program\n");
		return;
	}
	#undef dload
	#undef lbl
	#undef nexti
}

static int vdm_debugfs_streaming_write(void *data, u64 val)
{
        int err = 0;
        struct vdm_device *vdev = data;
	unsigned code = val & 0xffffffff;
	unsigned param = val >> 32;
        bool load_cal = code == 2;
        bool enable = code == 1;

        //dev_info(&client->dev, "%s: %s sensor\n",
                        //__func__, (enable ? "enabling" : "disabling"));

        //mutex_lock(&priv->streaming_lock);
	if (load_cal) {
		iowrite32(CONTROL_RESET_BIT_MASK, vdev->regs + CONTROL_OFFSET);
		iowrite32(0, vdev->regs + CONTROL_OFFSET);
		load_program(vdev, 1, param);
		iowrite32(CONTROL_RUN_BIT_MASK, vdev->regs + CONTROL_OFFSET);
	} else if (enable) {
		iowrite32(CONTROL_RESET_BIT_MASK, vdev->regs + CONTROL_OFFSET);
		iowrite32(0, vdev->regs + CONTROL_OFFSET);
		load_program(vdev, 0, 0);
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
	int i;
	ssize_t ret;
	struct vdm_device *vdev = file->private_data;
	unsigned status_reg;

	buff = kmalloc(PAGE_SIZE*2, GFP_KERNEL);
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
	desc += sprintf(buff + desc,
			"VDM PROGRAM DUMP\n"
			"----------------\n");
	for (i = 0; i < (consts_idx*4); i += 4) {
		unsigned int word = ioread32(vdev->regs + PROGRAM_OFFSET + i);
		desc += sprintf(buff + desc, "%04x %08x\n", i, word);
	}
	desc += sprintf(buff + desc,
			"VDM CONSTANTS DUMP\n"
			"----------------\n");
	for (i = (consts_idx*4); i < (consts_idx*4 + 16*4); i += 4) {
		unsigned int word = ioread32(vdev->regs + PROGRAM_OFFSET + i);
		desc += sprintf(buff + desc, "%04x %08x\n", i, word);
	}
	ret = simple_read_from_buffer(user_buf, count, ppos, buff, desc);
	kfree(buff);
	return ret;
}

static const struct file_operations vdm_debugfs_status_fops = {
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
	strncpy(vdev->node, devnode, sizeof(vdev->node));
	vdev->node[sizeof(vdev->node)-1] = 0;
	vdev->vdm_type = 0;
	if (0 == strncasecmp(vdev->node, "cam", 3)) {
		vdev->vdm_type = VDM_CAM;
		dev_info(vdev->dev, "detected cam VDM node %s\n", vdev->node);
	} else if (0 == strncasecmp(vdev->node, "mipi0", 5)) {
		vdev->vdm_type = VDM_MIPI;
		dev_info(vdev->dev, "detected live mipi VDM node %s\n", vdev->node);
	} else if (0 == strncasecmp(vdev->node, "mipi1", 5)) {
		vdev->vdm_type = VDM_MIPI_PLAYBACK;
		dev_info(vdev->dev, "detected playback mipi VDM node %s\n", vdev->node);
	}
	dev_info(vdev->dev, "vdev=%p\n", vdev);

        vdev->debugfs_dir = debugfs_create_dir(debugfs_dir, NULL);
        if (vdev->debugfs_dir == NULL)
		return -ENOMEM;

	if (!debugfs_create_file("streaming", 0644, vdev->debugfs_dir, vdev,
		&vdm_debugfs_streaming_fops))
			goto error;
	if (!debugfs_create_file("status", 0600, vdev->debugfs_dir, vdev,
		&vdm_debugfs_status_fops))
			goto error;

        return 0;

error:
        idt_debugfs_remove(pdev);
        return -ENOMEM;
}


static unsigned int
vdm_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	struct vdm_device *vdev = file->private_data;
	if (!vdev)
		return POLLNVAL | POLLHUP;
	poll_wait(file, &vdev->waitq, wait);
	//printk(KERN_INFO"poll called\n");
	if (vdev->line_irq) {
        	mask |= POLLIN | POLLRDNORM; // POLLPRI;
		vdev->line_irq = 0;
	}
	
	return mask;
}

static int vdm_reset(struct vdm_device *vdev)
{
	iowrite32(CONTROL_RESET_BIT_MASK, vdev->regs + CONTROL_OFFSET);
	return 0;
}

static long
vdm_dev_ioctl(struct file *fptr, unsigned int cmd, unsigned long data)
{
	struct vdm_device *vdev  = fptr->private_data;
	void __user *arg = NULL;
	int rval = -EINVAL;
	int err = 0;

	if (!vdev)
		return err;

	if (_IOC_TYPE(cmd) != VDM_MAGIC) {
		dev_err(vdev->dev, "Not a VDM ioctl");
		return -ENOTTY;
	}

	/* check if ioctl argument is present and valid */
	if (_IOC_DIR(cmd) != _IOC_NONE) {
		arg = (void __user *)data;
		if (!arg) {
			dev_err(vdev->dev, "vdm ioctl argument is NULL Pointer");
			return rval;
		}
	}

	/* Access check of the argument if present */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, arg, _IOC_SIZE(cmd));

	if (err) {
		dev_err(vdev->dev, "Invalid vdm ioctl argument");
		return -EFAULT;
	}

	switch (cmd) {
		case VDM_IOCTL_RESET:
			rval = vdm_reset(vdev);
        		break;
		default:
			dev_err(vdev->dev,
            			"vdm-%s ioctl not implemented",
            			vdev->node);
			break;
	}
	return rval;
}

static int
vdm_dev_open(struct inode *iptr, struct file *fptr)
{
    struct vdm_device *vdm;

    vdm = container_of(iptr->i_cdev, struct vdm_device, vdm_cdev);
    if (!vdm)
        return  -EAGAIN;

    /* Only one open per device at a time */
    if (!atomic_dec_and_test(&vdm->open_count)) {
        atomic_inc(&vdm->open_count);
        return -EBUSY;
    }

    fptr->private_data = vdm;
    return 0;
}

static int
vdm_dev_release(struct inode *iptr, struct file *fptr)
{
    struct vdm_device *vdm;

    vdm = container_of(iptr->i_cdev, struct vdm_device, vdm_cdev);
    if (!vdm)
        return -EAGAIN;

    atomic_inc(&vdm->open_count);
    return 0;
}


static const struct file_operations vdm_fops = {
    .owner = THIS_MODULE,
    .open = vdm_dev_open,
    .release = vdm_dev_release,
    .unlocked_ioctl = vdm_dev_ioctl,
    .poll = vdm_poll,
};

static irqreturn_t
vdm_irq_thread(int irq, void *dev_id)
{
	struct vdm_device *vdev = dev_id;
	irqreturn_t ret = IRQ_HANDLED;
	WARN_ON(vdev->irq != irq);
	if (0) {
		if (0 == (vdev->irq_cnt & 0xff))
			printk(KERN_INFO"vdm %s irq %d\n",
				vdev->node, vdev->irq_cnt);
	}
	vdev->irq_cnt++;
	vdev->line_irq = 1;
	wake_up_interruptible(&vdev->waitq);
	return ret;
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
	struct device *dev_create;
	struct resource *io;
	unsigned rev;
	int err;
	bool irq_enabled = true;

	/* Allocate inst struct */
	vdev = devm_kzalloc(&pdev->dev, sizeof(*vdev), GFP_KERNEL);
	if (!vdev)	
		return -ENOMEM;
	vdev->dev = &pdev->dev;
	vdev->vdm_id = atomic_read(&vdm_ndevs);

	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	vdev->regs = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(vdev->regs))
		return PTR_ERR(vdev->regs);

	rev = ioread32(vdev->regs);
	/* Check hardware revision */
	if (rev != VDM_CONTROLLER_REVISION) {
		dev_err(&pdev->dev, "Wrong HW version %d; support version is %d\n",
			rev, VDM_CONTROLLER_REVISION);
		return -EFAULT;
	}

	/* Map the aux regs block */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	vdev->regs1 = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(vdev->regs1)) {
		dev_err(&pdev->dev, "aux regs are not available");
		vdev->regs1 = NULL;
	}

	vdev->irq = platform_get_irq(pdev, 0);
	if (vdev->irq < 0) {
		dev_dbg(&pdev->dev, "platform_get_irq failed");
		irq_enabled = false;
	}

	platform_set_drvdata(pdev, vdev);

	if (irq_enabled) {
		init_waitqueue_head(&vdev->waitq);
		/* Register IRQ thread */
		err = devm_request_threaded_irq(&pdev->dev, vdev->irq, NULL,
			vdm_irq_thread,
			IRQF_ONESHOT,
			"vdm",
			vdev);
		if (err < 0) {
			dev_err(&pdev->dev, "unable to request IRQ%d", vdev->irq);
			goto err_vdm_dev;
		}
	}

	err = idt_debugfs_create(pdev);
	if (err)
		return err;

	cdev_init(&vdev->vdm_cdev, &vdm_fops);
	vdev->vdm_cdev.owner = THIS_MODULE;
	err = cdev_add(&vdev->vdm_cdev,
		MKDEV(MAJOR(vdm_devt), vdev->vdm_id), 1);
	if (err < 0) {
		dev_err(&pdev->dev, "cdev_add failed");
		err = -EIO;
		goto err_vdm_dev;
	}
	if (!vdm_class) {
		err = -EIO;
		dev_err(&pdev->dev, "vdm class not created correctly");
		goto err_vdm_cdev;
	}
	dev_create = device_create(vdm_class, vdev->dev,
			MKDEV(MAJOR(vdm_devt),
				vdev->vdm_id),
			vdev, "vdm%d", vdev->vdm_id);
	if (IS_ERR(dev_create)) {
		dev_err(&pdev->dev, "unable to create device");
		err = PTR_ERR(dev_create);
		goto err_vdm_cdev;
	}

	atomic_set(&vdev->open_count, 1);
	atomic_inc(&vdm_ndevs);
	printk(KERN_INFO"Loaded vdm driver ver=%d\n", rev);
	return 0;
err_vdm_cdev:
	cdev_del(&vdev->vdm_cdev);
err_vdm_dev:
	return err;
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
	device_destroy(vdm_class,
			MKDEV(MAJOR(vdm_devt), vdev->vdm_id));
	cdev_del(&vdev->vdm_cdev);
	atomic_dec(&vdm_ndevs);
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

//module_platform_driver(vdm_driver);

static int __init vdm_init_mod(void)
{
    int err;

    vdm_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(vdm_class)) {
        err = PTR_ERR(vdm_class);
        pr_err("%s : Unable to register vdm class", __func__);
        return err;
    }

    err = alloc_chrdev_region(&vdm_devt,
                  0, DRIVER_MAX_DEV, DRIVER_NAME);
    if (err < 0) {
        pr_err("%s : Unable to get major number", __func__);
        goto err_vdm_class;
    }

    err = platform_driver_register(&vdm_driver);
    if (err < 0) {
        pr_err("%s Unabled to register %s driver",
               __func__, DRIVER_NAME);
        goto err_vdm_drv;
    }
    return 0;

    /* Error Path */
err_vdm_drv:
    unregister_chrdev_region(vdm_devt, DRIVER_MAX_DEV);
err_vdm_class:
    class_destroy(vdm_class);
    return err;
}

static void __exit vdm_cleanup_mod(void)
{
    platform_driver_unregister(&vdm_driver);
    unregister_chrdev_region(vdm_devt, DRIVER_MAX_DEV);
    class_destroy(vdm_class);
    vdm_class = NULL;
}

module_init(vdm_init_mod);
module_exit(vdm_cleanup_mod);

MODULE_AUTHOR("Alex Ivanov");
MODULE_DESCRIPTION("Video Data Mover");
MODULE_LICENSE("GPL v2");
