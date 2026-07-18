#pragma once

#include "../controller-state/controller-state.h"
#include "freertos/idf_additions.h"
#include "../controller-state/controller-state.h"

typedef struct TimerInfo TimerInfo_t;

typedef void (*SchedulerCallback_t)(TimerInfo_t *handle);

struct TimerInfo {
    ControllerState_t *controller;
    JoystickButtonState_t operation;
    uint8_t timer_index;
    SchedulerCallback_t cb;
};

struct TimerInterface {
    // Returns null if unsuccessful
    TimerHandle_t (*create_and_start_timer)(
        uint32_t duration, 
        SchedulerCallback_t cb, 
        TimerInfo_t *info
    );
    void (*change_timer_duration)(TimerHandle_t handle, uint32_t duration_ms);
    void (*cancel_timer)(TimerHandle_t handle); 
    TimerInfo_t* (*get_timer_metadata)(TimerHandle_t handle);
};

typedef struct TimerInterface TimerInterface_t;

extern const TimerInterface_t DEFAULT_IMPL;

