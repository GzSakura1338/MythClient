#include "minecraft/accounts/AccountPortraits.h"
#include "ui/skin/SkinTextures.h"
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <map>

namespace {
std::map<std::string,std::uint64_t> handles;
int publications=0;
}
namespace nova::ui::skins {
std::uint64_t Find(const std::string& key){return handles.contains(key)?handles.at(key):0;}
std::uint64_t Store(const std::string& key,int width,int height,std::vector<std::uint32_t> pixels){
    assert(width>0&&width==height&&pixels.size()==size_t(width)*height);
    bool opaque=false;for(auto pixel:pixels)opaque|=(pixel>>24)!=0;assert(opaque);
    ++publications;return handles[key]=publications;
}
}
int main() {
    using namespace nova::minecraft::accounts;
    nova::accounts::Snapshot view;
    view.rows.push_back({"fixture","Notch","069a79f444e94726a5befca90e38aaf5"});
    const auto deadline=GetTickCount64()+20000;
    while(!view.rows[0].portrait&&GetTickCount64()<deadline){portraits::Update(view);Sleep(30);}
    if(!view.rows[0].portrait){portraits::Stop();portraits::WaitStopped(10000);puts("FAIL: portrait download/decode timed out");return 1;}
    for(int i=0;i<10;++i)portraits::Update(view);
    assert(publications==1);
    view.rows.push_back({"pending","fixture","00000000000000000000000000000001"});
    portraits::Update(view);portraits::Stop();assert(portraits::WaitStopped(10000));
    portraits::Update(view);assert(publications==1);
    puts("PASS: real Lunar skin HTTP + WIC decode, cached handle reuse, shutdown prevents late publication");
}
