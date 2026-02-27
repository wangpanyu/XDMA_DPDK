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
#include "xdma_common.h"

#define ETH_LINK_UP RTE_ETH_LINK_UP
#define ETH_LINK_FULL_DUPLEX RTE_ETH_LINK_FULL_DUPLEX
#define ETH_SPEED_NUM_200G RTE_ETH_SPEED_NUM_200G

void xdma_dev_ops_init(struct rte_eth_dev *dev);