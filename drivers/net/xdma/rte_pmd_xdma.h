

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
#include <ethdev_driver.h>
#include "xdma.h"
#include "xdma_access_common.h"
#include "xdma_rxtx.h"

/**
 * Enum to specify the completion trigger mode
 * @ingroup rte_pmd_qdma_enums
 */
enum rte_pmd_qdma_tigger_mode_t {
	/** Trigger mode disabled */
	RTE_PMD_QDMA_TRIG_MODE_DISABLE,
	/** Trigger mode every */
	RTE_PMD_QDMA_TRIG_MODE_EVERY,
	/** Trigger mode user count */
	RTE_PMD_QDMA_TRIG_MODE_USER_COUNT,
	/** Trigger mode user */
	RTE_PMD_QDMA_TRIG_MODE_USER,
	/** Trigger mode timer */
	RTE_PMD_QDMA_TRIG_MODE_USER_TIMER,
	/** Trigger mode timer + count */
	RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT,
	/** Trigger mode invalid */
	RTE_PMD_QDMA_TRIG_MODE_MAX,
};

void rte_pmd_xdma_compat_memzone_reserve_aligned(void);
struct rte_device *rte_pmd_xdma_get_device(int port_id);

int rte_pmd_xdma_dev_remove(int port_id);
int rte_pmd_xdma_dev_close(uint16_t port_id);

int rte_pmd_xdma_get_bar_details(int port_id, int32_t *config_bar_idx,
			int32_t *user_bar_idx, int32_t *bypass_bar_idx);

int rte_pmd_xdma_dev_fp_ops_config(int port_id);
uint32_t xdma_user_read(void *dev_hndl, uint32_t reg_offst);
int rte_user_write(int port_id, uint32_t reg_offst, uint32_t val);
uint32_t rte_user_read(int port_id, uint32_t reg_offst);
uint64_t rte_pmd_xdma_get_bar_addr(int port_id, int32_t bar_idx);
int rte_bar_load(int port_id, uint32_t offset, uint32_t size, int bar_idx, const char* filepath);
int rte_poll_complete(int port_id, int queue_id, int nb_pkts, bool tx);