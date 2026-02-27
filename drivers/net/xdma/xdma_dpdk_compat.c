#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <rte_memzone.h>
#include <rte_string_fns.h>
#include <rte_malloc.h>
#include <rte_dev.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_alarm.h>
#include <rte_cycles.h>
#include <rte_atomic.h>
#include <unistd.h>
#include <string.h>

#include "xdma.h"
#include "xdma_access_common.h"
#include "xdma_devops.h"
#include "rte_pmd_xdma.h"

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