#pragma once
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <cmath>

namespace nova::module::invmove::access {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
namespace mapping {
#define NOVA_INVMOVE_MAPPING
#define NOVA_INVMOVE(id,owner,name,desc) inline constexpr Member id{owner,name,desc};
#include "minecraft/compat/mappings/Lunar189.inc"
#undef NOVA_INVMOVE
#undef NOVA_INVMOVE_MAPPING
}
struct View { bool open=false,chat=false,inventory=false,chest=false,jumping=false; int speedLevel=0; };
inline bool Read(View& out){
    Frame f;if(!f||!f.player())return false;
    auto screen=f.get(f.client(),mapping::screen);out.open=screen!=nullptr;
    if(screen){out.chat=f.instance(screen,"net.minecraft.client.gui.GuiChat");out.inventory=f.instance(screen,"net.minecraft.client.gui.inventory.GuiInventory");out.chest=f.instance(screen,"net.minecraft.client.gui.inventory.GuiChest");}
    out.jumping=f.get<bool>(f.player(),mapping::jumping);
    auto effect=f.call(f.player(),mapping::effect,f.stat(mapping::speed));
    if(effect)out.speedLevel=1+f.call<int>(effect,mapping::amplifier);
    return f.check();
}
inline void ClearJump(){Frame f;if(f)f.set(f.get(f.get(f.client(),mapping::options),mapping::jumpKey),mapping::pressed,false);}
struct PacketFacts {bool move=false,inventory=false,clicked=false,openAttempt=false,click=false,close=false,open=false;};
inline bool ReadPacket(jobject packet,PacketFacts& out){
    Frame f;if(!f||!packet)return false;
    out.move=f.instance(packet,"net.minecraft.network.play.client.C03PacketPlayer");if(out.move)return f.check();
    auto screen=f.get(f.client(),mapping::screen);
    if(f.instance(packet,"net.minecraft.network.play.client.C0EPacketClickWindow")){
        out.click=true;
        out.inventory=true;
        out.clicked=f.instance(screen,"net.minecraft.client.gui.inventory.GuiInventory")&&f.call<int>(packet,mapping::clickMode)<1&&f.call(packet,mapping::clickItem)!=nullptr;
    }else if(f.instance(packet,"net.minecraft.network.play.client.C0DPacketCloseWindow"))out.inventory=out.close=true;
    else if(f.instance(packet,"net.minecraft.network.play.client.C16PacketClientStatus")){
        out.inventory=f.text(f.call(f.call(packet,mapping::status),{"java.lang.Enum","name","()Ljava/lang/String;"}))=="OPEN_INVENTORY_ACHIEVEMENT";
        out.open=out.inventory;
    }else if(f.instance(packet,"net.minecraft.network.play.client.C0BPacketEntityAction")){
        out.open=f.text(f.call(f.call(packet,mapping::entityAction),{"java.lang.Enum","name","()Ljava/lang/String;"}))=="OPEN_INVENTORY";
    }else if(!f.instance(screen,"net.minecraft.client.gui.inventory.GuiChest")){
        auto world=f.get(f.client(),mapping::world);if(!world)return false;
        if(f.instance(packet,"net.minecraft.network.play.client.C08PacketPlayerBlockPlacement")){
            auto pos=f.call(packet,mapping::placePos);
            auto block=f.call(f.call(world,mapping::state,pos),mapping::block);
            out.openAttempt=f.instance(block,"net.minecraft.block.BlockChest")||f.instance(block,"net.minecraft.block.BlockEnderChest");
        }else if(f.instance(packet,"net.minecraft.network.play.client.C02PacketUseEntity")){
            const auto action=f.text(f.call(f.call(packet,mapping::action),{"java.lang.Enum","name","()Ljava/lang/String;"}));
            auto entity=f.call(packet,mapping::entity,world);
            if(action!="ATTACK"&&entity&&!f.env->IsSameObject(entity,f.player())){
                if(f.instance(entity,"net.minecraft.entity.player.EntityPlayer"))
                    out.openAttempt=!f.call(f.call(f.client(),mapping::connection),mapping::info,f.call(entity,mapping::uuid));
                else out.openAttempt=!f.get<bool>(entity,mapping::dead)&&
                    std::abs(f.get<double>(entity,mapping::x)-f.get<double>(entity,mapping::lastX))<.03&&
                    std::abs(f.get<double>(entity,mapping::y)-f.get<double>(entity,mapping::lastY))<.03&&
                    std::abs(f.get<double>(entity,mapping::z)-f.get<double>(entity,mapping::lastZ))<.03&&
                    std::abs(f.get<double>(entity,mapping::mx))<.03&&std::abs(f.get<double>(entity,mapping::my))<.03&&std::abs(f.get<double>(entity,mapping::mz))<.03;
            }
        }
    }
    return f.check();
}
}
