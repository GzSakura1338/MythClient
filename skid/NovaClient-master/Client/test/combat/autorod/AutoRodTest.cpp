#include "minecraft/modules/combat/autorod/AutoRodLogic.h"
#include "bind/PhysicalKey.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nova::module::autorod;
struct IO {
    std::array<bool, 36> items{};
    std::array<bool, 36> rods{};
    bool switchReady = true;
    int selected = 2, uses = 0, syncs = 0;
    bool available = true, failSelect = false, failUse = false, throwUse = false;
    bool usedThisTick = false;
    int failSync = -1;
    std::vector<std::string> actions;
    bool Selected(int& out) { out = selected; return available; }
    bool Eligible(int slot) { assert(slot >= 0 && slot < 9); return items[slot]; }
    bool IsRod(int slot) { return rods[slot]; }
    bool CanSwitch() { return switchReady; }
    bool Select(int slot) { selected = slot; actions.push_back("slot" + std::to_string(slot)); return !failSelect; }
    bool Sync() {
        assert(!usedThisTick); // No held-item synchronization after use in the same tick.
        actions.push_back("sync"); return ++syncs != failSync;
    }
    bool Use() {
        assert(items[selected]); actions.push_back("use"); ++uses; usedThisTick = true;
        if (throwUse) throw std::runtime_error("use failed");
        return !failUse;
    }
};
bool Tick(State& state, IO& io, bool allowed, std::uint64_t delay, std::uint64_t now,
          std::uint64_t switchDelay = 0, std::uint64_t rodDelay = 500) {
    // The game tick boundary follows vanilla's movement update for the last tick.
    io.usedThisTick = false;
    return state.Tick(io, allowed, delay, now, switchDelay, rodDelay);
}
int main() {
    {
        State state; IO io; io.items[4] = true;
        assert(!state.OwnsItemUse(false) && state.OwnsItemUse(true));
        // Switching/using reserves the hotbar until the original slot is synced.
        assert(!Tick(state,io,true,500,0,100));
        assert(state.waitingToUse && state.OwnsItemUse(false));
        io.switchReady = false;
        assert(!Tick(state,io,false,500,50,100) && state.OwnsItemUse(false));
        io.switchReady = true;
        assert(!Tick(state,io,false,500,100,100) && !state.OwnsItemUse(false));
        assert(io.selected == 2 && io.uses == 0);
        assert(Tick(state,io,true,500,500));
        assert(state.OwnsItemUse(false));
        assert(!Tick(state,io,false,500,550) && !state.OwnsItemUse(false));
        // A cast from the already-selected rod has no slot restoration, but
        // still owns use until the rod wait is cancelled or completed.
        state = {}; io = {}; io.selected = 4; io.items[4] = io.rods[4] = true;
        assert(Tick(state,io,true,500,0) && state.originalSlot == -1 && state.OwnsItemUse(false));
        assert(!Tick(state,io,false,500,50) && !state.OwnsItemUse(false));
    }
    static_assert(nova::bind::BindingVirtualKey(-1) == VK_LBUTTON);
    static_assert(nova::bind::BindingVirtualKey(-2) == VK_RBUTTON);
    static_assert(nova::bind::BindingVirtualKey(-3) == VK_MBUTTON);
    static_assert(nova::bind::BindingVirtualKey(-4) == VK_XBUTTON1);
    static_assert(nova::bind::BindingVirtualKey(-5) == VK_XBUTTON2);
    static_assert(nova::bind::BindingVirtualKey(65) == 65);
    static_assert(nova::bind::BindingVirtualKey(0) == 0);
    static_assert(nova::bind::BindingVirtualKey(-6) == 0);
    static_assert(nova::bind::BindingVirtualKey(256) == 0);
    {
        KeyState key; State state; IO io; io.items[4] = true;
        bool sideButtonDown = false;
        const auto tick = [&](std::uint64_t now, bool allowed = true) {
            const int vk = nova::bind::BindingVirtualKey(-5);
            const bool down = vk == VK_XBUTTON2 && sideButtonDown;
            return Tick(state, io, key.Update(-5, down, allowed), 500, now);
        };
        assert(!tick(0)); sideButtonDown = true;
        assert(tick(1)); assert(!tick(500)); assert(tick(501));
        sideButtonDown = false; assert(!tick(1001)); assert(io.uses == 2);
        sideButtonDown = true; assert(!tick(1002, false));
        assert(!tick(1003)); // A held key after leaving a menu is not a new press.
        sideButtonDown = false; assert(!tick(1004));
        sideButtonDown = true; assert(tick(1005)); assert(io.selected == 4);
        sideButtonDown = false; assert(!tick(1055)); assert(io.selected == 2);
    }
    {
        KeyState key;
        assert(!key.Update(0,true,true));
        assert(!key.Update(65,false,true));
        assert(key.Update(65,true,true));
        assert(key.Update(65,true,true));
        assert(!key.Update(65,false,true));
        assert(key.Update(65,true,true));
        assert(!key.Update(65,true,false)); // Menu/focus/world/module gate.
        assert(!key.Update(65,true,true)); // No phantom press after gate clears.
        assert(!key.Update(65,false,true));
        assert(key.Update(65,true,true));
        assert(!key.Update(66,true,true)); // Rebind while held cancels.
        assert(!key.Update(66,false,true));
        assert(key.Update(66,true,true));
        assert(!key.Update(66,false,true)); // Always stop on release.
    }
    {
        State state; IO io; io.items[8] = true;
        assert(!Tick(state,io,false,500,100)); assert(io.actions.empty());
        assert(Tick(state,io,true,500,100));
        assert((io.actions == std::vector<std::string>{"slot8","sync","use"}));
        assert(io.selected == 8 && !state.running);
        assert(!Tick(state,io,true,500,599));
        assert(io.selected == 2);
        assert((io.actions == std::vector<std::string>{"slot8","sync","use","slot2","sync"}));
        assert(Tick(state,io,true,500,600)); assert(io.uses == 2);
        assert(!Tick(state,io,false,500,1100)); assert(io.uses == 2);
        assert(Tick(state,io,true,50,1100));
        assert(!Tick(state,io,true,50,1149));
        assert(Tick(state,io,true,50,1150));
    }
    {
        State state; IO io; io.items[2] = io.items[0] = true;
        assert(Tick(state,io,true,500,0));
        assert((io.actions == std::vector<std::string>{"slot0","sync","use"}));
        io.items[0]=false;
        assert(Tick(state,io,true,500,500)); // Left stack depleted; use the next eligible slot.
        assert(io.selected==2);
    }
    {
        State state; IO io; io.items[2]=true;
        assert(Tick(state,io,true,500,0,200)); // Already holding the leftmost item: no switch wait.
        assert((io.actions == std::vector<std::string>{"sync","use"}));
    }
    {
        State state; IO io; io.items[4]=true;
        assert(!Tick(state,io,true,500,100,150));
        assert(state.waitingToUse && io.selected==4 && io.uses==0);
        assert(!Tick(state,io,true,500,249,150));
        assert(Tick(state,io,true,500,250,150));
        assert(!state.waitingToUse && io.uses==1 && io.selected==4);
        assert(!Tick(state,io,false,500,300,150)); assert(io.selected==2);
        assert(!Tick(state,io,true,500,749,150)); // Delay measured from use, not the initial switch.
        assert(!Tick(state,io,true,500,750,150)); assert(state.waitingToUse);
        assert(!Tick(state,io,false,500,800,150)); // Release while waiting cancels the pending throw.
        assert(!state.waitingToUse && io.selected==2 && io.uses==1);
    }
    {
        State state; IO io; io.items[4]=true;
        assert(!Tick(state,io,true,500,0,100));
        io.selected=7;
        assert(!Tick(state,io,true,500,100,100)); // Manual slot change cancels without overriding it.
        assert(io.selected==7 && io.uses==0 && !state.waitingToUse);
        state={}; io.selected=2;
        assert(!Tick(state,io,true,500,0,100));
        io.items[4]=false;
        assert(!Tick(state,io,true,500,100,100)); // Item removed during wait.
        assert(io.selected==2 && io.uses==0);
    }
    {
        State state; IO io; io.items[9] = true;
        assert(!Tick(state,io,true,500,0)); assert(io.actions.empty());
        io.items[7] = io.items[1] = true;
        assert(Tick(state,io,true,500,0)); assert(io.actions[0] == "slot1");
    }
    for (int failure=0; failure<4; ++failure) {
        State state; IO io; io.items[4] = true;
        io.failSelect = failure==0; io.failSync = failure==1 ? 1 : -1;
        io.failUse = failure==2; io.throwUse = failure==3;
        try { assert(!Tick(state,io,true,500,0)); }
        catch (const std::runtime_error&) { assert(failure==3); }
        assert(!state.running && state.originalSlot==2);
        io.failSelect=false; io.failSync=-1;
        assert(!Tick(state,io,false,500,50)); // Cleanup even after disable/release/failure.
        assert(io.selected==2 && state.originalSlot==-1);
        assert(io.actions[io.actions.size()-2]=="slot2" && io.actions.back()=="sync");
        assert(!Tick(state,io,true,500,499));
        if (failure<2) assert(io.uses==0);
    }
    {
        State state; IO io; io.items[4]=true;
        assert(Tick(state,io,true,500,0));
        io.selected=7; // The player manually changed slots before cleanup.
        assert(!Tick(state,io,false,500,50)); assert(io.selected==7);
        assert(state.originalSlot==-1 && io.actions.back()=="sync");
    }
    {
        State state; IO io; io.items[4]=true;
        assert(Tick(state,io,true,500,0));
        io.failSync=2; assert(!Tick(state,io,true,500,500));
        assert(state.originalSlot==2 && io.uses==1); // Do not use again while restoration failed.
        io.failSync=-1; assert(!Tick(state,io,false,500,550));
        assert(state.originalSlot==-1 && io.selected==2);
        assert(Tick(state,io,true,500,600));
        state={}; io.selected=6; // World replacement discards the previous world's slot.
        assert(!Tick(state,io,false,500,650)); assert(io.selected==6);
    }
    {
        State state; IO io; io.items[4]=true;
        io.selected=-1; assert(!Tick(state,io,true,500,0));
        io.selected=9; assert(!Tick(state,io,true,500,0));
        io.selected=2; io.available=false; assert(!Tick(state,io,true,500,0));
        io.available=true; state.running=true; assert(!Tick(state,io,true,500,0));
        assert(io.actions.empty()); state={}; assert(Tick(state,io,true,500,0));
    }
    {
        State state; IO io; io.items[4]=true;
        io.switchReady=false; // Another module used an item after the last movement packet.
        assert(!Tick(state,io,true,500,0)); assert(io.actions.empty());
        io.switchReady=true; assert(Tick(state,io,true,500,50));
        io.switchReady=false;
        for (int time=100;time<1000;time+=50) {
            assert(!Tick(state,io,true,500,time));
            assert(io.selected==4 && io.uses==1); // No actual movement boundary yet.
        }
        io.switchReady=true;
        assert(!Tick(state,io,false,500,1000)); assert(io.selected==2);
    }
    {
        State state; IO io; io.items[4]=io.rods[4]=true;
        assert(Tick(state,io,true,100,0,0,600));
        assert(state.rodWaiting && io.selected==4 && io.uses==1);
        assert(!Tick(state,io,true,100,100,0,600)); // Normal delay must not reel early.
        assert(!Tick(state,io,true,100,599,0,600));
        io.switchReady=false; assert(!Tick(state,io,true,100,600,0,600));
        io.switchReady=true; assert(Tick(state,io,true,100,650,0,600));
        assert(!state.rodWaiting && io.selected==4 && io.uses==2);
        io.switchReady=false; assert(!Tick(state,io,false,100,700)); assert(io.selected==4);
        io.switchReady=true; assert(!Tick(state,io,false,100,750)); assert(io.selected==2);
    }
    {
        State state; IO io; io.items[4]=io.rods[4]=true; io.failUse=true;
        assert(!Tick(state,io,true,500,0,0,600)); // Legacy cast can return false despite sending.
        assert(state.rodWaiting);
        assert(!Tick(state,io,true,500,50,0,600)); assert(io.selected==4);
        assert(!Tick(state,io,false,500,100,0,600)); // Release cancels reeling too.
        assert(io.uses==1 && io.selected==2 && !state.rodWaiting);
    }
    std::puts("PASS: M5 release, delayed movement/action conflicts, leftmost selection, switch wait, rod return timing, restoration and world reset");
}
