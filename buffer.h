#include <termios.h>
#include <sys/select.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>


#define BUFFER_SIZE 100

typedef struct io_buf {
    char *io_buffer;
    char *cur_str;
    int io_buf_ind;
} io_buf_t;

void io_buf_init(io_buf_t *io_buf);
void flush_io_buf(io_buf_t *io_buf);
void io_buf_store(io_buf_t *io_buf, int key);
int kbhit();
char getch(void);
