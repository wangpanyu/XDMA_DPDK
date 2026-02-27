#include "xdma_devops.h"

int xdma_dev_configure(struct rte_eth_dev *dev)
{
	struct xdma_dev *xdma_dev = dev->data->dev_private;
	uint16_t qid = 0;
	int ret = 0, queue_base = -1;
	uint8_t stat_id;

	PMD_DRV_LOG(INFO, "Configure the xdma engines\n");

	// xdma_dev->qsets_en = RTE_MAX(dev->data->nb_rx_queues,
	// 				dev->data->nb_tx_queues);
	// if (qdma_dev->qsets_en > qdma_dev->dev_cap.num_qs) {
	// 	PMD_DRV_LOG(ERR, "PF-%d(DEVFN) Error: Number of Queues to be"
	// 			" configured are greater than the queues"
	// 			" supported by the hardware\n",
	// 			qdma_dev->func_id);
	// 	qdma_dev->qsets_en = 0;
	// 	return -1;
	// }

	/* Request queue base from the resource manager */
	// ret = xdma_dev_update(qdma_dev->dma_device_index, qdma_dev->func_id,
	// 		qdma_dev->qsets_en, &queue_base);
	// if (ret != QDMA_SUCCESS) {
	// 	PMD_DRV_LOG(ERR, "PF-%d(DEVFN) queue allocation failed: %d\n",
	// 		qdma_dev->func_id, ret);
	// 	return -1;
	// }

	// ret = qdma_dev_qinfo_get(qdma_dev->dma_device_index, qdma_dev->func_id,
	// 			(int *)&qdma_dev->queue_base,
	// 			&qdma_dev->qsets_en);
	// if (ret != QDMA_SUCCESS) {
	// 	PMD_DRV_LOG(ERR, "%s: Error %d querying qbase\n",
	// 			__func__, ret);
	// 	return -1;
	// }
	// PMD_DRV_LOG(INFO, "Bus: 0x%x\n",
	// 	xdma_dev->dma_device_index);

	// // xdma_dev->q_info = rte_zmalloc("qinfo", sizeof(struct queue_info) *
	// // 				(qdma_dev->qsets_en), 0);
	// // if (!qdma_dev->q_info) {
	// // 	PMD_DRV_LOG(ERR, "PF-%d(DEVFN) Cannot allocate "
	// // 			"memory for queue info\n", qdma_dev->func_id);
	// // 	return (-ENOMEM);
	// // }

	// /* Reserve memory for cmptq ring pointers
	//  * Max completion queues can be maximum of rx and tx queues.
	//  */
	// qdma_dev->cmpt_queues = rte_zmalloc("cmpt_queues",
	// 				    sizeof(qdma_dev->cmpt_queues[0]) *
	// 					qdma_dev->qsets_en,
	// 					RTE_CACHE_LINE_SIZE);
	// if (qdma_dev->cmpt_queues == NULL) {
	// 	PMD_DRV_LOG(ERR, "PF-%d(DEVFN) cmpt ring pointers memory "
	// 			"allocation failed:\n", qdma_dev->func_id);
	// 	rte_free(qdma_dev->q_info);
	// 	qdma_dev->q_info = NULL;
	// 	return -(ENOMEM);
	// }

	// for (qid = 0 ; qid < qdma_dev->qsets_en; qid++) {
	// 	/* Initialize queue_modes to all 1's ( i.e. Streaming) */
	// 	qdma_dev->q_info[qid].queue_mode = RTE_PMD_QDMA_STREAMING_MODE;

	// 	/* Disable the cmpt over flow check by default */
	// 	qdma_dev->q_info[qid].dis_cmpt_ovf_chk = 0;

	// 	qdma_dev->q_info[qid].trigger_mode = qdma_dev->trigger_mode;
	// 	qdma_dev->q_info[qid].timer_count =
	// 				qdma_dev->timer_count;
	// }

	// for (qid = 0 ; qid < dev->data->nb_rx_queues; qid++) {
	// 	qdma_dev->q_info[qid].cmpt_desc_sz = qdma_dev->cmpt_desc_len;
	// 	qdma_dev->q_info[qid].rx_bypass_mode =
	// 					qdma_dev->c2h_bypass_mode;
	// 	qdma_dev->q_info[qid].en_prefetch = qdma_dev->en_desc_prefetch;
	// 	qdma_dev->q_info[qid].immediate_data_state = 0;
	// }

	// for (qid = 0 ; qid < dev->data->nb_tx_queues; qid++)
	// 	qdma_dev->q_info[qid].tx_bypass_mode =
	// 					qdma_dev->h2c_bypass_mode;
	// for (stat_id = 0, qid = 0;
	// 	stat_id < RTE_ETHDEV_QUEUE_STAT_CNTRS;
	// 	stat_id++, qid++) {
	// 	/* Initialize map with qid same as stat_id */
	// 	qdma_dev->tx_qid_statid_map[stat_id] =
	// 		(qid < dev->data->nb_tx_queues) ? qid : -1;
	// 	qdma_dev->rx_qid_statid_map[stat_id] =
	// 		(qid < dev->data->nb_rx_queues) ? qid : -1;
	// }

	// ret = qdma_pf_fmap_prog(dev);
	// if (ret < 0) {
	// 	PMD_DRV_LOG(ERR, "FMAP programming failed\n");
	// 	rte_free(qdma_dev->q_info);
	// 	qdma_dev->q_info = NULL;
	// 	rte_free(qdma_dev->cmpt_queues);
	// 	qdma_dev->cmpt_queues = NULL;
	// 	return ret;
	// }

	xdma_dev->dev_configured = 1;

	return 0;
}

int xdma_dev_infos_get(struct rte_eth_dev *dev,
				struct rte_eth_dev_info *dev_info)
{
	struct xdma_dev *xdma_dev = dev->data->dev_private;

	dev_info->max_rx_queues = xdma_dev->c2h_channel_max;
	dev_info->max_tx_queues = xdma_dev->h2c_channel_max;
	dev_info->min_rx_bufsize = XDMA_MIN_RXBUFF_SIZE;
	dev_info->max_rx_pktlen = DMA_BRAM_SIZE;
	dev_info->max_mac_addrs = 1;

	return 0;
}
int xdma_dev_link_update(struct rte_eth_dev *dev,
				__rte_unused int wait_to_complete)
{
	dev->data->dev_link.link_status = ETH_LINK_UP;
	dev->data->dev_link.link_duplex = ETH_LINK_FULL_DUPLEX;

	/* TODO: Configure link speed by reading hardware capabilities */
	dev->data->dev_link.link_speed = ETH_SPEED_NUM_200G;

	PMD_DRV_LOG(INFO, "Link update done\n");
	return 0;
}


static struct eth_dev_ops xdma_eth_dev_ops = {
	.dev_configure            = xdma_dev_configure,
	.dev_infos_get            = xdma_dev_infos_get,
	.dev_start                = xdma_dev_start,
	.dev_stop                 = xdma_dev_stop,
	.dev_close                = xdma_dev_close,
	// .dev_reset                = xdma_dev_reset,
	.link_update              = xdma_dev_link_update,
	.rx_queue_setup           = xdma_rx_engine_setup,
	.tx_queue_setup           = xdma_tx_engine_setup,
	// .rx_queue_release         = xdma_dev_rx_queue_release,
	// .tx_queue_release         = xdma_dev_tx_queue_release,
	.rx_queue_start           = xdma_rx_engine_start,
	.rx_queue_stop            = xdma_rx_engine_stop,
	.tx_queue_start           = xdma_tx_engine_start,
	.tx_queue_stop            = xdma_tx_engine_stop,
	// .tx_done_cleanup          = xdma_dev_tx_done_cleanup,
	// .queue_stats_mapping_set  = xdma_dev_queue_stats_mapping,
	// .get_reg                  = xdma_dev_get_regs,
	// .stats_get                = xdma_dev_stats_get,
	// .stats_reset              = xdma_dev_stats_reset,
	// .rxq_info_get             = xdma_dev_rxq_info_get,
	// .txq_info_get             = xdma_dev_txq_info_get,
};

void xdma_dev_ops_init(struct rte_eth_dev *dev)
{
	dev->dev_ops = &xdma_eth_dev_ops;
	xdma_set_rx_function(dev);
	xdma_set_tx_function(dev);

}