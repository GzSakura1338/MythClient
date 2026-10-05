#include "cs2/hooks/ChamsHook.h"
#include "cs2/modules/render/chams/Chams.h"
#include "cs2/modules/ModuleManager.h"
#include "cs2/sdk/players/Offsets.h"
#include "MinHook.h"
#include <windows.h>
#include <array>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "Chams test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while(false)
namespace {
using namespace nova::cs2;
using namespace sdk::chams;
struct Memory {
    std::unordered_map<std::uintptr_t, unsigned char> bytes;
    static constexpr std::uintptr_t client=0x10000000, engine=0x20000000, network=0x30000000,
        world=0x40000000, chunk=0x50000000, local=0x60000000, target=0x61000000, controller=0x62000000, scene=0x63000000;
    bool disconnect=false; unsigned reads=0;
    template<class T> void Put(std::uintptr_t address, T value) {
        const auto* data = reinterpret_cast<const unsigned char*>(&value);
        for (unsigned i=0; i<sizeof(T); ++i) bytes[address+i]=data[i];
    }
    sdk::players::Memory Reader() {
        return {this, [](void* context, std::uintptr_t address, void* out, std::size_t size) {
            auto& m=*static_cast<Memory*>(context);
            if (address == network + sdk::players::offsets::signOnState && ++m.reads == 2 && m.disconnect) m.Put(address, 3);
            for (std::size_t i=0; i<size; ++i) {
                auto found=m.bytes.find(address+i); if (found==m.bytes.end()) return false;
                static_cast<unsigned char*>(out)[i]=found->second;
            }
            return true;
        }};
    }
};
void EnemyTests() {
    namespace p=sdk::players::offsets;
    Memory m;
    m.Put(m.engine+p::networkClient,m.network); m.Put(m.network+p::signOnState,6);
    m.Put(m.client+p::entitySystem,m.world); m.Put(m.world+0x10,m.chunk);
    m.Put(m.client+p::localPawn,m.local); m.Put(m.client+p::localController,m.controller);
    m.Put(m.controller+p::controllerPawn, std::uint32_t{0x8001});
    for (unsigned i=1;i<=2;++i) {
        const auto pawn=i==1?m.local:m.target, slot=m.chunk+i*p::stride;
        m.Put(slot,pawn); m.Put(slot+0x10,std::uint32_t{0x8000+i}); m.Put(slot+0x30,0u);
        m.Put(pawn+p::identity,slot); m.Put(pawn,m.client+pawnVtable);
    }
    m.Put(m.local+p::team,std::uint8_t{2}); m.Put(m.target+p::team,std::uint8_t{3});
    m.Put(m.target+p::health,100); m.Put(m.target+p::lifeState,std::uint8_t{0});
    m.Put(m.scene+sceneOwner,std::uint32_t{0x8002});
    auto valid=[&] { m.reads=0; return IsEnemy(m.Reader(),m.client,m.engine,m.scene); };
    CHECK(valid());
    m.Put(m.target+p::team,std::uint8_t{2}); CHECK(!valid()); m.Put(m.target+p::team,std::uint8_t{3});
    m.Put(m.target+p::health,0); CHECK(!valid()); m.Put(m.target+p::health,100);
    m.Put(m.target+p::lifeState,std::uint8_t{1}); CHECK(!valid()); m.Put(m.target+p::lifeState,std::uint8_t{0});
    m.Put(m.scene+sceneOwner,std::uint32_t{0x8001}); CHECK(!valid());
    m.Put(m.scene+sceneOwner,std::uint32_t{0x10002}); CHECK(!valid()); // Reused index, wrong serial.
    m.Put(m.scene+sceneOwner,std::uint32_t{0x8002});
    m.Put(m.target,m.client+pawnVtable+8); CHECK(!valid()); m.Put(m.target,m.client+pawnVtable);
    m.disconnect=true; CHECK(!valid()); m.disconnect=false; m.Put(m.network+p::signOnState,6);
    m.Put(m.target+p::identity,std::uintptr_t{0}); CHECK(!valid());
    CHECK(!ResolveBinding().target);
}
struct Call { Mesh* start; int count; std::vector<Mesh> meshes; };
std::vector<Call> calls;
std::atomic<bool> block{false},entered{false},released{false};
unsigned releasedMaterials=0,createLimit=4,throwAt=0;
bool throwEnemy=false;
void* MaterialAt(unsigned i) { return reinterpret_cast<void*>(std::uintptr_t{0x10000}+i*0x100); }
bool Create(Materials& materials) {
    for(unsigned i=0;i<createLimit;++i) materials.slots[i]={MaterialAt(i),MaterialAt(i),true};
    return createLimit==4;
}
void Release(Materials& materials) {
    for(auto& slot:materials.slots) if(slot.binding) { ++releasedMaterials; slot={}; }
}
bool Enemy(std::uintptr_t scene) {
    if(throwEnemy) RaiseException(0xE0424242,0,0,nullptr);
    return scene==1;
}
#pragma optimize("", off)
__declspec(noinline) std::uintptr_t __fastcall Draw(void* desc,void* dx,Mesh* meshes,int count,void* view,void* layer,void* context) {
    CHECK(desc==MaterialAt(10) && dx==MaterialAt(11) && view==MaterialAt(12) && layer==MaterialAt(13) && context==MaterialAt(14));
    Call call{meshes,count,{}};
    if(meshes && count>0 && count<=1024) call.meshes.assign(meshes,meshes+count);
    calls.push_back(std::move(call));
    if(throwAt && calls.size()==throwAt) RaiseException(0xE0424242,0,0,nullptr);
    if(block) { entered=true; while(!released) Sleep(1); }
    return 0x12345678;
}
using DrawFn=std::uintptr_t (__fastcall*)(void*,void*,Mesh*,int,void*,void*,void*);
DrawFn volatile draw=Draw;
std::uintptr_t Invoke(Mesh* mesh,int count) { return draw(MaterialAt(10),MaterialAt(11),mesh,count,MaterialAt(12),MaterialAt(13),MaterialAt(14)); }
#pragma optimize("", on)
bool Fault(Mesh* mesh,int count) {
    __try { Invoke(mesh,count); return false; }
    __except(GetExceptionCode()==0xE0424242?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) { return true; }
}
void HookTests() {
    const Binding binding{reinterpret_cast<void*>(&Draw),Enemy,Create,Release};
    CHECK(MH_Initialize()==MH_OK); CHECK(!hooks::chams::Install({}));
    createLimit=2; CHECK(!hooks::chams::Install(binding)); CHECK(releasedMaterials==2);
    createLimit=4; CHECK(hooks::chams::Install(binding)); CHECK(!hooks::chams::Install(binding));
    std::array<Mesh,5> meshes{};
    for(unsigned i=0;i<meshes.size();++i) { meshes[i].scene=(i==1||i==2||i==4)?1:2; meshes[i].material=MaterialAt(20+i); meshes[i].color=0xAABB0000+i; }
    const auto before=meshes;
    auto unchanged=[&] { CHECK(std::memcmp(meshes.data(),before.data(),sizeof(meshes))==0); };
    auto run=[&] { calls.clear(); CHECK(Invoke(meshes.data(),5)==0x12345678); unchanged(); };
    run(); CHECK(calls.size()==1 && calls[0].meshes[1].material==before[1].material);
    modules::ModuleManager manager; auto& module=manager.Create<modules::Chams>(); manager.Seal();
    CHECK(module.category()=="render" && !module.requestedEnabled());
    Properties::ModeProperty* material=nullptr; Properties::BoolProperty* walls=nullptr;
    unsigned colors=0;
    for(const auto& row:module.settings()) {
        const std::string_view name=row.getName();
        if(name=="Material") material=dynamic_cast<Properties::ModeProperty*>(row.getProp());
        if(name=="Through Walls") walls=dynamic_cast<Properties::BoolProperty*>(row.getProp());
        if(name=="Visible Color" || name=="Hidden Color") { CHECK(dynamic_cast<Properties::ColorProperty*>(row.getProp())); ++colors; }
    }
    CHECK(material && walls && colors==2 && !walls->get());
    module.setEnabled(true); manager.Tick(); run(); CHECK(module.suffix()=="White" && calls.size()==1);
    CHECK(calls[0].meshes[1].material==MaterialAt(0) && calls[0].meshes[1].color==0xFF00FF00);
    CHECK(calls[0].meshes[0].material==before[0].material);
    walls->set(true); material->set(1); manager.Tick(); run(); CHECK(calls.size()==3 && module.suffix()=="Illuminate");
    CHECK(calls[0].start==meshes.data()+1 && calls[0].count==2 && calls[1].start==meshes.data()+4 && calls[1].count==1);
    CHECK(calls[0].meshes[0].material==MaterialAt(3) && calls[0].meshes[0].color==0xFF0000FF);
    CHECK(calls[2].count==5 && calls[2].meshes[1].material==MaterialAt(2));
    CHECK(calls[2].meshes[3].material==before[3].material && calls[2].meshes[3].color==before[3].color);
    auto saved=nova::json::Value::Object(); manager.Save(saved);
    module.setEnabled(false); manager.Tick(); run(); CHECK(calls.size()==1 && calls[0].meshes[1].material==before[1].material);
    material->set(0); walls->set(false); manager.Load(saved); manager.Tick();
    CHECK(module.requestedEnabled() && material->get()==1 && walls->get());
    for(unsigned fault: {1u,3u}) { calls.clear(); throwAt=fault; CHECK(Fault(meshes.data(),5)); unchanged(); } throwAt=0;
    calls.clear(); Invoke(nullptr,0); CHECK(calls.size()==1);
    calls.clear(); Invoke(meshes.data(),1025); CHECK(calls.size()==1); unchanged();
    for(auto& mesh:meshes) mesh.scene=2;
    calls.clear(); Invoke(meshes.data(),5); CHECK(calls.size()==1); meshes=before;
    block=true; entered=false; released=false; calls.clear();
    std::thread pending([&] { Invoke(meshes.data(),5); });
    for(int i=0;i<1000 && !entered;++i) Sleep(1);
    CHECK(entered && !hooks::chams::Stop(10) && releasedMaterials==2);
    released=true; pending.join(); block=false; unchanged();
    CHECK(calls.size()==2 && calls.back().meshes[1].material==before[1].material);
    CHECK(hooks::chams::Stop(1000)); hooks::chams::Finalize(); CHECK(releasedMaterials==6); manager.Shutdown();
    CHECK(hooks::chams::Install(binding)); hooks::chams::Publish({true});
    throwEnemy=true; run(); CHECK(!hooks::chams::Available() && calls.size()==1); throwEnemy=false;
    CHECK(hooks::chams::Stop(1000)); hooks::chams::Finalize(); CHECK(releasedMaterials==10);
    hooks::chams::Finalize(); CHECK(releasedMaterials==10);
    CHECK(MH_Uninitialize()==MH_OK);
}
}
void TestChams() {
    EnemyTests(); HookTests();
    puts("PASS: Chams enemy identity, mixed batches, material/color restoration, config, native exceptions, callback drain, partial creation and reinstall; no game injection");
}
