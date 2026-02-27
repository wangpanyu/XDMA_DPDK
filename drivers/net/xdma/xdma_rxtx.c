#include <generic/rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_cycles.h>
#include "xdma.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "xdma_rxtx.h"
#include "xdma_access_common.h"



int reclaim_engine_mbuf(struct xdma_engine *engine,
			uint16_t desc_used, uint16_t free_cnt)
{
	int fl_desc = 0;
	uint16_t fl_desc_cnt;
	uint16_t count;
	uint16_t id = 0;

	fl_desc = engine->desc_used;

	if (unlikely(!fl_desc))
		return 0;


	if (free_cnt && (fl_desc > free_cnt))
		fl_desc = free_cnt;

	if (id < fl_desc) {
		fl_desc_cnt = ((uint16_t)fl_desc & 0xFFFF);
		rte_pktmbuf_free_bulk(&engine->sw_ring[id], fl_desc_cnt);
		for (count = 0; count < fl_desc_cnt; count++)
			engine->sw_ring[id++] = NULL;

		return fl_desc;
	}


	return fl_desc;
}


struct xdma_desc *get_desc(void *dev_hndl)
{
	struct xdma_desc *desc;
	struct xdma_engine *engine = (struct xdma_engine *)dev_hndl;
	struct xdma_desc *desc_ring =
					(struct xdma_desc *)engine->desc_ring;
	uint32_t id;

	id = engine->desc_used;
	desc =  (struct xdma_desc *)&desc_ring[id];

	id = (id + 1) % engine->desc_max;
	engine->desc_used = id;

	return desc;
}

int xdma_desc_control_set(struct xdma_desc *desc, uint32_t control_field)
{

	uint32_t control = desc->control;
	control |= control_field;
	desc->control = control;
	return 0;
}

int xdma_h2c_desc_set(void *dev_hndl, struct rte_mbuf *mb)
{
	struct xdma_desc *desc;
	int nsegs = mb->nb_segs;
	uint64_t src_addr;

	while (nsegs && mb) {
		desc = get_desc(dev_hndl);
		rte_xdma_prefetch(desc);
		//desc->control = 0xAD4B0000;
		desc->len = rte_pktmbuf_data_len(mb);
		src_addr = mb->buf_iova + mb->data_off;
		desc->src_addr_lo = (uint32_t)(src_addr & 0xFFFFFFFF);
		desc->src_addr_hi = (uint32_t)(src_addr >> 32);
		nsegs--;
		mb = mb->next;
		if (!nsegs)
		 	xdma_desc_control_set(desc, XDMA_DESC_EOP | XDMA_DESC_COMPLETED);
	}

	return 0;
}

int xdma_c2h_desc_set(void *dev_hndl,struct rte_mbuf *mb)
{
	struct timespec start_time, end_time_0, end_time_1, end_time_2;
	double duration_0, duration_1, duration_2;
	struct xdma_desc *desc;
	struct xdma_engine *engine = (struct xdma_engine *)dev_hndl;
	//int nsegs = mb->nb_segs;
	uint64_t dst_addr;
	uint64_t src_addr;
	uint64_t vir2phy = mb->buf_iova - (uint64_t)mb->buf_addr;
	//clock_gettime(CLOCK_MONOTONIC, &start_time);

	desc = get_desc(dev_hndl);

	dst_addr = mb->buf_iova + mb->data_off;
	//clock_gettime(CLOCK_MONOTONIC, &end_time_0);

	src_addr = (uint64_t)engine->desc_ring + XDMA_DESC_SIZE * engine->desc_max + XDMA_POLL_MODE_WB_SIZE;
	//src_addr = rte_mem_virt2phy((const void*)src_addr);
	src_addr = src_addr + vir2phy;
	//clock_gettime(CLOCK_MONOTONIC, &end_time_1);

	desc->dst_addr_lo = (uint32_t)(dst_addr & 0xFFFFFFFF);
	desc->dst_addr_hi = (uint32_t)(dst_addr >> 32);
	desc->src_addr_lo = (uint32_t)(src_addr & 0xFFFFFFFF);
	desc->src_addr_hi = (uint32_t)(src_addr >> 32);
	desc->len = 64;  //remember to fix!!!
	xdma_desc_control_set(desc, XDMA_DESC_COMPLETED);
	//clock_gettime(CLOCK_MONOTONIC, &end_time_2);
	// duration_0 = (end_time_0.tv_sec - start_time.tv_sec) +
	// (end_time_0.tv_nsec - start_time.tv_nsec) / 1e9;
	// duration_1 = (end_time_1.tv_sec - start_time.tv_sec) +
	// (end_time_1.tv_nsec - start_time.tv_nsec) / 1e9;
	// duration_2 = (end_time_2.tv_sec - start_time.tv_sec) +
	// (end_time_2.tv_nsec - start_time.tv_nsec) / 1e9;
	//PMD_DRV_LOG(INFO, "delay 0 = %f delay 1 = %f delay 2 = %f  1\n",duration_0, duration_1, duration_2);
	return 0;
}

int xdma_desc_adjacent_set(struct xdma_desc *desc, uint32_t nxt_adj)
{
	uint32_t control = desc->control & ~XDMA_DESC_NXT_ADJ_MASK;
	control |= nxt_adj << 8;
	desc->control = control;
	return 0; 
}

uint16_t xdma_xmit_pkts(void *dev_hndl, struct rte_mbuf **tx_pkts,
			uint16_t nb_pkts)
{
	struct timespec start_time, end_time_0, end_time_1, end_time_2;
	double duration_0, duration_1, duration_2;
	//clock_gettime(CLOCK_MONOTONIC, &start_time);
	struct rte_mbuf *mb = NULL;
	uint64_t pkt_len = 0;
	int avail, in_use, ret, nsegs;
	uint16_t count = 0, id;
	uint16_t head = 0;
	struct xdma_engine *engine = (struct xdma_engine *)dev_hndl;
	struct xdma_dev *xdma_dev = engine->xdev;
	struct xdma_poll_wb *wb_status;
	int tran_time = 0;

	if(engine->running) 
		return -1;
	else
	 	engine->running = 1;

	id = engine->desc_used;

	/* Make sure reads to Tx ring are synchronized before
	 * accessing the status descriptor.
	 */
	rte_rmb();




	/* Free transmitted mbufs back to pool */
	//reclaim_engine_mbuf(engine, id, 0);

	avail = engine->desc_max;
	if(avail < 0) {
		avail += engine->desc_max;
	}


	for (count = 0; count < nb_pkts; count++) {
		mb = tx_pkts[count];
		nsegs = mb->nb_segs;
		if (nsegs > avail) {
			/* Number of segments in current mbuf are greater
			 * than number of descriptors available,
			 * hence update PIDX and return
			 */
			break;
		}
		avail -= nsegs;
		id = engine->desc_used;
		//engine->sw_ring[id] = mb;
		pkt_len += rte_pktmbuf_pkt_len(mb);

		ret = xdma_h2c_desc_set(engine, mb);
		if (unlikely(ret < 0))
			break;
	}
	xdma_desc_control_set(&engine->desc_ring[engine->desc_used], 
				XDMA_DESC_COMPLETED | XDMA_DESC_STOPPED);

	for(int i = 0; i < count - 1; i++) {
			ret = xdma_desc_adjacent_set(&engine->desc_ring[(head + i) % engine->desc_max], 
							count - 2 - i);
	}

	wb_status = (struct xdma_poll_wb *)engine->poll_mode_addr_virt;
	//clock_gettime(CLOCK_MONOTONIC, &end_time_0);
	//wb_status->completed_desc_count = 0;


	xdma_dev->hw_access->xdma_tx_engine_start(engine, 
				(uint64_t)&engine->desc_ring[head], count - 2);

	//clock_gettime(CLOCK_MONOTONIC, &end_time_1);

	while (1) {
		//rte_delay_us(DELAY_TIME_US);
		if ((wb_status->completed_desc_count & 0xFFFFFF) == nb_pkts){
			//PMD_DRV_LOG(INFO, "xmit retry_time = %d\n", tran_time);
			break;
		}
		tran_time++ ;
	}

	//clock_gettime(CLOCK_MONOTONIC, &end_time_2);

	//*(uint32_t *)engine->poll_mode_addr_virt = 0;
	xdma_dev->hw_access->xdma_tx_engine_stop(engine);

	// duration_0 = (end_time_0.tv_sec - start_time.tv_sec) +
	// (end_time_0.tv_nsec - start_time.tv_nsec) / 1e9;
	// duration_1 = (end_time_1.tv_sec - start_time.tv_sec) +
	// (end_time_1.tv_nsec - start_time.tv_nsec) / 1e9;
	// duration_2 = (end_time_2.tv_sec - start_time.tv_sec) +
	// (end_time_2.tv_nsec - start_time.tv_nsec) / 1e9;
	//PMD_DRV_LOG(INFO, "\tdelay 0 = %f delay 1 = %f delay 2 = %f  tran_time = %d\n",duration_0, duration_1, duration_2, tran_time);

	return nb_pkts;
}

uint16_t xdma_recv_pkts(void *dev_hndl, struct rte_mbuf **rx_pkts,
			uint16_t nb_pkts)
{
	struct rte_mbuf *mb = NULL;
	uint16_t count_pkts;
	uint16_t nb_pkts_avail = 0;
	uint16_t nb_cmpt_desc = 0;
	uint16_t nb_prepared = 0;
	struct xdma_poll_wb *wb_status;
	struct xdma_engine *engine = (struct xdma_engine *)dev_hndl;
	struct xdma_dev *xdma_dev = engine->xdev;
	uint32_t old_wb_status_count;
	int tran_time = 0;

	wb_status = (struct xdma_poll_wb *)engine->poll_mode_addr_virt;
	engine->status = 0;


	if(engine->running == 0) {
		nb_pkts = RTE_MIN(nb_pkts, engine->desc_max);


		for (int count = 0; count < nb_pkts; count++) {
			rte_xdma_prefetch(rx_pkts[count]);
			mb = rx_pkts[count];
			//engine->sw_ring[count] = rx_pkts[count];
			xdma_c2h_desc_set(engine, mb);
		}
		//clock_gettime(CLOCK_MONOTONIC, &end_time_0);

		//wb_status = (struct xdma_poll_wb *)engine->poll_mode_addr_virt;
		//wb_status->completed_desc_count = 0;
		
		xdma_dev->hw_access->xdma_rx_engine_start(engine,
					(uint64_t)&engine->desc_ring[0]);

		//clock_gettime(CLOCK_MONOTONIC, &end_time_1);
	}


	while (1) {
		//rte_delay_us(DELAY_TIME_US);
		old_wb_status_count = wb_status->completed_desc_count;
//		rte_delay_us(DELAY_TIME_US);
		if((old_wb_status_count == wb_status->completed_desc_count) && 
					(old_wb_status_count != (uint32_t)engine->desc_used)) {
			count_pkts = prepare_packets(engine, rx_pkts, old_wb_status_count,engine->desc_used);
			//nb_cmpt_desc = old_wb_status_count;
			nb_cmpt_desc = old_wb_status_count - engine->desc_used;
			engine->desc_used = old_wb_status_count;
			engine->status = RX_COMMPLETED;
			//PMD_DRV_LOG(INFO, "recv retry_time = %d\n", tran_time);
			break;
		}
		tran_time++;

		if(tran_time > 100) {
			//PMD_DRV_LOG(INFO, "Over 2ms not recv data\n");
			tran_time = 0;
			break;
		}
	}
	if (engine->desc_used == nb_pkts)
		xdma_dev->hw_access->xdma_rx_engine_stop(engine);
	return nb_cmpt_desc;
}

uint16_t prepare_packets(void *dev_hndl, struct rte_mbuf **rx_pkts, uint16_t nb_pkts,
			uint16_t nb_cmpt_desc)
{
	struct xdma_engine *engine = (struct xdma_engine *)dev_hndl;
	uint16_t count_pkts = 0;

	struct rte_mbuf *mb;
	uint16_t count_desc = nb_cmpt_desc;
	uint16_t id = 0;
	while (count_desc < nb_pkts) {
		mb = prepare_segmented_packet(dev_hndl, &count_desc, rx_pkts);
		rte_xdma_prefetch(rx_pkts[count_desc+1]);
		rx_pkts[count_desc++] = mb;
	}
	return count_pkts;
}

struct rte_mbuf *prepare_segmented_packet(void *dev_hndl, uint16_t *id, struct rte_mbuf **rx_pkts)
{
	struct rte_mbuf *mb;
	struct rte_mbuf *first_seg = NULL;
	struct rte_mbuf *last_seg = NULL;
	struct xdma_engine *engine = (struct xdma_engine *)dev_hndl;
	struct xdma_c2h_stream_wb *wb;
	uint16_t wb_magic;
	bool eop;

	do {
		mb = rx_pkts[*id];
		wb = (struct xdma_c2h_stream_wb *)((uint64_t)engine->desc_ring + 
					XDMA_DESC_SIZE * engine->desc_max + XDMA_POLL_MODE_WB_SIZE);
		wb_magic = wb->control >> 16;
		eop = wb->control & 0x1;
		if (first_seg == NULL) {
			first_seg = mb;
			first_seg->nb_segs = 1;
			first_seg->pkt_len = wb->len;
			first_seg->data_len = wb->len;
			first_seg->packet_type = 0;
			first_seg->ol_flags = 0;
			first_seg->port = engine->port_id;
			first_seg->vlan_tci = 0;
			first_seg->hash.rss = 0;
		} else {
			first_seg->nb_segs++;
			if(last_seg != NULL)
				last_seg->next = mb;
		}

		last_seg = mb;
		mb->next = NULL;
		break; 
	} while ((wb_magic == 0x52b4) && eop);

	return first_seg;
}

uint32_t xdma_poll_rx_complete(void *dev_hndl, int queue_id)
{
	struct xdma_poll_wb *wb_status;
	struct xdma_dev *xdma_dev = (struct xdma_dev *)dev_hndl;
	struct xdma_engine *engine = &xdma_dev->engine_c2h[queue_id];
	uint32_t old_wb_status_count;
	int tran_time = 0;

	wb_status = (struct xdma_poll_wb *)engine->poll_mode_addr_virt;
	while (1) {
		//rte_delay_us(DELAY_TIME_US);
		old_wb_status_count = wb_status->completed_desc_count;
//		rte_delay_us(DELAY_TIME_US);
		if((old_wb_status_count == wb_status->completed_desc_count) && 
					(old_wb_status_count != (uint32_t)engine->desc_used)) {
			engine->status = RX_COMMPLETED;
			//PMD_DRV_LOG(INFO, "recv retry_time = %d\n", tran_time);
			break;
		}
//		rte_delay_us(DELAY_TIME_US);
		tran_time++;
		if(tran_time > 1000)
			//PMD_DRV_LOG(INFO, "Over 2ms not recv data\n");
			break;
	}
    return old_wb_status_count;
}


uint16_t xdma_poll_tx_complete(void *dev_hndl, uint16_t nb_pkts, int queue_id)
{
	struct xdma_dev *xdma_dev = (struct xdma_dev *)dev_hndl;
	struct xdma_engine *engine = &xdma_dev->engine_h2c[queue_id];
	struct xdma_poll_wb *wb_status;
	int tran_time = 0;

	wb_status = (struct xdma_poll_wb *)engine->poll_mode_addr_virt;

	//clock_gettime(CLOCK_MONOTONIC, &end_time_1);

	while (1) {
		//rte_delay_us(DELAY_TIME_US);
		if ((wb_status->completed_desc_count & 0xFFFFFF) == nb_pkts){
			//PMD_DRV_LOG(INFO, "xmit retry_time = %d\n", tran_time);
			break;
		}
		tran_time++ ;
	}
	return nb_pkts;
}

void __rte_cold
xdma_set_tx_function(struct rte_eth_dev *dev)
{
	dev->tx_pkt_burst = xdma_xmit_pkts;
	
}

void __rte_cold
xdma_set_rx_function(struct rte_eth_dev *dev)
{
	dev->rx_pkt_burst = xdma_recv_pkts;
}