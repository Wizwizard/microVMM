#include "timer.h"

int timer_should_fire(timer_t *timer) {

    if (timer->timer_enable) {
        long cur_time_ms = get_cur_time_ms();
        if (cur_time_ms - timer->last_fired_ms > timer->interval_ms) {
            timer->last_fired_ms = cur_time_ms;
            return 1; 
        }
    } 

    return 0;
}

void timer_update(timer_t *timer) {
    timer->timer_enable = 1;
    timer->last_fired_ms = get_cur_time_ms();
}
                            
                            