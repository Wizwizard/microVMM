#include "kvmsample.h"

int kvm_reset_vcpu(vcpu_t *vcpu) {
    int ret;

    ret = ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &vcpu->sregs);
    if (ret == -1) {
        err_exit("KVM_GET_SREGS");
    }
    vcpu->sregs.cs.base = 0;
    vcpu->sregs.cs.selector = 0;

    ret = ioctl(vcpufd, KVM_SET_SREGS, &vcpu->sregs);
    if (ret == -1) {
        err_exit("KVM_SET_SREGS");
    }

    // flags = 0x2 means x86 architecture
    vcpu->regs.rip = 0x1000;
    vcpu->regs.rax = 0;
    vcpu->regs.rbx = 0;
    vcpu->regs.rflags = 0x2;

    ret = ioctl(vcpufd, KVM_SET_REGS, &vcpu->regs);
    if (ret == -1) {
        err_exit("KVM_SET_REGS");
    }

    return 0;
}

int kvm_init_vcpu(kvm_t *kvm, vcpu_t *vcpu)
{
    int ret;

    vcpu->vcpu_id = VCPU_ID;
    vcpu->vcpu_fd = ioctl(kvm->vm_fd, KVM_CREATE_VCPU, vcpu->vcpu_id);
    if (vcpu->vcpu_fd == -1) {
        err_exit("KVM_CREATE_VCPU");
    }

    ret = ioctl(kvm->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, NULL);
    if (ret == -1) {
        err_exit("KVM_GET_VCPU_MMAP_SIZE");
    }

    vcpu->kvm_run_mmap_size = ret;
    if (vcpu->kvm_run_mmap_size < sizeof(vcpu->kvm_run)) {
        err_exit("KVM_GET_VCPU_MMAP_SIZE unexpectedly small");
    }
    
    vcpu->kvm_run = mmap(NULL, vcpu->kvm_run_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu->vcpu_fd, 0);
    if (!vcpu->kvm_run) {
        err_exit("mmap vcpu");
    }

    return 0;

}
int kvm_create_vm(kvm_t *kvm) {
    int ret;

    kvm->vm_fd = ioctl(kvm->kvm_fd, KVM_CREATE_VM, (unsigned long)0);
    if (kvm->vm_fd == -1) {
        err_exit("KVM_CREATE_VM");
    }

    kvm->ram_size = RAM_SIZE;
    kvm->ram_start = mmap(NULL, kvm->ram_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!kvm->ram_start) {
        err_exit("allocation guest memory");
    }

    kvm->region.slot = 0;
    kvm->region.guest_phys_addr = 0x1000;
    kvm->region.memory_size = kvm->ram_size;
    kvm->region.userspace_addr = kvm->ram_start;

    ret = ioctl(kvm->vm_fd, KVM_SET_USER_MEMORY_REGION, &(kvm->region));
    if (ret == -1) {
        err_exit("KVM_SET_USER_MEMORY_REGION");
    }

    return ret;
}

int kvm_init(kvm_t *kvm) {
    int ret;

    kvm->kvm_fd = open(KVM_FILE, O_RDWR | O_CLOEXEC);
    if (kvm->kvm_fd < 0)
        err_exit("Open /dev/kvm failed!\n");

    kvm->kvm_version = ioctl(kvm->kvm_fd, KVM_GET_API_VERSION, NULL);
    if (kvm->kvm_version == -1)
        err_exit("KVM_GET_API_VERSION");
    if (kvm->kvm_version != 12) {
        fprintf(stderr,"KVM_GET_API_VERSION %d, expected 12", ret);
        return 1;
    }
    
    ret = ioctl(kvm->kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_USER_MEMORY);
    if (ret == -1) {
        err_exit("KVM_CHECK_EXTENSION");
    }
    if (!ret) {
        err_exit("Required extension KVM_CAP_USER_MEM not available");
    }
}

void load_binary(kvm_t *kvm) {
    int fd = open(BINARY_FILE, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "can not open binary file\n");
        exit(1);
    }

    int ret = 0;

    while(1) {
        ret = read(fd, kvm->ram_start, 4096);
        if (ret <= 0) {
            break;
        }
        kvm->ram_start += ret;
    }
}


int kvm_run_vm(kvm_t *kvm, vcpu_t *vcpu) {

    int ret;
    struct kvm_run *run = vcpu->kvm_run;
    kvm_timer_t *timer;
    io_buf_t *io_buf;

    timer = (kvm_timer_t*)malloc(sizeof(kvm_timer_t));
    io_buf = (io_buf_t*)malloc(sizeof(io_buf_t));

    timer->timer_enable = 0;
    io_buf_init(io_buf);

    int key_in = 0;

    while (1) {
        ret = ioctl(vcpu->vcpu_fd, KVM_RUN, NULL);
        if (ret == -1) {
            err_exit("KVM_RUN");
        }

        if (timer_should_fire(timer)) {
            printf("%s\n", io_buf->cur_str);
        }

        switch (run->exit_reason) {
            // handle exit
            case KVM_EXIT_HLT:
                puts("KVM_EXIT_HLT");
                return 0;
            case KVM_EXIT_IO:
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
                         *(((char *)run) + run->io.data_offset) = timer->timer_enable;
                    }
                } else if (run->io.direction == KVM_EXIT_IO_OUT) {
                    // to-do ack_key? ack_key?
                    if (run->io.port == 0x42) {
                        char key = *(((char *)run) + run->io.data_offset);
                        if (key == '\n') {

                            flush_io_buf(io_buf);
                            timer_update(timer);

                        } else {
                            io_buf_store(io_buf, key);
                        }
                    } else if (run->io.port == 0x46) {
                        timer->interval_ms = ((int)(*(((char *)run) + run->io.data_offset))) * 1000;
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

    return 0;
}

void kvm_clean_vm(kvm_t *kvm) {
    close(kvm->vm_fd);
    munmap((void *)kvm->ram_start, kvm->ram_size);
}

void kvm_clean(kvm_t *kvm) {
    assert (kvm != NULL);
    close(kvm->kvm_fd);
    free(kvm);
}

void kvm_clean_vcpu(vcpu_t *vcpu) {
    munmap(vcpu->kvm_run, vcpu->kvm_run_mmap_size);
    close(vcpu->vcpu_fd);
}


int main() {
    int ret = 0;
    kvm_t *kvm;
    vcpu_t *vcpu; 
    
    kvm = (kvm_t*)malloc(sizeof(kvm_t));
    ret = kvm_init(kvm);
    if(ret < 0) {
        err_exit("kvm_init failure!\n");
    }

    if(kvm_create_vm(kvm) < 0) {
        err_exit("create vm failure\n");
    }

    load_binary(kvm);

    vcpu = (vcpu_t*)malloc(sizeof(vcpu_t));
    ret = kvm_init_vcpu(kvm, vcpu);
    if(ret < 0) {
        err_exit("kvm_init_vcpu failure!\n");
    }

    if(kvm_reset_vcpu(vcpu) < 0) {
        err_exit("kvm_reset_vcpu failure!\n");
    }

    kvm_run_vm(kvm, vcpu);

    kvm_clean_vm(kvm);
    kvm_clean_vcpu(vcpu);
    kvm_clean(kvm);
}