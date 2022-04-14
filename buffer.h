#include <termios.h>
#include <sys/select.h>

#define BUFFER_SIZE 100

typedef struct io_buf {
    char *io_buffer;
    char *cur_str;
    int io_buf_ind;
} io_buf_t;

extern void io_buf_init(io_buf_t *io_buf);
extern void flush_io_buf(io_buf_t *io_buf);
extern void io_buf_store(io_buf_t *io_buf, int key);
extern int kbhit();
extern char getch(void);
