#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include "config/ConfigManager.h"
#include "config/HostConfig.h"
#include "ui/overlay/WindowPlacement.h"

namespace test {
int saves=0,loads=0,value=1;
void Save(nova::json::Value& root){root["value"]=value;root["snapshot"]=++saves;}
void Load(const nova::json::Value&){++loads;}
nova::json::Value Read(const std::filesystem::path& path){
    std::ifstream file(path,std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(file)),{});
    auto parsed=nova::json::Parse(text);assert(parsed);return *parsed;
}
}
namespace nova::notifications {void Post(std::string,Type,Source){}}
namespace nova::ui {
json::Value SaveWindowPlacements(){auto out=json::Value::Object();out["hud"]=42;return out;}
void LoadWindowPlacements(const json::Value&){}
}
int main(){
    using namespace nova;
    const auto dir=std::filesystem::current_path()/std::to_string(GetCurrentProcessId());
    auto& manager=config::ConfigManager::Get();
    assert(!manager.SaveForUnload()); // Never accidentally write to the cwd.
    assert(config::InstallHostConfig({dir,test::Save,test::Load}));manager.Init();
    test::value=10;int before=test::saves;
    assert(manager.SaveForUnload()&&test::saves==before+1);
    assert(test::Read(dir/"default.json").Find("value")->asNumber(-1)==10);
    assert(manager.SaveAs("pot"));
    const auto current=manager.CurrentName();const auto serial=manager.LoadSerial();const int loads=test::loads;
    test::value=25;before=test::saves;
    assert(manager.SaveForUnload()&&test::saves==before+1);
    assert(manager.CurrentName()==current&&manager.LoadSerial()==serial&&test::loads==loads);
    auto named=test::Read(dir/"pot.json"),fallback=test::Read(dir/"default.json");
    assert(named.Find("name")->asString()=="pot"&&fallback.Find("name")->asString()=="default");
    assert(named.Find("snapshot")->asNumber(-1)==fallback.Find("snapshot")->asNumber(-2));
    named["name"]="default";assert(named.Dump()==fallback.Dump());
    // Failure of either file must still attempt the other destination and
    // preserve the old complete file, rather than truncating it.
    for(const auto* locked:{L"pot.json",L"default.json"}){
        const auto path=dir/locked;
        HANDLE handle=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        assert(handle!=INVALID_HANDLE_VALUE);++test::value;
        assert(!manager.SaveForUnload());CloseHandle(handle);
        const bool currentLocked=std::wstring_view(locked)==L"pot.json";
        assert(test::Read(dir/(currentLocked?"default.json":"pot.json")).Find("value")->asNumber(-1)==test::value);
        assert(test::Read(path).Find("value")->asNumber(-1)!=test::value);
        assert(manager.Status().find(currentLocked?"pot":"default")!=std::string::npos);
        assert(manager.CurrentName()=="pot");assert(manager.SaveForUnload());
    }
    assert(manager.SaveAs("Default"));before=test::saves;++test::value;
    assert(manager.SaveForUnload()&&test::saves==before+1&&manager.CurrentName()=="Default");
    assert(test::Read(dir/"default.json").Find("value")->asNumber(-1)==test::value);
    std::puts("PASS: unload saves current and default from one snapshot, preserves current selection, handles default alias and independent write failures atomically");
}
