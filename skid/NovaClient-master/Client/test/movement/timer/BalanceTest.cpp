#include "minecraft/modules/movement/timer/TimerBalance.h"
#include "minecraft/compat/easecation/TimerRate.h"
#include <limits>
#include <cassert>
#include <iostream>
int main() {
    nova::compat::easecation::TimerRate rate;
    assert(rate.Millis(50) == 50);
    assert(rate.Set(2) && rate.Millis(50) == 25);
    assert(rate.Millis(100) == 50); // Preserve a server's slower base tick rate.
    assert(rate.Set(.5f) && rate.Millis(50) == 100);
    assert(!rate.Set(0) && !rate.Set(-1));
    assert(!rate.Set(std::numeric_limits<float>::quiet_NaN()));
    assert(!rate.Set(std::numeric_limits<float>::infinity()));
    assert(rate.Millis(50) == 100); // Rejected values cannot poison the clock.
    rate.Reset(); assert(rate.Millis(50) == 50);
    std::cout << "PASS: EC game tick scaling, server period, invalid values and reset\n";
    nova::module::timer::Balance b;
    assert(!b.Press());
    for (int i = 0; i < 40; ++i) assert(!b.Movement(true, 1));
    assert(b.value == 40 && !b.compensating);
    b.Movement(false, 1); assert(b.value == 40);
    assert(b.Press() && b.compensating);
    assert(b.Press() && b.value == 40); // Repeated bind cannot toggle off/restart.
    for (int i = 0; i < 39; ++i) assert(!b.Movement(true, 10));
    assert(b.value == 1 && b.compensating);
    assert(b.Movement(true, 10) && !b.compensating);
    b.Reset(); assert(!b.Press());
    b.Movement(true, .1); assert(b.Press());
    assert(b.Movement(false, 1)); // Fractional balance still consumes one per packet.
    b.Reset(); b.Movement(true, 2.5); assert(b.Press());
    assert(!b.Movement(false, 1)); assert(!b.Movement(false, 1));
    assert(b.Movement(false, 1));
    b.Reset(); assert(b.value == 0 && !b.compensating);
    std::cout << "PASS: Rise balance accumulation, bind interception, fractional release and reset\n";
}
