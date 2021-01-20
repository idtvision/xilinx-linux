/*
 * xsm.c -- XStreamMini camera driver
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/interrupt.h> /* mark_bh */

#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>
#include <linux/version.h> 	/* LINUX_VERSION_CODE  */

#include <linux/of_address.h>
#include <linux/of_dma.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>

#define XSM_DEBUG 1
#include "xsm.h"

#include <linux/in6.h>
#include <asm/checksum.h>

#define DRIVER_NAME "xsm_vdm"
#define DRIVER_VERSION  "1.0"

/*
 * Transmitter lockup simulation, normally disabled.
 */
static int lockup = 0;
module_param(lockup, int, 0);

static int timeout = XSM_TIMEOUT;
module_param(timeout, int, 0);

/*
 * A structure representing an in-flight packet.
 */
struct xsm_packet {
	struct xsm_packet *next;
	struct net_device *dev;
	int	datalen;
	u8 data[ETH_DATA_LEN];
};

static int pool_size = 8;
module_param(pool_size, int, 0);

/*
 * This structure is private to each device. It is used to pass
 * packets in and out, so there is place for a packet
 */

struct xsm_priv {
	struct net_device_stats stats;
	int status;
	struct xsm_packet *ppool;
	struct xsm_packet *rx_queue;  /* List of incoming packets */
	int rx_int_enabled;
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	spinlock_t lock;
	struct net_device *dev;
};

static struct net_device *xsm_devs[2];

static const struct of_device_id xsm_of_ids[] = {
    { .compatible = "idt,xsm-1.00" },
    {}
};
MODULE_DEVICE_TABLE(of, xsm_of_ids);

/*
 * Set up a device's packet pool.
 */
static void xsm_setup_pool(struct net_device *dev)
{
	struct xsm_priv *priv = netdev_priv(dev);
	int i;
	struct xsm_packet *pkt;

	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof (struct xsm_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
			return;
		}
		pkt->dev = dev;
		pkt->next = priv->ppool;
		priv->ppool = pkt;
	}
}

static void xsm_teardown_pool(struct net_device *dev)
{
	struct xsm_priv *priv = netdev_priv(dev);
	struct xsm_packet *pkt;
    
	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		kfree (pkt);
		/* FIXME - in-flight packets ? */
	}
}    

/*
 * Buffer/pool management.
 */
static struct xsm_packet *xsm_get_tx_buffer(struct net_device *dev)
{
	struct xsm_priv *priv = netdev_priv(dev);
	unsigned long flags;
	struct xsm_packet *pkt;
    
	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->ppool;
	if(!pkt) {
		PDEBUG("Out of Pool\n");
		goto out;
	}
	priv->ppool = pkt->next;
	if (priv->ppool == NULL) {
		printk (KERN_INFO "Pool empty\n");
		netif_stop_queue(dev);
	}
       out:
	spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}


static void xsm_release_buffer(struct xsm_packet *pkt)
{
	unsigned long flags;
	struct xsm_priv *priv = netdev_priv(pkt->dev);
	
	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->ppool;
	priv->ppool = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
	if (netif_queue_stopped(pkt->dev) && pkt->next == NULL)
		netif_wake_queue(pkt->dev);
}

static void xsm_enqueue_buf(struct net_device *dev, struct xsm_packet *pkt)
{
	unsigned long flags;
	struct xsm_priv *priv = netdev_priv(dev);

	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->rx_queue;  /* FIXME - misorders packets */
	priv->rx_queue = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
}

static struct xsm_packet *xsm_dequeue_buf(struct net_device *dev)
{
	struct xsm_priv *priv = netdev_priv(dev);
	struct xsm_packet *pkt;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->rx_queue;
	if (pkt != NULL)
		priv->rx_queue = pkt->next;
	spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}

/*
 * Enable and disable receive interrupts.
 */
static void xsm_rx_ints(struct net_device *dev, int enable)
{
	struct xsm_priv *priv = netdev_priv(dev);
	priv->rx_int_enabled = enable;
}

    
/*
 * Open and close
 */

static int xsm_open(struct net_device *dev)
{
	/* request_region(), request_irq(), ....  (like fops->open) */

	/* 
	 * The first byte is '\0' to avoid being a multicast addr.
	 * Assign an IDT MAC address
	 */
	memcpy(dev->dev_addr, "\x00\x25\x16\x00\x00\x00", ETH_ALEN);
	if (dev == xsm_devs[1])
		dev->dev_addr[ETH_ALEN-1]++;
	netif_start_queue(dev);
	return 0;
}

static int xsm_release(struct net_device *dev)
{
	/* release ports, irq and such -- like fops->close */

	netif_stop_queue(dev); /* can't transmit any more */
	return 0;
}

/*
 * Configuration changes (passed on by ifconfig)
 */
static int xsm_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) /* can't act on a running interface */
		return -EBUSY;

	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "xsm: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}

	/* Allow changing the IRQ */
	if (map->irq != dev->irq) {
		dev->irq = map->irq;
        	/* request_irq() is delayed to open-time */
	}

	/* ignore other fields */
	return 0;
}

/*
 * Receive a packet: retrieve, encapsulate and pass over to upper levels
 */
static void xsm_rx(struct net_device *dev, struct xsm_packet *pkt)
{
	struct sk_buff *skb;
	struct xsm_priv *priv = netdev_priv(dev);

	/*
	 * The packet has been retrieved from the transmission
	 * medium. Build an skb around it, so upper layers can handle it
	 */
	skb = dev_alloc_skb(pkt->datalen + 2);
	if (!skb) {
		if (printk_ratelimit())
			printk(KERN_NOTICE "xsm rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		goto out;
	}
	skb_reserve(skb, 2); /* align IP on 16B boundary */  
	memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);

	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += pkt->datalen;
	netif_rx(skb);
  out:
	return;
}
    
/*
 * The typical interrupt entry point
 */
static void xsm_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	struct xsm_priv *priv;
	struct xsm_packet *pkt = NULL;
	/*
	 * As usual, check the "device" pointer to be sure it is
	 * really interrupting.
	 * Then assign "struct device *dev"
	 */
	struct net_device *dev = (struct net_device *)dev_id;
	/* ... and check with hw if it's really ours */

	/* paranoid */
	if (!dev)
		return;

	/* Lock the device */
	priv = netdev_priv(dev);
	spin_lock(&priv->lock);

	/* retrieve statusword: real netdevices use I/O instructions */
	statusword = priv->status;
	priv->status = 0;
	if (statusword & XSM_RX_INTR) {
		/* send it to xsm_rx for handling */
		pkt = priv->rx_queue;
		if (pkt) {
			priv->rx_queue = pkt->next;
			xsm_rx(dev, pkt);
		}
	}
	if (statusword & XSM_TX_INTR) {
		/* a transmission is over: free the skb */
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += priv->tx_packetlen;
		dev_kfree_skb(priv->skb);
	}

	/* Unlock the device and we are done */
	spin_unlock(&priv->lock);
	if (pkt) xsm_release_buffer(pkt); /* Do this outside the lock! */
	return;
}

/*
 * Transmit a packet (called by the kernel)
 */
static int xsm_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	char *data, shortpkt[ETH_ZLEN];
	struct xsm_priv *priv = netdev_priv(dev);

	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	netif_trans_update(dev);

	/* Remember the skb, so we can free it at interrupt time */
	priv->skb = skb;

	/* actual deliver of data is device-specific, and not shown here */
	//xsm_hw_tx(data, len, dev);

	/* Update TX counters */
	priv = netdev_priv(dev);
	spin_lock(&priv->lock);
	priv->status = 0;
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += len;
	dev_kfree_skb(priv->skb);
	spin_unlock(&priv->lock);

	return 0; /* Our simple device can not fail */
}

/**
* Deal with a transmit timeout.
* See https://github.com/torvalds/linux/commit/0290bd291cc0e0488e35e66bf39efcd7d9d9122b
* for signature change which occurred on kernel 5.6
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
static void xsm_tx_timeout (struct net_device *dev)
#else
static void xsm_tx_timeout (struct net_device *dev, unsigned int txqueue)
#endif
{
	struct xsm_priv *priv = netdev_priv(dev);
        struct netdev_queue *txq = netdev_get_tx_queue(dev, 0);

	PDEBUG("Transmit timeout at %ld, latency %ld\n", jiffies,
			jiffies - txq->trans_start);
        /* Simulate a transmission interrupt to get things moving */
	priv->status |= XSM_TX_INTR;
	//xsm_interrupt(0, dev, NULL);
	priv->stats.tx_errors++;

	/* Reset packet pool */
	spin_lock(&priv->lock);
	xsm_teardown_pool(dev);
	xsm_setup_pool(dev);
	spin_unlock(&priv->lock);

	netif_wake_queue(dev);
	return;
}



/*
 * Ioctl commands 
 */
static int xsm_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	PDEBUG("ioctl\n");
	return 0;
}

/*
 * Return statistics to the caller
 */
static struct net_device_stats *xsm_stats(struct net_device *dev)
{
	struct xsm_priv *priv = netdev_priv(dev);
	return &priv->stats;
}

/*
 * This function is called to fill up an eth header, since arp is not
 * available on the interface
 */
static int xsm_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *) skb->data;
	struct net_device *dev = skb->dev;
    
	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
	return 0;
}


static int xsm_header(struct sk_buff *skb, struct net_device *dev,
                unsigned short type, const void *daddr, const void *saddr,
                unsigned len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest,   daddr ? daddr : dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
	return (dev->hard_header_len);
}

/*
 * The "change_mtu" method is usually not needed.
 * If you need it, it must be like this.
 */
static int xsm_change_mtu(struct net_device *dev, int new_mtu)
{
	unsigned long flags;
	struct xsm_priv *priv = netdev_priv(dev);
	spinlock_t *lock = &priv->lock;
    
	/* check ranges */
	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;
	/*
	 * Do anything you need, and the accept the value
	 */
	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	return 0; /* success */
}

static const struct header_ops xsm_header_ops = {
        .create  = xsm_header,
};

static const struct net_device_ops xsm_netdev_ops = {
	.ndo_open            = xsm_open,
	.ndo_stop            = xsm_release,
	.ndo_start_xmit      = xsm_tx,
	.ndo_do_ioctl        = xsm_ioctl,
	.ndo_set_config      = xsm_config,
	.ndo_get_stats       = xsm_stats,
	.ndo_change_mtu      = xsm_change_mtu,
	.ndo_tx_timeout      = xsm_tx_timeout,
};

/*
 * The init function (sometimes called probe).
 * It is invoked by register_netdev()
 */
static void xsm_init(struct net_device *dev)
{
	struct xsm_priv *priv;
#if 0
    	/*
	 * Make the usual checks: check_region(), probe irq, ...  -ENODEV
	 * should be returned if no device found.  No resource should be
	 * grabbed: this is done on open(). 
	 */
#endif

    	/* 
	 * Then, assign other fields in dev, using ether_setup() and some
	 * hand assignments
	 */
	ether_setup(dev); /* assign some of the fields */
	dev->watchdog_timeo = timeout;
	dev->netdev_ops = &xsm_netdev_ops;
	dev->header_ops = &xsm_header_ops;
	/* keep the default flags, just add NOARP */
	dev->flags           |= IFF_NOARP | IFF_POINTOPOINT;
	/* No broadcast or multicast */
	dev->flags           &= ~(IFF_BROADCAST|IFF_MULTICAST);
	dev->features        |= NETIF_F_HW_CSUM;

	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct xsm_priv));
	spin_lock_init(&priv->lock);
	priv->dev = dev;

	netif_carrier_off(dev);

	xsm_rx_ints(dev, 1);		/* enable receive interrupts */
	xsm_setup_pool(dev);
}

static int free_net_devs(struct platform_device *pdev)
{
	int i;
    
	for (i = 0; i < 2;  i++) {
		if (xsm_devs[i]) {
			unregister_netdev(xsm_devs[i]);
			xsm_teardown_pool(xsm_devs[i]);
			free_netdev(xsm_devs[i]);
		}
	}
	return 0;
}

static int register_net_devs(struct platform_device *pdev)
{
	int result, i, ret = -ENOMEM;
	struct resource *io;
	void __iomem *regs;
	void __iomem *cam;
	unsigned rev;
	int irq;

	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cam = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(cam))
		return PTR_ERR(cam);
	rev = ioread32(cam);
	// TODO: need to name all the regs in both regions
	printk("camera ID is %x\n", rev);

	io = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	regs = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	rev = ioread32(regs);
	printk("reg0=%x\n", rev);
	printk("link %s\n", (1 & ioread32(regs + 0x1c))? "up": "down");
	rev = ioread32(regs);
	/* TODO: need to record these pointers into the dev structure */

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		printk("xsm: platform get irq failed");
		return irq;
	}

	/* Allocate the devices */
	xsm_devs[0] = alloc_netdev(sizeof(struct xsm_priv), "xsm%d",
			NET_NAME_UNKNOWN, xsm_init);
	xsm_devs[1] = alloc_netdev(sizeof(struct xsm_priv), "xsm%d",
			NET_NAME_UNKNOWN, xsm_init);
	if (xsm_devs[0] == NULL || xsm_devs[1] == NULL)
		goto out;

	ret = -ENODEV;
	for (i = 0; i < 2;  i++)
		if ((result = register_netdev(xsm_devs[i])))
			printk("xsm: error %i registering device \"%s\"\n",
					result, xsm_devs[i]->name);
		else
			ret = 0;
   out:
	if (ret) 
		free_net_devs(pdev);
	return ret;
}

static struct platform_driver xsm_driver = {
    .driver = {
        .name = "xsm",
        .of_match_table = xsm_of_ids,
    },
    .probe = register_net_devs,
    .remove = free_net_devs,
};

static void __exit xsm_cleanup(void)
{
	platform_driver_unregister(&xsm_driver);
}

static int __init xsm_init_module(void)
{
	int err;
	err = platform_driver_register(&xsm_driver);
	if (err < 0) {
		pr_err("%s Unabled to register %s driver",
				__func__, DRIVER_NAME);
		return err;
	}
	return 0;
}
module_init(xsm_init_module);
module_exit(xsm_cleanup);

MODULE_AUTHOR("Alexander Ivanov");
MODULE_DESCRIPTION("XStreamMini Camera");
MODULE_LICENSE("GPL v2");
