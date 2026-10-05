#pragma once
#include "minecraft/java/api/game_mace.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_slotspoof.h"
namespace nova::module::explosive {
class HotbarSession {
    const void* owner_;
    java::EntityHandle world_,player_,original_;
    bool active_=false,silent_=false,restored_=false;
    int old_=-1,used_=-1;
    bool SameWorld(){return java::PlayerEntity().IsSame(player_)&&java::LevelEntity().IsSame(world_);}
public:
    explicit HotbarSession(const void* owner):owner_(owner){}
    ~HotbarSession(){java::ReleaseSilentSlot(owner_);}
    bool Active()const{return active_;}
    bool Prepare(bool silent){
        int selected=-1;
        if(active_&&(!SameWorld()||silent!=silent_||!java::OwnsSlot(owner_)||
            (java::SelectedSlot(selected)&&selected!=(silent_||restored_?old_:used_))))return Restore()&&false;
        java::YieldSlot(owner_);return true;
    }
    bool Equip(int slot,bool silent){
        int selected=-1;if(!java::SelectedSlot(selected))return false;
        if(!active_){original_=java::MaceSlotIdentity(selected);if(!original_)return false;
            old_=used_=selected;world_=java::LevelEntity();player_=java::PlayerEntity();silent_=silent;active_=true;restored_=false;}
        if(silent_){if(!java::SetSilentSlot(owner_,slot))return false;}
        else if(!java::AcquirePhysicalSlot(owner_)||!java::SetOwnedPhysicalSlot(owner_,slot))return false;
        used_=slot;return true;
    }
    bool Restore(){
        if(!active_)return true;
        if(SameWorld()){
            if(silent_){const bool owned=java::OwnsSlot(owner_);java::ReleaseSilentSlot(owner_);
                if((owned||restored_)&&!java::SyncSelectedItem()){restored_=true;return false;}}
            else {
                if(!java::AcquirePhysicalSlot(owner_))return false;
                int selected=-1;if(!java::SelectedSlot(selected))return false;
                if(selected==(restored_?old_:used_)&&java::MaceSlotMatches(old_,original_)){
                    if(!restored_&&!java::SetOwnedPhysicalSlot(owner_,old_))return false;
                    restored_=true;if(!java::SyncSelectedItem())return false;
                }
            }
        }
        java::ReleaseSilentSlot(owner_);active_=restored_=false;original_.Reset();world_.Reset();player_.Reset();return true;
    }
};
}
