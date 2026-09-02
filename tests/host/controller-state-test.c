#include "unity.h"
#include "controller-state/controller-state.h"
#include "fake-timer.h"

void setUp() {
    start_timer_spy = (FunctionSpy_t){0};
    get_timer_data_spy = (FunctionSpy_t){0};
    change_timer_duration_spy = (FunctionSpy_t){0};
    cancel_timer_spy = (FunctionSpy_t){0};
} 

void tearDown() {
    start_timer_spy = (FunctionSpy_t){0};
    get_timer_data_spy = (FunctionSpy_t){0};
    change_timer_duration_spy = (FunctionSpy_t){0};
    cancel_timer_spy = (FunctionSpy_t){0};
} 


void initializes_controller() {
    ControllerState_t test_controller;
    init_controller(&test_controller, &fake_timer_impl);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(initializes_controller);
  return UNITY_END();
}
