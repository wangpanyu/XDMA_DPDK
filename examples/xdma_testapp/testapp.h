#include <stdint.h>
#include <stdio.h>
#include <string.h> /**> memset */
#include <signal.h>
#include <termios.h>
#include <rte_eal.h> /**> rte_eal_init */
#include <rte_debug.h> /**> for rte_panic */
#include <rte_ethdev.h> /**> rte_eth_rx_burst */
#include <rte_errno.h> /**> rte_errno global var */
#include <rte_memzone.h> /**> rte_memzone_dump */
#include <rte_memcpy.h>
#include <rte_malloc.h>
#include <rte_cycles.h>
#include <rte_log.h>
#include <rte_string_fns.h>
#include <rte_spinlock.h>
#include <ethdev_driver.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_socket.h>
#include <cmdline.h>
#include <time.h>  /** For SLEEP **/
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <rte_mbuf.h>
#include <stdlib.h>


#include "../../drivers/net/xdma/rte_pmd_xdma.h"
#include "../../drivers/net/xdma/uart.h"

#define XDMA_MAX_PORTS	256

#define MBUF_POOL_NAME_PORT   "mbuf_pool_%d"

#define NUM_TX_PKTS 32
#define NUM_RX_PKTS 32

#define SIZE_OF_PKTS_BUFFER 32

#define MP_CACHE_SZ     512

#define MAX_NUM_QUEUES  8


#define RX_TX_MAX_RETRY			1

#define STR_TOKEN_SIZE 128

/* size of a parsed multi string */
#define STR_MULTI_TOKEN_SIZE 4096


typedef char cmdline_fixed_string_t[STR_TOKEN_SIZE];

static int fpga_to_fpga(void* args);

static int switch_start(void* args);

double compute_latency(uint32_t addr, uint32_t count, uint32_t freq);

struct port_info {
	int config_bar_idx;
	int user_bar_idx;
	int bypass_bar_idx;
	unsigned int queue_base;
	unsigned int num_queues;
	unsigned int nb_descs;
	unsigned int st_queues;
	unsigned int buff_size;
	rte_spinlock_t port_update_lock;
	char mem_pool[RTE_MEMPOOL_NAMESIZE];
};

struct ThreadArgs {
	int src_port_id;
	int src_queue_id;
	int dst_port_id;
	int dst_queue_id;
	struct rte_mbuf **pkts;
};



struct thread_args {
    int src_port_id;
    int src_queue_id;
    int dst_port_id;
    int dst_queue_id;
    struct rte_mbuf **pkts;
    // 新增：专门用于传递内存池指针
    int sockfd; 
    struct sockaddr_in remote_addr;
};