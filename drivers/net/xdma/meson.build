cflags += ['-DRTE_LIBRTE_XDMA_PMD']
cflags += ['-DDMA_BRAM_SIZE=524288']
cflags += ['-DTHROUGHPUT_MEASUREMENT']

# Use XDMA_DPDK_23_11 compiler flag for DPDK v23.11
# Use XDMA_DPDK_22_11 compiler flag for DPDK v22.11
# Use XDMA_DPDK_21_11 compiler flag for DPDK v21.11
# Use XDMA_DPDK_20_11 compiler flag for DPDK v20.11
cflags += ['-DXDMA_DPDK_23_11']

includes += include_directories('.')

headers += files('rte_pmd_xdma.h')
headers += files('uart.h')

deps += ['mempool_ring']

sources = files(
    'xdma_access_common.c',
    'xdma_devops.c',
    'xdma_ethdev.c',
    'xdma_platform.c',
    'rte_pmd_xdma.c',
    'xdma_rxtx.c',
    'xdma_common.c',
    'uart.c'
)

