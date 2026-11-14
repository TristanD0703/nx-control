#pragma once
#include <stdint.h>

typedef void *AppTimerHandle_t;
typedef struct TimerMetadata TimerMetadata_t;

typedef void (*SchedulerCallback_t)(TimerMetadata_t *handle);

struct TimerMetadata {
    SchedulerCallback_t cb;  
};

TimerMetadata_t *get_timer_metadata(AppTimerHandle_t handle);
// Returns null if unsuccessful
AppTimerHandle_t create_and_start_timer(
    uint32_t duration, 
    SchedulerCallback_t cb, 
    TimerMetadata_t *info
);
void change_timer_duration(AppTimerHandle_t handle, uint32_t duration_ms);
void cancel_timer(AppTimerHandle_t handle);
