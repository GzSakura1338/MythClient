// Verified against Lunar 1.7.10 bake 10d9b7f1/aed3ca70, 2026-09-21.
// Repeated inclusion is intentional. Do not substitute Lunar189 signatures.
#if defined(NOVA_MLG_WATER_MAPPING)
NOVA_MLG(block,"net.minecraft.world.World","getBlock","(III)Lnet/minecraft/block/Block;")
NOVA_MLG(still,"net.minecraft.init.Blocks","water","Lnet/minecraft/block/Block;")
NOVA_MLG(flowing,"net.minecraft.init.Blocks","flowing_water","Lnet/minecraft/block/BlockLiquid;")
NOVA_MLG(metadata,"net.minecraft.world.World","getBlockMetadata","(III)I")
NOVA_MLG(lava,"net.minecraft.entity.Entity","handleLavaMovement","()Z")
NOVA_MLG(evaporates,"net.minecraft.world.WorldProvider","isHellWorld","Z")
#elif defined(NOVA_LUNAR1710_PROFILE)
namespace nova::compat {
inline constexpr GameMapping Lunar1710Mapping{
    "getMinecraft", "thePlayer", "Lnet/minecraft/client/entity/EntityClientPlayerMP;",
    "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;",
    "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;"
};
}
#elif defined(NOVA_LUNAR1710_EDITS)
const Redirect lunar1710EntityRedirects[] = {
    {"moveFlying", "(FFF)V", "net/minecraft/entity/Entity", "rotationYaw", "F",
     "strafeYaw", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4}
};
const Redirect lunar1710LivingRedirects[] = {
    {"jump", "()V", "net/minecraft/entity/EntityLivingBase", "rotationYaw", "F",
     "jumpYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"func_110146_f", "(FF)F", "net/minecraft/entity/EntityLivingBase", "rotationYaw", "F",
     "bodyYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
};
const StackLoad lunar1710LookLoads[] = {{0x19,0},{0x17,1}};
const Append lunar1710LookAppends[] = {
    {"getLook","(F)Lnet/minecraft/util/Vec3;","look17",
     "(Lnet/minecraft/util/Vec3;Lnet/minecraft/entity/EntityLivingBase;F)Lnet/minecraft/util/Vec3;",
     lunar1710LookLoads,2,false,false,true}
};
const Redirect lunar1710PlayerRedirects[] = {
    {"onUpdate", "()V", "net/minecraft/client/entity/EntityClientPlayerMP", "sendMotionUpdates", "()V",
     "walking17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)V", -1},
    {"sendMotionUpdates", "()V", "net/minecraft/client/entity/EntityClientPlayerMP", "rotationYaw", "F",
     "motionYaw17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)F", -1, -1, 0xB4},
    {"sendMotionUpdates", "()V", "net/minecraft/client/entity/EntityClientPlayerMP", "rotationPitch", "F",
     "motionPitch17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)F", -1, -1, 0xB4},
    {"sendMotionUpdates", "()V", "net/minecraft/client/entity/EntityClientPlayerMP", "onGround", "Z",
     "motionGround17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)Z", -1, -1, 0xB4}
};
const Redirect lunar1710InputRedirects[] = {
    {"onLivingUpdate", "()V", "net/minecraft/util/MovementInput", "updatePlayerMoveState", "()V",
     "input", "(Lnet/minecraft/util/MovementInput;)V", -1},
    {"onLivingUpdate", "()V", "net/minecraft/client/entity/AbstractClientPlayer", "onLivingUpdate", "()V",
     "livingUpdate", "(Lnet/minecraft/client/entity/AbstractClientPlayer;)V", -1}
};
const Guard lunar1710PlayerGuards[] = {
    {"onUpdate", "()V", "playerTick17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)Z", 1}
};
const Append lunar1710PlayerAppends[] = {
    {"onUpdate", "()V", "postPlayerTick", "()V", nullptr, 0, false, true}
};
const Append lunar1710MinecraftAppends[] = {
    {"runTick", "()V", "gameTick", "()V", nullptr, 0, true},
    {"runTick", "()V", "postGameTick", "()V", nullptr, 0, false, true}
};
const Redirect lunar1710MinecraftRedirects[] = {
    {"runTick","()V","net/minecraft/client/Minecraft","clickMouse","()V",
     "freeCamClick","(Lnet/minecraft/client/Minecraft;)V",-1},
    {"runTick", "()V", "net/minecraft/client/renderer/EntityRenderer", "getMouseOver", "(F)V",
     "clientRotation", "(Lnet/minecraft/client/renderer/EntityRenderer;F)V", -1},
    {"runGameLoop", "()V", "net/minecraft/client/renderer/EntityRenderer", "updateCameraAndRender", "(F)V",
     "renderFrame17", "(Lnet/minecraft/client/renderer/EntityRenderer;F)V", -1},
    {"runTick", "()V", "net/minecraft/client/settings/GameSettings", "chatVisibility",
     "Lnet/minecraft/entity/player/EntityPlayer$EnumChatVisibility;", "prePlayerInteract",
     "(Lnet/minecraft/client/settings/GameSettings;)Lnet/minecraft/entity/player/EntityPlayer$EnumChatVisibility;", -1, -1, 0xB4},
    {"runTick", "()V", "net/minecraft/client/settings/KeyBinding", "setKeyBindState", "(IZ)V",
     "blockMouseKey", "(IZ)V", -1},
    {"runTick", "()V", "net/minecraft/client/Minecraft", "rightClickMouse", "()V",
     "blockVanillaUse", "(Lnet/minecraft/client/Minecraft;)V", -1},
    // Also matches Lunar's generated MixinExtras bridge for the item-use fallback.
    {"*", "", "net/minecraft/client/multiplayer/PlayerControllerMP", "sendUseItem",
     "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z",
     "blockManualSwordUse", "(Lnet/minecraft/client/multiplayer/PlayerControllerMP;Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z", -1},
    {"runTick", "()V", "net/minecraft/client/multiplayer/PlayerControllerMP", "onStoppedUsingItem",
     "(Lnet/minecraft/entity/player/EntityPlayer;)V", "blockVanillaRelease",
     "(Lnet/minecraft/client/multiplayer/PlayerControllerMP;Lnet/minecraft/entity/player/EntityPlayer;)V", -1},
    {"runTick", "()V", "net/minecraft/client/Minecraft", "sendClickBlockToController", "(Z)V",
     "blockVanillaHitBlock", "(Lnet/minecraft/client/Minecraft;Z)V", -1}
};
const Redirect lunar1710BlockRenderRedirects[] = {
    {"updateEquippedItem", "()V", "net/minecraft/entity/player/InventoryPlayer", "getCurrentItem", "()Lnet/minecraft/item/ItemStack;",
     "visualInventoryHand", "(Lnet/minecraft/entity/player/InventoryPlayer;)Lnet/minecraft/item/ItemStack;", -1},
    {"renderItemInFirstPerson", "(F)V", "net/minecraft/client/entity/AbstractClientPlayer",
     "getItemInUseCount", "()I", "blockUseCount",
     "(Lnet/minecraft/client/entity/AbstractClientPlayer;)I", -1}
};
const Redirect lunar1710AttackRedirects[] = {
    {"attackTargetEntityWithCurrentItem", "(Lnet/minecraft/entity/Entity;)V",
     "net/minecraft/entity/player/EntityPlayer", "rotationYaw", "F",
     "attackYaw", "(Lnet/minecraft/entity/player/EntityPlayer;)F", -1, -1, 0xB4},
    {"attackTargetEntityWithCurrentItem", "(Lnet/minecraft/entity/Entity;)V",
     "net/minecraft/entity/player/EntityPlayer", "motionX", "D",
     "attackMotionX", "(Lnet/minecraft/entity/player/EntityPlayer;D)V", -1, -1, 0xB5},
    {"attackTargetEntityWithCurrentItem", "(Lnet/minecraft/entity/Entity;)V",
     "net/minecraft/entity/player/EntityPlayer", "motionZ", "D",
     "attackMotionZ", "(Lnet/minecraft/entity/player/EntityPlayer;D)V", -1, -1, 0xB5},
    {"attackTargetEntityWithCurrentItem", "(Lnet/minecraft/entity/Entity;)V",
     "net/minecraft/entity/player/EntityPlayer", "setSprinting", "(Z)V",
     "attackSprinting", "(Lnet/minecraft/entity/player/EntityPlayer;Z)V", -1}
};
const Redirect lunar1710ItemRedirects[] = {
    {"getMovingObjectPositionFromPlayer", "(Lnet/minecraft/world/World;Lnet/minecraft/entity/player/EntityPlayer;Z)Lnet/minecraft/util/MovingObjectPosition;",
     "net/minecraft/entity/player/EntityPlayer", "rotationYaw", "F",
     "itemYaw", "(Lnet/minecraft/entity/player/EntityPlayer;)F", -1, -1, 0xB4},
    {"getMovingObjectPositionFromPlayer", "(Lnet/minecraft/world/World;Lnet/minecraft/entity/player/EntityPlayer;Z)Lnet/minecraft/util/MovingObjectPosition;",
     "net/minecraft/entity/player/EntityPlayer", "rotationPitch", "F",
     "itemPitch", "(Lnet/minecraft/entity/player/EntityPlayer;)F", -1, -1, 0xB4}
};
const Redirect lunar1710RenderRedirects[] = {
    {"doRender", "(Lnet/minecraft/entity/EntityLivingBase;DDDFF)V",
     "net/minecraft/entity/EntityLivingBase", "rotationYawHead", "F",
     "headYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"doRender", "(Lnet/minecraft/entity/EntityLivingBase;DDDFF)V",
     "net/minecraft/entity/EntityLivingBase", "prevRotationYawHead", "F",
     "previousHeadYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"doRender", "(Lnet/minecraft/entity/EntityLivingBase;DDDFF)V",
     "net/minecraft/entity/EntityLivingBase", "rotationPitch", "F",
     "renderPitch", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"doRender", "(Lnet/minecraft/entity/EntityLivingBase;DDDFF)V",
     "net/minecraft/entity/EntityLivingBase", "prevRotationPitch", "F",
     "previousRenderPitch", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4}
};
const Redirect lunar1710NetworkRedirects[] = {
    {"scheduleOutboundPacket", "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V",
     "net/minecraft/network/NetworkManager", "dispatchPacket",
     "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V",
     "packetOut", "(Lnet/minecraft/network/NetworkManager;Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V", -1},
    {"flushOutboundQueue", "()V", "net/minecraft/network/NetworkManager", "dispatchPacket",
     "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V",
     "packetOut", "(Lnet/minecraft/network/NetworkManager;Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V", -1},
    {"processReceivedPackets", "()V", "net/minecraft/network/Packet", "processPacket", "(Lnet/minecraft/network/INetHandler;)V",
     "packetIn", "(Lnet/minecraft/network/Packet;Lnet/minecraft/network/INetHandler;)V", -1}
};
const Redirect lunar1710VelocityRedirects[] = {
    {"handleEntityVelocity", "(Lnet/minecraft/network/play/server/S12PacketEntityVelocity;)V",
     "net/minecraft/entity/Entity", "setVelocity", "(DDD)V",
     "appliedVelocity", "(Lnet/minecraft/entity/Entity;DDD)V", -1}
};
const StackLoad lunar1710AttackTargetLoads[] = {{0x19, 2}};
const Append lunar1710ControllerAppends[] = {
    {"attackEntity", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/entity/Entity;)V",
     "attackTarget", "(Ljava/lang/Object;)V", lunar1710AttackTargetLoads, 1, true}
};
const Redirect lunar1710SlotRedirects[] = {
    {"getCurrentItem", "()Lnet/minecraft/item/ItemStack;", "net/minecraft/entity/player/InventoryPlayer", "currentItem", "I",
     "spoofSlot", "(Lnet/minecraft/entity/player/InventoryPlayer;)I", -1, -1, 0xB4}
};
const Redirect lunar1710ControllerSlotRedirects[] = {
    {"sendUseItem", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z", "net/minecraft/entity/player/InventoryPlayer", "currentItem", "I",
     "spoofSlot", "(Lnet/minecraft/entity/player/InventoryPlayer;)I", -1, -1, 0xB4},
    {"syncCurrentPlayItem", "()V", "net/minecraft/entity/player/InventoryPlayer", "currentItem", "I",
     "spoofSlot", "(Lnet/minecraft/entity/player/InventoryPlayer;)I", -1, -1, 0xB4}
};
const StackLoad lunar1710FovLoads[] = {{0x15, 2}};
const StackLoad lunar1710PickLoads[] = {{0x17, 1}};
const Append lunar1710FovAppends[] = {
    {"getMouseOver", "(F)V", "cursorPickBegin", "()V", nullptr, 0, true},
    {"getMouseOver", "(F)V", "cursorPickEnd", "(F)V", lunar1710PickLoads, 1, false, true},
    {"getFOVModifier", "(FZ)F", "keepFov", "(FZ)F", lunar1710FovLoads, 1, false, false, true}
};
const Redirect lunar1710FreeLookCameraRedirects[] = {
    {"*", "", "net/minecraft/client/entity/EntityClientPlayerMP", "setAngles", "(FF)V", "freeCamAngles", "(Lnet/minecraft/entity/Entity;FF)V", -1},
    {"orientCamera", "(F)V", "net/minecraft/entity/EntityLivingBase", "rotationYaw", "F", "freeLookYaw17", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"orientCamera", "(F)V", "net/minecraft/entity/EntityLivingBase", "rotationPitch", "F", "freeLookPitch17", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"orientCamera", "(F)V", "net/minecraft/entity/EntityLivingBase", "prevRotationYaw", "F", "freeLookPreviousYaw17", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"orientCamera", "(F)V", "net/minecraft/entity/EntityLivingBase", "prevRotationPitch", "F", "freeLookPreviousPitch17", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"updateCameraAndRender", "(F)V", "net/minecraft/client/Minecraft", "inGameHasFocus", "Z", "freeLookMouse", "(Lnet/minecraft/client/Minecraft;)Z", -1, 0, 0xB4},
};
const StackLoad lunar1710FreeLookManagerLoads[] = {{0x19, 0}};
const Append lunar1710FreeLookManagerAppends[] = {
    {"cacheActiveRenderInfo", "(Lnet/minecraft/world/World;Lnet/minecraft/client/renderer/texture/TextureManager;Lnet/minecraft/client/gui/FontRenderer;Lnet/minecraft/entity/EntityLivingBase;Lnet/minecraft/entity/Entity;Lnet/minecraft/client/settings/GameSettings;F)V",
     "freeLookBillboard", "(Lnet/minecraft/client/renderer/entity/RenderManager;)V", lunar1710FreeLookManagerLoads, 1, false, true}
};
const StackLoad lunar1710FreeLookParticleLoads[] = {{0x15, 1}};
const Append lunar1710FreeLookParticleAppends[] = {
    {"updateRenderInfo", "(Lnet/minecraft/entity/player/EntityPlayer;Z)V", "freeLookParticles", "(Z)V", lunar1710FreeLookParticleLoads, 1, false, true}
};
const ClassEdits lunar1710Edits[] = {
    {"net/minecraft/client/renderer/entity/RenderManager", nullptr, 0, nullptr, 0, nullptr, 0, lunar1710FreeLookManagerAppends, 1},
    {"net/minecraft/client/renderer/ActiveRenderInfo", nullptr, 0, nullptr, 0, nullptr, 0, lunar1710FreeLookParticleAppends, 1},
    {"net/minecraft/client/renderer/EntityRenderer", lunar1710FreeLookCameraRedirects, static_cast<int>(std::size(lunar1710FreeLookCameraRedirects)), nullptr, 0, nullptr, 0, lunar1710FovAppends, static_cast<int>(std::size(lunar1710FovAppends))},
    {"net/minecraft/entity/player/InventoryPlayer", lunar1710SlotRedirects, 1},
    {"net/minecraft/client/multiplayer/PlayerControllerMP", lunar1710ControllerSlotRedirects, static_cast<int>(std::size(lunar1710ControllerSlotRedirects)), nullptr, 0, nullptr, 0, lunar1710ControllerAppends, 1},
    {"net/minecraft/client/network/NetHandlerPlayClient", lunar1710VelocityRedirects, 1},
    {"net/minecraft/entity/Entity", lunar1710EntityRedirects, 1},
    {"net/minecraft/entity/EntityLivingBase", lunar1710LivingRedirects, 2, nullptr,0,nullptr,0,lunar1710LookAppends,1},
    {"net/minecraft/client/entity/EntityPlayerSP", lunar1710InputRedirects, 2},
    {"net/minecraft/client/entity/EntityClientPlayerMP", lunar1710PlayerRedirects, static_cast<int>(std::size(lunar1710PlayerRedirects)),
     nullptr, 0, lunar1710PlayerGuards, 1, lunar1710PlayerAppends, 1},
    {"net/minecraft/client/Minecraft", lunar1710MinecraftRedirects, static_cast<int>(std::size(lunar1710MinecraftRedirects)), nullptr, 0, nullptr, 0, lunar1710MinecraftAppends, 2},
    {"net/minecraft/client/renderer/ItemRenderer", lunar1710BlockRenderRedirects, 2},
    {"net/minecraft/entity/player/EntityPlayer", lunar1710AttackRedirects, 4},
    {"net/minecraft/item/Item", lunar1710ItemRedirects, 2},
    {"net/minecraft/client/renderer/entity/RendererLivingEntity", lunar1710RenderRedirects, 4},
    {"net/minecraft/network/NetworkManager", lunar1710NetworkRedirects, 3}
};
#elif defined(NOVA_LUNAR1710_ALIAS)
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S03PacketTimeUpdate", "getTotalWorldTime", "()J", "net.minecraft.network.play.server.S03PacketTimeUpdate", "func_149366_c", "()J")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S02PacketChat", "getChatComponent", "()Lnet/minecraft/util/IChatComponent;", "net.minecraft.network.play.server.S02PacketChat", "func_148915_c", "()Lnet/minecraft/util/IChatComponent;")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S14PacketEntity", "entityId", "I", "net.minecraft.network.play.server.S14PacketEntity", "field_149074_a", "I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S14PacketEntity", "posX", "B", "net.minecraft.network.play.server.S14PacketEntity", "field_149072_b", "B")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S14PacketEntity", "posY", "B", "net.minecraft.network.play.server.S14PacketEntity", "field_149073_c", "B")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S14PacketEntity", "posZ", "B", "net.minecraft.network.play.server.S14PacketEntity", "field_149070_d", "B")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S18PacketEntityTeleport", "entityId", "I", "net.minecraft.network.play.server.S18PacketEntityTeleport", "field_149458_a", "I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S18PacketEntityTeleport", "posX", "I", "net.minecraft.network.play.server.S18PacketEntityTeleport", "field_149456_b", "I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S18PacketEntityTeleport", "posY", "I", "net.minecraft.network.play.server.S18PacketEntityTeleport", "field_149457_c", "I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S18PacketEntityTeleport", "posZ", "I", "net.minecraft.network.play.server.S18PacketEntityTeleport", "field_149454_d", "I")
NOVA_LUNAR1710_ALIAS("net.minecraft.util.RegistryNamespaced", "getNameForObject", "(Ljava/lang/Object;)Ljava/lang/Object;", "net.minecraft.util.RegistryNamespaced", "getNameForObject", "(Ljava/lang/Object;)Ljava/lang/String;")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.client.C07PacketPlayerDigging", "getStatus", "()Lnet/minecraft/network/play/client/C07PacketPlayerDigging$Action;", "net.minecraft.network.play.client.C07PacketPlayerDigging", "getDiggedBlockStatus", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.entity.Entity", "isInLava", "()Z", "net.minecraft.entity.Entity", "handleLavaMovement", "()Z")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.client.C03PacketPlayer", "onGround", "Z", "net.minecraft.network.play.client.C03PacketPlayer", "field_149474_g", "Z")
NOVA_LUNAR1710_ALIAS("net.minecraft.util.MovingObjectPosition", "sideHit", "Lnet/minecraft/util/EnumFacing;", "net.minecraft.util.MovingObjectPosition", "sideHit", "I")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.renderer.EntityRenderer", "updateCameraAndRender", "(FJ)V", "net.minecraft.client.renderer.EntityRenderer", "updateCameraAndRender", "(F)V")
NOVA_LUNAR1710_ALIAS("net.minecraft.potion.PotionEffect", "<init>", "(IIIZZ)V", "net.minecraft.potion.PotionEffect", "<init>", "(IIIZ)V")
NOVA_LUNAR1710_ALIAS("net.minecraft.world.World", "getBlockState", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/block/state/IBlockState;", "net.minecraft.world.World", "getBlock", "(III)Lnet/minecraft/block/Block;")
NOVA_LUNAR1710_ALIAS("net.minecraft.entity.Entity", "getLook", "(F)Lnet/minecraft/util/Vec3;", "net.minecraft.entity.EntityLivingBase", "getLook", "(F)Lnet/minecraft/util/Vec3;")
NOVA_LUNAR1710_ALIAS("net.minecraft.block.Block", "canCollideCheck", "(IZ)Z", "net.minecraft.block.Block", "canStopRayTrace", "(IZ)Z")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.client.C03PacketPlayer$C06PacketPlayerPosLook", "<init>", "(DDDFFZ)V", "net.minecraft.network.play.client.C03PacketPlayer$C06PacketPlayerPosLook", "<init>", "(DDDDFFZ)V")
NOVA_LUNAR1710_ALIAS("net.minecraft.entity.Entity", "getDisplayName", "()Lnet/minecraft/util/IChatComponent;", "net.minecraft.entity.Entity", "getFormattedCommandSenderName", "()Lnet/minecraft/util/IChatComponent;")
NOVA_LUNAR1710_ALIAS("net.minecraft.entity.Entity", "getName", "()Ljava/lang/String;", "net.minecraft.entity.Entity", "getCommandSenderName", "()Ljava/lang/String;")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.entity.EntityPlayerSP", "onUpdateWalkingPlayer", "()V", "net.minecraft.client.entity.EntityClientPlayerMP", "sendMotionUpdates", "()V")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S12PacketEntityVelocity", "getEntityID", "()I", "net.minecraft.network.play.server.S12PacketEntityVelocity", "func_149412_c", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S12PacketEntityVelocity", "getMotionX", "()I", "net.minecraft.network.play.server.S12PacketEntityVelocity", "func_149411_d", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S12PacketEntityVelocity", "getMotionY", "()I", "net.minecraft.network.play.server.S12PacketEntityVelocity", "func_149410_e", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S12PacketEntityVelocity", "getMotionZ", "()I", "net.minecraft.network.play.server.S12PacketEntityVelocity", "func_149409_f", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.settings.KeyBinding", "isKeyDown", "()Z", "net.minecraft.client.settings.KeyBinding", "getIsKeyPressed", "()Z")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.client.C08PacketPlayerBlockPlacement", "<init>", "(Lnet/minecraft/item/ItemStack;)V", "net.minecraft.network.play.client.C08PacketPlayerBlockPlacement", "<init>", "(IIIILnet/minecraft/item/ItemStack;FFF)V")
NOVA_LUNAR1710_ALIAS("net.minecraft.world.IWorldNameable", "getDisplayName", "()Lnet/minecraft/util/IChatComponent;", "net.minecraft.inventory.IInventory", "getInventoryName", "()Ljava/lang/String;")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.entity.EntityPlayerSP", "closeScreen", "()V", "net.minecraft.client.entity.EntityClientPlayerMP", "closeScreen", "()V")
NOVA_LUNAR1710_ALIAS("net.minecraft.block.Block", "blockRegistry", "Lnet/minecraft/util/RegistryNamespacedDefaultedByKey;", "net.minecraft.block.Block", "blockRegistry", "Lnet/minecraft/util/RegistryNamespaced;")
NOVA_LUNAR1710_ALIAS("net.minecraft.init.Blocks", "water", "Lnet/minecraft/block/BlockStaticLiquid;", "net.minecraft.init.Blocks", "water", "Lnet/minecraft/block/Block;")
NOVA_LUNAR1710_ALIAS("net.minecraft.init.Blocks", "lava", "Lnet/minecraft/block/BlockStaticLiquid;", "net.minecraft.init.Blocks", "lava", "Lnet/minecraft/block/Block;")
NOVA_LUNAR1710_ALIAS("net.minecraft.init.Blocks", "cactus", "Lnet/minecraft/block/BlockCactus;", "net.minecraft.init.Blocks", "cactus", "Lnet/minecraft/block/Block;")
NOVA_LUNAR1710_ALIAS("net.minecraft.init.Blocks", "oak_fence", "Lnet/minecraft/block/Block;", "net.minecraft.init.Blocks", "fence", "Lnet/minecraft/block/Block;")
NOVA_LUNAR1710_ALIAS("net.minecraft.init.Blocks", "flowing_water", "Lnet/minecraft/block/BlockDynamicLiquid;", "net.minecraft.init.Blocks", "flowing_water", "Lnet/minecraft/block/BlockLiquid;")
NOVA_LUNAR1710_ALIAS("net.minecraft.init.Blocks", "flowing_lava", "Lnet/minecraft/block/BlockDynamicLiquid;", "net.minecraft.init.Blocks", "flowing_lava", "Lnet/minecraft/block/BlockLiquid;")
NOVA_LUNAR1710_ALIAS("net.minecraft.item.ItemArmor$ArmorMaterial", "LEATHER", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;", "net.minecraft.item.ItemArmor$ArmorMaterial", "CLOTH", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_LUNAR1710_ALIAS("net.minecraft.item.ItemSword", "attackDamage", "F", "net.minecraft.item.ItemSword", "field_150934_a", "F")
NOVA_LUNAR1710_ALIAS("net.minecraft.item.ItemStack", "getItemDamage", "()I", "net.minecraft.item.ItemStack", "getMetadata", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.item.ItemStack", "getMaxDamage", "()I", "net.minecraft.item.ItemStack", "getMaxDurability", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.enchantment.Enchantment", "calcDamageByCreature", "(ILnet/minecraft/entity/EnumCreatureAttribute;)F", "net.minecraft.enchantment.Enchantment", "func_152376_a", "(ILnet/minecraft/entity/EnumCreatureAttribute;)F")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.renderer.ActiveRenderInfo", "PROJECTION", "Ljava/nio/FloatBuffer;", "net.minecraft.client.renderer.ActiveRenderInfo", "projection", "Ljava/nio/FloatBuffer;")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.renderer.ActiveRenderInfo", "MODELVIEW", "Ljava/nio/FloatBuffer;", "net.minecraft.client.renderer.ActiveRenderInfo", "modelview", "Ljava/nio/FloatBuffer;")
NOVA_LUNAR1710_ALIAS("net.minecraft.block.Block", "setBlockBoundsBasedOnState", "(Lnet/minecraft/world/IBlockAccess;Lnet/minecraft/util/BlockPos;)V", "net.minecraft.block.Block", "setBlockBoundsBasedOnState", "(Lnet/minecraft/world/IBlockAccess;III)V")
NOVA_LUNAR1710_ALIAS("net.minecraft.block.Block", "getSelectedBoundingBox", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;)Lnet/minecraft/util/AxisAlignedBB;", "net.minecraft.block.Block", "getSelectedBoundingBoxFromPool", "(Lnet/minecraft/world/World;III)Lnet/minecraft/util/AxisAlignedBB;")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.gui.ScaledResolution", "<init>", "(Lnet/minecraft/client/Minecraft;)V", "net.minecraft.client.gui.ScaledResolution", "<init>", "(Lnet/minecraft/client/Minecraft;II)V")
NOVA_LUNAR1710_ALIAS("net.minecraft.world.World", "getSpawnPoint", "()Lnet/minecraft/util/BlockPos;", "net.minecraft.world.World", "getSpawnPoint", "()Lnet/minecraft/util/ChunkCoordinates;")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S3CPacketUpdateScore", "getPlayerName", "()Ljava/lang/String;", "net.minecraft.network.play.server.S3CPacketUpdateScore", "func_149324_c", "()Ljava/lang/String;")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S3CPacketUpdateScore", "getObjectiveName", "()Ljava/lang/String;", "net.minecraft.network.play.server.S3CPacketUpdateScore", "func_149321_d", "()Ljava/lang/String;")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S3CPacketUpdateScore", "getScoreValue", "()I", "net.minecraft.network.play.server.S3CPacketUpdateScore", "func_149323_e", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S3CPacketUpdateScore", "getScoreAction", "()Lnet/minecraft/network/play/server/S3CPacketUpdateScore$Action;", "net.minecraft.network.play.server.S3CPacketUpdateScore", "func_149322_f", "()I")
NOVA_LUNAR1710_ALIAS("net.minecraft.block.Block", "getCollisionBoundingBox", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;Lnet/minecraft/block/state/IBlockState;)Lnet/minecraft/util/AxisAlignedBB;", "net.minecraft.block.Block", "getCollisionBoundingBoxFromPool", "(Lnet/minecraft/world/World;III)Lnet/minecraft/util/AxisAlignedBB;")
NOVA_LUNAR1710_ALIAS("net.minecraft.world.World", "getTileEntity", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/tileentity/TileEntity;", "net.minecraft.world.World", "getTileEntity", "(III)Lnet/minecraft/tileentity/TileEntity;")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.multiplayer.PlayerControllerMP", "onPlayerRightClick", "(Lnet/minecraft/client/entity/EntityPlayerSP;Lnet/minecraft/client/multiplayer/WorldClient;Lnet/minecraft/item/ItemStack;Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;Lnet/minecraft/util/Vec3;)Z", "net.minecraft.client.multiplayer.PlayerControllerMP", "onPlayerRightClick", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;IIIILnet/minecraft/util/Vec3;)Z")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.entity.EntityPlayerSP", "lastReportedYaw", "F", "net.minecraft.client.entity.EntityClientPlayerMP", "oldRotationYaw", "F")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.entity.EntityPlayerSP", "lastReportedPitch", "F", "net.minecraft.client.entity.EntityClientPlayerMP", "oldRotationPitch", "F")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.client.C07PacketPlayerDigging", "<init>", "(Lnet/minecraft/network/play/client/C07PacketPlayerDigging$Action;Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;)V", "net.minecraft.network.play.client.C07PacketPlayerDigging", "<init>", "(IIIII)V")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S19PacketEntityStatus", "entityId", "I", "net.minecraft.network.play.server.S19PacketEntityStatus", "field_149164_a", "I")
NOVA_LUNAR1710_ALIAS("net.minecraft.network.play.server.S19PacketEntityStatus", "getOpCode", "()B", "net.minecraft.network.play.server.S19PacketEntityStatus", "func_149160_c", "()B")
NOVA_LUNAR1710_ALIAS("net.minecraft.client.entity.EntityPlayerSP", "movementInput", "Lnet/minecraft/util/MovementInput;", "net.minecraft.client.entity.EntityClientPlayerMP", "movementInput", "Lnet/minecraft/util/MovementInput;")
#elif defined(NOVA_LUNAR1710_MEMBER)
NOVA_LUNAR1710_MEMBER(Material, "net.minecraft.block.Block", "getMaterial", "()Lnet/minecraft/block/material/Material;")
NOVA_LUNAR1710_MEMBER(Replaceable, "net.minecraft.block.material.Material", "isReplaceable", "()Z")
NOVA_LUNAR1710_MEMBER(Hardness, "net.minecraft.block.Block", "getBlockHardness", "(Lnet/minecraft/world/World;III)F")
NOVA_LUNAR1710_MEMBER(RelativeHardness, "net.minecraft.block.Block", "getPlayerRelativeBlockHardness", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;III)F")
NOVA_LUNAR1710_MEMBER(Collision, "net.minecraft.block.Block", "getCollisionBoundingBoxFromPool", "(Lnet/minecraft/world/World;III)Lnet/minecraft/util/AxisAlignedBB;")
NOVA_LUNAR1710_MEMBER(Clip, "net.minecraft.block.Block", "collisionRayTrace", "(Lnet/minecraft/world/World;IIILnet/minecraft/util/Vec3;Lnet/minecraft/util/Vec3;)Lnet/minecraft/util/MovingObjectPosition;")
NOVA_LUNAR1710_MEMBER(Damage, "net.minecraft.client.multiplayer.PlayerControllerMP", "onPlayerDamageBlock", "(IIII)V")
NOVA_LUNAR1710_MEMBER(Destroy, "net.minecraft.client.multiplayer.PlayerControllerMP", "onPlayerDestroyBlock", "(IIII)Z")
NOVA_LUNAR1710_MEMBER(ItemBlock, "net.minecraft.item.ItemBlock", "blockInstance", "Lnet/minecraft/block/Block;")
NOVA_LUNAR1710_MEMBER(CanPlace, "net.minecraft.item.ItemBlock", "func_150936_a", "(Lnet/minecraft/world/World;IIIILnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/item/ItemStack;)Z")
NOVA_LUNAR1710_MEMBER(Air, "net.minecraft.world.World", "isAirBlock", "(III)Z")
NOVA_LUNAR1710_MEMBER(Loaded, "net.minecraft.world.World", "blockExists", "(III)Z")
NOVA_LUNAR1710_MEMBER(AreaLoaded, "net.minecraft.world.World", "checkChunksExist", "(IIIIII)Z")
NOVA_LUNAR1710_MEMBER(Flow, "net.minecraft.block.Block", "modifyEntityVelocity", "(Lnet/minecraft/world/World;IIILnet/minecraft/entity/Entity;Lnet/minecraft/util/Vec3;)V")
NOVA_LUNAR1710_MEMBER(TabList, "net.minecraft.client.network.NetHandlerPlayClient", "playerInfoList", "Ljava/util/List;")
NOVA_LUNAR1710_MEMBER(TabName, "net.minecraft.client.gui.GuiPlayerInfo", "name", "Ljava/lang/String;")
NOVA_LUNAR1710_MEMBER(Ping, "net.minecraft.client.gui.GuiPlayerInfo", "responseTime", "I")
NOVA_LUNAR1710_MEMBER(BlockUpdateX, "net.minecraft.network.play.server.S23PacketBlockChange", "field_148887_a", "I")
NOVA_LUNAR1710_MEMBER(BlockUpdateY, "net.minecraft.network.play.server.S23PacketBlockChange", "field_148885_b", "I")
NOVA_LUNAR1710_MEMBER(BlockUpdateZ, "net.minecraft.network.play.server.S23PacketBlockChange", "field_148886_c", "I")
NOVA_LUNAR1710_MEMBER(BlockUpdateBlock, "net.minecraft.network.play.server.S23PacketBlockChange", "field_148883_d", "Lnet/minecraft/block/Block;")
NOVA_LUNAR1710_MEMBER(TeleportX, "net.minecraft.network.play.server.S08PacketPlayerPosLook", "field_148940_a", "D")
NOVA_LUNAR1710_MEMBER(TeleportY, "net.minecraft.network.play.server.S08PacketPlayerPosLook", "field_148938_b", "D")
NOVA_LUNAR1710_MEMBER(TeleportZ, "net.minecraft.network.play.server.S08PacketPlayerPosLook", "field_148939_c", "D")
NOVA_LUNAR1710_MEMBER(AddCollisions, "net.minecraft.block.Block", "addCollisionBoxesToList", "(Lnet/minecraft/world/World;IIILnet/minecraft/util/AxisAlignedBB;Ljava/util/List;Lnet/minecraft/entity/Entity;)V")
NOVA_LUNAR1710_MEMBER(View, "net.minecraft.client.Minecraft", "renderViewEntity", "Lnet/minecraft/entity/EntityLivingBase;")
NOVA_LUNAR1710_MEMBER(Options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_LUNAR1710_MEMBER(Gamma, "net.minecraft.client.settings.GameSettings", "gammaSetting", "F")
NOVA_LUNAR1710_MEMBER(JumpDelay, "net.minecraft.entity.EntityLivingBase", "jumpTicks", "I")
NOVA_LUNAR1710_MEMBER(Motion, "net.minecraft.client.entity.EntityClientPlayerMP", "sendMotionUpdates", "()V")
NOVA_LUNAR1710_MEMBER(LivingUpdate, "net.minecraft.client.entity.EntityPlayerSP", "onLivingUpdate", "()V")
NOVA_LUNAR1710_MEMBER(GetBlock, "net.minecraft.world.World", "getBlock", "(III)Lnet/minecraft/block/Block;")
NOVA_LUNAR1710_MEMBER(BlockMetadata, "net.minecraft.world.World", "getBlockMetadata", "(III)I")
NOVA_LUNAR1710_MEMBER(HitX, "net.minecraft.util.MovingObjectPosition", "blockX", "I")
NOVA_LUNAR1710_MEMBER(HitY, "net.minecraft.util.MovingObjectPosition", "blockY", "I")
NOVA_LUNAR1710_MEMBER(HitZ, "net.minecraft.util.MovingObjectPosition", "blockZ", "I")
NOVA_LUNAR1710_MEMBER(HitSide, "net.minecraft.util.MovingObjectPosition", "sideHit", "I")
NOVA_LUNAR1710_MEMBER(Dispatch, "net.minecraft.network.NetworkManager", "dispatchPacket", "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V")
NOVA_LUNAR1710_MEMBER(Receive, "net.minecraft.network.NetworkManager", "channelRead0", "(Lio/netty/channel/ChannelHandlerContext;Lnet/minecraft/network/Packet;)V")
NOVA_LUNAR1710_MEMBER(RenderManager, "net.minecraft.client.renderer.entity.RenderManager", "instance", "Lnet/minecraft/client/renderer/entity/RenderManager;")
#elif defined(NOVA_LUNAR1710_EXTRA)
NOVA_FIELD(YOffset17, Entity, "yOffset", "F")
NOVA_CLASS(Coordinates17, "net.minecraft.util.ChunkCoordinates")
NOVA_FIELD(HitX17, Hit, "blockX", "I")
NOVA_FIELD(HitY17, Hit, "blockY", "I")
NOVA_FIELD(HitZ17, Hit, "blockZ", "I")
NOVA_FIELD(Box17, Entity, "boundingBox", "Lnet/minecraft/util/AxisAlignedBB;")
NOVA_FIELD(ItemBlock17, BlockItem, "blockInstance", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Fps17, Minecraft, "debugFPS", "I")
NOVA_FIELD(View17, Minecraft, "renderViewEntity", "Lnet/minecraft/entity/EntityLivingBase;")
NOVA_FIELD(ClientNetwork17, NetworkManager, "isClientSide", "Z")
NOVA_FIELD(TileX17, TileEntity, "xCoord", "I")
NOVA_FIELD(TileY17, TileEntity, "yCoord", "I")
NOVA_FIELD(TileZ17, TileEntity, "zCoord", "I")
NOVA_FIELD(SpawnX17, Coordinates17, "posX", "I")
NOVA_FIELD(SpawnZ17, Coordinates17, "posZ", "I")
NOVA_METHOD(BlockActionX17, BlockActionPacket, "getX", "()I")
NOVA_METHOD(BlockActionY17, BlockActionPacket, "getY", "()I")
NOVA_METHOD(BlockActionZ17, BlockActionPacket, "getZ", "()I")
#elif defined(NOVA_LUNAR1710_RUNTIME)
NOVA_CLASS(InventoryInterface, "net.minecraft.inventory.IInventory")
NOVA_CLASS(Settings, "net.minecraft.client.settings.GameSettings")
NOVA_CLASS(KeyBinding, "net.minecraft.client.settings.KeyBinding")
NOVA_CLASS(Player, "net.minecraft.entity.player.EntityPlayer")
NOVA_CLASS(Entity, "net.minecraft.entity.Entity")
NOVA_CLASS(Living, "net.minecraft.entity.EntityLivingBase")
NOVA_CLASS(Abilities, "net.minecraft.entity.player.PlayerCapabilities")
NOVA_CLASS(Stack, "net.minecraft.item.ItemStack")
NOVA_CLASS(Sword, "net.minecraft.item.ItemSword")
NOVA_CLASS(Axe, "net.minecraft.item.ItemAxe")
NOVA_CLASS(Potion, "net.minecraft.potion.Potion")
NOVA_CLASS(Effect, "net.minecraft.potion.PotionEffect")
NOVA_CLASS(Mouse, "org.lwjgl.input.Mouse")
NOVA_CLASS(Keyboard, "org.lwjgl.input.Keyboard")
NOVA_CLASS(Minecraft, "net.minecraft.client.Minecraft")
NOVA_CLASS(Hit, "net.minecraft.util.MovingObjectPosition")
NOVA_CLASS(HitType, "net.minecraft.util.MovingObjectPosition$MovingObjectType")
NOVA_CLASS(World, "net.minecraft.world.World")
NOVA_CLASS(Air, "net.minecraft.block.BlockAir")
NOVA_CLASS(Liquid, "net.minecraft.block.BlockLiquid")
NOVA_CLASS(ContainerScreen, "net.minecraft.client.gui.inventory.GuiContainer")
NOVA_CLASS(Slot, "net.minecraft.inventory.Slot")
NOVA_CLASS(Renderer, "net.minecraft.client.renderer.EntityRenderer")
NOVA_CLASS(MovePacket, "net.minecraft.network.play.client.C03PacketPlayer")
NOVA_CLASS(PosRotPacket, "net.minecraft.network.play.client.C03PacketPlayer$C06PacketPlayerPosLook")
NOVA_CLASS(Vec, "net.minecraft.util.Vec3")
NOVA_CLASS(Box, "net.minecraft.util.AxisAlignedBB")
NOVA_CLASS(Chat, "net.minecraft.util.IChatComponent")
NOVA_CLASS(List, "java.util.List")
NOVA_CLASS(Timer, "net.minecraft.util.Timer")
NOVA_CLASS(Enum, "java.lang.Enum")
NOVA_CLASS(Villager, "net.minecraft.entity.passive.EntityVillager")
NOVA_CLASS(Monster, "net.minecraft.entity.monster.IMob")
NOVA_CLASS(Dragon, "net.minecraft.entity.boss.EntityDragon")
NOVA_CLASS(DragonPart, "net.minecraft.entity.boss.EntityDragonPart")
NOVA_CLASS(Animal, "net.minecraft.entity.passive.EntityAnimal")
NOVA_CLASS(Ambient, "net.minecraft.entity.passive.EntityAmbientCreature")
NOVA_CLASS(WaterAnimal, "net.minecraft.entity.passive.EntityWaterMob")
NOVA_CLASS(LocalPlayer, "net.minecraft.client.entity.EntityClientPlayerMP")
NOVA_CLASS(MovementInput, "net.minecraft.util.MovementInput")
NOVA_CLASS(NetworkManager, "net.minecraft.network.NetworkManager")
NOVA_CLASS(Packet, "net.minecraft.network.Packet")
NOVA_CLASS(NetHandler, "net.minecraft.client.network.NetHandlerPlayClient")
NOVA_CLASS(UsePacket, "net.minecraft.network.play.client.C08PacketPlayerBlockPlacement")
NOVA_CLASS(HeldSlotPacket, "net.minecraft.network.play.client.C09PacketHeldItemChange")
NOVA_METHOD(UseStack, UsePacket, "getStack", "()Lnet/minecraft/item/ItemStack;")
NOVA_CLASS(VelocityPacket, "net.minecraft.network.play.server.S12PacketEntityVelocity")
NOVA_CLASS(EntityMovePacket, "net.minecraft.network.play.server.S14PacketEntity")
NOVA_CLASS(TeleportPacket, "net.minecraft.network.play.server.S18PacketEntityTeleport")
NOVA_CLASS(PingPacket, "net.minecraft.network.play.server.S32PacketConfirmTransaction")
NOVA_CLASS(PositionPacket, "net.minecraft.network.play.server.S08PacketPlayerPosLook")
NOVA_CLASS(ExplodePacket, "net.minecraft.network.play.server.S27PacketExplosion")
NOVA_CLASS(DisconnectPacket, "net.minecraft.network.play.server.S40PacketDisconnect")
NOVA_CLASS(RespawnPacket, "net.minecraft.network.play.server.S07PacketRespawn")
NOVA_CLASS(Controller, "net.minecraft.client.multiplayer.PlayerControllerMP")
NOVA_CLASS(BlockItem, "net.minecraft.item.ItemBlock")
NOVA_CLASS(Inventory, "net.minecraft.entity.player.InventoryPlayer")
NOVA_CLASS(InventoryScreen, "net.minecraft.client.gui.inventory.GuiInventory")
NOVA_CLASS(Container, "net.minecraft.inventory.Container")
NOVA_CLASS(ChestContainer, "net.minecraft.inventory.ContainerChest")
NOVA_CLASS(I18n, "net.minecraft.client.resources.I18n")
NOVA_CLASS(Object, "java.lang.Object")
NOVA_CLASS(BlockGameProfile, "com.mojang.authlib.GameProfile")
NOVA_CLASS(BlockScoreTeam, "net.minecraft.scoreboard.ScorePlayerTeam")
NOVA_CLASS(ClosePacket, "net.minecraft.network.play.client.C0DPacketCloseWindow")
NOVA_CLASS(WindowClickPacket, "net.minecraft.network.play.client.C0EPacketClickWindow")
NOVA_CLASS(LoginStartPacket, "net.minecraft.network.login.client.C00PacketLoginStart")
NOVA_CLASS(HandshakePacket, "net.minecraft.network.handshake.client.C00Handshake")
NOVA_CLASS(BlockNameRegistry, "net.minecraft.util.RegistryNamespaced")
NOVA_CLASS(BlockResourceLocation, "net.minecraft.util.ResourceLocation")
NOVA_METHOD(RegistryName, BlockNameRegistry, "getNameForObject", "(Ljava/lang/Object;)Ljava/lang/String;")
NOVA_METHOD(ResourceString, BlockResourceLocation, "toString", "()Ljava/lang/String;")
NOVA_CLASS(InteractPacket, "net.minecraft.network.play.client.C02PacketUseEntity")
NOVA_CLASS(ActionPacket, "net.minecraft.network.play.client.C07PacketPlayerDigging")
NOVA_CLASS(KeepAliveReply, "net.minecraft.network.play.client.C00PacketKeepAlive")
NOVA_CLASS(TransactionReply, "net.minecraft.network.play.client.C0FPacketConfirmTransaction")
NOVA_METHOD(DigAction, ActionPacket, "getDiggedBlockStatus", "()I")

NOVA_FIELD(Options, Minecraft, "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_FIELD(PlayerInventory, Player, "inventory", "Lnet/minecraft/entity/player/InventoryPlayer;")
NOVA_FIELD(InventoryItems, Inventory, "mainInventory", "[Lnet/minecraft/item/ItemStack;")
NOVA_FIELD(InventoryArmor, Inventory, "armorInventory", "[Lnet/minecraft/item/ItemStack;")
NOVA_FIELD(PlayerMenu, Player, "inventoryContainer", "Lnet/minecraft/inventory/Container;")
NOVA_FIELD(OpenMenu, Player, "openContainer", "Lnet/minecraft/inventory/Container;")
NOVA_FIELD(ScreenMenu, ContainerScreen, "inventorySlots", "Lnet/minecraft/inventory/Container;")
NOVA_FIELD(MenuId, Container, "windowId", "I")
NOVA_FIELD(MenuSlots, Container, "inventorySlots", "Ljava/util/List;")
NOVA_FIELD(ChestRows, ChestContainer, "numRows", "I")
NOVA_FIELD(ChestInventory, ChestContainer, "lowerChestInventory", "Lnet/minecraft/inventory/IInventory;")
NOVA_FIELD(Gamma, Settings, "gammaSetting", "F")
NOVA_FIELD(SprintKey, Settings, "keyBindSprint", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_FIELD(AttackKey, Settings, "keyBindAttack", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_FIELD(KeyPressed, KeyBinding, "pressed", "Z")
NOVA_FIELD(KeyPressTime, KeyBinding, "pressTime", "I")
NOVA_FIELD(OnGround, Entity, "onGround", "Z")
NOVA_FIELD(Capabilities, Player, "capabilities", "Lnet/minecraft/entity/player/PlayerCapabilities;")
NOVA_FIELD(Creative, Abilities, "isCreativeMode", "Z")
NOVA_FIELD(Flying, Abilities, "isFlying", "Z")
NOVA_FIELD(JumpCooldown, Living, "jumpTicks", "I")
NOVA_METHOD(SetVelocity, Entity, "setVelocity", "(DDD)V")
NOVA_METHOD(InWater, Entity, "isInWater", "()Z")
NOVA_METHOD(InLava, Entity, "handleLavaMovement", "()Z")
NOVA_METHOD(OnLadder, Living, "isOnLadder", "()Z")
NOVA_FIELD(AllowEdit, Abilities, "allowEdit", "Z")
NOVA_FIELD(PotionId, Potion, "id", "I")
NOVA_STATIC_FIELD(NightVision, Potion, "nightVision", "Lnet/minecraft/potion/Potion;")
NOVA_FIELD(MissTime, Minecraft, "leftClickCounter", "I")
NOVA_FIELD(HitResult, Minecraft, "objectMouseOver", "Lnet/minecraft/util/MovingObjectPosition;")
NOVA_FIELD(Screen, Minecraft, "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;")
NOVA_FIELD(Level, Minecraft, "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;")
NOVA_FIELD(HitKind, Hit, "typeOfHit", "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;")
NOVA_STATIC_FIELD(BlockType, HitType, "BLOCK", "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;")
NOVA_FIELD(HoveredSlot, ContainerScreen, "theSlot", "Lnet/minecraft/inventory/Slot;")
NOVA_FIELD(SlotIndex, Slot, "slotNumber", "I")
NOVA_FIELD(Yaw, Entity, "rotationYaw", "F")
NOVA_FIELD(Pitch, Entity, "rotationPitch", "F")
NOVA_FIELD(PreviousYaw, Entity, "prevRotationYaw", "F")
NOVA_FIELD(PreviousPitch, Entity, "prevRotationPitch", "F")
NOVA_FIELD(X, Entity, "posX", "D")
NOVA_FIELD(Y, Entity, "posY", "D")
NOVA_FIELD(Z, Entity, "posZ", "D")
NOVA_FIELD(Sensitivity, Settings, "mouseSensitivity", "F")
NOVA_FIELD(RenderChunks, Settings, "renderDistanceChunks", "I")
NOVA_FIELD(Fov, Settings, "fovSetting", "F")
NOVA_FIELD(GameRenderer, Minecraft, "entityRenderer", "Lnet/minecraft/client/renderer/EntityRenderer;")
NOVA_FIELD(PacketX, MovePacket, "x", "D")
NOVA_FIELD(PacketY, MovePacket, "y", "D")
NOVA_FIELD(PacketZ, MovePacket, "z", "D")
NOVA_FIELD(PacketYaw, MovePacket, "yaw", "F")
NOVA_FIELD(PacketPitch, MovePacket, "pitch", "F")
NOVA_FIELD(PacketOnGround, MovePacket, "field_149474_g", "Z")
NOVA_FIELD(PreviousX, Entity, "prevPosX", "D")
NOVA_FIELD(PreviousY, Entity, "prevPosY", "D")
NOVA_FIELD(PreviousZ, Entity, "prevPosZ", "D")
NOVA_FIELD(EntityDead, Entity, "isDead", "Z")
NOVA_FIELD(RenderPreviousX, Entity, "lastTickPosX", "D")
NOVA_FIELD(RenderPreviousY, Entity, "lastTickPosY", "D")
NOVA_FIELD(RenderPreviousZ, Entity, "lastTickPosZ", "D")
NOVA_FIELD(Ticks, Entity, "ticksExisted", "I")
NOVA_FIELD(Riding, Entity, "ridingEntity", "Lnet/minecraft/entity/Entity;")
NOVA_FIELD(FallDistance, Entity, "fallDistance", "F")
NOVA_FIELD(HurtTime, Living, "hurtTime", "I")
NOVA_FIELD(DeathTime, Living, "deathTime", "I")
NOVA_FIELD(VecX, Vec, "xCoord", "D")
NOVA_FIELD(VecY, Vec, "yCoord", "D")
NOVA_FIELD(VecZ, Vec, "zCoord", "D")
NOVA_FIELD(MinX, Box, "minX", "D")
NOVA_FIELD(MinY, Box, "minY", "D")
NOVA_FIELD(MinZ, Box, "minZ", "D")
NOVA_FIELD(MaxX, Box, "maxX", "D")
NOVA_FIELD(MaxY, Box, "maxY", "D")
NOVA_FIELD(MaxZ, Box, "maxZ", "D")
NOVA_FIELD(Players, World, "playerEntities", "Ljava/util/List;")
NOVA_FIELD(GameTimer, Minecraft, "timer", "Lnet/minecraft/util/Timer;")
NOVA_FIELD(PartialTicks, Timer, "renderPartialTicks", "F")
NOVA_FIELD(HitVec, Hit, "hitVec", "Lnet/minecraft/util/Vec3;")
NOVA_FIELD(HitSide, Hit, "sideHit", "I")
NOVA_FIELD(MoveForward, MovementInput, "moveForward", "F")
NOVA_FIELD(MoveStrafe, MovementInput, "moveStrafe", "F")
NOVA_FIELD(MoveJump, MovementInput, "jump", "Z")
NOVA_FIELD(MoveSneak, MovementInput, "sneak", "Z")
NOVA_FIELD(HeadYaw, Living, "rotationYawHead", "F")
NOVA_FIELD(PreviousHeadYaw, Living, "prevRotationYawHead", "F")
NOVA_FIELD(GameController, Minecraft, "playerController", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;")
NOVA_FIELD(HitEntity, Hit, "entityHit", "Lnet/minecraft/entity/Entity;")
NOVA_STATIC_FIELD(EntityType, HitType, "ENTITY", "Lnet/minecraft/util/MovingObjectPosition$MovingObjectType;")
NOVA_FIELD(PlaceDelay, Minecraft, "rightClickDelayTimer", "I")
NOVA_FIELD(UseKey, Settings, "keyBindUseItem", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_FIELD(JumpKey, Settings, "keyBindJump", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_FIELD(SneakKey, Settings, "keyBindSneak", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_FIELD(MotionX, Entity, "motionX", "D")
NOVA_FIELD(MotionY, Entity, "motionY", "D")
NOVA_FIELD(MotionZ, Entity, "motionZ", "D")

NOVA_METHOD(UsingItem, Player, "isUsingItem", "()Z")
NOVA_METHOD(ItemUseCount, Player, "getItemInUseCount", "()I")
NOVA_METHOD(RightClickMouse, Minecraft, "rightClickMouse", "()V")
NOVA_METHOD(RenderFrame, Renderer, "updateCameraAndRender", "(F)V")
NOVA_METHOD(StartUseItem, Controller, "sendUseItem", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z")
NOVA_METHOD(StopUseItem, Controller, "onStoppedUsingItem", "(Lnet/minecraft/entity/player/EntityPlayer;)V")
NOVA_METHOD(InteractAction, InteractPacket, "getAction", "()Lnet/minecraft/network/play/client/C02PacketUseEntity$Action;")
NOVA_METHOD(HeldItem, Player, "getHeldItem", "()Lnet/minecraft/item/ItemStack;")
NOVA_METHOD(GetItem, Stack, "getItem", "()Lnet/minecraft/item/Item;")
NOVA_METHOD(AddEffect, Living, "addPotionEffect", "(Lnet/minecraft/potion/PotionEffect;)V")
NOVA_METHOD(RemoveEffect, Living, "removePotionEffect", "(I)V")
NOVA_METHOD(EffectCtor, Effect, "<init>", "(IIIZ)V")
NOVA_METHOD(KeyCode, KeyBinding, "getKeyCode", "()I")
NOVA_STATIC_METHOD(KeyTick, KeyBinding, "onTick", "(I)V")
NOVA_STATIC_METHOD(KeyState, KeyBinding, "setKeyBindState", "(IZ)V")
NOVA_STATIC_METHOD(MouseButton, Mouse, "isButtonDown", "(I)Z")
NOVA_STATIC_METHOD(KeyboardKey, Keyboard, "isKeyDown", "(I)Z")
NOVA_FIELD(ChatVisibility, Settings, "chatVisibility", "Lnet/minecraft/entity/player/EntityPlayer$EnumChatVisibility;")
NOVA_STATIC_METHOD(MouseGrabbed, Mouse, "isGrabbed", "()Z")
NOVA_METHOD(GetBlockState, World, "getBlock", "(III)Lnet/minecraft/block/Block;")
NOVA_METHOD(ClickSlot, ContainerScreen, "handleMouseClick", "(Lnet/minecraft/inventory/Slot;III)V")
NOVA_METHOD(EyeHeight, Entity, "getEyeHeight", "()F")
NOVA_METHOD(Pick, Renderer, "getMouseOver", "(F)V")
NOVA_METHOD(PosRotCtor, PosRotPacket, "<init>", "(DDDDFFZ)V")
NOVA_METHOD(IsPickable, Entity, "canBeCollidedWith", "()Z")
NOVA_METHOD(IsAlive, Entity, "isEntityAlive", "()Z")
NOVA_METHOD(IsInvisible, Entity, "isInvisible", "()Z")
NOVA_METHOD(EntityId, Entity, "getEntityId", "()I")
NOVA_METHOD(PickRadius, Entity, "getCollisionBorderSize", "()F")
NOVA_METHOD(DisplayName, Entity, "getFormattedCommandSenderName", "()Lnet/minecraft/util/IChatComponent;")
NOVA_METHOD(BlockUuid, Entity, "getUniqueID", "()Ljava/util/UUID;")
NOVA_METHOD(BlockProfileName, BlockGameProfile, "getName", "()Ljava/lang/String;")
NOVA_STATIC_METHOD(BlockFormatName, BlockScoreTeam, "formatPlayerName", "(Lnet/minecraft/scoreboard/Team;Ljava/lang/String;)Ljava/lang/String;")
NOVA_METHOD(GetName, Entity, "getCommandSenderName", "()Ljava/lang/String;")
NOVA_METHOD(FormattedText, Chat, "getFormattedText", "()Ljava/lang/String;")
NOVA_METHOD(GetHealth, Living, "getHealth", "()F")
NOVA_METHOD(GetAbsorption, Living, "getAbsorptionAmount", "()F")
NOVA_METHOD(GetMaxHealth, Living, "getMaxHealth", "()F")
NOVA_METHOD(SetHealth, Living, "setHealth", "(F)V")
NOVA_METHOD(ThrowableLook, Living, "getLookVec", "()Lnet/minecraft/util/Vec3;")
NOVA_METHOD(ThrowableCanSee, Living, "canEntityBeSeen", "(Lnet/minecraft/entity/Entity;)Z")
NOVA_METHOD(ThrowableTeam, Living, "getTeam", "()Lnet/minecraft/scoreboard/Team;")
NOVA_METHOD(SameTeam, Living, "isOnSameTeam", "(Lnet/minecraft/entity/EntityLivingBase;)Z")
NOVA_METHOD(EntityById, World, "getEntityByID", "(I)Lnet/minecraft/entity/Entity;")
NOVA_METHOD(GetEntities, World, "getEntitiesWithinAABBExcludingEntity", "(Lnet/minecraft/entity/Entity;Lnet/minecraft/util/AxisAlignedBB;)Ljava/util/List;")
NOVA_METHOD(ListSize, List, "size", "()I")
NOVA_METHOD(ListGet, List, "get", "(I)Ljava/lang/Object;")
NOVA_METHOD(BoxCtor, Box, "<init>", "(DDDDDD)V")
NOVA_METHOD(VecCtor, Vec, "<init>", "(DDD)V")
NOVA_METHOD(Ordinal, Enum, "ordinal", "()I")
NOVA_METHOD(RayTrace, World, "rayTraceBlocks", "(Lnet/minecraft/util/Vec3;Lnet/minecraft/util/Vec3;ZZZ)Lnet/minecraft/util/MovingObjectPosition;")
NOVA_METHOD(EntityHitCtor, Hit, "<init>", "(Lnet/minecraft/entity/Entity;Lnet/minecraft/util/Vec3;)V")
NOVA_METHOD(BoxIntercept, Box, "calculateIntercept", "(Lnet/minecraft/util/Vec3;Lnet/minecraft/util/Vec3;)Lnet/minecraft/util/MovingObjectPosition;")
NOVA_METHOD(UpdateInput, MovementInput, "updatePlayerMoveState", "()V")
NOVA_METHOD(WalkingUpdate, LocalPlayer, "sendMotionUpdates", "()V")
NOVA_METHOD(CurrentNetHandler, Minecraft, "getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;")
NOVA_METHOD(IsClientThread, Minecraft, "isCallingFromMinecraftThread", "()Z")
NOVA_METHOD(ScheduleTask, Minecraft, "addScheduledTask", "(Ljava/lang/Runnable;)Lcom/google/common/util/concurrent/ListenableFuture;")
NOVA_METHOD(GetNetworkManager, NetHandler, "getNetworkManager", "()Lnet/minecraft/network/NetworkManager;")
NOVA_METHOD(SendPacket, NetHandler, "addToSendQueue", "(Lnet/minecraft/network/Packet;)V")
NOVA_METHOD(DispatchPacket, NetworkManager, "dispatchPacket", "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V")
NOVA_METHOD(ProcessPacket, Packet, "processPacket", "(Lnet/minecraft/network/INetHandler;)V")
NOVA_METHOD(StatusPacketCtor, MovePacket, "<init>", "(Z)V")
NOVA_METHOD(UseDirection, UsePacket, "getPlacedBlockDirection", "()I")
NOVA_METHOD(VelocityId, VelocityPacket, "func_149412_c", "()I")
NOVA_METHOD(VelocityX, VelocityPacket, "func_149411_d", "()I")
NOVA_METHOD(VelocityY, VelocityPacket, "func_149410_e", "()I")
NOVA_METHOD(VelocityZ, VelocityPacket, "func_149409_f", "()I")
NOVA_METHOD(Attack, Controller, "attackEntity", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/entity/Entity;)V")
NOVA_METHOD(Swing, Living, "swingItem", "()V")
NOVA_METHOD(SetSprinting, Entity, "setSprinting", "(Z)V")
NOVA_METHOD(IsSprinting, Entity, "isSprinting", "()Z")
NOVA_METHOD(MoveSpeed, Living, "getAIMoveSpeed", "()F")
NOVA_METHOD(KeyDown, KeyBinding, "getIsKeyPressed", "()Z")
NOVA_METHOD(VelocityCtor, VelocityPacket, "<init>", "(IDDD)V")
NOVA_METHOD(UsePacketCtor, UsePacket, "<init>", "(IIIILnet/minecraft/item/ItemStack;FFF)V")
NOVA_METHOD(MenuGetSlot, Container, "getSlot", "(I)Lnet/minecraft/inventory/Slot;")
NOVA_METHOD(SlotStack, Slot, "getStack", "()Lnet/minecraft/item/ItemStack;")
NOVA_METHOD(InventoryTitle, InventoryInterface, "getInventoryName", "()Ljava/lang/String;")
NOVA_METHOD(UnformattedText, Chat, "getUnformattedText", "()Ljava/lang/String;")
NOVA_STATIC_METHOD(Translate, I18n, "format", "(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;")
NOVA_METHOD(WindowClick, Controller, "windowClick", "(IIIILnet/minecraft/entity/player/EntityPlayer;)Lnet/minecraft/item/ItemStack;")
NOVA_METHOD(CloseScreen, LocalPlayer, "closeScreen", "()V")
NOVA_METHOD(ClosePacketCtor, ClosePacket, "<init>", "(I)V")
// Inventory
NOVA_CLASS(Items, "net.minecraft.init.Items")
NOVA_CLASS(Blocks, "net.minecraft.init.Blocks")
NOVA_CLASS(Item, "net.minecraft.item.Item")
NOVA_CLASS(Block, "net.minecraft.block.Block")
NOVA_STATIC_FIELD(BlockRegistry, Block, "blockRegistry", "Lnet/minecraft/util/RegistryNamespaced;")
NOVA_CLASS(Pickaxe, "net.minecraft.item.ItemPickaxe")
NOVA_CLASS(Shovel, "net.minecraft.item.ItemSpade")
NOVA_CLASS(Bow, "net.minecraft.item.ItemBow")
NOVA_CLASS(FishingRod, "net.minecraft.item.ItemFishingRod")
NOVA_CLASS(Armor, "net.minecraft.item.ItemArmor")
NOVA_CLASS(SkullItem, "net.minecraft.item.ItemSkull")
NOVA_CLASS(Seeds, "net.minecraft.item.ItemSeeds")
NOVA_CLASS(SeedFood, "net.minecraft.item.ItemSeedFood")
NOVA_CLASS(Book, "net.minecraft.item.ItemBook")
NOVA_CLASS(ExpBottle, "net.minecraft.item.ItemExpBottle")
NOVA_CLASS(Firework, "net.minecraft.item.ItemFirework")
NOVA_CLASS(Flower, "net.minecraft.block.BlockFlower")
NOVA_CLASS(Bush, "net.minecraft.block.BlockBush")
NOVA_CLASS(Mushroom, "net.minecraft.block.BlockMushroom")
NOVA_CLASS(Crop, "net.minecraft.block.BlockCrops")
NOVA_CLASS(Slab, "net.minecraft.block.BlockSlab")
NOVA_CLASS(ArmorMaterial, "net.minecraft.item.ItemArmor$ArmorMaterial")
NOVA_CLASS(Enchantment, "net.minecraft.enchantment.Enchantment")
NOVA_CLASS(EnchantHelper, "net.minecraft.enchantment.EnchantmentHelper")
NOVA_CLASS(CreatureAttribute, "net.minecraft.entity.EnumCreatureAttribute")
NOVA_STATIC_FIELD(ItemGoldenAxe, Items, "golden_axe", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemSlimeBall, Items, "slime_ball", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemStick, Items, "stick", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemGoldenApple, Items, "golden_apple", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemCompass, Items, "compass", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemWaterBucket, Items, "water_bucket", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemLavaBucket, Items, "lava_bucket", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemArrow, Items, "arrow", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemSnowball, Items, "snowball", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemEgg, Items, "egg", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemFishingRod, Items, "fishing_rod", "Lnet/minecraft/item/ItemFishingRod;")
NOVA_STATIC_FIELD(ItemWoodenAxe, Items, "wooden_axe", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemStoneAxe, Items, "stone_axe", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemIronAxe, Items, "iron_axe", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemDiamondAxe, Items, "diamond_axe", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemWheatSeeds, Items, "wheat_seeds", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemMelonSeeds, Items, "melon_seeds", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemPumpkinSeeds, Items, "pumpkin_seeds", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemFlintAndSteel, Items, "flint_and_steel", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemBow, Items, "bow", "Lnet/minecraft/item/ItemBow;")
NOVA_STATIC_FIELD(ItemEnderPearl, Items, "ender_pearl", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(ItemFireCharge, Items, "fire_charge", "Lnet/minecraft/item/Item;")
NOVA_STATIC_FIELD(Block_enchanting_table, Blocks, "enchanting_table", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_web, Blocks, "web", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_stone, Blocks, "stone", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_log, Blocks, "log", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_dirt, Blocks, "dirt", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_air, Blocks, "air", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_water, Blocks, "water", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_lava, Blocks, "lava", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_glass_pane, Blocks, "glass_pane", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_iron_bars, Blocks, "iron_bars", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_snow_layer, Blocks, "snow_layer", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_coal_ore, Blocks, "coal_ore", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_diamond_ore, Blocks, "diamond_ore", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_emerald_ore, Blocks, "emerald_ore", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_chest, Blocks, "chest", "Lnet/minecraft/block/BlockChest;")
NOVA_STATIC_FIELD(Block_trapped_chest, Blocks, "trapped_chest", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_torch, Blocks, "torch", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_anvil, Blocks, "anvil", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_noteblock, Blocks, "noteblock", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_jukebox, Blocks, "jukebox", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_tnt, Blocks, "tnt", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_gold_ore, Blocks, "gold_ore", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_iron_ore, Blocks, "iron_ore", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_lapis_ore, Blocks, "lapis_ore", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_stone_pressure_plate, Blocks, "stone_pressure_plate", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_light_weighted_pressure_plate, Blocks, "light_weighted_pressure_plate", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_heavy_weighted_pressure_plate, Blocks, "heavy_weighted_pressure_plate", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_stone_button, Blocks, "stone_button", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_lever, Blocks, "lever", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_tallgrass, Blocks, "tallgrass", "Lnet/minecraft/block/BlockTallGrass;")
NOVA_STATIC_FIELD(Block_tripwire, Blocks, "tripwire", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_tripwire_hook, Blocks, "tripwire_hook", "Lnet/minecraft/block/BlockTripWireHook;")
NOVA_STATIC_FIELD(Block_rail, Blocks, "rail", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_red_mushroom, Blocks, "red_mushroom", "Lnet/minecraft/block/BlockBush;")
NOVA_STATIC_FIELD(Block_brown_mushroom, Blocks, "brown_mushroom", "Lnet/minecraft/block/BlockBush;")
NOVA_STATIC_FIELD(Block_vine, Blocks, "vine", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_double_plant, Blocks, "double_plant", "Lnet/minecraft/block/BlockDoublePlant;")
NOVA_STATIC_FIELD(Block_ladder, Blocks, "ladder", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_furnace, Blocks, "furnace", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_sand, Blocks, "sand", "Lnet/minecraft/block/BlockSand;")
NOVA_STATIC_FIELD(Block_cactus, Blocks, "cactus", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_dispenser, Blocks, "dispenser", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_dropper, Blocks, "dropper", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_crafting_table, Blocks, "crafting_table", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_pumpkin, Blocks, "pumpkin", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_cobblestone_wall, Blocks, "cobblestone_wall", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_oak_fence, Blocks, "fence", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_redstone_torch, Blocks, "redstone_torch", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_flower_pot, Blocks, "flower_pot", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_flowing_water, Blocks, "flowing_water", "Lnet/minecraft/block/BlockLiquid;")
NOVA_STATIC_FIELD(Block_flowing_lava, Blocks, "flowing_lava", "Lnet/minecraft/block/BlockLiquid;")
NOVA_STATIC_FIELD(Block_lit_furnace, Blocks, "lit_furnace", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_unlit_redstone_torch, Blocks, "unlit_redstone_torch", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Enchant_sharpness, Enchantment, "sharpness", "Lnet/minecraft/enchantment/Enchantment;")
NOVA_STATIC_FIELD(Enchant_knockback, Enchantment, "knockback", "Lnet/minecraft/enchantment/Enchantment;")
NOVA_STATIC_FIELD(Enchant_punch, Enchantment, "punch", "Lnet/minecraft/enchantment/Enchantment;")
NOVA_STATIC_FIELD(Enchant_power, Enchantment, "power", "Lnet/minecraft/enchantment/Enchantment;")
NOVA_STATIC_FIELD(Enchant_infinity, Enchantment, "infinity", "Lnet/minecraft/enchantment/Enchantment;")
NOVA_STATIC_FIELD(Enchant_flame, Enchantment, "flame", "Lnet/minecraft/enchantment/Enchantment;")
NOVA_STATIC_FIELD(Enchant_efficiency, Enchantment, "efficiency", "Lnet/minecraft/enchantment/Enchantment;")
NOVA_STATIC_FIELD(Enchant_protection, Enchantment, "protection", "Lnet/minecraft/enchantment/Enchantment;")
NOVA_STATIC_FIELD(Material_LEATHER, ArmorMaterial, "CLOTH", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(Material_CHAIN, ArmorMaterial, "CHAIN", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(Material_IRON, ArmorMaterial, "IRON", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(Material_GOLD, ArmorMaterial, "GOLD", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(Material_DIAMOND, ArmorMaterial, "DIAMOND", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(CreatureUndefined, CreatureAttribute, "UNDEFINED", "Lnet/minecraft/entity/EnumCreatureAttribute;")
NOVA_FIELD(StackCount, Stack, "stackSize", "I")
NOVA_FIELD(SwordAttackDamage, Sword, "field_150934_a", "F")
NOVA_FIELD(ArmorSlot, Armor, "armorType", "I")
NOVA_FIELD(EnchantId, Enchantment, "effectId", "I")
NOVA_METHOD(StackName, Stack, "getDisplayName", "()Ljava/lang/String;")
NOVA_METHOD(StackDamage, Stack, "getMetadata", "()I")
NOVA_METHOD(StackMaxDamage, Stack, "getMaxDurability", "()I")
NOVA_METHOD(StackMaxCount, Stack, "getMaxStackSize", "()I")
NOVA_METHOD(StackSpeed, Stack, "getStrVsBlock", "(Lnet/minecraft/block/Block;)F")
NOVA_METHOD(GetArmorMaterial, Armor, "getArmorMaterial", "()Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_METHOD(SharpnessDamage, Enchantment, "func_152376_a", "(ILnet/minecraft/entity/EnumCreatureAttribute;)F")
NOVA_STATIC_METHOD(EnchantmentLevel, EnchantHelper, "getEnchantmentLevel", "(ILnet/minecraft/item/ItemStack;)I")
// Visuals
NOVA_CLASS(ScaledResolution, "net.minecraft.client.gui.ScaledResolution")
NOVA_CLASS(WorldProvider, "net.minecraft.world.WorldProvider")
NOVA_CLASS(ClientPlayer, "net.minecraft.client.entity.AbstractClientPlayer")
NOVA_CLASS(TextureManager, "net.minecraft.client.renderer.texture.TextureManager")
NOVA_CLASS(TextureObject, "net.minecraft.client.renderer.texture.ITextureObject")
NOVA_CLASS(ScorePacket, "net.minecraft.network.play.server.S3CPacketUpdateScore")
NOVA_CLASS(ActiveRenderInfo, "net.minecraft.client.renderer.ActiveRenderInfo")
NOVA_CLASS(FloatBuffer, "java.nio.FloatBuffer")
NOVA_CLASS(TileEntity, "net.minecraft.tileentity.TileEntity")
NOVA_CLASS(ChestTile, "net.minecraft.tileentity.TileEntityChest")
NOVA_CLASS(ChestBlock, "net.minecraft.block.BlockChest")
NOVA_CLASS(BlockActionPacket, "net.minecraft.network.play.server.S24PacketBlockAction")
NOVA_STATIC_FIELD(ProjectionBuffer, ActiveRenderInfo, "projection", "Ljava/nio/FloatBuffer;")
NOVA_STATIC_FIELD(ModelViewBuffer, ActiveRenderInfo, "modelview", "Ljava/nio/FloatBuffer;")
NOVA_FIELD(LoadedTiles, World, "loadedTileEntityList", "Ljava/util/List;")
NOVA_METHOD(BufferFloat, FloatBuffer, "get", "(I)F")
NOVA_METHOD(BlockBounds, Block, "setBlockBoundsBasedOnState", "(Lnet/minecraft/world/IBlockAccess;III)V")
NOVA_METHOD(SelectedBox, Block, "getSelectedBoundingBoxFromPool", "(Lnet/minecraft/world/World;III)Lnet/minecraft/util/AxisAlignedBB;")
NOVA_METHOD(BlockActionType, BlockActionPacket, "getBlockType", "()Lnet/minecraft/block/Block;")
NOVA_METHOD(BlockActionId, BlockActionPacket, "getData1", "()I")
NOVA_METHOD(BlockActionParameter, BlockActionPacket, "getData2", "()I")
NOVA_FIELD(DisplayWidth, Minecraft, "displayWidth", "I")
NOVA_FIELD(DisplayHeight, Minecraft, "displayHeight", "I")
NOVA_FIELD(Provider, World, "provider", "Lnet/minecraft/world/WorldProvider;")
NOVA_METHOD(ScaledResolutionCtor, ScaledResolution, "<init>", "(Lnet/minecraft/client/Minecraft;II)V")
NOVA_METHOD(ScaleFactor, ScaledResolution, "getScaleFactor", "()I")
NOVA_METHOD(SurfaceWorld, WorldProvider, "isSurfaceWorld", "()Z")
NOVA_METHOD(SpawnPoint, World, "getSpawnPoint", "()Lnet/minecraft/util/ChunkCoordinates;")
NOVA_METHOD(SkinLocation, ClientPlayer, "getLocationSkin", "()Lnet/minecraft/util/ResourceLocation;")
NOVA_METHOD(GetTextureManager, Minecraft, "getTextureManager", "()Lnet/minecraft/client/renderer/texture/TextureManager;")
NOVA_METHOD(GetTexture, TextureManager, "getTexture", "(Lnet/minecraft/util/ResourceLocation;)Lnet/minecraft/client/renderer/texture/ITextureObject;")
NOVA_METHOD(TextureId, TextureObject, "getGlTextureId", "()I")
NOVA_METHOD(ScoreOwner, ScorePacket, "func_149324_c", "()Ljava/lang/String;")
NOVA_METHOD(ScoreObjective, ScorePacket, "func_149321_d", "()Ljava/lang/String;")
NOVA_METHOD(ScoreValue, ScorePacket, "func_149323_e", "()I")
NOVA_METHOD(ScoreAction, ScorePacket, "func_149322_f", "()I")
// Placement
NOVA_CLASS(Facing, "net.minecraft.util.EnumFacing")
NOVA_CLASS(Workbench, "net.minecraft.block.BlockWorkbench")
NOVA_CLASS(EnchantingTable, "net.minecraft.block.BlockEnchantmentTable")
NOVA_CLASS(Anvil, "net.minecraft.block.BlockAnvil")
NOVA_CLASS(EnderChest, "net.minecraft.block.BlockEnderChest")
NOVA_CLASS(Ore, "net.minecraft.block.BlockOre")
NOVA_CLASS(RedstoneOre, "net.minecraft.block.BlockRedstoneOre")
NOVA_CLASS(RotationPacket, "net.minecraft.network.play.client.C03PacketPlayer$C05PacketPlayerLook")
NOVA_CLASS(SwingPacket, "net.minecraft.network.play.client.C0APacketAnimation")
NOVA_FIELD(SelectedSlot, Inventory, "currentItem", "I")
NOVA_FIELD(Destroying, Controller, "isHittingBlock", "Z")
NOVA_STATIC_FIELD(FacingDown, Facing, "DOWN", "Lnet/minecraft/util/EnumFacing;")
NOVA_STATIC_FIELD(FacingUp, Facing, "UP", "Lnet/minecraft/util/EnumFacing;")
NOVA_STATIC_FIELD(FacingNorth, Facing, "NORTH", "Lnet/minecraft/util/EnumFacing;")
NOVA_STATIC_FIELD(FacingSouth, Facing, "SOUTH", "Lnet/minecraft/util/EnumFacing;")
// 1.7 names are reversed: EAST is side 4 / X-1, WEST is side 5 / X+1.
// Match the protocol side and offset used by ray hits and C08, not the name.
NOVA_STATIC_FIELD(FacingWest, Facing, "EAST", "Lnet/minecraft/util/EnumFacing;")
NOVA_STATIC_FIELD(FacingEast, Facing, "WEST", "Lnet/minecraft/util/EnumFacing;")
NOVA_METHOD(CollisionBox, Block, "getCollisionBoundingBoxFromPool", "(Lnet/minecraft/world/World;III)Lnet/minecraft/util/AxisAlignedBB;")
NOVA_METHOD(GetTileEntity, World, "getTileEntity", "(III)Lnet/minecraft/tileentity/TileEntity;")
NOVA_METHOD(FacingX, Facing, "getFrontOffsetX", "()I")
NOVA_METHOD(FacingY, Facing, "getFrontOffsetY", "()I")
NOVA_METHOD(FacingZ, Facing, "getFrontOffsetZ", "()I")
NOVA_METHOD(UseBlock, Controller, "onPlayerRightClick", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;IIIILnet/minecraft/util/Vec3;)Z")
NOVA_METHOD(RotationPacketCtor, RotationPacket, "<init>", "(FFZ)V")
NOVA_METHOD(SwingPacketCtor, SwingPacket, "<init>", "()V")
// Leader Hypixel AutoBlock immediate packet / local-use operations.
NOVA_FIELD(ReportedYaw, LocalPlayer, "oldRotationYaw", "F")
NOVA_FIELD(ReportedPitch, LocalPlayer, "oldRotationPitch", "F")
NOVA_CLASS(InteractActionEnum, "net.minecraft.network.play.client.C02PacketUseEntity$Action")
NOVA_CLASS(ChatPacket, "net.minecraft.network.play.client.C01PacketChatMessage")
NOVA_STATIC_FIELD(InteractEntityAction, InteractActionEnum, "INTERACT", "Lnet/minecraft/network/play/client/C02PacketUseEntity$Action;")
NOVA_METHOD(HeldSlotCtor, HeldSlotPacket, "<init>", "(I)V")
NOVA_METHOD(DigPacketCtor, ActionPacket, "<init>", "(IIIII)V")
NOVA_METHOD(InteractEntityCtor, InteractPacket, "<init>", "(Lnet/minecraft/entity/Entity;Lnet/minecraft/network/play/client/C02PacketUseEntity$Action;)V")
NOVA_METHOD(SyncHeldItem, Controller, "syncCurrentPlayItem", "()V")
NOVA_METHOD(SetItemInUse, Player, "setItemInUse", "(Lnet/minecraft/item/ItemStack;I)V")
NOVA_METHOD(StopItemInUse, Player, "stopUsingItem", "()V")
NOVA_METHOD(MaxItemUseDuration, Stack, "getMaxItemUseDuration", "()I")
NOVA_FIELD(TimerSpeed, Timer, "timerSpeed", "F")
NOVA_METHOD(ClickBlock, Minecraft, "sendClickBlockToController", "(Z)V")
// Leader Prediction velocity, all Lunar bindings remain in this file.
NOVA_FIELD(LoadedEntities, World, "loadedEntityList", "Ljava/util/List;")
NOVA_CLASS(EntityStatusPacket, "net.minecraft.network.play.server.S19PacketEntityStatus")
NOVA_CLASS(KeepAlivePacket, "net.minecraft.network.play.server.S00PacketKeepAlive")
NOVA_CLASS(JoinPacket, "net.minecraft.network.play.server.S01PacketJoinGame")
NOVA_FIELD(StatusEntityId, EntityStatusPacket, "field_149164_a", "I")
NOVA_METHOD(StatusOpcode, EntityStatusPacket, "func_149160_c", "()B")
NOVA_FIELD(InWeb, Entity, "isInWeb", "Z")
NOVA_STATIC_FIELD(JumpPotion, Potion, "jump", "Lnet/minecraft/potion/Potion;")
NOVA_METHOD(PotionActive, Living, "isPotionActive", "(Lnet/minecraft/potion/Potion;)Z")
NOVA_METHOD(ParentLivingUpdate, ClientPlayer, "onLivingUpdate", "()V")
NOVA_FIELD(PredictionInput, LocalPlayer, "movementInput", "Lnet/minecraft/util/MovementInput;")
NOVA_FIELD(ForwardKey, Settings, "keyBindForward", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_FIELD(BackKey, Settings, "keyBindBack", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_FIELD(LeftKey, Settings, "keyBindLeft", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_FIELD(RightKey, Settings, "keyBindRight", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_STATIC_FIELD(AttackEntityAction, InteractActionEnum, "ATTACK", "Lnet/minecraft/network/play/client/C02PacketUseEntity$Action;")
NOVA_FIELD(YOffset17, Entity, "yOffset", "F")
NOVA_CLASS(Coordinates17, "net.minecraft.util.ChunkCoordinates")
NOVA_FIELD(HitX17, Hit, "blockX", "I")
NOVA_FIELD(HitY17, Hit, "blockY", "I")
NOVA_FIELD(HitZ17, Hit, "blockZ", "I")
NOVA_FIELD(Box17, Entity, "boundingBox", "Lnet/minecraft/util/AxisAlignedBB;")
NOVA_FIELD(ItemBlock17, BlockItem, "blockInstance", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Fps17, Minecraft, "debugFPS", "I")
NOVA_FIELD(View17, Minecraft, "renderViewEntity", "Lnet/minecraft/entity/EntityLivingBase;")
NOVA_FIELD(ClientNetwork17, NetworkManager, "isClientSide", "Z")
NOVA_FIELD(TileX17, TileEntity, "xCoord", "I")
NOVA_FIELD(TileY17, TileEntity, "yCoord", "I")
NOVA_FIELD(TileZ17, TileEntity, "zCoord", "I")
NOVA_FIELD(SpawnX17, Coordinates17, "posX", "I")
NOVA_FIELD(SpawnZ17, Coordinates17, "posZ", "I")
NOVA_METHOD(BlockActionX17, BlockActionPacket, "getX", "()I")
NOVA_METHOD(BlockActionY17, BlockActionPacket, "getY", "()I")
NOVA_METHOD(BlockActionZ17, BlockActionPacket, "getZ", "()I")
#else
#error "Select a Lunar1710 mapping section before including this file"
#endif
