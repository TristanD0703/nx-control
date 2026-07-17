#include <stdint.h>
#include "timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/timers.h"

TimerHandle_t create_and_start_timer(uint32_t duration, SchedulerCallback_t cb) {
    TimerHandle_t curr_timer = xTimerCreate(
        "Button Timer", 
        pdMS_TO_TICKS(durationTicks), 
        pdFALSE, 
        (void *)info, 
        scheduler_callback
    );

    if (xTimerStart(curr_timer, 0) != pdPASS)
        return NULL;

    return curr_timer;
}

int change_timer_duration(TimerHandle_t handle, uint32_t duration_ms) {
    xTimerChangePeriod(
        handle, 
        pdMS_TO_TICKS(duration_ms), 
        0
    );
}

int cancel_timer(TimerHandle_t handle) {
}
