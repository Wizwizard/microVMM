#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

int main(int argc, char *argv[])
{
    char ch[1];
    char * buffer = (char*)malloc(100*sizeof(char));
    
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    memset(buffer, 100, 0);
    int i = 0;
    int numRead = 0;
    while(1) {
        numRead = read(0, ch, 1);

        if(numRead > 1) {
            printf("ch:%c", ch[0]);
            
            if(ch[0] == '\n') {
                printf("%s", buffer);
                memset(buffer, 100, 0);
                i = 0;
            } else {
                *buffer = ch[0];
                i += 1;
            }

        }
    }
}
