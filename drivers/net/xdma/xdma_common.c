#include "xdma_common.h"
#include "xdma.h"

static int config_bar_idx_handler(__rte_unused const char *key,
					const char *value,  void *opaque)
{
	struct xdma_dev *xdma_dev = (struct xdma_dev *)opaque;
	char *end = NULL;

	PMD_DRV_LOG(INFO, "QDMA devargs trigger mode: %s\n", value);
	xdma_dev->config_bar_idx =  (int)strtoul(value, &end, 10);

	if (xdma_dev->config_bar_idx >= XDMA_BAR_NUM ||
			xdma_dev->config_bar_idx < 0) {
		PMD_DRV_LOG(INFO, "QDMA devargs config bar idx invalid: %d\n",
				xdma_dev->config_bar_idx);
		return -1;
	}
	return 0;
}

/* Process the all devargs */
int xdma_check_kvargs(struct rte_devargs *devargs,
						struct xdma_dev *qdma_dev)
{
	struct rte_kvargs *kvlist;
	// const char *pfetch_key        = "desc_prefetch";
	// const char *cmpt_desc_len_key = "cmpt_desc_len";
	// const char *trigger_mode_key  = "trigger_mode";
	const char *config_bar_key    = "config_bar";
	// const char *c2h_byp_mode_key  = "c2h_byp_mode";
	// const char *h2c_byp_mode_key  = "h2c_byp_mode";
#ifdef TANDEM_BOOT_SUPPORTED
	const char *en_st_key         = "en_st";
#endif
	int ret = 0;

	if (!devargs)
		return 0;

	kvlist = rte_kvargs_parse(devargs->args, NULL);
	if (!kvlist)
		return 0;

	// /* process the desc_prefetch*/
	// if (rte_kvargs_count(kvlist, pfetch_key)) {
	// 	ret = rte_kvargs_process(kvlist, pfetch_key,
	// 					pfetch_check_handler, qdma_dev);
	// 	if (ret) {
	// 		rte_kvargs_free(kvlist);
	// 		return ret;
	// 	}
	// }

	/* process the cmpt_desc_len*/
	// if (rte_kvargs_count(kvlist, cmpt_desc_len_key)) {
	// 	ret = rte_kvargs_process(kvlist, cmpt_desc_len_key,
	// 				 cmpt_desc_len_check_handler, qdma_dev);
	// 	if (ret) {
	// 		rte_kvargs_free(kvlist);
	// 		return ret;
	// 	}
	// }

	// /* process the trigger_mode*/
	// if (rte_kvargs_count(kvlist, trigger_mode_key)) {
	// 	ret = rte_kvargs_process(kvlist, trigger_mode_key,
	// 					trigger_mode_handler, qdma_dev);
	// 	if (ret) {
	// 		rte_kvargs_free(kvlist);
	// 		return ret;
	// 	}
	// }

	/* process the config bar*/
	if (rte_kvargs_count(kvlist, config_bar_key)) {
		ret = rte_kvargs_process(kvlist, config_bar_key,
					   config_bar_idx_handler, qdma_dev);
		if (ret) {
			rte_kvargs_free(kvlist);
			return ret;
		}
	}

	// /* process c2h_byp_mode*/
	// if (rte_kvargs_count(kvlist, c2h_byp_mode_key)) {
	// 	ret = rte_kvargs_process(kvlist, c2h_byp_mode_key,
	// 				  c2h_byp_mode_check_handler, qdma_dev);
	// 	if (ret) {
	// 		rte_kvargs_free(kvlist);
	// 		return ret;
	// 	}
	// }

	// /* process h2c_byp_mode*/
	// if (rte_kvargs_count(kvlist, h2c_byp_mode_key)) {
	// 	ret = rte_kvargs_process(kvlist, h2c_byp_mode_key,
	// 				  h2c_byp_mode_check_handler, qdma_dev);
	// 	if (ret) {
	// 		rte_kvargs_free(kvlist);
	// 		return ret;
	// 	}
	// }

	rte_kvargs_free(kvlist);
	return ret;
}

int xdma_identify_bars(struct rte_eth_dev *dev)
{
	uint64_t bar_len, i, ret;
	uint8_t  usr_bar;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	struct xdma_dev *dma_priv;

	dma_priv = (struct xdma_dev *)dev->data->dev_private;

	/* Config bar */
	bar_len = pci_dev->mem_resource[dma_priv->config_bar_idx].len;
	if (!bar_len) {
		PMD_DRV_LOG(INFO, "QDMA config BAR index :%d is not enabled",
					dma_priv->config_bar_idx);
		return -1;
	}

	/* Find AXI Master Lite(user bar) */
	ret = dma_priv->hw_access->xdma_get_user_bar(dev, &usr_bar);
	if ((ret != 0) ||
			(pci_dev->mem_resource[usr_bar].len == 0)) {
		if (dma_priv) {
			if (pci_dev->mem_resource[1].len == 0)
				dma_priv->user_bar_idx = 2;
			else
				dma_priv->user_bar_idx = 1;
		} else {
			dma_priv->user_bar_idx = -1;
			PMD_DRV_LOG(INFO, "Cannot find AXI Master Lite BAR");
		}
	} else
		dma_priv->user_bar_idx = usr_bar;

	/* Find AXI Bridge Master bar(bypass bar) */
	for (i = 0; i < XDMA_BAR_NUM; i++) {
		bar_len = pci_dev->mem_resource[i].len;
		if (!bar_len) /* Bar not enabled ? */
			continue;
		if (dma_priv->user_bar_idx != i &&
				dma_priv->config_bar_idx != i) {
			dma_priv->bypass_bar_idx = i;
			break;
		}
	}

	PMD_DRV_LOG(INFO, "QDMA config bar idx :%d\n",
			dma_priv->config_bar_idx);
	PMD_DRV_LOG(INFO, "QDMA AXI Master Lite bar idx :%d\n",
			dma_priv->user_bar_idx);
	PMD_DRV_LOG(INFO, "QDMA AXI Bridge Master bar idx :%d\n",
			dma_priv->bypass_bar_idx);

	return 0;
}
