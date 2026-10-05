#pragma once
#include "minecraft/modules/movement/elytrafly/EquipLogic.h"
#include "minecraft/modules/movement/elytrafly/FireworkLogic.h"
#include "minecraft/java/api/game_entity.h"
namespace nova::module::elytrafly {
class AutoFire {
public:
    void Reset();
    void Tick(bool enabled);
    void Interact(bool enabled);
private:
    bool Observe();
    bool SameWorld() const;
    FireworkPolicy policy_;
    Selection selection_;
    java::EntityHandle player_, world_;
    std::uint64_t tick_ = 0;
};
}
