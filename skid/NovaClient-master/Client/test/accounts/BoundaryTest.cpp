#include "minecraft/accounts/AccountValidation.h"
#include "minecraft/accounts/AccountNavigation.h"
#include "contracts/AccountService.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <thread>
#include <vector>

int main() {
    using namespace nova::minecraft::accounts;
    assert(ValidName("Player_0123") && ValidName("1234567890123456"));
    for(const auto* name:{"","ab","12345678901234567","has space","../account","new\nline","中文"})assert(!ValidName(name));
    assert(ValidToken(std::string(8192,'a')));
    for(const auto& token:{std::string(8193,'a'),std::string("short"),std::string("aaaaaaaaaaaaaaaa\r\nX-Header: value"),std::string("aaaaaaaaaaaaaaaa\0suffix",23)})assert(!ValidToken(token));
    assert(NormalizeUuid("069A79F4-44E9-4726-A5BE-FCA90E38AAF5")=="069a79f444e94726a5befca90e38aaf5");
    assert(DashedUuid("069a79f444e94726a5befca90e38aaf5")=="069a79f4-44e9-4726-a5be-fca90e38aaf5");
    assert(NormalizeUuid("bad_uuid").empty());
    using namespace nova::accounts;
    Snapshot offline;offline.supported=true;offline.rows.push_back({"offline:test","LocalPlayer","test",Source::Offline,true});
    assert(CanOpenGameMenu("SINGLEPLAYER",offline,"test","LocalPlayer"));
    assert(CanOpenGameMenu("MULTIPLAYER",offline,"test","LocalPlayer"));
    for(auto screen:{"REALMS","COSMETICS","XBOX_FRIENDS",""})assert(!CanOpenGameMenu(screen,offline,"test","LocalPlayer"));
    assert(!CanOpenGameMenu("SINGLEPLAYER",offline,"different","LocalPlayer"));
    assert(!CanOpenGameMenu("SINGLEPLAYER",offline,"test","Different"));
    offline.busy=true;assert(!CanOpenGameMenu("MULTIPLAYER",offline,"test","LocalPlayer"));offline.busy=false;
    offline.rows[0].source=Source::Current;assert(!CanOpenGameMenu("MULTIPLAYER",offline,"test","LocalPlayer"));
    offline.rows[0].source=Source::Microsoft;assert(CanOpenGameMenu("MULTIPLAYER",offline,"test","LocalPlayer"));
    offline.rows[0].invalid=true;assert(!CanOpenGameMenu("MULTIPLAYER",offline,"test","LocalPlayer"));
    assert(!Submit(Action::Token,"temporary-test-token"));
    Snapshot snapshot;snapshot.supported=true;snapshot.currentName="Player";Publish(snapshot);
    std::atomic<int> accepted=0;std::vector<std::thread> threads;
    for(int i=0;i<16;++i)threads.emplace_back([&]{if(Submit(Action::Token,"temporary-test-token"))++accepted;});
    for(auto& t:threads)t.join();assert(accepted==1);
    assert(Read().currentName=="Player" && Read().rows.empty());
    Request request{Action::Offline,{}};assert(Take(request));assert(request.action==Action::Token && request.value=="temporary-test-token");assert(!Take(request));
    snapshot.busy=true;Publish(snapshot);assert(!Submit(Action::Offline,"Player"));
    assert(Submit(Action::Cancel));assert(Take(request)&&request.action==Action::Cancel);
    assert(!SubmitNative(Action::Token,"not-allowed"));
    assert(SubmitNative(Action::Microsoft));assert(!SubmitNative(Action::Microsoft));
    assert(Take(request)&&request.action==Action::Microsoft);
    RequestOpen();assert(ConsumeOpen());assert(!ConsumeOpen());RequestOpen();
    snapshot.busy=false;Publish(snapshot);assert(Submit(Action::Token,"pending-test-token"));Stop();
    assert(!Take(request) && !Submit(Action::Microsoft) && !SubmitNative(Action::Microsoft) && !ConsumeOpen());Publish(snapshot);assert(!Read().supported);
    std::puts("PASS: names, UUIDs, token/header bounds, concurrent request exclusion, shutdown queue disposal");
}
