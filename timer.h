#include <time.h>

typedef struct kvm_timer {
    long interval_ms;
    long last_fired_ms;
    int timer_enable;
} kvm_timer_t;
    

extern int timer_should_fire(kvm_timer_t *timer);
extern void timer_update(kvm_timer_t *timer);
