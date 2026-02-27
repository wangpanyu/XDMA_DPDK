#ifndef __XDMA_H__
#define __XDMA_H__

#include <stdbool.h>
#include <stdint.h>
#include <rte_dev.h>
#include <rte_ethdev.h>
#include <rte_spinlock.h>
#include <rte_log.h>
#include <rte_cycles.h>
#include <rte_byteorder.h>
#include <rte_memzone.h>
#include <linux/pci.h>
#include <ethdev_driver.h>
#include <bus_pci_driver.h>


#define XDMA_BAR_NUM (6)
#define XDMA_CHANNEL_NUM_MAX (4)

#define XDMA_MIN_RXBUFF_SIZE	(256)

#define XDMA_DESC_NUM_MAX  0x800

#define DEFAULT_CONFIG_BAR (1)
#define DEFAULT_USER_BAR (-1)
#define DEFAULT_BYPASS_BAR (-1)
#define BAR_ID_INVALID (-1)

#define XDMA_DESC_CONTROL_MASK 0x000000FFUL
#define XDMA_DESC_NXT_ADJ_MASK 0x00003F00UL

#define XDMA_DESC_STOPPED	(1UL << 0)
#define XDMA_DESC_COMPLETED	(1UL << 1)
#define XDMA_DESC_EOP		(1UL << 4)

#define XDMA_POLL_MODE_WB_SIZE	32

#define XDMA_DESC_SIZE 64

#define XDMA_DESC_MAX 32

#define XDMA_ENGINE_MAGIC_NUMBER 	0x1df34c32

#define XDMA_ALIGN 4096

#define MAX_EXRTA_ADJ  32

#define MAX_RETRY	32

#define DELAY_TIME_US 0

#define RX_COMMPLETED 1

enum dma_data_direction {
	C2H = 0,
	H2C = 1,
	UNKNOWN = 2,
};


struct xdma_engine {
	uint32_t magic;	/* structure ID for sanity checks */
	struct xdma_dev *xdev;	/* parent device */
	char name[16];		/* name of this engine */
	int version;		/* version of this engine */

	/* HW register address offsets */
	struct engine_regs *regs;		/* Control reg BAR offset */
	struct engine_sgdma_regs *sgdma_regs;	/* SGDAM reg BAR offset */
	uint32_t bypass_offset;			/* Bypass mode BAR offset */

	enum dma_data_direction dir;
	uint8_t addr_align;		/* source/dest alignment in bytes */
	uint8_t len_granularity;	/* transfer length multiple */
	uint8_t addr_bits;		/* HW datapath address width */
	uint8_t channel:2;		/* engine indices */
	uint8_t streaming:1;
	uint8_t device_open:1;	/* flag if engine node open, ST mode only */
	uint8_t running:1;		/* flag if the driver started engine */
	uint8_t non_incr_addr:1;	/* flag if non-incremental addressing used */
	uint8_t eop_flush:1;		/* st c2h only, flush up the data with eop */
	uint8_t filler:1;

	int max_extra_adj;	/* descriptor prefetch capability */
	int desc_dequeued;	/* num descriptors of completed transfers */
	uint32_t desc_max;		/* max # descriptors per xfer */
	uint32_t status;		/* last known status of device */
	/* only used for MSIX mode to store per-engine interrupt mask value */
	uint32_t interrupt_enable_mask_value;

	/* Members associated with polled mode support */
	uint64_t *poll_mode_addr_virt;	/* virt addr for descriptor writeback */
	uint64_t poll_mode_bus;	/* bus addr for descriptor writeback */


	uint64_t desc_bus;
	struct xdma_desc *desc_ring;
	// int desc_idx;			/* current descriptor index */
	// int desc_used;			/* total descriptors used */
	int desc_used;

	struct rte_mbuf			**sw_ring;/* SW ring virtual address*/

	uint32_t port_id;
	uint64_t virt2phy_offset;

	const struct rte_memzone *mz;
};

struct xdma_dev {
	unsigned long magic;		/* structure ID for sanity checks */

	int idx;		/* dev index */

	const char *name;		/* name of module owning the dev */


	/* PCIe BAR management */
	void *bar[XDMA_BAR_NUM];	/* addresses for mapped BARs */
	int user_bar_idx;	/* BAR index of user logic */
	int config_bar_idx;	/* BAR index of XDMA config logic */
	int bypass_bar_idx;	/* BAR index of XDMA bypass logic */
	int regions_in_use;	/* flag if dev was in use during probe() */
	int got_regions;	/* flag if probe() obtained the regions */

	int user_max;
	int c2h_channel_max;
	int h2c_channel_max;

	struct xdma_engine engine_h2c[XDMA_CHANNEL_NUM_MAX];
	struct xdma_engine engine_c2h[XDMA_CHANNEL_NUM_MAX];

	struct xdma_hw_access *hw_access;

	/* DMA identifier used by the resource manager
	 * for the DMA instances used by this driver
	 */
	uint32_t dma_device_index;

	uint8_t trigger_mode;
	uint8_t timer_count;

	uint8_t dev_configured:1;
	uint8_t is_vf:1;
	uint8_t is_master:1;
	uint8_t en_desc_prefetch:1;

	// /* Reset state */
	// uint8_t reset_in_progress;
	// enum reset_state_t reset_state;

	/* Hardware version info*/
	uint32_t vivado_rel:4;
	uint32_t rtl_version:4;
	uint32_t device_type:4;
	uint32_t ip_type:4;

	//struct queue_info *q_info;
	// struct qdma_dev_mbox mbox;
	//uint8_t init_q_range;
};



struct  xdma_desc {
	uint32_t control;
	uint32_t len;		/* transfer length in bytes */
	uint32_t src_addr_lo;	/* source address (low 32-bit) */
	uint32_t src_addr_hi;	/* source address (high 32-bit) */
	uint32_t dst_addr_lo;	/* destination address (low 32-bit) */
	uint32_t dst_addr_hi;	/* destination address (high 32-bit) */
	/*
	 * next descriptor in the single-linked list of descriptors;
	 * this is the PCIe (bus) address of the next descriptor in the
	 * root complex memory
	 */
	uint32_t next_lo;		/* next desc address (low 32-bit) */
	uint32_t next_hi;		/* next desc address (high 32-bit) */
};

struct engine_regs {
	uint32_t identifier;
	uint32_t control;
	uint32_t control_w1s;
	uint32_t control_w1c;
	uint32_t reserved_1[12];	/* padding */

	uint32_t status;
	uint32_t status_rc;
	uint32_t completed_desc_count;
	uint32_t alignments;
	uint32_t reserved_2[14];	/* padding */

	uint32_t poll_mode_wb_lo;
	uint32_t poll_mode_wb_hi;
	uint32_t interrupt_enable_mask;
	uint32_t interrupt_enable_mask_w1s;
	uint32_t interrupt_enable_mask_w1c;
	uint32_t reserved_3[9];	/* padding */

	uint32_t perf_ctrl;
	uint32_t perf_cyc_lo;
	uint32_t perf_cyc_hi;
	uint32_t perf_dat_lo;
	uint32_t perf_dat_hi;
	uint32_t perf_pnd_lo;
	uint32_t perf_pnd_hi;
} ;

struct config_regs {
	uint32_t identifier;
	uint32_t reserved_1[4];
	uint32_t msi_enable;
};

struct engine_sgdma_regs {
	uint32_t identifier;
	uint32_t reserved_1[31];	/* padding */

	/* bus address to first descriptor in Root Complex Memory */
	uint32_t first_desc_lo;
	uint32_t first_desc_hi;
	/* number of adjacent descriptors at first_desc */
	uint32_t first_desc_adjacent;
	uint32_t credits;
} ;

struct sgdma_common_regs {
	uint32_t padding[8];
	uint32_t credit_mode_enable;
	uint32_t credit_mode_enable_w1s;
	uint32_t credit_mode_enable_w1c;
} ;

struct xdma_poll_wb {
	uint32_t completed_desc_count;
	//uint32_t reserved_1;
};

struct xdma_c2h_stream_wb {
	uint32_t control;
	uint32_t len;
};


void __rte_cold xdma_set_tx_function(struct rte_eth_dev *dev);
void __rte_cold xdma_set_rx_function(struct rte_eth_dev *dev);

#endif /* ifndef __XDMA_H__ */