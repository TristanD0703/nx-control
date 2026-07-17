typedef struct {
    ControllerState_t *controller;
    JoystickButtonState_t operation;
    uint8_t timer_index;
} TimerInfo_t;

typedef SchedulerCallback_t void (*scheduler_callback)(TimerInfo_t *info);

typedef struct {
    TimerHandle_t (*create_and_start_timer)(int duration, SchedulerCallback_t cb);
    int (*change_timer_duration)(TimerHandle_t handle, int duration);
    int (*cancel_timer)(TimerHandle_t handle); 
} TimerInterface_t;
