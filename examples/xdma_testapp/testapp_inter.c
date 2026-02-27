#include "testapp_lat.h"
#include "ethdev_driver.h"
#include "generic/rte_cycles.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 1024

#define THREAD_NUM 3

int num_ports;
struct port_info pinfo[XDMA_MAX_PORTS];
char *filename;


struct rte_mbuf *pkts_0[NUM_RX_PKTS] = { NULL };
struct rte_mbuf *pkts_1[NUM_RX_PKTS] = { NULL };
struct rte_mbuf *pkts_2[NUM_RX_PKTS] = { NULL };
struct rte_mbuf *pkts_3[NUM_RX_PKTS] = { NULL };
struct rte_mbuf *pkts_4[NUM_RX_PKTS] = { NULL };
struct rte_mbuf *pkts_5[NUM_RX_PKTS] = { NULL };
struct rte_mempool *mp;

struct rte_mbuf **pkts[6] = { pkts_0, pkts_1, pkts_2, pkts_3, pkts_4, pkts_5 };

int ready_count = 0;          // 记录已到达同步点的 fpga_to_fpga 线程数
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // 保护 ready_count 的互斥锁
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;     // 通知 switch_start 的条件变量

int pkt_num = 16;

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

int
port_init_nic(uint16_t port_id,
              uint16_t nb_rx_queues,
              uint16_t nb_tx_queues,
              uint16_t nb_descs,
              uint16_t buff_size)
{
    struct rte_mempool *mbuf_pool;
    struct rte_eth_conf port_conf;
    struct rte_eth_txconf tx_conf;
    struct rte_eth_rxconf rx_conf;
    int ret;
    uint16_t q;
    uint32_t nb_buff;

    printf("Setting up NIC port :%d.\n", port_id);

    /* 检查 port 存在 */
    if (!rte_eth_dev_is_valid_port(port_id)) {
        printf("Invalid NIC port id %d\n", port_id);
        return -1;
    }

    /* 建一个 mempool（你可以和 xdma 共享一个，也可以分开建） */
    char pool_name[RTE_MEMPOOL_NAMESIZE];
    snprintf(pool_name, sizeof(pool_name), "NIC_MB_POOL_%u", port_id);

    nb_buff = (uint32_t)nb_descs * (nb_rx_queues + nb_tx_queues) * 2;
    nb_buff = RTE_MAX(nb_buff, (uint32_t)MP_CACHE_SZ * 2);

    mbuf_pool = rte_pktmbuf_pool_create(pinfo[port_id].mem_pool,
                                        nb_buff,
                                        MP_CACHE_SZ,
                                        0,
                                        buff_size + RTE_PKTMBUF_HEADROOM,
                                        rte_socket_id());
    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create NIC mbuf pool\n");

    memset(&port_conf, 0, sizeof(port_conf));
    memset(&tx_conf,  0, sizeof(tx_conf));
    memset(&rx_conf,  0, sizeof(rx_conf));

    /* 一些常见配置示例，可根据需要调整 */
    port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_NONE;
    port_conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;

    ret = rte_eth_dev_configure(port_id, nb_rx_queues, nb_tx_queues, &port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure NIC port %u (err=%d)\n", port_id, ret);

    /* 可选：根据设备能力获取默认 RX/TX 队列配置 */
    struct rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(port_id, &dev_info);
    rx_conf = dev_info.default_rxconf;
    tx_conf = dev_info.default_txconf;
    rx_conf.offloads = port_conf.rxmode.offloads;
    tx_conf.offloads = port_conf.txmode.offloads;

    for (q = 0; q < nb_rx_queues; q++) {
        ret = rte_eth_rx_queue_setup(port_id, q, nb_descs,
                                     rte_eth_dev_socket_id(port_id),
                                     &rx_conf, mbuf_pool);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "Cannot setup NIC RX queue %u on port %u (err=%d)\n",
                     q, port_id, ret);
    }

    for (q = 0; q < nb_tx_queues; q++) {
        ret = rte_eth_tx_queue_setup(port_id, q, nb_descs,
                                     rte_eth_dev_socket_id(port_id),
                                     &tx_conf);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "Cannot setup NIC TX queue %u on port %u (err=%d)\n",
                     q, port_id, ret);
    }

    ret = rte_eth_dev_start(port_id);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot start NIC port %u (err=%d)\n",
                 port_id, ret);

    /* 典型：打开 promiscuous 模式方便调试 */
    rte_eth_promiscuous_enable(port_id);

    return 0;
}

int port_init_xdma(int port_id, int num_queues, int st_queues,
				int nb_descs, int buff_size)
{
	struct rte_mempool *mbuf_pool;
	struct rte_eth_conf	    port_conf;
	struct rte_eth_txconf   tx_conf;
	struct rte_eth_rxconf   rx_conf;
	int                     diag, x;
	uint32_t                queue_base, nb_buff;

	printf("Setting up XDMA port :%d.\n", port_id);

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

	ret = port_init_xdma(port_id, num_queues, st_queues,
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

uint32_t reg_soft_clear = LATPKG_REG_SOFT_CLEAR;

uint32_t reg_mode[CHANNEL_NUM] = {LATPKG_REG_MODE_0, LATPKG_REG_MODE_1, LATPKG_REG_MODE_2};
uint32_t reg_interval[CHANNEL_NUM] = {LATPKG_REG_INTERVAL_0, LATPKG_REG_INTERVAL_1, LATPKG_REG_INTERVAL_2};
uint32_t reg_explambda[CHANNEL_NUM] = {LATPKG_REG_EXPLAMBDA_0, LATPKG_REG_EXPLAMBDA_1, LATPKG_REG_EXPLAMBDA_2};
uint32_t reg_packetnum[CHANNEL_NUM] = {LATPKG_REG_PACKETNUM_0, LATPKG_REG_PACKETNUM_1, LATPKG_REG_PACKETNUM_2};
uint32_t reg_seed[CHANNEL_NUM] = {LATPKG_REG_SEED_0, LATPKG_REG_SEED_1, LATPKG_REG_SEED_2};
uint32_t reg_start[CHANNEL_NUM] = {LATPKG_REG_START_0, LATPKG_REG_START_1, LATPKG_REG_START_2};
uint32_t reg_stop[CHANNEL_NUM] = {LATPKG_REG_STOP_0, LATPKG_REG_STOP_1, LATPKG_REG_STOP_2};
uint32_t reg_done[CHANNEL_NUM] = {LATPKG_REG_DONE_0, LATPKG_REG_DONE_1, LATPKG_REG_DONE_2};
uint32_t reg_recvcount[CHANNEL_NUM] = {LATPKG_REG_RECVCOUNT_0, LATPKG_REG_RECVCOUNT_1, LATPKG_REG_RECVCOUNT_2};
uint32_t reg_rx_done[CHANNEL_NUM] = {LATPKG_REG_RX_DONE_0, LATPKG_REG_RX_DONE_1, LATPKG_REG_RX_DONE_2};



uint32_t reg_accum_hi[CHANNEL_NUM][ACCUM_REG_NUM] = 
{
    LATPKG_REG_ACCUM_HI_0_0, LATPKG_REG_ACCUM_HI_0_1, LATPKG_REG_ACCUM_HI_0_2, LATPKG_REG_ACCUM_HI_0_3, LATPKG_REG_ACCUM_HI_0_4, LATPKG_REG_ACCUM_HI_0_5, LATPKG_REG_ACCUM_HI_0_6, LATPKG_REG_ACCUM_HI_0_7, LATPKG_REG_ACCUM_HI_0_8,
    LATPKG_REG_ACCUM_HI_1_0, LATPKG_REG_ACCUM_HI_1_1, LATPKG_REG_ACCUM_HI_1_2, LATPKG_REG_ACCUM_HI_1_3, LATPKG_REG_ACCUM_HI_1_4, LATPKG_REG_ACCUM_HI_1_5, LATPKG_REG_ACCUM_HI_1_6, LATPKG_REG_ACCUM_HI_1_7, LATPKG_REG_ACCUM_HI_1_8,
    LATPKG_REG_ACCUM_HI_2_0, LATPKG_REG_ACCUM_HI_2_1, LATPKG_REG_ACCUM_HI_2_2, LATPKG_REG_ACCUM_HI_2_3, LATPKG_REG_ACCUM_HI_2_4, LATPKG_REG_ACCUM_HI_2_5, LATPKG_REG_ACCUM_HI_2_6, LATPKG_REG_ACCUM_HI_2_7, LATPKG_REG_ACCUM_HI_2_8,
};
uint32_t reg_accum_lo[CHANNEL_NUM][ACCUM_REG_NUM] = 
{
    LATPKG_REG_ACCUM_LO_0_0, LATPKG_REG_ACCUM_LO_0_1, LATPKG_REG_ACCUM_LO_0_2, LATPKG_REG_ACCUM_LO_0_3, LATPKG_REG_ACCUM_LO_0_4, LATPKG_REG_ACCUM_LO_0_5, LATPKG_REG_ACCUM_LO_0_6, LATPKG_REG_ACCUM_LO_0_7, LATPKG_REG_ACCUM_LO_0_8,
    LATPKG_REG_ACCUM_LO_1_0, LATPKG_REG_ACCUM_LO_1_1, LATPKG_REG_ACCUM_LO_1_2, LATPKG_REG_ACCUM_LO_1_3, LATPKG_REG_ACCUM_LO_1_4, LATPKG_REG_ACCUM_LO_1_5, LATPKG_REG_ACCUM_LO_1_6, LATPKG_REG_ACCUM_LO_1_7, LATPKG_REG_ACCUM_LO_1_8,
    LATPKG_REG_ACCUM_LO_2_0, LATPKG_REG_ACCUM_LO_2_1, LATPKG_REG_ACCUM_LO_2_2, LATPKG_REG_ACCUM_LO_2_3, LATPKG_REG_ACCUM_LO_2_4, LATPKG_REG_ACCUM_LO_2_5, LATPKG_REG_ACCUM_LO_2_6, LATPKG_REG_ACCUM_LO_2_7, LATPKG_REG_ACCUM_LO_2_8,
};





uint32_t devmem_read(uint32_t addr) {
    uint32_t ret;
    ret = rte_user_read(1, addr);
    return ret;
}

uint64_t devmem_read_64(uint32_t addr) {
    uint32_t ret_hi, ret_lo;
    uint64_t ret;
    ret_hi = rte_user_read(1, addr);
    ret_lo = rte_user_read(1, addr + 4);
    ret = ((uint64_t)ret_hi << 32) | (uint64_t)ret_lo;
    return ret;
}

int devmem_write(uint32_t addr, uint32_t value) {
    int ret;
    ret = rte_user_write(1, addr, value);
    return ret;
}

int configure_channel(int ch, uint32_t mode, uint32_t interval, uint32_t exp_lambda, uint32_t packet_num, uint32_t seed) {
    devmem_write(reg_mode[ch], mode);
    devmem_write(reg_interval[ch],interval);
    devmem_write(reg_explambda[ch], exp_lambda);
    devmem_write(reg_packetnum[ch], packet_num);
    devmem_write(reg_seed[ch], seed);
    return 0;
}

int start_channel(int ch) {
    devmem_write(reg_start[ch], 1);
    return 0;
}

int stop_channel(int ch) {
    devmem_write(reg_stop[ch], 1);
    return 0;
}

int clear_accum() {
    devmem_write(reg_soft_clear, 1);
	return 0;
}

bool wait_rx_done(int ch, double timeout) 
{
    uint32_t val;
    struct timespec start_time, end_time, sleep_time;
    double duration;
    int ret;
	sleep_time.tv_sec  = 0;
	sleep_time.tv_nsec = 10000000L;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    while(1) {
        val = devmem_read(reg_rx_done[ch]) & 0x1;
        if(val){
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            duration = (end_time.tv_sec - start_time.tv_sec) +
		  		(end_time.tv_nsec - start_time.tv_nsec) / 1e9;
				printf("rx done time %.3f seconds\n",duration);
            return true;
        }
        else {
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            duration = (end_time.tv_sec - start_time.tv_sec) +
		  		(end_time.tv_nsec - start_time.tv_nsec) / 1e9;
            if(duration > timeout)
                return false;
        }
        nanosleep(&sleep_time, NULL);
    }
}

int read_results(int ch, uint64_t* accum) {
    for(int i = 0; i < ACCUM_REG_NUM; i++) {
        accum[i] = devmem_read_64(reg_accum_hi[ch][i]);
    }
    return 0;
}

uint32_t read_recvcount(int ch) {
    uint64_t ret;
    ret = devmem_read(reg_recvcount[ch]);
    return ret;
}

int init() {
    devmem_write(0x100000, 1);
	//rte_user_write(1, 0x100000, 1);
    sleep(1);
    devmem_write(0x100000, 0);
	//rte_user_write(1, 0x100000, 0);
	return 0;
}

static int fpga_to_fpga(void* args) {
	struct thread_args *arg = (struct thread_args*)args;
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
	int transfer = 0;
	int complete = 0;
	int target_num = pkt_num;

	unsigned int lcore_id = rte_lcore_id();
	RTE_LOG(INFO, USER1, "FPGA Thread running on lcore %u\n", lcore_id);

	pthread_mutex_lock(&mutex);
    ready_count++;  // 计数器加1

    // 如果两个线程都到达，唤醒等待的 switch_start 线程
    if (ready_count == 2) {
        pthread_cond_signal(&cond);  // 发送条件信号
    }

    pthread_mutex_unlock(&mutex);

	sleep(1);

	while(1) {
		//clock_gettime(CLOCK_MONOTONIC, &recv_start_time);
		ret = rte_eth_rx_burst(src_port_id, src_queue_id, pkts,
			SIZE_OF_PKTS_BUFFER);
		if (ret < 0) {
			printf("ret = %d Recv Failed!\n", ret);
			rte_exit(EXIT_FAILURE, "Recv Failed!\n");
		}
		 //clock_gettime(CLOCK_MONOTONIC, &recv_end_time);
		//  duration = (recv_end_time.tv_sec - recv_start_time.tv_sec) +
		//  		(recv_end_time.tv_nsec - recv_start_time.tv_nsec) / 1e9;
		//  recv_delay += duration;
		// for(int i = pkts_buffer_idx; i < pkts_buffer_idx + ret; i++) {
		// 	test_bit_addr = (unsigned char *)pkts[i] + pkts[i]->data_off;
		// 	pkts_len = pkts[i]->data_len;
		// 	int bit = (*test_bit_addr >> 7) & 0x01;
		// 	if(bit && ~bit) {
		// 		printf("Test Bit Error!");
		// 		rte_exit(EXIT_FAILURE, "Test Bit Error!\n");
		// 	}
		// }
		if(ret > 0) {
			recv_pkts += ret;
			recv_batch++;
			tmp = ret;
			if(src_port_id == 1) {
				printf("FPGA --> NIC ");
			} else {
				printf("NIC --> FPGA ");
			}
			printf("thread %d\t (%d):recv count: %d, total recv count: %d\n", src_port_id, recv_batch, ret, recv_pkts);
		}
		if(ret > 0) {
			//clock_gettime(CLOCK_MONOTONIC, &xmit_start_time);
			ret = rte_eth_tx_burst(dst_port_id, dst_queue_id, &pkts[pkts_buffer_idx], ret);
			if (ret < 0) {
				printf("ret = %d Xmit Failed!\n", ret);
				rte_exit(EXIT_FAILURE, "Xmit Failed!\n");
			}
			//  clock_gettime(CLOCK_MONOTONIC, &xmit_end_time);
			//  duration = (xmit_end_time.tv_sec - xmit_start_time.tv_sec) +
			//  		(xmit_end_time.tv_nsec - xmit_start_time.tv_nsec) / 1e9;
			// xmit_delay += duration;
			//pkts_buffer_idx = (pkts_buffer_idx + ret) % SIZE_OF_PKTS_BUFFER;
			//printf("pkts_buffer_idx = %d\n",pkts_buffer_idx);
			if(ret >= 0) {
				xmit_pkts += ret;
				xmit_batch++;
				if(src_port_id == 1) {
					printf("FPGA --> NIC ");
				} else {
					printf("NIC --> FPGA ");
				}
				printf("thread %d\t (%d):xmit count: %d, total xmit count: %d\n", src_port_id, xmit_batch, ret, xmit_pkts);			
			}
		} 
		// clock_gettime(CLOCK_MONOTONIC, &end_time);
		// duration = (end_time.tv_sec - start_time.tv_sec) +
        //           (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
		// if (duration > 1.0) 
		// {
		// 	printf("There is thread %d\n",src_port_id);
		// 	printf("%d to %d :data_len = %d\n",src_port_id, dst_port_id, pkts_len);
		// 	printf("%d to %d :xmit count: %d recv count: %d\n",src_port_id, dst_port_id, xmit_pkts, recv_pkts);
		// 	printf("%d to %d :xmit batch: %d recv batch: %d\n",src_port_id, dst_port_id, xmit_batch, recv_batch);
		// 	printf("%d to %d :recv_delay = %.10f, ave_recv_delay = %.10f\n",src_port_id, dst_port_id, recv_delay, recv_delay / recv_batch);
		// 	printf("%d to %d :xmit_delay = %.10f, ave_xmit_delay = %.10f\n",src_port_id, dst_port_id, xmit_delay, xmit_delay / xmit_batch);
		// 	fflush(stdout);
		// 	break;
		// }
		if((xmit_pkts == pkt_num) & !complete){
			//struct timespec ts;
			if(src_port_id == 0)
				printf("Patr --> Main ");
			else
			 	printf("Main --> Patr ");
			complete = 1;
			//clock_gettime(CLOCK_REALTIME, &ts);
			printf("All packets is completed pkt_num = %d\n",xmit_pkts);
			//printf("Time: %ld s, %ld ns\n", ts.tv_sec, ts.tv_nsec);
			break;
		}
		else if((xmit_pkts > target_num)) {
			printf("ERROR! pkt_num = %d\n",xmit_pkts);
			target_num = xmit_pkts;
		}
	}
	//fclose(file);
	return 0;
}



static int  switch_start(void* args) 
{
	while (ready_count < 2) {
        pthread_cond_wait(&cond, &mutex);  // 阻塞并释放锁，直到条件满足
    }

    unsigned int lcore_id = rte_lcore_id();
    RTE_LOG(INFO, USER1, "FPGA Thread running on lcore %u\n", lcore_id);

    int ch = 0;
    int mode = 3;
    int interval = 2000;
    int exp_lambda = 16;
    int packet_num = pkt_num;
	int seed = 1;
	int timeout = 1000;
	int role_freq = 50, shell_freq = 250;
    uint64_t accum[ACCUM_REG_NUM];
    double main_role_in, main_role_out, main_shell_in, main_shell_out;
	double part_role_in, part_role_out, part_shell_in, part_shell_out;
    double main_role_busy, main_role_to_shell, main_shell_busy, shell_to_shell, part_role_busy, part_shell_busy, part_shell_to_role;
    init();
    clear_accum();
	configure_channel(ch, mode, interval, exp_lambda, packet_num, seed);
	printf("===初始化===\n");
	printf("[配置通道%d] mode=%d, interval=%d, exp_lambda=%d, packet_num=%d, seed=%d\n",ch, mode, interval, exp_lambda, packet_num, seed);
    start_channel(ch);
    if(wait_rx_done(ch, timeout)) {
        int recv_count = read_recvcount(ch);
        read_results(ch, accum);
        printf("Channel %d recved %d packets\n", ch, recv_count);
        for(int i = 0; i < ACCUM_REG_NUM; i++) {
            printf("ACCUM%d: %ld\n", i, (long int)accum[i]);
        }
        if(recv_count > 0) {
            main_role_in = (long int)accum[1] / (double)recv_count / role_freq;
            main_role_out = (long int)accum[2] / (double)recv_count / role_freq;
            main_shell_in = (long int)accum[3] / (double)recv_count / shell_freq;
            main_shell_out = (long int)accum[4] / (double)recv_count / shell_freq;
            part_role_in = (long int)accum[5] / (double)recv_count / role_freq;
            part_role_out = (long int)accum[6] / (double)recv_count / role_freq;
            part_shell_in = (long int)accum[7] / (double)recv_count / shell_freq;
            part_shell_out = (long int)accum[8] / (double)recv_count / shell_freq;
			printf("=== 统计结果 ===\n");
			printf("main_role RTT: %.3f us\n", main_role_in);
            printf("main_role_out: %.3f us\n", main_role_out);
            printf("main_shell_in: %.3f us\n", main_shell_in);
            printf("main_shell_out: %.3f us\n", main_shell_out);
            printf("part_role_in: %.3f us\n", part_role_in);
            printf("part_role_out: %.3f us\n", part_role_out);
            printf("part_shell_in: %.3f us\n", part_shell_in);
            printf("part_shell_out: %.3f us\n", part_shell_out);
            main_role_busy = main_role_in - main_role_out;
            main_role_to_shell = main_role_out - main_shell_in;
            main_shell_busy = main_shell_in - main_shell_out;
            shell_to_shell = main_shell_out - part_shell_in;
            part_role_busy = part_role_in - part_role_out;
            part_shell_busy= part_shell_in - part_shell_out;
            part_shell_to_role = part_shell_out - part_role_in;
            printf("main_role_to_shell: %.3f us\n", main_role_to_shell);
            printf("main_role_busy: %.3f us\n", main_role_busy);
            printf("main_shell_busy: %.3f us\n", main_shell_busy);
            printf("shell_to_shell: %.3f us\n", shell_to_shell);
            printf("part_role_busy: %.3f us\n", part_role_busy);
            printf("part_shell_busy: %.3f us\n", part_shell_busy);
            printf("part_shell_to_role: %.3f us\n", part_shell_to_role);
        }
    } else {
        printf("Failed: RX_DONE timeout\n");
    }

	return 0;
}




int main(int argc, char **argv)
{
	const struct rte_memzone *mz = 0;
	int port_id   = 0;
	int ret = 0;
	int curr_avail_ports = 0;
	int ifd,ifd_0;

	int queue_id = 0;

	const char *eal_args[] = {
        "xdma_testapp",
        "-l", "0,4,6,8",         
        "--main-lcore", "0",   
        "-n", "4"                
    };

	int eal_argc = sizeof(eal_args)/sizeof(eal_args[0]);

	printf("XDMA testapp rte eal init...\n");

	/* Make sure things are initialized ... */
	ret = rte_eal_init(eal_argc, (char**)eal_args);
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

	ret = port_init_nic(0, 4, 4, 32, 1024);
	ret = port_init_xdma(1, 4, 4, 32, 1024);

	port_id = 0;
	mp = rte_mempool_lookup(pinfo[port_id].mem_pool);


	if (mp == NULL) {
		printf("Could not find mempool with name %s\n",
				pinfo[port_id].mem_pool);
		rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}
	for(int i=0; i < 32; i++) {
		pkts_1[i] = rte_pktmbuf_alloc(mp);
		pkts_1[i]->data_len = 65535;
        pkts_3[i] = rte_pktmbuf_alloc(mp);
		pkts_3[i]->data_len = 65535;
        pkts_5[i] = rte_pktmbuf_alloc(mp);
		pkts_5[i]->data_len = 65535;
	}

	port_id = 1;
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
        pkts_2[i] = rte_pktmbuf_alloc(mp);
		pkts_2[i]->data_len = 65535;
        pkts_4[i] = rte_pktmbuf_alloc(mp);
		pkts_4[i]->data_len = 65535;
	}

	unsigned int worker_lcores[] = {4, 6, 8}; 

	struct thread_args args[RTE_MAX_LCORE];
    memset(args, 0, sizeof(args));

	for(int i = 0; i < THREAD_NUM - 1; i++) {
		args[worker_lcores[i]].src_port_id = i % 2;
		args[worker_lcores[i]].src_queue_id = i / 2;
		args[worker_lcores[i]].dst_port_id = (i + 1) % 2;
		args[worker_lcores[i]].dst_queue_id = i / 2;
		args[worker_lcores[i]].pkts = pkts[i];
	}

	if (rte_lcore_id() == rte_get_main_lcore()) {
 		RTE_LOG(INFO, USER1, "Master thread running on lcore %u\n", 
               rte_lcore_id());
        
        // 启动worker线程
        for (int i = 0; i < THREAD_NUM; i++) {
            if (!rte_lcore_is_enabled(worker_lcores[i])) {
                rte_exit(EXIT_FAILURE, "Lcore %u not enabled\n", 
                         worker_lcores[i]);
            }
            
            // 异步启动工作线程
            rte_eal_remote_launch(
                (i == (THREAD_NUM - 1)) ? switch_start : fpga_to_fpga,
                &args[worker_lcores[i]], 
                worker_lcores[i]
            );
        }
    }

	int lcore_id;

	RTE_LCORE_FOREACH_WORKER(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
            RTE_LOG(ERR, USER1, "Lcore %u exited abnormally\n", lcore_id);
        }
    }

	rte_eal_cleanup();


	// struct ThreadArgs args[6] = 
	// {
	// 	{0, 0, 1, 0, pkts_0},
	// 	{1, 0, 0, 0, pkts_1},
	// 	{0, 1, 1, 1, pkts_2},
	// 	{1, 1, 0, 1, pkts_3},
	// 	{0, 2, 1, 2, pkts_4},
	// 	{1, 2, 0, 2, pkts_5},
	// };

	// //struct ThreadArgs args_0 = {0, 0, 0, 0, pkts_0};

	// pthread_create(&tid_0, NULL, fpga_to_fpga, &args[0]);
	// pthread_create(&tid_1, NULL, fpga_to_fpga, &args[1]);
    // // pthread_create(&tid_2, NULL, fpga_to_fpga, &args_2);
	// // pthread_create(&tid_3, NULL, fpga_to_fpga, &args_3);
    // // pthread_create(&tid_4, NULL, fpga_to_fpga, &args_4);
	// // pthread_create(&tid_5, NULL, fpga_to_fpga, &args_5);
	// pthread_create(&tid_6, NULL, switch_start, NULL);

	// pthread_join(tid_0, NULL);
	// pthread_join(tid_1, NULL);
	// // pthread_join(tid_2, NULL);
    // // pthread_join(tid_3, NULL);
    // // pthread_join(tid_4, NULL);
    // // pthread_join(tid_5, NULL);
	// pthread_join(tid_6, NULL);

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
