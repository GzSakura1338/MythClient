#include "minecraft/compat/minecraft263/Minecraft263Game.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include <array>
#include <algorithm>

namespace nova::compat::mc263 {
using namespace names;
using access::Frame;
namespace {
constexpr names::Member size{"java.util.List","size","()I"};
constexpr names::Member get{"java.util.List","get","(I)Ljava/lang/Object;"};
constexpr names::Member toString{"java.lang.Object","toString","()Ljava/lang/String;"};
jobject PlayerInventory(Frame& f) { return f.call(f.player(),Player_getInventory); }
std::string ItemId(Frame& f,jobject item) {
    auto registry=f.stat(BuiltInRegistries_Field_ITEM);
    return f.text(f.call(f.call(registry,Registry_getKey,item),Identifier_getPath));
}
java::KnownItem Known(const std::string& id) {
    using K=java::KnownItem;
    static const std::pair<const char*,K> table[]={{"golden_axe",K::GoldenAxe},{"slime_ball",K::SlimeBall},{"stick",K::Stick},
        {"enchanted_golden_apple",K::EnchantedGoldenApple},{"golden_apple",K::GoldenApple},{"end_crystal",K::EndCrystal},
        {"totem_of_undying",K::TotemOfUndying},{"compass",K::Compass},{"water_bucket",K::WaterBucket},{"lava_bucket",K::LavaBucket},
        {"arrow",K::Arrow},{"snowball",K::Snowball},{"egg",K::Egg},{"fishing_rod",K::FishingRod},{"wooden_axe",K::WoodenAxe},
        {"stone_axe",K::StoneAxe},{"iron_axe",K::IronAxe},{"diamond_axe",K::DiamondAxe},{"wheat_seeds",K::WheatSeeds},
        {"beetroot_seeds",K::BeetrootSeeds},{"melon_seeds",K::MelonSeeds},{"pumpkin_seeds",K::PumpkinSeeds},{"flint_and_steel",K::FlintAndSteel},
        {"bow",K::Bow},{"ender_pearl",K::EnderPearl},{"fire_charge",K::FireCharge},{"enchanting_table",K::EnchantingTableBlock},
        {"cobweb",K::CobwebBlock},{"stone",K::StoneBlock},{"oak_log",K::OakLogBlock},{"dirt",K::DirtBlock},
        {"wind_charge",K::WindCharge},{"elytra",K::Elytra},{"firework_rocket",K::FireworkRocket},{"mace",K::Mace}};
    for (auto& entry:table) if(id==entry.first)return entry.second;
    return K::Nothing;
}
}
bool ReadItemFacts(jobject stack, java::ItemFacts& out) {
    out={}; if (!stack) return true; Frame f;
    out.empty=f.call<bool>(stack,ItemStack_isEmpty); if (!f || out.empty)return f.check();
    out.count=f.call<int>(stack,ItemStack_getCount); out.maxStack=f.call<int>(stack,ItemInstance_getMaxStackSize);
    out.damageValue=f.call<int>(stack,ItemStack_getDamageValue);out.maxDamage=f.call<int>(stack,ItemStack_getMaxDamage);
    auto item=f.call(stack,ItemStack_getItem); const auto id=ItemId(f,item);out.item=Known(id);
    auto tag=[&](names::Member m,unsigned flag){if(f.call<bool>(stack,TypedInstance_is,f.stat(m)))out.kinds|=flag;};
    tag(ItemTags_Field_SWORDS,java::kItemSword);tag(ItemTags_Field_AXES,java::kItemAxe);
    tag(ItemTags_Field_PICKAXES,java::kItemPickaxe);tag(ItemTags_Field_SHOVELS,java::kItemShovel);
    auto kind=[&](const char* type,unsigned flag){if(f.instance(item,type))out.kinds|=flag;};
    kind(BlockItem,java::kItemBlock);kind(BowItem,java::kItemBow);kind(CrossbowItem,java::kItemCrossbow);
    kind(ArrowItem,java::kItemArrow);kind(FishingRodItem,java::kItemFishingRod);kind(PlayerHeadItem,java::kItemHead);
    if(id=="book"||id=="enchanted_book")out.kinds|=java::kItemBook;
    if(id=="experience_bottle")out.kinds|=java::kItemBottle;
    if(id=="firework_rocket")out.kinds|=java::kItemFirework;
    auto component=[&](names::Member m){return f.call(stack,DataComponentHolder_get,f.stat(m));};
    auto equipment=component(DataComponents_Field_EQUIPPABLE);
    if(equipment){
        auto slot=f.call(equipment,Equippable_slot);
        auto name=f.text(f.call(slot,EquipmentSlot_getName));
        if(name=="head")out.armorSlot=java::kArmorHead;
        if(name=="chest")out.armorSlot=java::kArmorChest;
        if(name=="legs")out.armorSlot=java::kArmorLegs;
        if(name=="feet")out.armorSlot=java::kArmorFeet;
        if(out.armorSlot!=java::kArmorNone)out.kinds|=java::kItemArmor;
    }
    using M=java::KnownMaterial;
    const std::pair<const char*,M> materials[]={{"leather_",M::Leather},{"chainmail_",M::Chain},{"iron_",M::Iron},
        {"golden_",M::Gold},{"diamond_",M::Diamond},{"netherite_",M::Netherite}};
    for(auto& m:materials)if(id.starts_with(m.first))out.material=m.second;
    auto modifiers=component(DataComponents_Field_ATTRIBUTE_MODIFIERS);
    if(modifiers && (IsEaseCation() || IsNeoForge12110())){
        auto entries=f.call(modifiers,easecation::names::Attributes);
        auto damage=f.stat(Attributes_Field_ATTACK_DAMAGE),hand=f.stat(EquipmentSlot_Field_MAINHAND);
        double add=0,baseMultiplier=0,totalMultiplier=1;
        const int size=f.call<int>(entries,{"java.util.List","size","()I"});
        for(int i=0;i<size&&f.check();++i){
            Frame item(f.env);auto entry=item.call(entries,{"java.util.List","get","(I)Ljava/lang/Object;"},i);
            auto attribute=item.call(entry,easecation::names::Attribute);
            if(!item.call<bool>(attribute,{"java.lang.Object","equals","(Ljava/lang/Object;)Z"},damage)||
                !item.call<bool>(item.call(entry,easecation::names::ModifierSlot),easecation::names::SlotMatches,hand))continue;
            auto modifier=item.call(entry,easecation::names::Modifier);
            double amount=item.call<double>(modifier,easecation::names::Amount);
            int operation=item.call<int>(item.call(modifier,easecation::names::Operation),{"java.lang.Enum","ordinal","()I"});
            if(operation==0)add+=amount;else if(operation==1)baseMultiplier+=amount;else if(operation==2)totalMultiplier*=1+amount;
            if(!item.check())return false;
        }
        out.swordDamage=static_cast<float>(add*(1+baseMultiplier)*totalMultiplier);
    }else if(modifiers)out.swordDamage=static_cast<float>(f.call<double>(modifiers,ItemAttributeModifiers_compute,
        f.stat(Attributes_Field_ATTACK_DAMAGE),0.0,f.stat(EquipmentSlot_Field_MAINHAND)));
    auto tool=component(DataComponents_Field_TOOL);
    if(tool){
        const auto block=(out.kinds&java::kItemAxe)?Blocks_Field_OAK_LOG:(out.kinds&java::kItemShovel)?Blocks_Field_DIRT:Blocks_Field_STONE;
        auto state=f.call(f.stat(block),Block_defaultBlockState);
        out.destroySpeed=f.call<float>(stack,ItemStack_getDestroySpeed,state);
    }
    auto enchantments=component(DataComponents_Field_ENCHANTMENTS);
    if(enchantments){
        auto it=f.call(f.call(enchantments,ItemEnchantments_keySet),{"java.lang.Iterable","iterator","()Ljava/util/Iterator;"});
        while(f.call<bool>(it,{"java.util.Iterator","hasNext","()Z"})){
            auto holder=f.call(it,{"java.util.Iterator","next","()Ljava/lang/Object;"});
            int level=f.call<int>(enchantments,ItemEnchantments_getLevel,holder);
            const std::pair<names::Member,int*> ench[]={{Enchantments_Field_SHARPNESS,&out.sharpness},{Enchantments_Field_KNOCKBACK,&out.knockback},
                {Enchantments_Field_PUNCH,&out.punch},{Enchantments_Field_POWER,&out.power},{Enchantments_Field_INFINITY,&out.infinity},
                {Enchantments_Field_FLAME,&out.flame},{Enchantments_Field_EFFICIENCY,&out.efficiency},{Enchantments_Field_PROTECTION,&out.protection},
                {Enchantments_Field_QUICK_CHARGE,&out.quickCharge},{Enchantments_Field_MULTISHOT,&out.multishot},{Enchantments_Field_PIERCING,&out.piercing}};
            for(auto& e:ench)if(f.call<bool>(holder,Holder_is_2,f.stat(e.first)))*e.second=level;
            if(holder)f.env->DeleteLocalRef(holder);
        }
        out.sharpnessBonus=out.sharpness>0?0.5f*out.sharpness+0.5f:0.0f;
    }
    if(out.kinds&java::kItemBlock){
        auto block=f.call(item,BlockItem_getBlock);
        if(f.instance(block,FlowerBlock))out.blockKinds|=java::kBlockFlower;
        if(f.instance(block,BushBlock))out.blockKinds|=java::kBlockBush;
        if(f.instance(block,CropBlock))out.blockKinds|=java::kBlockCrop;
        if(f.instance(block,SlabBlock))out.blockKinds|=java::kBlockSlab;
        if(f.instance(block,SkullBlock))out.kinds|=java::kItemSkullBlock;
        auto state=f.call(block,Block_defaultBlockState);
        out.blacklistedBlock=f.call<bool>(state,BlockBehaviour_BlockStateBase_canBeReplaced_3)||out.blockKinds!=0||
            id=="tnt"||id=="chest"||id=="trapped_chest"||id=="ender_chest"||id=="crafting_table"||id=="furnace"||
            id=="sand"||id=="red_sand"||id=="gravel"||id=="cobweb"||id=="ladder"||id=="scaffolding"||
            id.ends_with("_door")||id.ends_with("_trapdoor")||id.ends_with("_button")||id.ends_with("_pressure_plate");
    }
    return f.check();
}
bool ItemDisplayName(jobject stack,std::string& out){Frame f;auto text=f.call(f.call(stack,ItemStack_getHoverName),Component_getString);out=f.text(text);return text&&f.check();}
bool InventoryStack(int index,bool armor,jobject& out){
    out=nullptr;if(index<0||index>=(armor?4:36))return false;Frame f;
    auto stack=f.call(PlayerInventory(f),Inventory_getItem,index+(armor?36:0));out=f.keep(stack);return out!=nullptr;
}
bool InventoryOffhand(int index,jobject& out){out=nullptr;if(index!=0)return false;Frame f;out=f.keep(f.call(PlayerInventory(f),Inventory_getItem,40));return out!=nullptr;}
int InventoryCount(bool armor){Frame f;return f.player()?(armor?4:36):0;}
bool InventoryHasItem(java::KnownItem target){
    Frame f;auto inventory=PlayerInventory(f);if(!inventory)return false;
    for(int i=0;i<41;++i){auto stack=f.call(inventory,Inventory_getItem,i);if(!f.call<bool>(stack,ItemStack_isEmpty)&&Known(ItemId(f,f.call(stack,ItemStack_getItem)))==target)return true;if(stack)f.env->DeleteLocalRef(stack);}
    return false;
}
jobject CurrentScreen(){Frame f;return f.keep(mc263::Screen(f.env,f.client()));}
bool ScreenIsContainerScreen(jobject s){Frame f;return f.instance(s,AbstractContainerScreen);}
bool ScreenIsInventoryScreen(jobject s){Frame f;return f.instance(s,InventoryScreen);}
bool ScreenMenu(jobject s,jobject& out){Frame f;out=nullptr;if(!f.instance(s,AbstractContainerScreen))return false;out=f.keep(f.call(s,AbstractContainerScreen_getMenu));return out!=nullptr;}
bool MenuContainerId(jobject menu,int& out){Frame f;if(!menu)return false;out=f.get<int>(menu,AbstractContainerMenu_Field_containerId);return f.check();}
bool MenuIsPlayerInventory(jobject menu){Frame f;return f.instance(menu,InventoryMenu);}
bool MenuIsChest(jobject menu){Frame f;return f.instance(menu,ChestMenu);}
bool MenuRowCount(jobject menu,int& out){Frame f;if(!f.instance(menu,ChestMenu))return false;out=f.call<int>(menu,ChestMenu_getRowCount);return f.check();}
bool MenuSlotStack(jobject menu,int slot,jobject& out){
    Frame f;out=nullptr;auto slots=f.get(menu,AbstractContainerMenu_Field_slots);
    if(!slots||slot<0||slot>=f.call<int>(slots,size))return false;
    out=f.keep(f.call(f.call(slots,get,slot),Slot_getItem));return out!=nullptr;
}
bool PlayerMenu(jobject& out){Frame f;out=f.keep(f.get(f.player(),Player_Field_inventoryMenu));return out!=nullptr;}
bool ScreenTitle(jobject screen,std::string& out){Frame f;auto value=f.call(f.call(screen,Screen_getTitle),Component_getString);out=f.text(value);return value&&f.check();}
bool TranslatedText(const char* key,std::string& out){Frame f;if(!f||!key)return false;auto str=f.env->NewStringUTF(key);auto component=f.staticCall(Component_translatable,str);out=f.text(f.call(component,Component_getString));return component&&f.check();}
bool ClickMenuSlot(int id,int slot,int button,java::ClickKind click){
    Frame f;auto mc=f.client();auto player=f.player();if(!mc||!player)return false;
    // Refuse stale menus: a delayed action must not mutate a newly-opened container.
    auto menu=f.get(player,Player_Field_containerMenu);
    if(!menu||f.get<int>(menu,AbstractContainerMenu_Field_containerId)!=id)return false;
    auto input=f.stat(click==java::ClickKind::QuickMove?ContainerInput_Field_QUICK_MOVE:click==java::ClickKind::Swap?ContainerInput_Field_SWAP:
        click==java::ClickKind::Throw?ContainerInput_Field_THROW:ContainerInput_Field_PICKUP);
    return f.invoke(f.get(mc,Minecraft_Field_gameMode),MultiPlayerGameMode_handleContainerInput,id,slot,button,input,player);
}
bool CloseContainer(){Frame f;return f.invoke(f.player(),LocalPlayer_closeContainer);}
bool SendContainerClose(int id){Frame f;return mc263::SendPacket(f.create(ServerboundContainerClosePacket_Ctor,id));}
bool SelectedSlot(int& out){Frame f;auto inv=PlayerInventory(f);out=f.call<int>(inv,Inventory_getSelectedSlot);return inv&&f.check();}
bool SetSelectedSlot(int slot){if(slot<0||slot>8)return false;Frame f;return f.invoke(PlayerInventory(f),Inventory_setSelectedSlot,slot);}
bool HoldingWeapon(){Frame f;java::ItemFacts facts;return mc263::ReadItemFacts(f.call(f.player(),LivingEntity_getMainHandItem),facts)&&!facts.empty&&(facts.kinds&(java::kItemSword|java::kItemAxe));}
bool HoldingSword(){Frame f;return f.call<bool>(f.call(f.player(),LivingEntity_getMainHandItem),TypedInstance_is,f.stat(ItemTags_Field_SWORDS));}
bool HoldingBlockItem(){Frame f;return f.instance(f.call(f.call(f.player(),LivingEntity_getMainHandItem),ItemStack_getItem),BlockItem);}
bool HotbarSlotPlaceable(int slot){Frame f;jobject stack;java::ItemFacts facts;return InventoryStack(slot,false,stack)&&mc263::ReadItemFacts(stack,facts)&&!facts.empty&&(facts.kinds&java::kItemBlock)&&!facts.blacklistedBlock;}
int HotbarSlotCount(int slot){Frame f;jobject stack;return InventoryStack(slot,false,stack)?f.call<int>(stack,ItemStack_getCount):0;}
bool InventoryScreenOpen(){Frame f;return f.instance(mc263::Screen(f.env,f.client()),InventoryScreen);}
bool ClickHoveredSlot(bool quickMove){
    Frame f;auto screen=mc263::Screen(f.env,f.client());if(!f.instance(screen,AbstractContainerScreen))return false;
    auto slot=f.get(screen,AbstractContainerScreen_Field_hoveredSlot);if(!slot)return false;
    auto menu=f.call(screen,AbstractContainerScreen_getMenu);
    return mc263::ClickMenuSlot(f.get<int>(menu,AbstractContainerMenu_Field_containerId),f.get<int>(slot,Slot_Field_index),0,quickMove?java::ClickKind::QuickMove:java::ClickKind::Pickup);
}
bool AddNightVision(){Frame f;auto effect=f.create(MobEffectInstance_Ctor_5,f.stat(MobEffects_Field_NIGHT_VISION),600,0,JNI_FALSE,JNI_FALSE,JNI_FALSE);return f.call<bool>(f.player(),LivingEntity_addEffect,effect)&&f.check();}
bool RemoveNightVision(){Frame f;auto p=f.player();if(!p)return false;f.call<bool>(p,LivingEntity_removeEffect,f.stat(MobEffects_Field_NIGHT_VISION));return f.check();}
}
