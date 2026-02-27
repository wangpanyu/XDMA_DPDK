#include "xdma_access_common.h"
#include "rte_mempool.h"
#include <stdint.h>

int xdma_desc_ring_init(struct xdma_engine *engine)
{
    struct xdma_desc *desc;
    uint64_t next_addr;
    for(uint32_t i = 0; i < engine->desc_max; i++) {
        desc = &engine->desc_ring[i];
        desc->control = 0xAD4B0000;
        desc->len = 0;
        desc->dst_addr_hi = 0;
        desc->dst_addr_lo = 0;
        desc->src_addr_hi = 0;
        desc->src_addr_lo = 0;
        if (i == engine->desc_max - 1) {
            next_addr = 0x0;
            desc->control = 0xAD4B0013;
        }
        else {
            next_addr = (uint64_t)&engine->desc_ring[i+1];
            next_addr = rte_mem_virt2phy((const void*)next_addr);
        }
        desc->next_lo = (uint32_t)(next_addr & 0xFFFFFFFF);
        desc->next_hi = (uint32_t)(next_addr >> 32);
    }
    return 0;
}


int xdma_desc_ring_clear(struct xdma_engine *engine)
{
    struct xdma_desc *desc;
    for(uint32_t i = 0; i < engine->desc_max; i++) {
        desc = &engine->desc_ring[i];
        desc->len = 0;
        desc->dst_addr_hi = 0;
        desc->dst_addr_lo = 0;
        desc->src_addr_hi = 0;
        desc->src_addr_lo = 0;
        if(engine->dir == H2C) {
            desc->control = 0xAD4B0000;
        }
    }
    return 0;
}

static void user_interrupts_enable(struct xdma_dev *xdma_dev)
{
    uint32_t reg_addr, reg_val;
    reg_addr = IRQ_USER_INTERRUPT_ENABLE_MASK_W1S;
	reg_val = ~0;

	xdma_reg_write(xdma_dev, reg_addr, reg_val);
}

/* channel_interrupts_disable -- Disable interrupts we not interested in */
static void user_interrupts_disable(struct xdma_dev *xdma_dev)
{
    uint32_t reg_addr, reg_val;
    reg_addr = IRQ_USER_INTERRUPT_ENABLE_MASK_W1C;
	reg_val = ~0;

	xdma_reg_write(xdma_dev, reg_addr, reg_val);
}
/* channel_interrupts_enable -- Enable interrupts we are interested in */
static void channel_interrupts_enable(struct xdma_dev *xdma_dev)
{
    uint32_t reg_addr, reg_val;
    reg_addr = IRQ_CHANNEL_INTERRUPT_ENABLE_MASK_W1S;
	reg_val = ~0;

	xdma_reg_write(xdma_dev, reg_addr, reg_val);
}

/* channel_interrupts_disable -- Disable interrupts we not interested in */
static void channel_interrupts_disable(struct xdma_dev *xdma_dev)
{
    uint32_t reg_addr, reg_val;
    reg_addr = IRQ_CHANNEL_INTERRUPT_ENABLE_MASK_W1C;
	reg_val = ~0;

	xdma_reg_write(xdma_dev, reg_addr, reg_val);
}

const struct rte_memzone *xdma_zone_reserve(struct rte_eth_dev *dev,
					const char *ring_name,
					uint32_t channel_id,
					uint32_t ring_size,
					int socket_id)
{
    char z_name[RTE_MEMZONE_NAMESIZE];
	snprintf(z_name, sizeof(z_name), "%s%s%d_%u",
			dev->device->driver->name, ring_name,
			dev->data->port_id, channel_id);
	return rte_memzone_reserve_aligned(z_name, (uint64_t)ring_size,
						socket_id, 0, XDMA_ALIGN);
}

int xdma_tx_engine_setup(struct rte_eth_dev *dev, uint16_t channel_id, uint16_t nb_desc,
                    unsigned int socket_id, struct rte_mempool *mb_pool)
{
    struct xdma_dev *xdma_dev = dev->data->dev_private;
    struct xdma_engine *engine = NULL;
    struct xdma_desc *desc;
    uint32_t size;
    int err = 0;
    
    engine = rte_zmalloc_socket("XDMA_TX", sizeof(struct xdma_engine), RTE_CACHE_LINE_SIZE, socket_id);

    engine->magic = XDMA_ENGINE_MAGIC_NUMBER;
    engine->xdev = xdma_dev;
    engine->dir = H2C;
    engine->channel = channel_id;
    engine->streaming = 1;
    engine->running = 0;

    engine->max_extra_adj = MAX_EXRTA_ADJ;
    engine->desc_max = XDMA_DESC_MAX;

    size = engine->desc_max * sizeof(struct xdma_desc) + sizeof(struct xdma_poll_wb);
    engine->mz = xdma_zone_reserve(dev, "TxHwRn", channel_id, size, socket_id);

	size = engine->desc_max * sizeof(struct rte_mbuf *);
	engine->sw_ring = rte_zmalloc_socket("TxSwRn", size,
				RTE_CACHE_LINE_SIZE, socket_id);

    engine->desc_ring = engine->mz->addr;

    engine->virt2phy_offset = rte_mem_virt2phy(engine->sw_ring) - (uint64_t)engine->sw_ring;

    xdma_desc_ring_init(engine);

    engine->poll_mode_addr_virt = engine->mz->addr + engine->desc_max * sizeof(struct xdma_desc);

    xdma_dev->engine_h2c[channel_id] = *engine;

    dev->data->tx_queues[channel_id] = engine;

    return 0;

}

int xdma_rx_engine_setup(struct rte_eth_dev *dev, uint16_t channel_id, uint16_t nb_desc,
                    unsigned int socket_id, struct rte_mempool *mb_pool)
{
    struct xdma_dev *xdma_dev = dev->data->dev_private;
    struct xdma_engine *engine = NULL;
    struct xdma_desc *desc;
    uint32_t size;
    int err = 0;
    
    engine = rte_zmalloc_socket("XDMA_RX", sizeof(struct xdma_engine), RTE_CACHE_LINE_SIZE, socket_id);

    engine->magic = XDMA_ENGINE_MAGIC_NUMBER;
    engine->xdev = xdma_dev;
    engine->dir = C2H;
    engine->channel = channel_id;
    engine->streaming = 1;
    engine->running = 0;

    engine->max_extra_adj = MAX_EXRTA_ADJ;
    engine->desc_max = XDMA_DESC_MAX;

    size = engine->desc_max * sizeof(struct xdma_desc) + sizeof(struct xdma_poll_wb) + sizeof(struct xdma_c2h_stream_wb);
    engine->mz = xdma_zone_reserve(dev, "RxHwRn", channel_id, size, socket_id);

	size = engine->desc_max * sizeof(struct rte_mbuf *);
	engine->sw_ring = rte_zmalloc_socket("RxSwRn", size,
				RTE_CACHE_LINE_SIZE, socket_id);

    engine->desc_ring = engine->mz->addr;

    engine->virt2phy_offset = rte_mem_virt2phy(engine->sw_ring) - (uint64_t)engine->sw_ring;

    xdma_desc_ring_init(engine);

    engine->poll_mode_addr_virt = engine->mz->addr + engine->desc_max * sizeof(struct xdma_desc);

    xdma_dev->engine_c2h[channel_id] = *engine;

    dev->data->rx_queues[channel_id] = engine;

    return 0;

}





int xdma_tx_engine_start(struct xdma_engine *engine, 
                    uint64_t first_desc_addr, int desc_adj)
{
    uint32_t desc_addr_lo, desc_addr_hi;
    uint32_t poll_mode_wb_addr_lo, poll_mode_wb_addr_hi;
    uint32_t reg_addr, reg_val;
    uint64_t tmp, poll_mode_wb_addr;
    struct xdma_dev *xdma_dev = engine->xdev;
    engine->running = 1;

    // first_desc_addr = rte_mem_virt2phy((const void*)first_desc_addr);
    // poll_mode_wb_addr = rte_mem_virt2phy((const void*)engine->poll_mode_addr_virt);

    first_desc_addr += engine->virt2phy_offset;
    poll_mode_wb_addr = (uint64_t)engine->poll_mode_addr_virt + engine->virt2phy_offset;

    desc_addr_lo = (uint32_t)(first_desc_addr & 0xFFFFFFFF);
    desc_addr_hi = (uint32_t)(first_desc_addr >> 32);

    poll_mode_wb_addr_lo = (uint32_t)(poll_mode_wb_addr & 0xFFFFFFFF);
    poll_mode_wb_addr_hi = (uint32_t)(poll_mode_wb_addr >> 32);

    reg_addr = H2C_SGDMA_DESC_LO_ADDR + engine->channel * 0x100;
    reg_val = desc_addr_lo;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    reg_addr = H2C_SGDMA_DESC_HI_ADDR + engine->channel * 0x100;
    reg_val = desc_addr_hi;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    reg_addr = H2C_SGDMA_DESC_ADJ + engine->channel * 0x100;
    reg_val = (desc_adj > 0)? desc_adj : 0;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    reg_addr = H2C_POLL_MODE_LO_WB_ADDR + engine->channel * 0x100;
    reg_val = poll_mode_wb_addr_lo;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    reg_addr = H2C_POLL_MODE_HI_WB_ADDR + engine->channel * 0x100;
    reg_val = poll_mode_wb_addr_hi;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);


    reg_addr = H2C_CHANNEL_CTRL + engine->channel * 0x100;
    reg_val = (uint32_t)CTRL_RUN_STOP;
    reg_val |= (uint32_t)CTRL_IE_DESC_STOPPED;
    reg_val |= (uint32_t)CTRL_IE_DESC_COMPLETED;
    reg_val |= (uint32_t)CTRL_IE_DESC_ALIGN_MISMATCH;
    reg_val |= (uint32_t)CTRL_IE_MAGIC_STOPPED;
	reg_val |= (uint32_t)CTRL_IE_READ_ERROR;
	reg_val |= (uint32_t)CTRL_IE_DESC_ERROR;
    reg_val |= (uint32_t)CTRL_POLL_MODE_WB;

    rte_wmb();
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    return 0;
}

int xdma_rx_engine_start(struct xdma_engine *engine, 
                    uint64_t first_desc_addr)
{
    uint32_t desc_addr_lo, desc_addr_hi, poll_mode_wb_addr_lo, poll_mode_wb_addr_hi;
    uint32_t wb_addr_lo, wb_addr_hi;
    uint32_t reg_addr, reg_val;
    uint64_t tmp, poll_mode_wb_addr;
    struct xdma_dev *xdma_dev = engine->xdev;

    engine->running = 1;

    // first_desc_addr = rte_mem_virt2phy((const void*)first_desc_addr);
    // poll_mode_wb_addr = rte_mem_virt2phy((const void*)engine->poll_mode_addr_virt);

    first_desc_addr += engine->virt2phy_offset;
    poll_mode_wb_addr = (uint64_t)engine->poll_mode_addr_virt + engine->virt2phy_offset;

    desc_addr_lo = (uint32_t)(first_desc_addr & 0xFFFFFFFF);
    desc_addr_hi = (uint32_t)(first_desc_addr >> 32);

    poll_mode_wb_addr_lo = (uint32_t)(poll_mode_wb_addr & 0xFFFFFFFF);
    poll_mode_wb_addr_hi = (uint32_t)(poll_mode_wb_addr >> 32);

    reg_addr = C2H_SGDMA_DESC_LO_ADDR + engine->channel * 0x100;
    reg_val = desc_addr_lo;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    reg_addr = C2H_SGDMA_DESC_HI_ADDR + engine->channel * 0x100;
    reg_val = desc_addr_hi;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    reg_addr = C2H_POLL_MODE_LO_WB_ADDR + engine->channel * 0x100;
    reg_val = poll_mode_wb_addr_lo;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    reg_addr = C2H_POLL_MODE_HI_WB_ADDR + engine->channel * 0x100;
    reg_val = poll_mode_wb_addr_hi;
    xdma_reg_write(xdma_dev, reg_addr, reg_val);



    reg_addr = C2H_CHANNEL_CTRL + engine->channel * 0x100;
    reg_val = (uint32_t)CTRL_RUN_STOP;
    reg_val |= (uint32_t)CTRL_IE_DESC_STOPPED;
    reg_val |= (uint32_t)CTRL_IE_DESC_COMPLETED;
    reg_val |= (uint32_t)CTRL_IE_DESC_ALIGN_MISMATCH;
    reg_val |= (uint32_t)CTRL_IE_MAGIC_STOPPED;
	reg_val |= (uint32_t)CTRL_IE_READ_ERROR;
	reg_val |= (uint32_t)CTRL_IE_DESC_ERROR;
    reg_val |= (uint32_t)CTRL_POLL_MODE_WB;

    rte_wmb();
    xdma_reg_write(xdma_dev, reg_addr, reg_val);

    return 0;
}

int xdma_tx_engine_stop(struct xdma_engine *engine)
{
    uint32_t reg_addr, reg_val;
    struct xdma_dev *xdma_dev = engine->xdev;
    struct xdma_poll_wb *wb_status;
    
    reg_addr = H2C_CHANNEL_CTRL + engine->channel * 0x100;
	reg_val =  0x0;
    reg_val |= (uint32_t)CTRL_IE_DESC_STOPPED;
    reg_val |= (uint32_t)CTRL_IE_DESC_COMPLETED;
    reg_val |= (uint32_t)CTRL_IE_DESC_ALIGN_MISMATCH;
    reg_val |= (uint32_t)CTRL_IE_MAGIC_STOPPED;
	reg_val |= (uint32_t)CTRL_IE_READ_ERROR;
	reg_val |= (uint32_t)CTRL_IE_DESC_ERROR;
    reg_val |= (uint32_t)CTRL_POLL_MODE_WB;
    rte_wmb();
    xdma_reg_write(xdma_dev, reg_addr, reg_val);
    xdma_desc_ring_clear(engine);

    engine->running = 0;
    engine->desc_used = 0;

    wb_status = (struct xdma_poll_wb *)engine->poll_mode_addr_virt;
    wb_status->completed_desc_count = 0;

    return 0;
}

int xdma_rx_engine_stop(struct xdma_engine *engine)
{
    uint32_t reg_addr, reg_val;
    struct xdma_dev *xdma_dev = engine->xdev;
    struct xdma_poll_wb *wb_status;

    reg_addr = C2H_CHANNEL_CTRL + engine->channel * 0x100;

	reg_val =  0x0;
    reg_val |= (uint32_t)CTRL_IE_DESC_STOPPED;
    reg_val |= (uint32_t)CTRL_IE_DESC_COMPLETED;
    reg_val |= (uint32_t)CTRL_IE_DESC_ALIGN_MISMATCH;
    reg_val |= (uint32_t)CTRL_IE_MAGIC_STOPPED;
	reg_val |= (uint32_t)CTRL_IE_READ_ERROR;
	reg_val |= (uint32_t)CTRL_IE_DESC_ERROR;
    reg_val |= (uint32_t)CTRL_POLL_MODE_WB;
    rte_wmb();
    xdma_reg_write(xdma_dev, reg_addr, reg_val);
    xdma_desc_ring_clear(engine);

    wb_status = (struct xdma_poll_wb *)engine->poll_mode_addr_virt;
    wb_status->completed_desc_count = 0;

    engine->running = 0;
    engine->desc_used = 0;

    return 0;
}

int xdma_dev_start(struct rte_eth_dev *dev)
{
    struct xdma_engine *engine;
    uint32_t channel_id;
    user_interrupts_enable(dev->data->dev_private);
    channel_interrupts_enable(dev->data->dev_private);
    // for(channel_id = 0; channel_id < dev->data->nb_tx_queues; channel_id++) {
    //     engine = (struct xdma_engine *)dev->data->tx_queues[channel_id];
    //     xdma_tx_engine_setup(dev, channel_id, XDMA_DESC_NUM_MAX, 0, NULL);
    // }

    // for(channel_id = 0; channel_id < dev->data->nb_rx_queues; channel_id++) {
    //     engine = (struct xdma_engine *)dev->data->rx_queues[channel_id];
    //     xdma_tx_engine_setup(dev, channel_id, XDMA_DESC_NUM_MAX, 0, NULL);
    // }
    return 0;
}

int xdma_dev_stop(struct rte_eth_dev *dev)
{
	uint32_t channel_id;

    
	for (channel_id = 0; channel_id < dev->data->nb_tx_queues; channel_id++)
		xdma_tx_engine_stop(dev->data->tx_queues[channel_id]);
	for (channel_id = 0; channel_id < dev->data->nb_rx_queues; channel_id++)
		xdma_rx_engine_stop(dev->data->rx_queues[channel_id]);

    //user_interrupts_disable(dev->data->dev_private);
    //channel_interrupts_disable(dev->data->dev_private);

	return 0;
}

int xdma_dev_close(struct rte_eth_dev *dev)
{
	struct xdma_dev *xdma_dev = dev->data->dev_private;
	struct xdma_engine *engine;
    int channel_id;

	int ret = 0;


	if (dev->data->dev_started)
		xdma_dev_stop(dev);


	/* iterate over rx queues */
	for (channel_id = 0; channel_id < dev->data->nb_rx_queues; channel_id++) {
		engine = dev->data->rx_queues[channel_id];
		if (engine->dir == C2H) {

			if (engine->sw_ring)
				rte_free(engine->sw_ring);

			if (engine->mz)
				rte_memzone_free(engine->mz);
			rte_free(engine);
		}
	}

	/* iterate over tx queues */
	for (channel_id = 0; channel_id < dev->data->nb_tx_queues; channel_id++) {
		engine = dev->data->tx_queues[channel_id];
		if (engine->dir == H2C) {

			if (engine->sw_ring)
				rte_free(engine->sw_ring);
			if (engine->mz)
				rte_memzone_free(engine->mz);
			rte_free(engine);
		}
	}

	return 0;
}

int xdma_get_user_bar(void *dev_hndl, uint8_t *user_bar)
{
	uint8_t bar_found = 0;
	uint8_t bar_idx = 0;
	uint32_t user_bar_id = 0;
//	uint32_t reg_addr = QDMA_OFFSET_GLBL2_PF_BARLITE_EXT;

	if (!dev_hndl) {
		// xdma_log_error("%s: dev_handle is NULL, err:%d\n",
		// 		__func__, -QDMA_ERR_INV_PARAM);
		return -1;
	}

	if (!user_bar) {
		// xdma_log_error("%s: user_bar is NULL, err:%d\n",
		// 			__func__, -QDMA_ERR_INV_PARAM);
		return -1;
	}

	user_bar_id = 1;


	for (bar_idx = 0; bar_idx < XDMA_BAR_NUM; bar_idx++) {
		if (user_bar_id & (1 << bar_idx)) {
			*user_bar = bar_idx;
			bar_found = 1;
			break;
		}
	}
	if (bar_found == 0) {
		*user_bar = 0;
		return -1;
	}

	return 0;
}


int xdma_hw_access_init(void *dev_hndl, 
				struct xdma_hw_access *hw_access)
{
    hw_access->xdma_tx_engine_start = &xdma_tx_engine_start;
    hw_access->xdma_rx_engine_start = &xdma_rx_engine_start;
    hw_access->xdma_tx_engine_stop  = &xdma_tx_engine_stop;
    hw_access->xdma_rx_engine_stop  = &xdma_rx_engine_stop;
    hw_access->xdma_tx_engine_setup = &xdma_tx_engine_setup;
    hw_access->xdma_rx_engine_setup = &xdma_rx_engine_setup;
    hw_access->xdma_dev_start       = &xdma_dev_start;
    hw_access->xdma_dev_stop        = &xdma_dev_stop;
    hw_access->xdma_dev_close       = &xdma_dev_close;
    hw_access->xdma_get_user_bar    = &xdma_get_user_bar;
    return 0;
}



