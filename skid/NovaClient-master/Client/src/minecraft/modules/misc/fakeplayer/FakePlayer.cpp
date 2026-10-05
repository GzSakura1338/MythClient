#include "minecraft/modules/Module.h"
#include "minecraft/java/api/game_fakeplayer.h"
#include "ui/notifications/Notifications.h"
#include <format>
namespace nova::module {
namespace {
class FakePlayer final:public Module {
public:
    FakePlayer():Module("FakePlayer",Category::Misc,"General","General") {
        equipment_=&addBool("Copy Equipment",true);
        health_=&addNumber("Health",1,20,1,20,{},[](double n){return std::format("{:.0f}",n);});
        reset_=&addBool("Auto Reset",true);
        damage_=&addBool("Simulate Damage",true);
    }
private:
    void Publish(){java::ConfigureFakePlayer({true,equipment_->get(),damage_->get(),reset_->get(),static_cast<float>(health_->get())});}
    void onEnable()override {
        if(!java::FakePlayerSupported()) {
            notifications::Post("FakePlayer: requires Lunar 1.21.11 or Minecraft 26.3",notifications::Type::Warning);
            setEnabled(false);return;
        }
        Publish();
    }
    void onUpdate()override {
        const auto status=java::GetFakePlayerStatus();
        if(status==java::FakePlayerStatus::Failed||status==java::FakePlayerStatus::Finished) {
            if(status==java::FakePlayerStatus::Failed)notifications::Post("FakePlayer: could not create/maintain the dummy; see nova-base.log",notifications::Type::Warning);
            setEnabled(false);return;
        }
        Publish();
    }
    void onDisable()override {java::ConfigureFakePlayer({});}
    Properties::BoolProperty *equipment_{},*damage_{},*reset_{};
    Properties::NumberProp* health_{};
};
}
std::unique_ptr<Module> MakeFakePlayerModule(){return std::make_unique<FakePlayer>();}
}
