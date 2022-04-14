#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <sys/select.h>

#define err_exit(x) do{perror((x));return 1;} while(0)

#define KVM_FILE "/dev/kvm"
#define BINARY_FILE "smallkern"
#define BUFFER_SIZE 100
#define MIL 1000000

struct termios orig_termios;

const uint8_t code[] = {
    0xba, 0xf8, 0x03, /* mov $0x3f8, %dx */
    0x00, 0xd8,       /* add %bl, %al */
    0x04, '0',
    0xee,
    0xb0, '\n',
    0xee,
    0xf4,
};


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

void load_binary(char *mem_start) {
    int fd = open(BINARY_FILE, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "can not open binary file\n");
        exit(1);
    }

    int ret = 0;

    while(1) {
        ret = read(fd, mem_start, 4096);
        if (ret <= 0) {
            break;
        }
        mem_start += ret;

    }
}

void flush(char * buffer) {
    memset(buffer, BUFFER_SIZE, 0);
}

int timer_should_fired(int timer_enable)
{
    if (!timer_enable) {
        return 0;
    }

}

long get_cur_time_ms() {
    
    struct timespec s;
    clock_gettime(CLOCK_REALTIME, &s);

    return (s.tv_nsec/MIL) + s.tv_sec * 1000;
    
}

int kbhit() 
{
    fd_set fds;
    FD_ZERO(&fds);
    // 0 is fd of stdin
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, NULL) > 0;
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


int main() {
    int kvm;
    int ret;
    int vmfd;
    int vcpufd;
    uint8_t *mem;
    struct kvm_sregs sregs;
    size_t mmap_size;
    struct kvm_run *run;

    kvm = open(KVM_FILE, O_RDWR | O_CLOEXEC);
    if (kvm == -1)
        err_exit("Open /dev/kvm failed!\n");

    ret = ioctl(kvm, KVM_GET_API_VERSION, NULL);
    if (ret == -1)
        err_exit("KVM_GET_API_VERSION");
    if (ret != 12) {
        fprintf(stderr,"KVM_GET_API_VERSION %d, expected 12", ret);
        return 1;
    }
    
    ret = ioctl(kvm, KVM_CHECK_EXTENSION, KVM_CAP_USER_MEMORY);
    if (ret == -1) {
        err_exit("KVM_CHECK_EXTENSION");
    }
    if (!ret) {
        err_exit("Required extension KVM_CAP_USER_MEM not available");
    }

    // what 0 represents?
    vmfd = ioctl(kvm, KVM_CREATE_VM, (unsigned long)0);
    if (vmfd == -1) {
        err_exit("KVM_CREATE_VM");
    }

    // start, length, prot, flags, fd, offset
    mem = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!mem) {
        err_exit("allocation guest memory");
    }

    // memcpy(mem, code, sizeof(code));
    load_binary(mem);

    // slot 0 identify each region of memory
    // guest_phys_addr "physical" address as seen from the guest
    struct kvm_userspace_memory_region region = {
        .slot = 0,
        .guest_phys_addr = 0x1000,
        .memory_size = 0x1000,
        .userspace_addr = (uint64_t)mem,
    };

    ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &region);
    if (ret == -1) {
        err_exit("KVM_SET_USER_MEMORY_REGION");
    }

    // 0 represents a sequential virtual CPU index
    vcpufd = ioctl(vmfd, KVM_CREATE_VCPU, (unsigned long)0);
    if (vcpufd == -1) {
        err_exit("KVM_CREATE_VCPU");
    }

    //
    ret = ioctl(kvm, KVM_GET_VCPU_MMAP_SIZE, NULL);
    if (ret == -1) {
        err_exit("KVM_GET_VCPU_MMAP_SIZE");
    }

    mmap_size = ret;
    if (mmap_size < sizeof(*run)) {
        err_exit("KVM_GET_VCPU_MMAP_SIZE unexpectedly small");
    }
    
    run = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpufd, 0);
    if (!run) {
        err_exit("mmap vcpu");
    }

    ret = ioctl(vcpufd, KVM_GET_SREGS, &sregs);
    if (ret == -1) {
        err_exit("KVM_GET_SREGS");
    }
    sregs.cs.base = 0;
    sregs.cs.selector = 0;

    ret = ioctl(vcpufd, KVM_SET_SREGS, &sregs);
    if (ret == -1) {
        err_exit("KVM_SET_SREGS");
    }

    // flags = 0x2 means x86 architecture
    struct kvm_regs regs = {
        .rip = 0x1000,
        .rax = 0,
        .rbx = 0,
        .rflags = 0x2,
    };
    ret = ioctl(vcpufd, KVM_SET_REGS, &regs);
    if (ret == -1) {
        err_exit("KVM_SET_REGS");
    }

    char * io_buffer = (char*) malloc(BUFFER_SIZE*sizeof(char));
    char * cur_str = (char*) malloc(BUFFER_SIZE*sizeof(char));
    flush(io_buffer);


    int i = 0;
    int key_in = 0;

    long interval_ms;
    long last_fired_ms;
    long cur_time_ms;
    int timer_enable = 0;

    while (1) {
        ret = ioctl(vcpufd, KVM_RUN, NULL);
        if (ret == -1) {
            err_exit("KVM_RUN");
        }

        if (timer_enable) {
            cur_time_ms = get_cur_time_ms();
            if (cur_time_ms - last_fired_ms > interval_ms) {
                printf("%s\n", cur_str);
                last_fired_ms = cur_time_ms;
            }
        }

        switch (run->exit_reason) {
            // handle exit
            case KVM_EXIT_HLT:
                puts("KVM_EXIT_HLT");
                return 0;
            case KVM_EXIT_IO:
                printf("kvm_exit_io, direction:%d, port:%x", run->io.direction, run->io.port);
                if (run->io.direction == KVM_EXIT_IO_IN) {
                    if (run->io.port == 0x44) {
                        if(kbhit()) {
                            *(((char *)run) + run->io.data_offset) = getch();
                            key_in = 1;
                        } else {
                            key_in = 0;
                        }
                    } else if (run->io.port == 0x45) {
                        *(((char *)run) + run->io.data_offset) = key_in;
                    } else if (run->io.port == 0x47) {
                         *(((char *)run) + run->io.data_offset) = timer_enable;
                    }
                } else if (run->io.direction == KVM_EXIT_IO_OUT) {
                    // to-do ack_key? ack_key?
                    if (run->io.port == 0x42) {
                        char key = *(((char *)run) + run->io.data_offset);
                        if (key == '\n') {
                            printf("%s\n", io_buffer);
                            flush(io_buffer);
                            flush(cur_str);
                            memcpy(cur_str, io_buffer, BUFFER_SIZE);
                            i = 0;
                            timer_enable = 1;
                            last_fired_ms = get_cur_time_ms();
                        } else {
                            *(io_buffer + i) = key;
                            i ++;
                        }
                    } else if (run->io.port == 0x46) {
                        interval_ms = ((int)(*(((char *)run) + run->io.data_offset))) * 1000;
                        printf("%l", interval_ms);
                    }
                }
                break;
            case KVM_EXIT_FAIL_ENTRY:
                fprintf(stderr, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx", 
                        (unsigned long long)run->fail_entry.hardware_entry_failure_reason);
                return 1;
            case KVM_EXIT_INTERNAL_ERROR:
                fprintf(stderr, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x", run->internal.suberror);
                return 1;
            default:
                fprintf(stderr, "exit_reason=0x%x\n", run->exit_reason);
                return 0;
        }
    }
}
