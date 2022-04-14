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
#include <assert.h>
#include "timer.h"
#include "buffer.h"

#define err_exit(x) do{perror((x));return -1;} while(0)

#define KVM_FILE "/dev/kvm"
#define BINARY_FILE "smallkern"
#define RAM_SIZE 0x1000
#define VCPU_ID (unsigned long)0

#define MIL 1000000

typedef struct kvm {
    int kvm_fd;
    int kvm_version;
    int vm_fd;
    int ram_size;
    uint8_t *ram_start;
    
    struct kvm_userspace_memory_region region;
    struct vcpu *vcpus;
    int vcpu_number;

    kvm_timer_t timer;
} kvm_t;

typedef struct vcpu {
    int vcpu_id;
    int vcpu_fd;
    struct kvm_run *kvm_run;
    int kvm_run_mmap_size;
    struct kvm_regs regs;
    struct kvm_sregs sregs;
} vcpu_t;

int kvm_init(kvm_t *kvm) ;
int kvm_create_vm(kvm_t *kvm);
int kvm_init_vcpu(kvm_t *kvm, vcpu_t *vcpu);
void load_binary(kvm_t *kvm);