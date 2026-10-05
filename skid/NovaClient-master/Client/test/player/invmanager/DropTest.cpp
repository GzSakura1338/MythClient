#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace java {
constexpr unsigned kItemArmor=1;
struct ItemFacts { bool empty=true; unsigned kinds=0; int armorSlot=0; float score=0; };
enum class ClickKind { Throw,QuickMove };
struct Click { int slot,button; ClickKind kind; };
std::vector<Click> clicks;
bool success=true;
bool ClickMenuSlot(int,int slot,int button,ClickKind kind) {
    if(success) clicks.push_back({slot,button,kind});
    return success;
}
}
namespace inventory {
struct Entry { int slot=-1; java::ItemFacts facts; std::string name; };
struct Snapshot { std::vector<Entry> items,armor; };
float Protection(const java::ItemFacts& f) { return f.empty ? 0 : f.score; }
float BestArmorScore(const Snapshot& s,int type) {
    float best=0; for(const auto& e:s.items) if(e.facts.armorSlot==type && !e.facts.empty && e.facts.score>best) best=e.facts.score;
    return best;
}
float CurrentArmorScore(const Snapshot& s,int type) { return Protection(s.armor[3-type].facts); }
int FirstEmptyInventorySlot(const Snapshot& s) { for(const auto& e:s.items) if(e.slot>=9 && e.facts.empty) return e.slot; return -1; }
int FirstEmptyHotbarSlot(const Snapshot& s) { for(const auto& e:s.items) if(e.slot<9 && e.facts.empty) return e.slot; return -1; }
const Entry* At(const Snapshot& s,int slot) { for(const auto& e:s.items) if(e.slot==slot) return &e; return nullptr; }
bool IsNameValid(const java::ItemFacts&,const std::string&) { return true; }
}
struct Timer { bool ready=true; int resets=0; bool Delay(int) { return ready; } void Reset() { ++resets; } } g_timer;
struct Bool { bool value=false; bool get() { return value; } } drop;
struct Module {
    Bool* drop_=&drop;
    bool inventoryOpen_=false;
    bool ClickSlot(int container,int slot,int button,java::ClickKind kind) {
        return java::ClickMenuSlot(container,slot,button,kind);
    }
    // @PRODUCTION_INVENTORY_METHODS@
};
int main() {
    Module m;
    inventory::Snapshot s;
    for(int i=0;i<36;++i) s.items.push_back({i,{false,0,0,0},{}});
    for(int i=0;i<4;++i) s.armor.push_back({i,{},{}});
    s.armor[3].facts={false,java::kItemArmor,0,2};
    s.items[10].facts={false,java::kItemArmor,0,4};
    // Closed Drop suppresses all callers of ThrowItem, even with the delay ready.
    m.ThrowItem(10,s,0,0); assert(java::clicks.empty() && g_timer.resets==0);
    m.AutoArmor(s,0,0); assert(java::clicks.empty()); // Full inventory: no loss or repeated shuffle.
    s.items[12].facts.empty=true;
    m.AutoArmor(s,0,0);
    assert(java::clicks.size()==1 && java::clicks.back().slot==5 && java::clicks.back().kind==java::ClickKind::QuickMove);
    // Next snapshot: old armor was moved back, replacement is equipped normally.
    s.items[12].facts=s.armor[3].facts; s.armor[3].facts={};
    m.AutoArmor(s,0,0);
    assert(java::clicks.size()==2 && java::clicks.back().slot==10 && java::clicks.back().kind==java::ClickKind::QuickMove);
    // Open Drop preserves old disposal behavior, including replacement with a full bag.
    java::clicks.clear(); drop.value=true; s.armor[3].facts={false,java::kItemArmor,0,2};
    m.AutoArmor(s,0,0);
    assert(java::clicks.size()==1 && java::clicks.back().kind==java::ClickKind::Throw && java::clicks.back().button==1);
    m.ThrowItem(0,s,0,0); assert(java::clicks.back().slot==36 && java::clicks.back().kind==java::ClickKind::Throw);
    const auto count=java::clicks.size(); drop.value=false;
    m.ThrowItem(10,s,0,0); assert(java::clicks.size()==count); // Live switch stops subsequent throws.
    java::clicks.clear(); s.items[0].facts.empty=true; java::success=false;
    m.AutoArmor(s,0,0); assert(java::clicks.empty());
    java::success=true; g_timer.ready=false; m.AutoArmor(s,0,0); assert(java::clicks.empty());
    g_timer.ready=true; m.AutoArmor(s,0,0);
    assert(java::clicks.size()==1 && java::clicks.back().kind==java::ClickKind::QuickMove); // Hotbar-only space works.
    std::puts("InvManager Drop: disposal gate, safe armor replacement, full inventory, click failure and delay passed");
}
