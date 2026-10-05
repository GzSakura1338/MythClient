// Independent Minecraft 1.21.10 / NeoForge 21.10.64 profile, Java 21, GLFW/OpenGL.
// Recaf: minecraft-client-patched-21.10.64.jar (Mojang runtime namespace).
#if defined(NOVA_NEOFORGE12110_PROFILE)
namespace nova::compat {
inline constexpr GameMapping NeoForge12110Mapping{
    "getInstance","player","Lnet/minecraft/client/player/LocalPlayer;",
    "level","Lnet/minecraft/client/multiplayer/ClientLevel;",
    "screen","Lnet/minecraft/client/gui/screens/Screen;"
};
}
#elif defined(NOVA_TEAMS_MAPPING)
NOVA_TEAM(Format,"net.minecraft.world.scores.PlayerTeam","getColor","()Lnet/minecraft/ChatFormatting;")
NOVA_TEAM(Color,"net.minecraft.ChatFormatting","getColor","()Ljava/lang/Integer;")
NOVA_TEAM(Value,"java.lang.Integer","intValue","()I")
#elif defined(NOVA_BACKTRACK_MAPPING)
NOVA_BT("dx","net.minecraft.network.protocol.game.ClientboundMoveEntityPacket","xa","S")
NOVA_BT("dy","net.minecraft.network.protocol.game.ClientboundMoveEntityPacket","ya","S")
NOVA_BT("dz","net.minecraft.network.protocol.game.ClientboundMoveEntityPacket","za","S")
NOVA_BT("syncPosition","net.minecraft.network.protocol.game.ClientboundEntityPositionSyncPacket","values","Lnet/minecraft/world/entity/PositionMoveRotation;")
#elif defined(NOVA_NEOFORGE12110_TYPE)
NOVA_NEOFORGE12110_TYPE("net.minecraft.world.entity.animal.fish.WaterAnimal","net.minecraft.world.entity.animal.WaterAnimal")
NOVA_NEOFORGE12110_TYPE("net.minecraft.world.entity.boss.enderdragon.EnderDragonPart","net.minecraft.world.entity.boss.EnderDragonPart")
NOVA_NEOFORGE12110_TYPE("net.minecraft.world.entity.monster.cubemob.Slime","net.minecraft.world.entity.monster.Slime")
NOVA_NEOFORGE12110_TYPE("net.minecraft.world.entity.npc.villager.AbstractVillager","net.minecraft.world.entity.npc.AbstractVillager")
NOVA_NEOFORGE12110_TYPE("net.minecraft.resources.Identifier","net.minecraft.resources.ResourceLocation")
NOVA_NEOFORGE12110_TYPE("net.minecraft.world.inventory.ContainerInput","net.minecraft.world.inventory.ClickType")
NOVA_NEOFORGE12110_TYPE("net.minecraft.core.TypedInstance","net.minecraft.world.item.ItemStack")
NOVA_NEOFORGE12110_TYPE("net.minecraft.world.item.ItemInstance","net.minecraft.world.item.ItemStack")
#elif defined(NOVA_NEOFORGE12110_ALIAS)
NOVA_NEOFORGE12110_ALIAS("net.minecraft.client.Minecraft","setScreenAndShow","(Lnet/minecraft/client/gui/screens/Screen;)V","net.minecraft.client.Minecraft","setScreen","(Lnet/minecraft/client/gui/screens/Screen;)V")
NOVA_NEOFORGE12110_ALIAS("net.minecraft.client.multiplayer.MultiPlayerGameMode","handleContainerInput","(IIILnet/minecraft/world/inventory/ContainerInput;Lnet/minecraft/world/entity/player/Player;)V","net.minecraft.client.multiplayer.MultiPlayerGameMode","handleInventoryMouseClick","(IIILnet/minecraft/world/inventory/ClickType;Lnet/minecraft/world/entity/player/Player;)V")
NOVA_NEOFORGE12110_ALIAS("net.minecraft.network.protocol.game.ClientboundSetEntityMotionPacket","id","()I","net.minecraft.network.protocol.game.ClientboundSetEntityMotionPacket","getId","()I")
NOVA_NEOFORGE12110_ALIAS("net.minecraft.network.protocol.game.ClientboundSetEntityMotionPacket","movement","()Lnet/minecraft/world/phys/Vec3;","net.minecraft.network.protocol.game.ClientboundSetEntityMotionPacket","getMovement","()Lnet/minecraft/world/phys/Vec3;")
#elif defined(NOVA_NEOFORGE12110_MEMBER)
NOVA_NEOFORGE12110_MEMBER(Screen,"net.minecraft.client.Minecraft","screen","Lnet/minecraft/client/gui/screens/Screen;")
NOVA_NEOFORGE12110_MEMBER(SwingPacket,"net.minecraft.network.protocol.game.ServerboundSwingPacket","<init>","(Lnet/minecraft/world/InteractionHand;)V")
NOVA_NEOFORGE12110_MEMBER(Swing,"net.minecraft.world.entity.LivingEntity","swing","(Lnet/minecraft/world/InteractionHand;)V")
NOVA_NEOFORGE12110_MEMBER(Camera,"net.minecraft.client.renderer.GameRenderer","getMainCamera","()Lnet/minecraft/client/Camera;")
NOVA_NEOFORGE12110_MEMBER(CameraReady,"net.minecraft.client.Camera","isInitialized","()Z")
NOVA_NEOFORGE12110_MEMBER(CameraPos,"net.minecraft.client.Camera","getPosition","()Lnet/minecraft/world/phys/Vec3;")
NOVA_NEOFORGE12110_MEMBER(CameraYaw,"net.minecraft.client.Camera","getYRot","()F")
NOVA_NEOFORGE12110_MEMBER(CameraPitch,"net.minecraft.client.Camera","getXRot","()F")
NOVA_NEOFORGE12110_MEMBER(BobView,"net.minecraft.client.Options","bobView","()Lnet/minecraft/client/OptionInstance;")
NOVA_NEOFORGE12110_MEMBER(Fov,"net.minecraft.client.renderer.GameRenderer","getFov","(Lnet/minecraft/client/Camera;FZ)F")
NOVA_NEOFORGE12110_MEMBER(Projection,"net.minecraft.client.renderer.GameRenderer","getProjectionMatrix","(F)Lorg/joml/Matrix4f;")
NOVA_NEOFORGE12110_MEMBER(Pick,"net.minecraft.client.renderer.GameRenderer","pick","(F)V")
NOVA_NEOFORGE12110_MEMBER(ImagePixels,"com.mojang.blaze3d.platform.NativeImage","pixels","J")
NOVA_NEOFORGE12110_MEMBER(Attributes,"net.minecraft.world.item.component.ItemAttributeModifiers","modifiers","()Ljava/util/List;")
NOVA_NEOFORGE12110_MEMBER(Attribute,"net.minecraft.world.item.component.ItemAttributeModifiers$Entry","attribute","()Lnet/minecraft/core/Holder;")
NOVA_NEOFORGE12110_MEMBER(Modifier,"net.minecraft.world.item.component.ItemAttributeModifiers$Entry","modifier","()Lnet/minecraft/world/entity/ai/attributes/AttributeModifier;")
NOVA_NEOFORGE12110_MEMBER(ModifierSlot,"net.minecraft.world.item.component.ItemAttributeModifiers$Entry","slot","()Lnet/minecraft/world/entity/EquipmentSlotGroup;")
NOVA_NEOFORGE12110_MEMBER(SlotMatches,"net.minecraft.world.entity.EquipmentSlotGroup","test","(Lnet/minecraft/world/entity/EquipmentSlot;)Z")
NOVA_NEOFORGE12110_MEMBER(Amount,"net.minecraft.world.entity.ai.attributes.AttributeModifier","amount","()D")
NOVA_NEOFORGE12110_MEMBER(Operation,"net.minecraft.world.entity.ai.attributes.AttributeModifier","operation","()Lnet/minecraft/world/entity/ai/attributes/AttributeModifier$Operation;")
#elif defined(NOVA_NEOFORGE12110_EDITS)
constexpr Append nfClientAppends[]{
    {"getTickTargetMillis","(F)F","timerMillis","(F)F",nullptr,0,false,false,true},
    {"tick","()V","gameTick","()V",nullptr,0,true},
    {"tick","()V","postGameTick","()V",nullptr,0,false,true},
    {"handleKeybinds","()V","interact","()V",nullptr,0,true},
    {"runTick","(Z)V","renderFrame","()V",nullptr,0,true}
};
constexpr Redirect nfAttackRedirects[]{
    {"attack","(Lnet/minecraft/world/entity/Entity;)V","net/minecraft/world/entity/player/Player","setDeltaMovement","(Lnet/minecraft/world/phys/Vec3;)V","attackVelocity","(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/phys/Vec3;)V",-1},
    {"attack","(Lnet/minecraft/world/entity/Entity;)V","net/minecraft/world/entity/player/Player","setSprinting","(Z)V","attackSprint","(Lnet/minecraft/world/entity/player/Player;Z)V",-1},
    {"attack","(Lnet/minecraft/world/entity/Entity;)V","net/minecraft/world/entity/player/Player","getYRot","()F","attackYaw","(Lnet/minecraft/world/entity/player/Player;)F",-1}
};
constexpr Redirect nfGameModeRedirects[]{
    {"ensureHasSentCarriedItem","()V","net/minecraft/world/entity/player/Inventory","getSelectedSlot","()I","spoofSlot","(Lnet/minecraft/world/entity/player/Inventory;)I",-1},
    {"lambda$useItem$6","(Lnet/minecraft/world/InteractionHand;Lnet/minecraft/world/entity/player/Player;Lorg/apache/commons/lang3/mutable/MutableObject;I)Lnet/minecraft/network/protocol/Packet;","net/minecraft/world/entity/player/Player","getYRot","()F","useYaw","(Lnet/minecraft/world/entity/player/Player;)F",-1},
    {"lambda$useItem$6","(Lnet/minecraft/world/InteractionHand;Lnet/minecraft/world/entity/player/Player;Lorg/apache/commons/lang3/mutable/MutableObject;I)Lnet/minecraft/network/protocol/Packet;","net/minecraft/world/entity/player/Player","getXRot","()F","usePitch","(Lnet/minecraft/world/entity/player/Player;)F",-1}
};
constexpr Redirect nfVelocityRedirects[]{
    {"handleSetEntityMotion","(Lnet/minecraft/network/protocol/game/ClientboundSetEntityMotionPacket;)V","net/minecraft/world/entity/Entity","lerpMotion","(Lnet/minecraft/world/phys/Vec3;)V","velocity","(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/world/phys/Vec3;)V",-1}
};
constexpr Redirect nfCameraRedirects[]{
    {"setup","(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/world/entity/Entity;ZZF)V","net/minecraft/world/entity/Entity","getViewYRot","(F)F","freeLookViewYaw","(Lnet/minecraft/world/entity/Entity;F)F",-1},
    {"setup","(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/world/entity/Entity;ZZF)V","net/minecraft/world/entity/Entity","getViewXRot","(F)F","freeLookViewPitch","(Lnet/minecraft/world/entity/Entity;F)F",-1}
};
constexpr Redirect nfHandRedirects[]{
    {"tick","()V","net/minecraft/client/player/LocalPlayer","getMainHandItem","()Lnet/minecraft/world/item/ItemStack;","visualPlayerHand","(Lnet/minecraft/client/player/LocalPlayer;)Lnet/minecraft/world/item/ItemStack;",-1},
    {"evaluateWhichHandsToRender","(Lnet/minecraft/client/player/LocalPlayer;)Lnet/minecraft/client/renderer/ItemInHandRenderer$HandRenderSelection;","net/minecraft/client/player/LocalPlayer","getMainHandItem","()Lnet/minecraft/world/item/ItemStack;","visualPlayerHand","(Lnet/minecraft/client/player/LocalPlayer;)Lnet/minecraft/world/item/ItemStack;",-1}
};
constexpr Append nfFovAppends[]{
    {"render","(Lnet/minecraft/client/DeltaTracker;Z)V","resetWorldCamera","()V",nullptr,0,true},
    {"pick","(F)V","cursorPickEnd","(F)V",mc263PickLoads,1,false,true},
    {"getFov","(Lnet/minecraft/client/Camera;FZ)F","keepFov","(F)F",nullptr,0,false,false,true}
};
constexpr StackLoad nfWorldCameraLoads[]{{0x19,4},{0x19,5},{0x19,6}};
constexpr Append nfWorldCameraAppends[]{
    // Slot 7 is the culling projection; ESP uses the actual projection at slot 6.
    {"renderLevel","(Lcom/mojang/blaze3d/resource/GraphicsResourceAllocator;Lnet/minecraft/client/DeltaTracker;ZLnet/minecraft/client/Camera;Lorg/joml/Matrix4f;Lorg/joml/Matrix4f;Lorg/joml/Matrix4f;Lcom/mojang/blaze3d/buffers/GpuBufferSlice;Lorg/joml/Vector4f;Z)V",
     "worldCamera","(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V",nfWorldCameraLoads,3,true}
};
constexpr Guard nfPlayerGuards[]{
    {"tick","()V","playerTick","(Lnet/minecraft/client/player/LocalPlayer;)Z",1,true}
};
constexpr ClassEdits neoforge12110Edits[]{
    {"net/minecraft/client/renderer/LevelRenderer",nullptr,0,nullptr,0,nullptr,0,nfWorldCameraAppends,1},
    {"net/minecraft/client/MouseHandler",mc263FreeLookMouseRedirects,1},
    {"net/minecraft/client/Camera",nfCameraRedirects,2},
    {"net/minecraft/client/renderer/GameRenderer",nullptr,0,nullptr,0,nullptr,0,nfFovAppends,static_cast<int>(std::size(nfFovAppends))},
    {"net/minecraft/world/entity/player/Inventory",mc263SlotRedirects,2},
    {"net/minecraft/client/renderer/ItemInHandRenderer",nfHandRedirects,2},
    {"net/minecraft/client/Minecraft",mc263BreakerClientRedirects,static_cast<int>(std::size(mc263BreakerClientRedirects)),nullptr,0,mc263ClientGuards,1,nfClientAppends,static_cast<int>(std::size(nfClientAppends))},
    {"net/minecraft/client/player/LocalPlayer",mc263PlayerRedirects,static_cast<int>(std::size(mc263PlayerRedirects)),nullptr,0,nfPlayerGuards,1,mc263PlayerAppends,static_cast<int>(std::size(mc263PlayerAppends))},
    {"net/minecraft/client/player/KeyboardInput",nullptr,0,nullptr,0,nullptr,0,mc263InputAppends,1},
    {"net/minecraft/world/entity/Entity",mc263EntityRedirects,3},
    {"net/minecraft/world/entity/LivingEntity",mc263LivingRedirects,2},
    {"net/minecraft/world/entity/player/Player",nfAttackRedirects,3},
    {"net/minecraft/world/item/Item",mc263ItemRedirects,2},
    {"net/minecraft/client/multiplayer/MultiPlayerGameMode",nfGameModeRedirects,3,nullptr,0,nullptr,0,mc263GameModeAppends,1},
    {"net/minecraft/client/renderer/entity/LivingEntityRenderer",nullptr,0,nullptr,0,nullptr,0,mc263RendererAppends,1},
    {"net/minecraft/network/Connection",mc263ConnectionRedirects,1,nullptr,0,mc263ConnectionGuards,1},
    {"net/minecraft/client/multiplayer/ClientPacketListener",nfVelocityRedirects,1}
};
#else
#error "Select a NeoForge 1.21.10 mapping section"
#endif
