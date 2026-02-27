#include "testapp.h"
#include "generic/rte_cycles.h"
#include <stdio.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int num_ports;
struct port_info pinfo[XDMA_MAX_PORTS];
char *filename;

struct rte_mbuf *pkts_0[NUM_RX_PKTS] = { NULL };
struct rte_mbuf *pkts_1[NUM_RX_PKTS] = { NULL };
struct rte_mempool *mp;

void port_close(int port_id)
{
	struct rte_mempool *mp;
	int user_bar_idx;
	int reg_val;
	int ret = 0;

	if (rte_pmd_xdma_get_device(port_id) == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
		return;
	}

	user_bar_idx = pinfo[port_id].user_bar_idx;

	rte_eth_dev_stop(port_id);

	rte_pmd_xdma_dev_close(port_id);

	pinfo[port_id].num_queues = 0;

	mp = rte_mempool_lookup(pinfo[port_id].mem_pool);

	if (mp != NULL)
		rte_mempool_free(mp);
}

int port_remove(int port_id)
{
	int ret = 0;

	printf("%s is received\n", __func__);

	/* Detach the port, it will invoke device remove/uninit */
	printf("Removing a device with port id %d\n", port_id);
	if (rte_pmd_xdma_get_device(port_id) == NULL) {
		printf("Port id %d already removed\n", port_id);
		return 0;
	}

	rte_spinlock_lock(&pinfo[port_id].port_update_lock);

	port_close(port_id);

	ret = rte_pmd_xdma_dev_remove(port_id);
	if (ret < 0)
		printf("Failed to remove device on port_id: %d\n", port_id);

	rte_spinlock_unlock(&pinfo[port_id].port_update_lock);

	if (!ret)
		printf("Port remove done successfully on port-id: %d\n",
			port_id);

	return ret;
}

int port_init(int port_id, int num_queues, int st_queues,
				int nb_descs, int buff_size)
{
	struct rte_mempool *mbuf_pool;
	struct rte_eth_conf	    port_conf;
	struct rte_eth_txconf   tx_conf;
	struct rte_eth_rxconf   rx_conf;
	int                     diag, x;
	uint32_t                queue_base, nb_buff;

	printf("Setting up port :%d.\n", port_id);

	if (rte_pmd_xdma_get_device(port_id) == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
		return -1;
	}

	snprintf(pinfo[port_id].mem_pool, RTE_MEMPOOL_NAMESIZE,
			MBUF_POOL_NAME_PORT, port_id);

	/* Mbuf packet pool */
	nb_buff = ((nb_descs) * num_queues * 2);

	/* NUM_TX_PKTS should be added to every queue as that many descriptors
	 * can be pending with application after Rx processing but before
	 * consumed by application or sent to Tx
	 */
	nb_buff += ((NUM_TX_PKTS) * num_queues);

	/*
	* rte_mempool_create_empty() has sanity check to refuse large cache
	* size compared to the number of elements.
	* CACHE_FLUSHTHRESH_MULTIPLIER (1.5) is defined in a C file, so using a
	* constant number 2 instead.
	*/
	nb_buff = RTE_MAX(nb_buff, MP_CACHE_SZ * 2);

	mbuf_pool = rte_pktmbuf_pool_create(pinfo[port_id].mem_pool, nb_buff,
			MP_CACHE_SZ, 0, buff_size +
			RTE_PKTMBUF_HEADROOM,
			rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, " Cannot create mbuf pkt-pool\n");
#ifdef DUMP_MEMPOOL_USAGE_STATS
	printf("%s(): %d: mpool = %p, mbuf_avail_count = %d,"
			" mbuf_in_use_count = %d,"
			"nb_buff = %u\n", __func__, __LINE__, mbuf_pool,
			rte_mempool_avail_count(mbuf_pool),
			rte_mempool_in_use_count(mbuf_pool), nb_buff);
#endif //DUMP_MEMPOOL_USAGE_STATS

	/*
	 * Make sure the port is configured.  Zero everything and
	 * hope for sane defaults
	 */
	memset(&port_conf, 0x0, sizeof(struct rte_eth_conf));
	memset(&tx_conf, 0x0, sizeof(struct rte_eth_txconf));
	memset(&rx_conf, 0x0, sizeof(struct rte_eth_rxconf));
	diag = rte_pmd_xdma_get_bar_details(port_id,
				&(pinfo[port_id].config_bar_idx),
				&(pinfo[port_id].user_bar_idx),
				&(pinfo[port_id].bypass_bar_idx));

	if (diag < 0)
		rte_exit(EXIT_FAILURE, "rte_pmd_xdma_get_bar_details failed\n");

	printf("XDMA Config bar idx: %d\n", pinfo[port_id].config_bar_idx);
	printf("XDMA AXI Master Lite bar idx: %d\n", pinfo[port_id].user_bar_idx);
	//printf("XDMA AXI Bridge Master bar idx: %d\n", pinfo[port_id].bypass_bar_idx);

	/* configure the device to use # queues */
	diag = rte_eth_dev_configure(port_id, num_queues, num_queues,
			&port_conf);
	if (diag < 0)
		rte_exit(EXIT_FAILURE, "Cannot configure port %d (err=%d)\n",
				port_id, diag);

	pinfo[port_id].num_queues = num_queues;
	pinfo[port_id].st_queues = st_queues;
	pinfo[port_id].buff_size = buff_size;
	pinfo[port_id].nb_descs = nb_descs;
    for (x = 0; x < num_queues; x++) {
		diag = rte_eth_tx_queue_setup(port_id, x, nb_descs, 0,
				&tx_conf);
		if (diag < 0)
			rte_exit(EXIT_FAILURE, "Cannot setup port %d "
					"TX Queue id:%d "
					"(err=%d)\n", port_id, x, diag);
		diag = rte_eth_rx_queue_setup(port_id, x, nb_descs, 0,
				&rx_conf, mbuf_pool);
		if (diag < 0)
			rte_exit(EXIT_FAILURE, "Cannot setup port %d "
					"RX Queue 0 (err=%d)\n", port_id, diag);
	
	}
	diag = rte_eth_dev_start(port_id);
	if (diag < 0)
		rte_exit(EXIT_FAILURE, "Cannot start port %d (err=%d)\n",
				port_id, diag);

	rte_pmd_xdma_dev_fp_ops_config(port_id);
	return 0;
}

int port_reset(int port_id, int num_queues, int st_queues,
				int nb_descs, int buff_size)
{
	int ret = 0;

	printf("%s is received\n", __func__);

	if (rte_pmd_xdma_get_device(port_id) == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
		return -1;
	}

	rte_spinlock_lock(&pinfo[port_id].port_update_lock);

	port_close(port_id);

	ret = rte_eth_dev_reset(port_id);
	if (ret < 0) {
		printf("Error: Failed to reset device for port: %d\n", port_id);
		rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}

	ret = port_init(port_id, num_queues, st_queues,
				nb_descs, buff_size);
	if (ret < 0)
		printf("Error: Failed to initialize port: %d\n", port_id);

	rte_spinlock_unlock(&pinfo[port_id].port_update_lock);

	if (!ret)
		printf("Port reset done successfully on port-id: %d\n",
			port_id);

	return ret;
}




static int dev_remove_callback(uint16_t port_id,
				enum rte_eth_event_type type,
				void *param __rte_unused, void *ret_param)
{
	int ret = 0;

	RTE_SET_USED(ret_param);
	printf("%s is received\n", __func__);

	if (type != RTE_ETH_EVENT_INTR_RMV) {
		printf("Error: Invalid event value. "
				"Expected = %d, Received = %d\n",
				RTE_ETH_EVENT_INTR_RMV, type);
		return -ENOMSG;
	}

	ret = port_remove(port_id);
	if (ret < 0)
		printf("Error: Failed to remove port: %d\n", port_id);

	return 0;
}

static int dev_reset_callback(uint16_t port_id,
				enum rte_eth_event_type type,
				void *param __rte_unused, void *ret_param)
{
	int ret = 0;

	RTE_SET_USED(ret_param);
	printf("%s is received\n", __func__);

	if (type != RTE_ETH_EVENT_INTR_RESET) {
		printf("Error: Invalid event value. "
				"Expected = %d, Received = %d\n",
				RTE_ETH_EVENT_INTR_RESET, type);
		return -ENOMSG;
	}

	ret = port_reset(port_id, pinfo[port_id].num_queues,
			pinfo[port_id].st_queues,
			pinfo[port_id].nb_descs,
			pinfo[port_id].buff_size);
	if (ret < 0)
		printf("Error: Failed to reset port: %d\n", port_id);

	return ret;
}

static struct option const long_opts[] = {
{"filename", 1, 0, 0},
{NULL, 0, 0, 0}
};


void* fpga_to_fpga(void* args) {
	struct ThreadArgs *arg = (struct ThreadArgs*)args;
	int src_port_id = arg->src_port_id, src_queue_id = arg->src_queue_id;
	int dst_port_id = arg->dst_port_id, dst_queue_id = arg->dst_queue_id;
	struct rte_mbuf **pkts = arg->pkts;
	int ret = 0, tmp = 0;
    int recv_batch = 0, recv_pkts = 0;
	int xmit_batch = 0, xmit_pkts = 0;
	int pkts_buffer_idx = 0;
	unsigned char *test_bit_addr;
	bool test;
	int pkts_len;
	struct timespec start_time, end_time;
	struct timespec recv_start_time, recv_end_time;
	struct timespec xmit_start_time, xmit_end_time;
	double duration, xmit_delay = 0, recv_delay = 0;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &recv_start_time);
		ret = rte_eth_rx_burst(src_port_id, src_queue_id, pkts,
			SIZE_OF_PKTS_BUFFER);
		if (ret < 0) {
			printf("ret = %d Recv Failed!\n", ret);
			rte_exit(EXIT_FAILURE, "Recv Failed!\n");
		}
		 clock_gettime(CLOCK_MONOTONIC, &recv_end_time);
		 duration = (recv_end_time.tv_sec - recv_start_time.tv_sec) +
		 		(recv_end_time.tv_nsec - recv_start_time.tv_nsec) / 1e9;
		 recv_delay += duration;
		for(int i = pkts_buffer_idx; i < pkts_buffer_idx + ret; i++) {
			test_bit_addr = (unsigned char *)pkts[i] + pkts[i]->data_off;
			pkts_len = pkts[i]->data_len;
			int bit = (*test_bit_addr >> 7) & 0x01;
			if(bit && ~bit) {
				printf("Test Bit Error!");
				rte_exit(EXIT_FAILURE, "Test Bit Error!\n");
			}
		}
		if(ret > 0) {
			recv_pkts += ret;
			recv_batch++;
			tmp = ret;
			if(src_port_id == 1)
				printf("\t");
			printf("thread %d\t (%d):recv count: %d, total recv count: %d, delay = %.10f, total recv delay = %.10f\n", src_port_id, recv_batch, ret, recv_pkts, duration, recv_delay);
			clock_gettime(CLOCK_MONOTONIC, &xmit_start_time);
			ret = rte_eth_tx_burst(dst_port_id, dst_queue_id, &pkts[pkts_buffer_idx], ret);
			if (ret < 0) {
				printf("ret = %d Xmit Failed!\n", ret);
				rte_exit(EXIT_FAILURE, "Xmit Failed!\n");
			}
			 clock_gettime(CLOCK_MONOTONIC, &xmit_end_time);
			 duration = (xmit_end_time.tv_sec - xmit_start_time.tv_sec) +
			 		(xmit_end_time.tv_nsec - xmit_start_time.tv_nsec) / 1e9;
			xmit_delay += duration;
			pkts_buffer_idx = (pkts_buffer_idx + ret) % SIZE_OF_PKTS_BUFFER;
		} 
		// else {
		// 	rte_delay_us(DELAY_TIME_US);
		// }
		if(ret > 0) {
			xmit_pkts += ret;
			xmit_batch++;
			if(dst_port_id == 0)
				printf("\t");
			printf("thread %d\t (%d):xmit count: %d, total xmit count: %d, delay = %.10f, total xmit delay = %.10f\n", src_port_id, xmit_batch, ret, xmit_pkts, duration, xmit_delay);
		}
		clock_gettime(CLOCK_MONOTONIC, &end_time);
		duration = (end_time.tv_sec - start_time.tv_sec) +
                  (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
		if (duration > 1.0) 
		{
			printf("There is thread %d\n",src_port_id);
			printf("%d to %d :data_len = %d\n",src_port_id, dst_port_id, pkts_len);
			printf("%d to %d :xmit count: %d recv count: %d\n",src_port_id, dst_port_id, xmit_pkts, recv_pkts);
			printf("%d to %d :xmit batch: %d recv batch: %d\n",src_port_id, dst_port_id, xmit_batch, recv_batch);
			printf("%d to %d :recv_delay = %.10f, ave_recv_delay = %.10f\n",src_port_id, dst_port_id, recv_delay, recv_delay / recv_batch);
			printf("%d to %d :xmit_delay = %.10f, ave_xmit_delay = %.10f\n",src_port_id, dst_port_id, xmit_delay, xmit_delay / xmit_batch);
			fflush(stdout);
			break;
		}
	}
	return NULL;
}




int main(int argc, char **argv)
{
	const struct rte_memzone *mz = 0;
	int port_id   = 0;
	int ret = 0;
	int curr_avail_ports = 0;
	int ifd,ifd_0;
	pthread_t tid_0, tid_1;

	int queue_id = 0;

	printf("XDMA testapp rte eal init...\n");

	/* Make sure things are initialized ... */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	rte_log_set_global_level(RTE_LOG_DEBUG);

	printf("Ethernet Device Count: %d\n", (int)rte_eth_dev_count_avail());

	printf("Logical Core Count: %d\n", rte_lcore_count());

	num_ports = rte_eth_dev_count_avail();
	if (num_ports < 1)
		rte_exit(EXIT_FAILURE, "No Ethernet devices found."
			" Try updating the FPGA image.\n");

	for (port_id = 0; port_id < num_ports; port_id++)
		rte_spinlock_init(&pinfo[port_id].port_update_lock);

	ret = rte_eth_dev_callback_register(RTE_ETH_ALL, RTE_ETH_EVENT_INTR_RMV,
				dev_remove_callback, NULL);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Failed to register dev_remove_callback\n");

	ret = rte_eth_dev_callback_register(RTE_ETH_ALL,
			RTE_ETH_EVENT_INTR_RESET,
			dev_reset_callback, NULL);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Failed to register dev_reset_callback\n");




	/* Allocate aligned mezone */
	rte_pmd_xdma_compat_memzone_reserve_aligned();


	rte_eth_dev_callback_unregister(RTE_ETH_ALL, RTE_ETH_EVENT_INTR_RMV,
			dev_remove_callback, NULL);

	rte_eth_dev_callback_unregister(RTE_ETH_ALL, RTE_ETH_EVENT_INTR_RESET,
			dev_reset_callback, NULL);

	ret = port_init(0, 4, 4, 1, 1024);
	ret = port_init(1, 4, 4, 1, 1024);

	port_id = 0;
	mp = rte_mempool_lookup(pinfo[port_id].mem_pool);
	/* get the mempool from which to acquire buffers */
	if (mp == NULL) {
		printf("Could not find mempool with name %s\n",
				pinfo[port_id].mem_pool);
		rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}
	for(int i=0; i < 32; i++) {
		pkts_0[i] = rte_pktmbuf_alloc(mp);
		pkts_0[i]->data_len = 65535;
		pkts_1[i] = rte_pktmbuf_alloc(mp);
		pkts_1[i]->data_len = 65535;
	}
	 struct ThreadArgs args_0 = {0, 0, 1, 0, pkts_0};
	 struct ThreadArgs args_1 = {1, 0, 0, 0, pkts_1};

	//struct ThreadArgs args_0 = {0, 0, 0, 0, pkts_0};

	pthread_create(&tid_0, NULL, fpga_to_fpga, &args_0);
	pthread_create(&tid_1, NULL, fpga_to_fpga, &args_1);
	pthread_join(tid_0, NULL);
	pthread_join(tid_1, NULL);

	curr_avail_ports = rte_eth_dev_count_avail();
	if (!curr_avail_ports)
		printf("Ports already removed\n");
	else {
		for (port_id = num_ports - 1; port_id >= 0; port_id--) {

			if (pinfo[port_id].num_queues)
				port_close(port_id);

			printf("Removing a device with port id %d\n", port_id);
			if (rte_pmd_xdma_get_device(port_id) == NULL) {
				printf("Port id %d already removed\n", port_id);
				continue;
			}
			/* Detach the port, it will invoke
			 * device remove/uninit
			 */
			if (rte_pmd_xdma_dev_remove(port_id))
				printf("Failed to detach port '%d'\n", port_id);
		}
	}

	return 0;
}
