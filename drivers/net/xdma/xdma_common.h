#include <stdint.h>
#include <rte_malloc.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_kvargs.h>
#include "xdma.h"
#include "xdma_access_common.h"
#include "xdma_log.h"
#include <fcntl.h>
#include <unistd.h>
#include <bus_pci_driver.h>

int xdma_check_kvargs(struct rte_devargs *devargs,
						struct xdma_dev *xdma_dev);

static int config_bar_idx_handler(__rte_unused const char *key,
					const char *value,  void *opaque);

int xdma_identify_bars(struct rte_eth_dev *dev);