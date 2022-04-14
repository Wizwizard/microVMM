#include "buffer.h"


void flush(char * buffer) {
    memset(buffer, BUFFER_SIZE, 0);
}

void io_buf_store(io_buf_t *io_buf, int key) {
    *(io_buf->io_buffer + io_buf->io_buf_ind) = key;
    io_buf->io_buf_ind ++;
}

void flush_io_buf(io_buf_t *io_buf){
    printf("%s\n", io_buf->io_buffer);
    flush(io_buf->io_buffer);
    flush(io_buf->cur_str);
    memcpy(io_buf->cur_str, io_buf->io_buffer, BUFFER_SIZE);
    io_buf->io_buf_ind = 0;
}

void io_buf_init(io_buf_t *io_buf) {
    io_buf->io_buffer = (char*) malloc(BUFFER_SIZE*sizeof(char));
    io_buf->cur_str = (char*) malloc(BUFFER_SIZE*sizeof(char));
    flush(io_buf->io_buffer);
    flush(io_buf->cur_str):
    io_buf->io_buf_ind = 0;
}

int kbhit() 
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    // 0 is fd of stdin
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

char getch(void)
{
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if(tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if(read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if(tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
}