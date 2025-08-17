#include "tt_kernel.h"
#include <Tactility/kernel/Kernel.h>

extern "C" {

void tt_kernel_delay_millis(uint32_t milliseconds) {
    tt::kernel::delayMillis(milliseconds);
}

void tt_kernel_delay_micros(uint32_t microSeconds) {
    tt::kernel::delayMicros(microSeconds);
}

void tt_kernel_delay_ticks(TickType ticks) {
    tt::kernel::delayTicks((TickType_t)ticks);
}

TickType tt_kernel_get_ticks() {
    return tt::kernel::getTicks();
}

TickType tt_kernel_millis_to_ticks(uint32_t milliSeconds) {
    return tt::kernel::millisToTicks(milliSeconds);
}

bool tt_kernel_delay_until_tick(TickType tick) {
    return tt::kernel::delayUntilTick(tick);
}

uint32_t tt_kernel_get_tick_frequency() {
    return tt::kernel::getTickFrequency();
}

uint32_t tt_kernel_get_millis() {
    return tt::kernel::getMillis();
}

unsigned long tt_kernel_get_micros() {
    return tt::kernel::getMicros();
}

}