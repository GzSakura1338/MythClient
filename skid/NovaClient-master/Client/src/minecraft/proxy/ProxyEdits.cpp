#include "minecraft/proxy/ProxyHooks.h"
#include <cstring>
#include <vector>
#include <string>
namespace nova::minecraft::proxyadapter {
namespace {
const hook::ClassEdits* last=nullptr;int lastCount=0;Version lastVersion=Version::Unsupported;
std::vector<hook::ClassEdits> merged;
std::vector<std::vector<hook::Redirect>> redirects;
std::vector<std::vector<hook::Append>> appends;
const hook::StackLoad threadLoad[]{{0x19,0}};
const hook::Append joinRules[]{
    {"run","()V","proxyJoinBegin","(Ljava/lang/Object;)V",threadLoad,1,true},
    {"run","()V","proxyJoinEnd","()V",nullptr,0,false,true}
};
const hook::Redirect transportRule{
    "","","io/netty/bootstrap/Bootstrap","connect","(Ljava/net/InetAddress;I)Lio/netty/channel/ChannelFuture;",
    "proxyConnect","(Ljava/lang/Object;Ljava/net/InetAddress;I)Lio/netty/channel/ChannelFuture;",-1
};
}
const hook::ClassEdits* ExtendEdits(const hook::ClassEdits* base,int& count,Version version){
    if(version==Version::Unsupported)return base;
    // A caller can hand back a shallow composition of our final table. Do not
    // duplicate rules or clear the backing vectors those borrowed rows use.
    bool hasTransport=false,hasJoin=false,hasEnd=false;
    for(int i=0;i<count;++i){
        for(int r=0;r<base[i].count;++r)hasTransport|=std::strcmp(base[i].redirects[r].hook,"proxyConnect")==0;
        for(int a=0;a<base[i].appendCount;++a){
            hasJoin|=std::strcmp(base[i].appends[a].hook,"proxyJoinBegin")==0;
            hasEnd|=std::strcmp(base[i].appends[a].hook,"proxyJoinEnd")==0;
        }
    }
    if(hasTransport&&hasJoin&&hasEnd)return base;
    if(base==last&&count==lastCount&&version==lastVersion){count=int(merged.size());return merged.data();}
    last=base;lastCount=count;lastVersion=version;
    merged.assign(base,base+count);redirects.clear();redirects.resize(count+2);appends.clear();appends.resize(count+2);
    const bool legacy=version==Version::Lunar189||version==Version::Lunar1710;
    const char* network=legacy?"net/minecraft/network/NetworkManager":version==Version::Lunar12111?"net/minecraft/class_2535":"net/minecraft/network/Connection";
    const char* connector=legacy?"net/minecraft/client/multiplayer/GuiConnecting$1":version==Version::Lunar12111?"net/minecraft/class_412$1":"net/minecraft/client/gui/screens/ConnectScreen$1";
    auto add=[&](const char* name,bool transport){
        size_t index=0;while(index<merged.size()&&std::strcmp(merged[index].internalName,name)!=0)++index;
        if(index==merged.size())merged.push_back({name,nullptr,0});
        auto& e=merged[index];
        if(transport){auto& values=redirects[index];if(e.count)values.assign(e.redirects,e.redirects+e.count);
            auto rule=transportRule;
            if(version==Version::Lunar189){rule.method="createNetworkManagerAndConnect";rule.methodDesc="(Ljava/net/InetAddress;IZ)Lnet/minecraft/network/NetworkManager;";}
            else if(version==Version::Lunar1710){rule.method="provideLanClient";rule.methodDesc="(Ljava/net/InetAddress;I)Lnet/minecraft/network/NetworkManager;";}
            else if(version==Version::Lunar12111){rule.method="method_52271";rule.methodDesc="(Ljava/net/InetSocketAddress;Lnet/minecraft/class_12239;Lnet/minecraft/class_2535;)Lio/netty/channel/ChannelFuture;";}
            else if(version==Version::Minecraft263){rule.method="connect";rule.methodDesc="(Ljava/net/InetSocketAddress;Lnet/minecraft/server/network/EventLoopGroupHolder;Lnet/minecraft/network/Connection;)Lio/netty/channel/ChannelFuture;";}
            else {rule.method=version==Version::Forge1201?"m_290025_":"connect";rule.methodDesc="(Ljava/net/InetSocketAddress;ZLnet/minecraft/network/Connection;)Lio/netty/channel/ChannelFuture;";}
            values.push_back(rule);e.redirects=values.data();e.count=int(values.size());
        }else{auto& values=appends[index];if(e.appendCount)values.assign(e.appends,e.appends+e.appendCount);
            values.insert(values.end(),std::begin(joinRules),std::end(joinRules));e.appends=values.data();e.appendCount=int(values.size());
        }
    };
    add(network,true);add(connector,false);count=int(merged.size());return merged.data();
}
}
