#include <time.h>

typedef struct timer {
    long interval_ms;
    long last_fired_ms;
    int timer_enable;
} timer_t;
    

int timer_should_fire(timer_t *timer);
void timer_update(timer_t *timer);