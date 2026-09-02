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
extern const TimerInterface_t fake_timer_impl;


TimerMetadata_t *get_timer_metadata_stub(AppTimerHandle_t handle);
AppTimerHandle_t create_and_start_timer_stub(
    uint32_t duration,
    SchedulerCallback_t cb,
    TimerMetadata_t *info
);
void change_timer_duration_stub(AppTimerHandle_t handle, uint32_t duration_ms);
void cancel_timer_stub(AppTimerHandle_t handle);
