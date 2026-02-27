#include "xdma.h"
#include "xdma_log.h"
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define CHUNK_SIZE 64 * 1024 * 1024

uint32_t xdma_reg_read(void *dev_hndl, uint32_t reg_offst);
void xdma_reg_write(void *dev_hndl, uint32_t reg_offst, uint32_t val);
uint32_t xdma_user_read(void *dev_hndl, uint32_t reg_offst);
void xdma_user_write(void *dev_hndl, uint32_t reg_offst, uint32_t val);
void xdma_bar_load(void *dev_hndl, uint32_t offset, uint32_t size, int bar_idx, const char* filepath);