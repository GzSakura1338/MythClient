#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct TestClock {
    using time_point=std::chrono::steady_clock::time_point;
    static inline time_point time{};
    static time_point now() { return time; }
};
namespace Properties {
struct ModeProperty { int value=0;int get()const{return value;} };
struct NumberProp { double value=15;double get()const{return value;} };
}
namespace nova::module {
enum class Category { Render };
class Module {
    bool enabled_=false;
public:
    Properties::ModeProperty mode;
    Properties::NumberProp brightness;
    Module(const char*,Category,const char*,const char*) {}
    virtual ~Module()=default;
    void setEnabled(bool value) {
        if(value==enabled_)return;
        enabled_=value;if(value)onEnable();else onDisable();
    }
    void tick() { if(enabled_)onUpdate(); }
protected:
    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onUpdate() {}
    Properties::ModeProperty& addMode(const char*,int value,std::vector<std::string>) { mode.value=value;return mode; }
    Properties::NumberProp& addNumber(const char*,double,double,double,double value,
        std::vector<int>,std::function<std::string(double)>,std::function<bool()>) { brightness.value=value;return brightness; }
};
}
namespace nova::java {
double gamma=.5;
bool readable=true,writable=true,nightVision=false;
int writes=0;
bool GetGamma(double* out) { if(!readable)return false;*out=gamma;return true; }
bool SetGamma(double value) { if(!writable)return false;gamma=value;++writes;return true; }
void AddNightVision() { nightVision=true; }
void RemoveNightVision() { nightVision=false; }
}

// @PRODUCTION_MODULE@

void Expect(bool result,const char* message) { if(!result)throw std::runtime_error(message); }
bool Near(double a,double b) { return std::abs(a-b)<.00001; }
void Tick(nova::module::Module& module,int count=1) {
    for(int i=0;i<count;++i){TestClock::time+=std::chrono::milliseconds(51);module.tick();}
}
int main() {
    using namespace nova;
    auto module=module::MakeFullBrightModule();
    java::readable=false;module->setEnabled(true);Tick(*module,5);
    Expect(java::writes==0,"Unavailable gamma must not overwrite the baseline");
    java::readable=true;Tick(*module);
    Expect(Near(java::gamma,.6),"Config-enabled module must recover without a toggle");
    Tick(*module,160);Expect(Near(java::gamma,15),"Gamma must reach configured brightness");
    const int settledWrites=java::writes;Tick(*module,5);
    Expect(java::writes==settledWrites,"Do not rewrite an unchanged option every tick");
    java::gamma=.5;Tick(*module);
    Expect(Near(java::gamma,15),"Game option reload must not leave cached gamma inactive");
    module->brightness.value=3;Tick(*module,125);
    Expect(Near(java::gamma,3),"Reducing brightness must update gamma");

    module->mode.value=1;Tick(*module);
    Expect(java::nightVision && Near(java::gamma,.5),"NightVision must restore original gamma");
    module->mode.value=0;Tick(*module);
    Expect(!java::nightVision && Near(java::gamma,.6),"Returning to Gamma must start applying again");
    module->setEnabled(false);Expect(Near(java::gamma,.5),"Unload must restore original brightness");
    module.reset();module=module::MakeFullBrightModule();
    module->setEnabled(true);Tick(*module);
    Expect(Near(java::gamma,.6),"New injected instance with saved Enabled must apply gamma");

    java::writable=false;Tick(*module,50);java::writable=true;Tick(*module);
    Expect(Near(java::gamma,.7),"Failed writes must retry without advancing the ramp");
    java::writable=false;module->mode.value=1;Tick(*module);
    java::writable=true;Tick(*module);
    Expect(Near(java::gamma,.5),"Failed mode-switch restoration must retry");
    java::gamma=.8;module->mode.value=0;Tick(*module);
    Expect(Near(java::gamma,.9),"New gamma activation must capture the current baseline");
    module->setEnabled(false);Expect(Near(java::gamma,.8),"Restore the newly captured baseline");
    std::cout<<"FullBright: config enable, reinjection, transient JNI failures, option reset, mode switch and brightness adjustment passed\n";
}
