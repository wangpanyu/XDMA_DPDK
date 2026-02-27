#ifndef __XDMA_ACCESS_COMMOH_H__
#define __XDMA_ACCESS_COMMOH_H__

#include "xdma.h"
#include "rte_malloc.h"
#include "rte_mempool.h"
#include "xdma.h"
#include "xdma_regs.h"
#include "xdma_log.h"
#include "xdma_platform.h"
#include <stdint.h>

int xdma_desc_ring_clear(struct xdma_engine *engine);

struct xdma_hw_access {
    int (*xdma_dev_start)(struct rte_eth_dev *dev);
    int (*xdma_dev_stop)(struct rte_eth_dev *dev);
    int (*xdma_dev_close)(struct rte_eth_dev *dev);
    int (*xdma_tx_engine_start)(struct xdma_engine *engine, 
                        uint64_t first_desc_addr, int desc_adj);
     int (*xdma_rx_engine_start)(struct xdma_engine *engine, 
                        uint64_t first_desc_addr);
    int (*xdma_tx_engine_stop)(struct xdma_engine *engine);
    int (*xdma_rx_engine_stop)(struct xdma_engine *engine);
    int (*xdma_tx_engine_setup)(struct rte_eth_dev *dev, uint16_t channel_id, uint16_t nb_desc,
                        unsigned int socket_id, struct rte_mempool *mb_pool);
    int (*xdma_rx_engine_setup)(struct rte_eth_dev *dev, uint16_t channel_id, uint16_t nb_desc,
                        unsigned int socket_id, struct rte_mempool *mb_pool);
    int (*xdma_get_user_bar)(void *dev_hndl, uint8_t *user_bar);
};

int xdma_hw_access_init(void *dev_hndl, 
				struct xdma_hw_access *hw_access);

int xdma_dev_start(struct rte_eth_dev *dev);
int xdma_dev_stop(struct rte_eth_dev *dev);
int xdma_dev_close(struct rte_eth_dev *dev);
int xdma_tx_engine_start(struct xdma_engine *engine, 
                    uint64_t first_desc_addr, int desc_adj);
int xdma_rx_engine_start(struct xdma_engine *engine, 
                    uint64_t first_desc_addr);
int xdma_tx_engine_stop(struct xdma_engine *engine);
int xdma_rx_engine_stop(struct xdma_engine *engine);
int xdma_tx_engine_setup(struct rte_eth_dev *dev, uint16_t channel_id, uint16_t nb_desc,
                    unsigned int socket_id, struct rte_mempool *mb_pool);
int xdma_rx_engine_setup(struct rte_eth_dev *dev, uint16_t channel_id, uint16_t nb_desc,
                    unsigned int socket_id, struct rte_mempool *mb_pool);

const struct rte_memzone *xdma_zone_reserve(struct rte_eth_dev *dev,
					const char *ring_name,
					uint32_t channel_id,
					uint32_t ring_size,
					int socket_id);

#endif