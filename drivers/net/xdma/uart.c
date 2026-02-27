#include <stdint.h>
#include <uart.h>


static volatile sig_atomic_t special_char = 0;
static volatile sig_atomic_t running = 1;



void init_term() {
    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &t);
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
}


char uart_recv(uint64_t devmem) {
    size_t offset = 0x8;
    if (!((((volatile uint32_t *)devmem)[offset >> 2])& (1 << 0)))
        return 0;
    offset = 0x0;
    return((volatile uint32_t *)devmem)[offset >> 2];
}

bool uart_send(uint64_t devmem, char ch) {
    int max_retry = 1000;
    for (int i=0; i<max_retry; i++) {
        if (!((((volatile uint32_t *)devmem)[0x8 >> 2]) & (1 << 3))) {
            ((volatile uint32_t *)devmem)[0x4 >> 2] = ch;
            return true;
        }
    }
    return false;
}

int uart_start(int port_id, uint64_t base) {


    struct rte_eth_dev *dev = &rte_eth_devices[port_id];
    struct xdma_dev *xdma_dev = dev->data->dev_private;
    uint64_t devmem = (uint64_t)xdma_dev->bar[xdma_dev->user_bar_idx] + base;

    init_term();
    fprintf(stderr, "Escape key is ctrl+\\ (SIGQUIT)\n");

    while (running) {
        while (running) {
            char ch = uart_recv(devmem);
            if (!ch)
                break;

            write(1, &ch, 1);
        }

        char ch = 0;
        
        if (special_char) {
            ch = special_char;
            special_char = 0;
        }
        else {
            read(0, &ch, 1);
        }

        if (ch) {
            uart_send(devmem, ch);
        }

        usleep(2);
    }
    return 0;
}