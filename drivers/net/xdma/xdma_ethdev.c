#include "xdma_ethdev.h"

static struct rte_pci_id xdma_pci_id_tbl[] = {
#define RTE_PCI_DEV_ID_DECL(vend, dev) {RTE_PCI_DEVICE(vend, dev)},
#ifndef PCI_VENDOR_ID_XILINX
#define PCI_VENDOR_ID_XILINX 0x10ee
#endif

	/** Gen 1 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9011)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9111)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9211)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9311)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9014)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9114)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9214)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9314)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9018)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9118)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9218)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9318)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x901f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x911f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x921f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x931f)	/** PF 3 */

	/** Gen 2 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9021)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9121)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9221)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9321)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9024)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9124)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9224)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9324)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9028)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9128)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9228)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9328)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x902f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x912f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x922f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x932f)	/** PF 3 */

	/** Gen 3 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9031)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9131)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9231)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9331)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9034)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9134)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9234)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9334)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9038)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9138)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9238)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9338)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x903f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x913f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x923f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x933f)	/** PF 3 */

	/** Gen 4 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9041)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9141)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9241)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9341)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9044)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9144)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9244)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9344)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9048)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9148)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9248)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0x9348)	/** PF 3 */

	/** Versal */
	/** Gen 1 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb011)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb111)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb211)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb311)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb014)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb114)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb214)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb314)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb018)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb118)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb218)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb318)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb01f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb11f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb21f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb31f)	/** PF 3 */

	/** Gen 2 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb021)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb121)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb221)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb321)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb024)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb124)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb224)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb324)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb028)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb128)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb228)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb328)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb02f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb12f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb22f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb32f)	/** PF 3 */

	/** Gen 3 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb031)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb131)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb231)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb331)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb034)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb134)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb234)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb334)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb038)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb138)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb238)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb338)	/** PF 3 */
	/** PCIe lane width x16 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb03f)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb13f)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb23f)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb33f)	/** PF 3 */

	/** Gen 4 PF */
	/** PCIe lane width x1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb041)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb141)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb241)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb341)	/** PF 3 */
	/** PCIe lane width x4 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb044)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb144)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb244)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb344)	/** PF 3 */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb048)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb148)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb248)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb348)	/** PF 3 */

	/** Gen 5 PF */
	/** PCIe lane width x8 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb058)	/** PF 0 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb158)	/** PF 1 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb258)	/** PF 2 */
	RTE_PCI_DEV_ID_DECL(PCI_VENDOR_ID_XILINX, 0xb358)	/** PF 3 */

	{ .vendor_id = 0, /* sentinel */ },
};

int xdma_eth_dev_init(struct rte_eth_dev *dev)
{
	struct xdma_dev *dma_priv;
	uint8_t *baseaddr;
	int i, idx, ret;
	struct rte_pci_device *pci_dev;


	/* sanity checks */
	if (dev == NULL)
		return -EINVAL;
	if (dev->data == NULL)
		return -EINVAL;
	if (dev->data->dev_private == NULL)
		return -EINVAL;

	pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	if (pci_dev == NULL)
		return -EINVAL;

	/* for secondary processes, we don't initialise any further as primary
	 * has already done this work.
	 */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY) {
		xdma_dev_ops_init(dev);
		return 0;
	}

	/* allocate space for a single Ethernet MAC address */
	dev->data->mac_addrs = rte_zmalloc("xdma", RTE_ETHER_ADDR_LEN * 1, 0);
	if (dev->data->mac_addrs == NULL)
		return -ENOMEM;

	/* Copy some dummy Ethernet MAC address for XDMA device
	 * This will change in real NIC device...
	 */
	for (i = 0; i < RTE_ETHER_ADDR_LEN; ++i)
		dev->data->mac_addrs[0].addr_bytes[i] = 0x15 + i;

	/* Init system & device */
    dma_priv = (struct xdma_dev *)dev->data->dev_private;
    dma_priv->config_bar_idx = DEFAULT_CONFIG_BAR;
	dma_priv->bypass_bar_idx = BAR_ID_INVALID;
	dma_priv->user_bar_idx = BAR_ID_INVALID;
    dma_priv->c2h_channel_max = XDMA_CHANNEL_NUM_MAX;
    dma_priv->h2c_channel_max = XDMA_CHANNEL_NUM_MAX;

	/* Check and handle device devargs*/
	if (xdma_check_kvargs(dev->device->devargs, dma_priv)) {
		PMD_DRV_LOG(INFO, "devargs failed\n");
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	/* Store BAR address and length of Config BAR */
	baseaddr = (uint8_t *)
			pci_dev->mem_resource[dma_priv->config_bar_idx].addr;
	dma_priv->bar[dma_priv->config_bar_idx] = baseaddr;

	/*Assigning XDMA access layer function pointers based on the HW design*/
	dma_priv->hw_access = rte_zmalloc("hwaccess",
					sizeof(struct xdma_hw_access), 0);   //fix!!!
	if (dma_priv->hw_access == NULL) {
		rte_free(dev->data->mac_addrs);
		return -ENOMEM;
	}
	idx = xdma_hw_access_init(dev, dma_priv->hw_access);  //fix!!!
	if (idx < 0) {
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}
	// idx = xdma_get_hw_version(dev);
	// if (idx < 0) {
	// 	rte_free(dma_priv->hw_access);
	// 	rte_free(dev->data->mac_addrs);
	// 	return -EINVAL;
	// }

	idx = xdma_identify_bars(dev);
	if (idx < 0) {
		rte_free(dma_priv->hw_access);
		rte_free(dev->data->mac_addrs);
		return -EINVAL;
	}

	/* Store BAR address and length of AXI Master Lite BAR(user bar) */
	if (dma_priv->user_bar_idx >= 0) {
		baseaddr = (uint8_t *)
			    pci_dev->mem_resource[dma_priv->user_bar_idx].addr;
		dma_priv->bar[dma_priv->user_bar_idx] = baseaddr;
	}

	if (dma_priv->bypass_bar_idx >= 0) {
		baseaddr = (uint8_t *)
			    pci_dev->mem_resource[dma_priv->bypass_bar_idx].addr;
		dma_priv->bar[dma_priv->bypass_bar_idx] = baseaddr;
	}


	xdma_dev_ops_init(dev);   //fix!!!

	/* Setting default Mode to RTE_PMD_QDMA_TRIG_MODE_USER_TIMER */
	dma_priv->trigger_mode = RTE_PMD_QDMA_TRIG_MODE_USER_TIMER;

	// ret = xdma_master_resource_create(pci_dev->addr.bus, max_pci_bus,
	// 			qbase, dma_priv->dev_cap.num_qs,
	// 			&dma_priv->dma_device_index);
	// if (ret == -QDMA_ERR_NO_MEM) {
	// 	rte_free(dma_priv->hw_access);
	// 	rte_free(dev->data->mac_addrs);
	// 	return -ENOMEM;
	// }

	return 0;
}

int xdma_eth_dev_uninit(struct rte_eth_dev *dev)
{
	struct xdma_dev *xdma_dev = dev->data->dev_private;
	//struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);

	/* only uninitialize in the primary process */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return -EPERM;

	if (xdma_dev->dev_configured)
		xdma_dev_close(dev);


	// /* cancel pending polls*/
	// if (xdma_dev->is_master)
	// 	rte_eal_alarm_cancel(xdma_check_errors, (void *)dev);

	/* Remove the device node from the board list */
	// xdma_dev_entry_destroy(xdma_dev->dma_device_index,
	// 		qdma_dev->func_id);
	// xdma_master_resource_destroy(xdma_dev->dma_device_index);

	dev->dev_ops = NULL;
	dev->rx_pkt_burst = NULL;
	dev->tx_pkt_burst = NULL;
	dev->data->nb_rx_queues = 0;
	dev->data->nb_tx_queues = 0;


	if (dev->data->mac_addrs != NULL) {
		rte_free(dev->data->mac_addrs);
		dev->data->mac_addrs = NULL;
	}

	// if (xdma_dev->q_info != NULL) {
	// 	rte_free(qdma_dev->q_info);
	// 	xdma_dev->q_info = NULL;
	// }

	if (xdma_dev->hw_access != NULL) {
		rte_free(xdma_dev->hw_access);
		xdma_dev->hw_access = NULL;
	}

#ifdef LATENCY_MEASUREMENT
	rte_memzone_free(txq_lat_buf_mz);
	rte_memzone_free(rxq_lat_buf_mz);
#endif

	return 0;
}

static int eth_xdma_pci_probe(struct rte_pci_driver *pci_drv __rte_unused,
				struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_probe(pci_dev,
						sizeof(struct xdma_dev),
						xdma_eth_dev_init);
}

/* Detach a ethdev interface */
static int eth_xdma_pci_remove(struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_remove(pci_dev, xdma_eth_dev_uninit);
}

static struct rte_pci_driver rte_xdma_pmd = {
	.id_table = xdma_pci_id_tbl,
	.drv_flags = RTE_PCI_DRV_NEED_MAPPING,
	.probe = eth_xdma_pci_probe,
	.remove = eth_xdma_pci_remove,
};

RTE_PMD_REGISTER_PCI(net_xdma, rte_xdma_pmd);
RTE_PMD_REGISTER_PCI_TABLE(net_xdma, xdma_pci_id_tbl);