#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/mman.h>
#include "xdma.h"

void init_term(void);
char uart_recv(uint64_t devmem);
bool uart_send(uint64_t devmem, char ch);
int uart_start(int port_id, uint64_t base);
//int uart_start(const char *dev_name, uint64_t base);