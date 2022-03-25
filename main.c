#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kvm.h>

#define KVM_FILE "/dev/kvm"

int main() {
    int dev;
    int ret;

    dev = open(KVM_FILE, O_RDWR|O_NDELAY);
    ret = ioctl(dev, KVM_GET_API_VERSION, 0);
    printf("---KVM API version is --%d--\n", ret);

    ret = ioctl(dev, KVM_CHECK_EXTENSION, KVM_CAP_MAX_VCPUS);
    printf("---KVM support MAX_VCPUS per guest(VM) is %d--\n", ret);

    ret = ioctl(dev, KVM_CHECK_EXTENSION, KVM_CAP_IOMMU);
    if(ret != 0) {
        printf("---KVM supports IOMMU\n");
    } else {
        printf("---KVM doesn't support IOMMU\n");
    }

    return 0;
}