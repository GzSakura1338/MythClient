#include "minecraft/modules/combat/autoblock/HypixelAutoBlock.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using nova::module::HypixelAutoBlock;
using nova::module::HypixelBlockSettings;

void Check(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

struct IO {
    HypixelAutoBlock* state;
    bool allowed = true, sword = true, valid = true, ready = true, hit = true;
    bool usingItem = false, busy = false, blinking = false, noSlow = false;
    int slot = 0, attacks = 0, swings = 0;
    std::vector<std::string> sent, queued;
    std::vector<std::string> actions;
    explicit IO(HypixelAutoBlock& s) : state(&s) {}
    void Packet(std::string p) { (blinking ? queued : sent).push_back(p); actions.push_back(p); }
    bool CanAttack() const { return allowed; }
    bool CanBlock() const { return sword; }
    bool HasBlockTarget() const { return valid; }
    bool Busy() const { return busy; }
    bool IsPlayerBlocking() const { return usingItem && sword; }
    bool NoSlowEnabled() const { return noSlow; }
    bool InSwingRange() const { return true; }
    bool AttackReady() const { return ready; }
    void ClearExtraAttack() {}
    bool PredictionAttack(const HypixelBlockSettings&, int&) { return true; }
    int NormalAttackDelay() const { return 71; }
    int CurrentSlot() const { return slot; }
    int AltSlot(int handle) const { return handle % 8 + 1; }
    int SwapSlot() const { return (slot + 4) % 9; }
    void Slot(int value) { Packet("C09:" + std::to_string(value)); if (state->isBlocking) usingItem = false; }
    void Start() { Packet("C08"); usingItem = busy = true; }
    void Stop() { Packet("C07"); usingItem = false; busy = true; }
    void Swing() { Packet("C0A"); ++swings; }
    bool Attack() { if (!hit) return false; Packet("ATTACK"); ++attacks; return true; }
    void InteractBlock() { Packet("INTERACT_AT"); Packet("INTERACT"); Start(); }
    void Blink(bool on) {
        actions.push_back(on ? "BLINK+" : "BLINK-");
        if (!on) { sent.insert(sent.end(), queued.begin(), queued.end()); queued.clear(); }
        blinking = on;
    }
    void Movement() { Packet("C03"); busy = false; }
};

void Tick(HypixelAutoBlock& state, HypixelBlockSettings& settings, IO& io) {
    state.Pre(settings, io); io.Movement(); state.Post(io);
}

int main() {
    // Packet traces transcribed from Leader KillAura.onUpdate, with all
    // independent modules disabled and 10 APS. Compare actions across an actual
    // movement boundary, not merely the machine's next integer state.
    const std::vector<std::vector<std::string>> expected = {
        {"C0A","ATTACK","INTERACT_AT","INTERACT","C08","C03","C07","C03","C0A","ATTACK","INTERACT_AT","INTERACT","C08","C03"},
        {"C0A","ATTACK","INTERACT_AT","INTERACT","C08","C03","C09:1","C09:2","C09:0","C03","C0A","ATTACK","C03"},
        {"C0A","ATTACK","INTERACT_AT","INTERACT","C08","C03","C03","C09:1","C09:0","C07","C03","C0A","ATTACK","C03"},
        {"C0A","ATTACK","INTERACT_AT","INTERACT","C08","C03","C07","C03","C0A","ATTACK","INTERACT_AT","INTERACT","C08","C03","C07","C03","C0A","ATTACK","C03"},
        {"C0A","ATTACK","INTERACT_AT","INTERACT","C08","C03","C07","C03","C03","C03","C0A","ATTACK","C03"}
    };
    const int counts[] = {3, 3, 4, 5, 5};
    for (int mode = 0; mode < 5; ++mode) {
        HypixelAutoBlock state; HypixelBlockSettings s; s.mode = 3; s.lagMode = mode;
        IO io(state);
        for (int tick = 0; tick < counts[mode]; ++tick) Tick(state, s, io);
        io.Blink(false);
        if (io.sent != expected[mode]) {
            std::cerr << "Lag " << mode << " trace:";
            for (const auto& p : io.sent) std::cerr << ' ' << p;
            std::cerr << '\n';
            Check(false, "Leader packet trace");
        }
    }
    for (int mode = 0; mode < 2; ++mode) {
        HypixelAutoBlock state; HypixelBlockSettings s; s.mode = mode;
        IO io(state);
        Tick(state,s,io); Tick(state,s,io); Tick(state,s,io);
        Check(io.attacks == 1 && !io.usingItem && io.blinking, "old/without release phase");
        Tick(state,s,io);
        Check(io.attacks == 2 && io.usingItem && state.blockTick == 1, "old/without cycle reblocks");
    }
    {
        HypixelAutoBlock state; HypixelBlockSettings s; s.mode = 3; s.lagMode = 1; s.c09Instead = false;
        IO io(state); Tick(state,s,io); Tick(state,s,io);
        Check(io.actions[io.actions.size()-2] == "C07", "3Tick C07 alternative");
        Check(!io.usingItem && io.attacks == 1, "release cannot attack before C03");
    }
    {
        HypixelAutoBlock state; HypixelBlockSettings s; s.mode = 2;
        s.maxTick = 4; s.startBlockTick = 1; s.attackTick = 2; s.postBlock = true;
        s.swapTick = s.switchBackTick = s.stopBlockTick = 5;
        IO io(state); state.Pre(s,io);
        Check(state.postBlock && !io.usingItem, "Custom PostBlock waits for RETURN");
        io.Movement(); state.Post(io);
        Check(io.usingItem && io.sent == std::vector<std::string>{"C03","C08"}, "Custom POST packet ordering");
        Tick(state,s,io);
        Check(state.blockTick == 1, "POST C08 keeps placing set until the next C03");
        Tick(state,s,io); Tick(state,s,io);
        Check(state.blockTick == 0, "Custom MaxTick - 1 wrap");
    }
    {
        HypixelAutoBlock state; HypixelBlockSettings s; s.mode = 2;
        s.maxTick = 5; s.swapTick = 1; s.switchBackTick = 4;
        IO io(state); Tick(state,s,io);
        Check(state.swapped, "Custom remembers swapped server slot");
        io.allowed = false; state.Pre(s,io);
        Check(!state.swapped && io.actions.back() == "C09:0", "target loss restores original server slot");
    }
    {
        HypixelAutoBlock state; HypixelBlockSettings s; s.mode = 3; s.lagMode = 0; s.aps = 6;
        IO io(state); Tick(state,s,io);
        Check(state.attackDelayMS == 166, "APS uses float division truncated to milliseconds");
        Tick(state,s,io); Check(state.blockTick == 1, "APS can extend release phase");
        Tick(state,s,io); Check(state.blockTick == 1, "APS threshold is 50 ms");
        Tick(state,s,io); Check(state.blockTick == 0, "APS permits next cycle");
    }
    {
        HypixelAutoBlock state; HypixelBlockSettings s; IO io(state); io.hit = false;
        Tick(state,s,io);
        Check(io.swings == 1 && io.attacks == 0 && state.attackDelayMS == 100 && io.usingItem,
              "ray miss consumes swing/timer and blocks without interaction");
        Check(std::find(io.sent.begin(),io.sent.end(),"INTERACT") == io.sent.end(), "miss has no interact packet");
    }
    {
        HypixelAutoBlock state; HypixelBlockSettings s; IO io(state); io.busy = true;
        Tick(state,s,io);
        Check(state.blockTick == 0 && io.attacks == 0, "digging/placing freezes progression");
        Tick(state,s,io); Check(io.attacks == 1, "movement clears packet busy flags");
        io.allowed = false; state.Pre(s,io);
        Check(!state.isBlocking && !state.fakeBlockState && !io.blinking && state.blockTick == 0,
              "target loss clears cycle and flushes packets");
    }
    // Exercise all exposed mode switches, slot wrap, low APS and optional C09
    // branches over sustained operation. Catch stalls and leftover slot swaps.
    for (int mode=0; mode<4; ++mode) for(int lag=0;lag<5;++lag)
    for(int aps : {1,6,10,20}) for(int slot : {0,7,8}) for(bool full : {false,true}) {
        HypixelAutoBlock state; HypixelBlockSettings s; IO io(state);
        s.mode=mode; s.lagMode=lag; s.aps=aps; s.fullC09=full; s.noSwap=false;
        io.slot=slot;
        if(mode==2) { s.attackTick=1; s.startBlockTick=1; s.startBlinkTick=1; s.maxTick=4; }
        for(int n=0;n<200;++n) Tick(state,s,io);
        Check(io.attacks>0, "mode never attacks");
        state.Disable(s,io);
        Check(!io.blinking && io.queued.empty() && !state.swapped, "disable flushes queue and restores slot");
    }
    std::cout << "PASS: Leader Hypixel packet traces, all modes, APS, Custom POST, cleanup (480 sustained cases)\n";
}
