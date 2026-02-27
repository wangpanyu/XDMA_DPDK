#include <stdint.h>
#include <stdbool.h>
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
#include <ethdev_driver.h>
#include <ethdev_pci.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <bus_pci_driver.h>

#include "xdma.h"
#include "xdma_access_common.h"
#include "xdma_devops.h"
#include "xdma_log.h"
#include "xdma_common.h"
#include "rte_pmd_xdma.h"

int xdma_eth_dev_init(struct rte_eth_dev *dev);
int xdma_eth_dev_uninit(struct rte_eth_dev *dev);
