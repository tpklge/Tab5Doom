#include "tic_clock.h"
#include "i_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <assert.h>

static ChexTicClock clock_state;
static bool initialized;
static uint64_t now_us() { return static_cast<uint64_t>(esp_timer_get_time()); }
static void ensure_initialized()
{
    if (!initialized) { clock_state.reset(now_us()); initialized = true; }
}
static int get_time(bool save)
{
    ensure_initialized();
    return clock_state.ticks(now_us(), save);
}
static int wait_for_tic(int previous)
{
    ensure_initialized();
    assert(!clock_state.is_frozen());
    int current;
    while ((current = get_time(false)) <= previous) vTaskDelay(1);
    return current;
}
static void freeze_time(bool frozen)
{
    ensure_initialized();
    clock_state.freeze(now_us(), frozen);
}
int (*I_GetTime)(bool) = get_time;
int (*I_WaitForTic)(int) = wait_for_tic;
void (*I_FreezeTime)(bool) = freeze_time;
unsigned int I_MSTime()
{
    ensure_initialized();
    return static_cast<unsigned int>(clock_state.realtime(now_us()) / 1000);
}
unsigned int I_FPSTime() { return static_cast<unsigned int>(now_us() / 1000); }
fixed_t I_GetTimeFrac(uint32 *deadline)
{
    ensure_initialized();
    uint32_t portable_deadline;
    const fixed_t fraction = clock_state.fraction(now_us(), &portable_deadline);
    if (deadline) *deadline = static_cast<uint32>(portable_deadline);
    return fraction;
}
void I_InitTimer() { clock_state.reset(now_us()); initialized = true; }
void I_ShutdownTimer() { initialized = false; }
