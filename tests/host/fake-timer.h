#include <stdint.h>
#include <timer/timer.h>

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

extern FunctionSpy_t start_timer_spy;
extern FunctionSpy_t get_timer_data_spy;
extern FunctionSpy_t change_timer_duration_spy;
extern FunctionSpy_t cancel_timer_spy;
