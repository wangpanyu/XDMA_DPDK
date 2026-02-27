
#include <stdint.h>
#include <time.h>
#include "xdma.h"


#define rte_xdma_prefetch(p)   rte_prefetch0(p)

/*Supporting functions for user logic pluggability*/

struct xdma_desc *get_desc(void *dev_hndl);
uint16_t prepare_packets(void *dev_hndl, struct rte_mbuf **rx_pkts, uint16_t nb_pkts,
	uint16_t nb_cmpt_desc);
struct rte_mbuf *prepare_segmented_packet(void *dev_hndl, uint16_t *id, struct rte_mbuf **rx_pkts);
uint32_t xdma_poll_rx_complete(void *dev_hndl, int queue_id);
uint16_t xdma_poll_tx_complete(void *dev_hndl, uint16_t nb_pkts, int queue_id);
