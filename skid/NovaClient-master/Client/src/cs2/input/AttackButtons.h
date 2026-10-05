#pragma once
#include "cs2/sdk/weapons/Weapons.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace nova::cs2::input {
struct AttackButtons { std::uint64_t held = 0, pressed = 0, released = 0; };
struct ButtonMove {
    float fraction = 0;
    std::uint32_t padding = 0;
    std::uint64_t mask = 0;
    bool down = false;
    std::array<unsigned char, 7> reserved{};
    float pitch = 0, yaw = 0;
};
struct InputSample {
    AttackButtons buttons;
    std::array<unsigned char, 20> movement{};
    int count = 0;
    std::array<ButtonMove, 32> moves{};
    sdk::weapons::Angle angle;
    float fraction = 0;
};
static_assert(sizeof(ButtonMove) == 32 && offsetof(ButtonMove, mask) == 8 && offsetof(ButtonMove, down) == 16);
static_assert(sizeof(InputSample) == 0x440 && offsetof(InputSample, angle) == 0x430 && offsetof(InputSample, moves) == 48);
// Operate on a copy: invalid/full history cannot leave a partially written press.
// Native movement/secondary buttons are preserved. No global kbutton is held.
inline bool MergeAttack(InputSample& sample, bool down, bool previous, float fraction) {
    if (!down && !previous) return true;
    if (sample.count < 0 || sample.count > 32 || !std::isfinite(fraction) || fraction < 0 || fraction >= 1) return false;
    auto out = sample;
    constexpr std::uint64_t attack = 1;
    const bool manual = (out.buttons.held & attack) != 0;
    const bool press = !previous && down && !manual;
    const bool release = previous && !down && !manual;
    if (!std::isfinite(out.fraction) || out.fraction < 0 || out.fraction >= 1) return false;
    fraction = std::max(fraction, out.fraction);
    int count = 0;
    for (int i = 0; i < out.count; ++i) {
        auto move = out.moves[i];
        if (!std::isfinite(move.fraction) || move.fraction < 0 || move.fraction >= 1) return false;
        fraction = std::max(fraction, move.fraction);
        // A newly acquired auto request starts at this sample's end; earlier
        // manual clicks keep their original timing. An existing auto hold
        // masks manual edges until the combined input actually becomes up.
        if (previous && (move.mask & attack)) { move.mask &= ~attack; if (!move.mask) continue; }
        out.moves[count++] = move;
    }
    if (press || release) {
        if (count == 32) return false;
        out.moves[count++] = {fraction, 0, attack, press, {}, out.angle.pitch, out.angle.yaw};
    }
    out.count = count;
    out.buttons.held = (out.buttons.held & ~attack) | ((down || manual) ? attack : 0);
    out.buttons.pressed = (previous ? out.buttons.pressed & ~attack : out.buttons.pressed) | (press ? attack : 0);
    out.buttons.released = (previous ? out.buttons.released & ~attack : out.buttons.released) | (release ? attack : 0);
    if (count) out.fraction = out.moves[0].fraction;
    sample = out; return true;
}
}
