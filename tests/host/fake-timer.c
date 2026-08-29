#include "timer.h"
#include <stdint.h>

typedef struct {
    uint32_t duration;
    SchedulerCallback_t cb;
    TimerMetadata_t *info;
} StartTimerArgs_t;

typedef struct {
    AppTimerHandle_t handle;
    uint32_t duration_ms;
} ChangeTimerDurationArgs_t;

typedef struct {
    void *call_args;
    void *returns;
    uint32_t call_count;
} FunctionSpy_t;

FunctionSpy_t start_timer_spy = {0};
FunctionSpy_t get_timer_data_spy = {0};
FunctionSpy_t change_timer_duration_spy = {0};
FunctionSpy_t cancel_timer_spy = {0};

TimerMetadata_t *get_timer_metadata_stub(AppTimerHandle_t handle) {
    const uint32_t call_count = get_timer_data_spy.call_count;
    AppTimerHandle_t *call_args = (AppTimerHandle_t *) get_timer_data_spy.call_args;
    TimerMetadata_t **curr_ret = (TimerMetadata_t **) get_timer_data_spy.returns;

    call_args[call_count] = handle;

    get_timer_data_spy.call_count += 1; 
    return curr_ret[call_count];
}

AppTimerHandle_t create_and_start_timer_stub(
    uint32_t duration,
    SchedulerCallback_t cb,
    TimerMetadata_t *info
) {
    const uint32_t call_count = start_timer_spy.call_count;
    StartTimerArgs_t *call_args = (StartTimerArgs_t *) start_timer_spy.call_args;
    AppTimerHandle_t *curr_ret = (AppTimerHandle_t *) start_timer_spy.returns;

    StartTimerArgs_t args = {
        .duration = duration,
        .info = info,
        .cb = cb
    };

    call_args[call_count] = args;

    start_timer_spy.call_count += 1; 
    return curr_ret[call_count];
}


void change_timer_duration_stub(AppTimerHandle_t handle, uint32_t duration_ms) {
    const uint32_t call_count = change_timer_duration_spy.call_count;
    ChangeTimerDurationArgs_t *call_args = (ChangeTimerDurationArgs_t *) change_timer_duration_spy.call_args;

    ChangeTimerDurationArgs_t args = {
        .handle = handle,
        .duration_ms = duration_ms
    };

    call_args[call_count] = args;
    change_timer_duration_spy.call_count += 1; 
}


void cancel_timer_stub(AppTimerHandle_t handle) {
    const uint32_t call_count = cancel_timer_spy.call_count;
    AppTimerHandle_t *call_args = (AppTimerHandle_t *) cancel_timer_spy.call_args;


    call_args[call_count] = handle;
    cancel_timer_spy.call_count += 1; 
}

const TimerInterface_t fake_timer_impl = {
    .cancel_timer = cancel_timer_stub,
    .change_timer_duration = change_timer_duration_stub,
    .create_and_start_timer = create_and_start_timer_stub,
    .get_timer_metadata = get_timer_metadata_stub 
};
