#include "xdma_platform.h"
#include "rte_common.h"
#include <stddef.h>

uint32_t xdma_reg_read(void *dev_hndl, uint32_t reg_offst)
{
	struct xdma_dev *xdma_dev;
	uint64_t bar_addr;
	uint32_t val;

	xdma_dev = (struct xdma_dev *)dev_hndl;
	bar_addr = (uint64_t)xdma_dev->bar[xdma_dev->config_bar_idx];
	val = *((volatile uint32_t *)(bar_addr + reg_offst));

	return val;
}

void xdma_reg_write(void *dev_hndl, uint32_t reg_offst, uint32_t val)
{
	struct xdma_dev *xdma_dev;
	uint64_t bar_addr;

	xdma_dev = (struct xdma_dev *)dev_hndl;
	bar_addr = (uint64_t)xdma_dev->bar[xdma_dev->config_bar_idx];
	*((volatile uint32_t *)(bar_addr + reg_offst)) = val;
}

void xdma_user_write(void *dev_hndl, uint32_t reg_offst, uint32_t val)
{
	struct xdma_dev *xdma_dev;
	uint64_t bar_addr;

	xdma_dev = (struct xdma_dev *)dev_hndl;
	bar_addr = (uint64_t)xdma_dev->bar[xdma_dev->user_bar_idx];
	*((volatile uint32_t *)(bar_addr + reg_offst)) = val;	
}

uint32_t xdma_user_read(void *dev_hndl, uint32_t reg_offst)
{
	struct xdma_dev *xdma_dev;
	uint64_t bar_addr;
	uint32_t val;

	xdma_dev = (struct xdma_dev *)dev_hndl;
	bar_addr = (uint64_t)xdma_dev->bar[xdma_dev->user_bar_idx];
	val = *((volatile uint32_t *)(bar_addr + reg_offst));

	return val;
}


void xdma_bar_load(void *dev_hndl, uint32_t offset, uint32_t size, int bar_idx, const char* filepath)
{
	struct xdma_dev *xdma_dev;
	uint64_t addr;

	xdma_dev = (struct xdma_dev *)dev_hndl;
	addr = (uint64_t)xdma_dev->bar[bar_idx] + offset;
	int fd = open(filepath, O_RDONLY);
	if (fd < 0) {
		PMD_DRV_LOG(INFO, "Failed to open file\n");
		rte_exit(EXIT_FAILURE, "Failed to open file during load file\n");
	}
	size_t total_written = 0;
	size_t file_size = lseek(fd, 0, SEEK_END);
	file_size = RTE_MIN(file_size, size);
	lseek(fd, 0, SEEK_SET);
	while (total_written < file_size) {
        size_t chunk = RTE_MIN(CHUNK_SIZE, file_size - total_written);

        // 将文件块映射到内存（零拷贝）
        void *file_chunk = mmap(NULL, chunk, PROT_READ, MAP_PRIVATE, fd, total_written);
        if (file_chunk == MAP_FAILED) {
            perror("File mmap failed");
            break;
        }

        // 直接写入 BAR 空间（无需复制到缓冲区）
        rte_memcpy((char *)addr + total_written, file_chunk, chunk);

        // 清理文件块映射
        munmap(file_chunk, chunk);

        total_written += chunk;
		if(total_written > 1 * 1024 * 1024)
        	PMD_DRV_LOG(INFO, "Written %zu MB\n", total_written / (1024 * 1024));
		else if(total_written > 1 * 1024)
			PMD_DRV_LOG(INFO, "Written %zu KB\n", total_written / 1024);
		else 
			PMD_DRV_LOG(INFO, "Written %zu B\n", total_written);
    }

}

