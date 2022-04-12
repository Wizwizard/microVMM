#include <stdio.h>
#include <memory.h>
#include <sys/mman.h>
#include <pthread.h>
#include <linux/kvm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>

#define KVM_DEVICE "/dev/kvm"
#define RAM_SIZE 512000000
#define CODE_START 0x1000
#define BINARY_FILE "smallkern"

struct kvm {
    int dev_fd;
    int vm_fd;
    __u64 ram_size;
    __u64 ram_start;
    int kvm_version;
    struct kvm_userspace_memory_region_mem;

    struct vcpu *vcpus;
    int vcpu_number;
};

struct vcpu {
    int vcpu_id;
    int vcpu_fd;
    pthread_t vcpu_thread;
    struct kvm_run *kvm_run;
    int kvm_run_mmap_size;
    struct kvm_regs regs;
    struct kvm_sregs sregs;
    void *(*vcpu_thread_func)(void *);
};

void kvm_reset_vcpu(struct vcpu *vcpu) {
    if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &(vcpu->sregs)) < 0) {
        perror("can not get sregs\n");
        exit(1);
    }

    vcpu->sregs.cs.selector = CODE_START;
    vcpu->sregs.cs.base = CODE_START * 16;
    vcpu->sregs.ss.selector = CODE_START;
    vcpu->sregs.ss.base = CODE_START * 16;
    vcpu->sregs.ds.selector = CODE_START;
    vcpu->sregs.ds.base = CODE_START * 16;
    vcpu->sregs.es.selector = CODE_START;
    vcpu->sregs.es.base = CODE_START * 16;
    vcpu->sregs.fs.selector = CODE_START;
    vcpu->sregs.fs.base = CODE_START * 16;
    vcpu->sregs.gs.selector = CODE_START;

    if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &vcpu->sregs) < 0) {
        perror("can not set sregs");
        exit(1);
    }

    vcpu->regs.rflags = 0x0000000000000002ULL;
    vcpu->regs.rip = 0;
    vcpu->regs.rsp = 0xffffffff;
    vcpu->regs.rbp = 0;

    if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &(vcpu->regs)) < 0) {
        perror("KVM SET REGS\n")
        exit(1);
    }
}

void *kvm_cpu_thread(void *data) {
    struct kvm *kvm = (struct kvm *)data;
    int ret = 0;
    kvm_reset_vcpu(kvm->vcpus);

    while (1) {
        printf("KVM start run\n");
        ret = ioctl(kvm->vcpu->vcpu_fd, KVM_RUN, 0);

        if (ret < 0) {
            fprintf(stderr, "KVM_RUN failed\n");
            exit(1);
        }

        switch (kvm->vcpus->kvm_run->exit_reason) {
            case KVM_EXIT_UNKNOWN:
                printf("KVM_EXIT_UNKNOWN\n");
                break;
            case KVM_EXIT_DEBUG:
                printf("KVM_EXIT_DEBUG\n");
                break;
            case KVM_EXIT_IO:
                printf("KVM_EXIT_IO\n");
                printf("out port: %d, data: %d\n",
                    kvm->vcpus->kvm_run->io.port,
                    *(int *)((char *)(kvm->vcpu->kvm_run) + kvm->vcpus->kvm_run->io.data_offset)
                    );
                sleep(1);
                break;
            case KVM_EXIT_MMIO:
                printf("KVM_EXIT_MMIO\n");
                break;
            case KVM_EXIT_INTR:
                printf("KVM_EXIT_INTR\n");
                break;
            case KVM_EXIT_SHUTDOWN:
                printf("KVM_EXIT_SHUTDOWN\n");
                goto exit_kvm;
                break;
            default:
                printf("KVM PANIC\n");
                goto exit_kvm;
        }
    }
    
    exit_kvm:
        return 0;
}


void load_binary(struct kvm *kvm) {
    int fd = open(BINARY_FILE, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "can not open binary file\n");
        exit(1);
    }

    int ret = 0;
    char *p = (char *)kvm->ram_start;

    while(1) {
        ret = read(fd, p, 4096);
        if (ret <= 0) {
            break;
        }
        printf("read size: %d", ret);
        p += ret;

    }
}

struct kvm *kvm_init(void) {
    struct kvm *kvm = malloc(sizeof(struct kvm));


