#pragma once
#include "cs2/sdk/players/Players.h"
#include <deque>

namespace nova::cs2::sdk::hud {
struct Snapshot {
    std::string player, server;
    int ping = -1, serverTick = -1, serverCount = -1;
    std::uintptr_t session = 0;
    players::Vec3 position;
    bool hasPosition = false;
    double speed = -1; // Horizontal Source units per second.
};
bool ReadSnapshot(const players::Memory&, std::uintptr_t client, std::uintptr_t engine, Snapshot&);
bool Capture(Snapshot&);

// Server tick progress over wall time, not a fixed tickrate or render FPS.
class TickRate {
public:
    double Update(const Snapshot&, double now);
    void Reset();
private:
    struct Sample { double time; int tick; };
    std::deque<Sample> samples_;
    std::uintptr_t session_ = 0;
    int serverCount_ = -1;
};
}
