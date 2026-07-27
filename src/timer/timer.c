#include <stdint.h>
#include "timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/timers.h"

TimerMetadata_t *get_timer_metadata(AppTimerHandle_t handle) {
    return (TimerMetadata_t *)pvTimerGetTimerID(
        handle 
    );
}

static void internal_cb(TimerHandle_t handle) {
    struct TimerMetadata *data = (struct TimerMetadata *)pvTimerGetTimerID(
        handle 
    );

    data->cb((TimerMetadata_t *)data); 
}

AppTimerHandle_t create_and_start_timer(
    uint32_t duration, 
    SchedulerCallback_t cb, 
    TimerMetadata_t *info
) {
    info->cb = cb;

    TimerHandle_t curr_timer = xTimerCreate(
        "Button Timer", 
        pdMS_TO_TICKS(duration), 
        pdFALSE, 
        (void *)info, 
        internal_cb 
    );

    if (!curr_timer || xTimerStart(curr_timer, 0) != pdPASS)
        return NULL;

    return curr_timer;
}

void change_timer_duration(AppTimerHandle_t handle, uint32_t duration_ms) {
    xTimerChangePeriod(
        handle, 
        pdMS_TO_TICKS(duration_ms), 
        0
    );
}

void cancel_timer(AppTimerHandle_t handle) {
    xTimerStop(handle, 0);
    xTimerDelete(handle, 0);
}


const TimerInterface_t DEFAULT_IMPL = {
    .create_and_start_timer = create_and_start_timer,
    .change_timer_duration = change_timer_duration,
    .cancel_timer = cancel_timer,
    .get_timer_metadata = get_timer_metadata
};
