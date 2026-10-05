#include "minecraft/modules/combat/antikb/JumpResetWindow.h"
#include <cstdio>
#include <cstdlib>

using nova::module::antikb::JumpResetWindow;

void Expect(bool actual, bool expected, const char* scenario) {
    if (actual == expected) return;
    std::fprintf(stderr, "%s failed\n", scenario);
    std::exit(1);
}

int main() {
    JumpResetWindow window;
    Expect(window.Consume(100, 1, true, 2), false, "no applied velocity");
    window.Arm(100, 1, true);
    Expect(window.Consume(100, 1, true, 2), true, "same tick application");
    Expect(window.Consume(101, 1, true, 2), false, "request consumed once");
    window.Arm(110, 1, true);
    Expect(window.Consume(110, 1, true, 0), false, "no second jump in same tick");
    window.Arm(150, 2, true);
    Expect(window.Consume(150, 2, true, 2), false, "cooldown enforced");
    window.Arm(200, 3, true);
    Expect(window.Consume(200, 3, true, 2), true, "cooldown expires");
    window.Reset();
    window.Arm(300, 4, false);
    Expect(window.Consume(350, 5, true, 0), false, "airborne hit cannot jump on landing");
    window.Arm(400, 6, true);
    Expect(window.Consume(450, 7, false, 0), false, "manual jump sneak or invalid environment");
    Expect(window.Consume(451, 7, true, 0), false, "failed request cannot leak to later input");
    window.Arm(500, 8, true);
    Expect(window.Consume(700, 9, true, 0), false, "wall clock expiration");
    window.Arm(800, 10, true);
    Expect(window.Consume(850, 12, true, 0), false, "tick expiration");
    window.Arm(900, 13, true);
    window.Reset();
    Expect(window.Consume(950, 14, true, 0), false, "correction respawn disable clears request");
    window.Arm(1000, 15, true);
    window.Arm(1001, 15, false);
    Expect(window.Consume(1050, 16, true, 0), false, "later non-knockback replaces pending hit");
    window.Arm(1100, 17, true);
    Expect(window.Consume(1150, 18, true, 0), true, "next tick application");
    std::puts("Jump reset: 14 scenarios passed");
}
