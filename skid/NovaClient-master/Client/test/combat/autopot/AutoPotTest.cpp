#include "minecraft/modules/combat/autopot/AutoPotLogic.h"
#include <array>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>

using nova::module::autopot::State;
struct IO {
    std::array<bool, 36> healing{};
    int selected = 2, uses = 0, syncs = 0;
    bool selectionAvailable = true, failSelect = false, failUse = false, throwUse = false;
    int failSync = -1;
    std::vector<std::string> actions;
    bool Selected(int& out) { out = selected; return selectionAvailable; }
    bool Healing(int slot) { assert(slot >= 0 && slot < 9); return healing[slot]; }
    bool Select(int slot) { selected = slot; actions.push_back("slot" + std::to_string(slot)); return !failSelect; }
    bool Sync() { actions.push_back("sync"); return ++syncs != failSync; }
    bool Use() {
        actions.push_back("use"); ++uses;
        assert(healing[selected]);
        if (throwUse) throw std::runtime_error("use failed");
        return !failUse;
    }
};
int main() {
    {
        State state; IO io; io.healing[8] = true;
        assert(!state.Tick(io, false, 8, 12, 100));
        assert(!state.Tick(io, true, 13, 12, 100));
        assert(!state.Tick(io, true, 0, 12, 100));
        assert(!state.Tick(io, true, std::numeric_limits<float>::quiet_NaN(), 12, 100));
        assert(!state.Tick(io, true, 8, std::numeric_limits<double>::quiet_NaN(), 100));
        assert(io.actions.empty());
        assert(state.Tick(io, true, 12, 12, 100));
        assert((io.actions == std::vector<std::string>{"slot8", "sync", "use", "slot2", "sync"}));
        assert(io.selected == 2 && io.uses == 1 && !state.running);
        assert(!state.Tick(io, true, 8, 12, 599));
        assert(state.Tick(io, true, 8, 12, 600));
        assert(io.uses == 2);
    }
    {
        State state; IO io; io.healing[2] = io.healing[0] = true;
        assert(state.Tick(io, true, 4, 12, 0));
        assert((io.actions == std::vector<std::string>{"sync", "use"}));
    }
    {
        State state; IO io; io.healing[9] = true;
        assert(!state.Tick(io, true, 4, 12, 0)); // inventory rows are not hotbar
        assert(io.actions.empty());
        io.healing[7] = io.healing[1] = true;
        assert(state.Tick(io, true, 4, 12, 0));
        assert(io.actions[0] == "slot1");
    }
    for (int failure = 0; failure < 4; ++failure) {
        State state; IO io; io.healing[4] = true;
        io.failSelect = failure == 0; io.failSync = failure == 1 ? 1 : -1;
        io.failUse = failure == 2; io.throwUse = failure == 3;
        try { assert(!state.Tick(io, true, 4, 12, 0)); }
        catch (const std::runtime_error&) { assert(failure == 3); }
        assert(io.selected == 2 && !state.running);
        assert(io.actions[io.actions.size() - 2] == "slot2" && io.actions.back() == "sync");
        assert(!state.Tick(io, true, 4, 12, 499));
        if (failure < 2) assert(io.uses == 0);
    }
    {
        State state; IO io; io.healing[4] = true;
        io.selected = -1; assert(!state.Tick(io, true, 8, 12, 0));
        io.selected = 9; assert(!state.Tick(io, true, 8, 12, 0));
        io.selected = 2; io.selectionAvailable = false;
        assert(!state.Tick(io, true, 8, 12, 0));
        io.selectionAvailable = true; state.running = true;
        assert(!state.Tick(io, true, 8, 12, 0));
        assert(io.actions.empty());
        state = {}; // module disable/world exit cancels cooldown/state
        assert(state.Tick(io, true, 8, 12, 0));
    }
    std::puts("PASS: health threshold, hotbar-only selection, cooldown, single use, slot/packet order, failure restoration and reset");
}
