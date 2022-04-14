#include <time.h>

#define MIL 1000000

typedef struct kvm_timer {
    long interval_ms;
    long last_fired_ms;
    int timer_enable;
} kvm_timer_t;
    

int timer_should_fire(kvm_timer_t *timer);
void timer_update(kvm_timer_t *timer);
