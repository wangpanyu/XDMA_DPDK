#include "rte_pmd_xdma.h"
#include "ethdev_driver.h"
#include "xdma.h"
#include <stdint.h>

void rte_pmd_xdma_compat_memzone_reserve_aligned(void)
{
	const struct rte_memzone *mz = 0;

	mz = rte_memzone_reserve_aligned("eth_devices", RTE_MAX_ETHPORTS *
					  sizeof(*rte_eth_devices), 0, 0, 4096);

	if (mz == NULL)
		rte_exit(EXIT_FAILURE, "Failed to allocate aligned memzone\n");

	memcpy(mz->addr, &rte_eth_devices[0], RTE_MAX_ETHPORTS *
					sizeof(*rte_eth_devices));

    return ;
}

struct rte_device *rte_pmd_xdma_get_device(int port_id)
{
	struct rte_device *dev;
	dev = rte_eth_devices[port_id].device;
	return dev;
}

int rte_pmd_xdma_dev_remove(int port_id)
{
	struct rte_device *dev;
	dev = rte_eth_devices[port_id].device;
	return rte_dev_remove(dev);
}

int rte_pmd_xdma_dev_close(uint16_t port_id)
{
	struct rte_eth_dev *dev;
	struct xdma_dev *xdma_dev;

	dev = &rte_eth_devices[port_id];
	xdma_dev = dev->data->dev_private;

	dev->data->dev_started = 0;

	xdma_dev_close(dev);

	dev->data->nb_rx_queues = 0;
	rte_free(dev->data->rx_queues);
	dev->data->rx_queues = NULL;
	dev->data->nb_tx_queues = 0;
	rte_free(dev->data->tx_queues);
	dev->data->tx_queues = NULL;

	return 0;
}

int rte_pmd_xdma_get_bar_details(int port_id, int32_t *config_bar_idx,
			int32_t *user_bar_idx, int32_t *bypass_bar_idx)
{
	struct rte_eth_dev *dev;
	struct xdma_dev *dma_priv;


	dev = &rte_eth_devices[port_id];
	dma_priv = dev->data->dev_private;


	if (config_bar_idx != NULL)
		*(config_bar_idx) = dma_priv->config_bar_idx;

	if (user_bar_idx != NULL)
		*(user_bar_idx) = dma_priv->user_bar_idx;

	if (bypass_bar_idx != NULL)
		*(bypass_bar_idx) = dma_priv->bypass_bar_idx;

	return 0;
}

uint64_t rte_pmd_xdma_get_bar_addr(int port_id, int32_t bar_idx)
{
	struct rte_eth_dev *dev;
	struct xdma_dev *dma_priv;
	struct rte_pci_device *pci_dev;
	uint64_t baseaddr;


	dev = &rte_eth_devices[port_id];
	pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	dma_priv = dev->data->dev_private;

	baseaddr = (uint64_t)
			    pci_dev->mem_resource[bar_idx].phys_addr;
	return baseaddr;
}

int rte_pmd_xdma_dev_fp_ops_config(int port_id)
{


	struct rte_eth_dev *dev;
	struct rte_eth_fp_ops *fpo = rte_eth_fp_ops;

	if (port_id < 0 || port_id >= rte_eth_dev_count_avail()) {
		PMD_DRV_LOG(ERR,
			"%s:%d Wrong port id %d\n",
			__func__, __LINE__, port_id);
		return -ENOTSUP;
	}
	dev = &rte_eth_devices[port_id];

	fpo[port_id].rx_pkt_burst = dev->rx_pkt_burst;
	fpo[port_id].tx_pkt_burst = dev->tx_pkt_burst;
	fpo[port_id].rx_queue_count = dev->rx_queue_count;
	fpo[port_id].rx_descriptor_status = dev->rx_descriptor_status;
	fpo[port_id].tx_descriptor_status = dev->tx_descriptor_status;
	fpo[port_id].rxq.data = dev->data->rx_queues;
	fpo[port_id].txq.data = dev->data->tx_queues;

	return 0;

}

int rte_user_write(int port_id, uint32_t reg_offst, uint32_t val) {
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	xdma_user_write(dev->data->dev_private,  reg_offst, val);
	return 0;
}

uint32_t rte_user_read(int port_id, uint32_t reg_offst) {
	uint32_t ret;
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	ret = xdma_user_read(dev->data->dev_private, reg_offst);
	return ret;
}

int rte_bar_load(int port_id, uint32_t offset, uint32_t size, int bar_idx, const char* filepath) {
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	xdma_bar_load(dev->data->dev_private, offset, size, bar_idx, filepath);
	return 0;
}

int rte_poll_complete(int port_id, int queue_id, int nb_pkts, bool tx) 
{
	struct rte_eth_dev *dev = &rte_eth_devices[port_id];
	int ret;
	if(tx) 
		ret = xdma_poll_tx_complete(dev->data->dev_private, nb_pkts, queue_id);
	else
	 	ret = xdma_poll_rx_complete(dev->data->dev_private, queue_id);
	return ret;
}