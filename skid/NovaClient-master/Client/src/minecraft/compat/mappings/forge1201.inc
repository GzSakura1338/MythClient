// Forge 1.20.1 mappings. Intentionally no include guard.
// Select one section at each include site; runtime handles stay in their modules.
#if defined(NOVA_ELYTRA_MAPPING)
NOVA_ELYTRA(flying,"net.minecraft.world.entity.LivingEntity","m_21255_","()Z")
NOVA_ELYTRA(velocity,"net.minecraft.world.entity.Entity","m_20184_","()Lnet/minecraft/world/phys/Vec3;")
NOVA_ELYTRA(look,"net.minecraft.world.entity.Entity","m_20154_","()Lnet/minecraft/world/phys/Vec3;")
NOVA_ELYTRA(id,"net.minecraft.world.entity.Entity","m_19879_","()I")
NOVA_ELYTRA(removed,"net.minecraft.world.entity.Entity","m_213877_","()Z")
NOVA_ELYTRA(data,"net.minecraft.world.entity.Entity","m_20088_","()Lnet/minecraft/network/syncher/SynchedEntityData;")
NOVA_ELYTRA(dataGet,"net.minecraft.network.syncher.SynchedEntityData","m_135370_","(Lnet/minecraft/network/syncher/EntityDataAccessor;)Ljava/lang/Object;")
NOVA_ELYTRA(target,"net.minecraft.world.entity.projectile.FireworkRocketEntity","f_37020_","Lnet/minecraft/network/syncher/EntityDataAccessor;")
NOVA_ELYTRA(entities,"net.minecraft.client.multiplayer.ClientLevel","m_104735_","()Ljava/lang/Iterable;")
NOVA_ELYTRA(component,"net.minecraft.world.item.ItemStack","m_41737_","(Ljava/lang/String;)Lnet/minecraft/nbt/CompoundTag;")
NOVA_ELYTRA(duration,"net.minecraft.nbt.CompoundTag","m_128451_","(Ljava/lang/String;)I")
NOVA_ELYTRA(x,"net.minecraft.world.phys.Vec3","f_82479_","D")
NOVA_ELYTRA(y,"net.minecraft.world.phys.Vec3","f_82480_","D")
NOVA_ELYTRA(z,"net.minecraft.world.phys.Vec3","f_82481_","D")
#elif defined(NOVA_INVENTORY_STACK_MAPPING)
NOVA_STACK(copy, "m_41777_", "()Lnet/minecraft/world/item/ItemStack;")
NOVA_STACK(matches, "m_41728_", "(Lnet/minecraft/world/item/ItemStack;Lnet/minecraft/world/item/ItemStack;)Z")
NOVA_STACK(count, "f_41587_", "I")
NOVA_STACK(empty, "m_41619_", "()Z")
#elif defined(NOVA_GHOSTHAND_MAPPING)
NOVA_GHOSTHAND(view, "net.minecraft.client.Minecraft", "m_91288_", "()Lnet/minecraft/world/entity/Entity;")
NOVA_GHOSTHAND(look, "net.minecraft.world.entity.Entity", "m_20252_", "(F)Lnet/minecraft/world/phys/Vec3;")
NOVA_GHOSTHAND(reach, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "m_105286_", "()F")
NOVA_GHOSTHAND(extended, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "m_105290_", "()Z")
NOVA_GHOSTHAND(pointed, "net.minecraft.client.Minecraft", "f_91076_", "Lnet/minecraft/world/entity/Entity;")
NOVA_GHOSTHAND(renderer, "", "", "")
NOVA_GHOSTHAND(rendererPointed, "", "", "")
NOVA_GHOSTHAND(stackItem, "net.minecraft.world.item.ItemStack", "m_41720_", "()Lnet/minecraft/world/item/Item;")
NOVA_GHOSTHAND(vx, "net.minecraft.world.phys.Vec3", "f_82479_", "D")
NOVA_GHOSTHAND(vy, "net.minecraft.world.phys.Vec3", "f_82480_", "D")
NOVA_GHOSTHAND(vz, "net.minecraft.world.phys.Vec3", "f_82481_", "D")
NOVA_GHOSTHAND(hitLocation, "net.minecraft.world.phys.HitResult", "m_82450_", "()Lnet/minecraft/world/phys/Vec3;")
NOVA_GHOSTHAND(canCollide, "", "", "")
NOVA_GHOSTHAND(vehicle, "net.minecraft.world.entity.Entity", "m_20202_", "()Lnet/minecraft/world/entity/Entity;")
NOVA_GHOSTHAND(riderInteract, "net.minecraft.world.entity.Entity", "canRiderInteract", "()Z")
NOVA_GHOSTHAND_CLASSES("net.minecraft.world.item.DiggerItem", "net.minecraft.world.item.HoeItem", "net.minecraft.world.item.ShearsItem", "net.minecraft.world.item.BucketItem")
#elif defined(NOVA_FREECAM_MAPPING)
NOVA_FREECAM(attack,"net.minecraft.client.Minecraft","m_91278_","()Z")
NOVA_FREECAM(controlled,"net.minecraft.client.player.LocalPlayer","m_108636_","()Z")
NOVA_FREECAM(ctor,"net.minecraft.client.player.RemotePlayer","<init>","(Lnet/minecraft/client/multiplayer/ClientLevel;Lcom/mojang/authlib/GameProfile;)V")
NOVA_FREECAM(profile,"net.minecraft.world.entity.player.Player","m_36316_","()Lcom/mojang/authlib/GameProfile;")
NOVA_FREECAM(view,"net.minecraft.client.Minecraft","m_91288_","()Lnet/minecraft/world/entity/Entity;")
NOVA_FREECAM(setView,"net.minecraft.client.Minecraft","m_91118_","(Lnet/minecraft/world/entity/Entity;)V")
NOVA_FREECAM(move,"net.minecraft.world.entity.Entity","m_6034_","(DDD)V")
NOVA_FREECAM(yaw,"net.minecraft.world.entity.Entity","f_19857_","F")
NOVA_FREECAM(pitch,"net.minecraft.world.entity.Entity","f_19858_","F")
NOVA_FREECAM(oldYaw,"net.minecraft.world.entity.Entity","f_19859_","F")
NOVA_FREECAM(oldPitch,"net.minecraft.world.entity.Entity","f_19860_","F")
NOVA_FREECAM(xo,"net.minecraft.world.entity.Entity","f_19854_","D")
NOVA_FREECAM(yo,"net.minecraft.world.entity.Entity","f_19855_","D")
NOVA_FREECAM(zo,"net.minecraft.world.entity.Entity","f_19856_","D")
NOVA_FREECAM(oldX,"net.minecraft.world.entity.Entity","f_19790_","D")
NOVA_FREECAM(oldY,"net.minecraft.world.entity.Entity","f_19791_","D")
NOVA_FREECAM(oldZ,"net.minecraft.world.entity.Entity","f_19792_","D")
NOVA_FREECAM(headYaw,"net.minecraft.world.entity.LivingEntity","f_20885_","F")
NOVA_FREECAM(invisible,"net.minecraft.world.entity.Entity","m_6842_","(Z)V")
NOVA_FREECAM(velocity,"net.minecraft.world.entity.Entity","m_20334_","(DDD)V")
NOVA_FREECAM(sprint,"net.minecraft.world.entity.Entity","m_6858_","(Z)V")
NOVA_FREECAM(sneak,"net.minecraft.world.entity.Entity","m_20260_","(Z)V")
NOVA_FREECAM(forward,"net.minecraft.world.entity.LivingEntity","f_20902_","F")
NOVA_FREECAM(strafe,"net.minecraft.world.entity.LivingEntity","f_20900_","F")
NOVA_FREECAM(add,"net.minecraft.client.multiplayer.ClientLevel","m_104630_","(ILnet/minecraft/client/player/AbstractClientPlayer;)V")
NOVA_FREECAM(remove,"net.minecraft.world.entity.Entity","m_146870_","()V")
NOVA_FREECAM(refresh,"net.minecraft.client.multiplayer.ClientLevel","m_104793_","(III)V")
NOVA_FREECAM(arm,"net.minecraft.client.player.LocalPlayer","f_108586_","F")
NOVA_FREECAM(oldArm,"net.minecraft.client.player.LocalPlayer","f_108588_","F")
NOVA_FREECAM(minSection,"net.minecraft.world.level.LevelHeightAccessor","m_151560_","()I")
NOVA_FREECAM(sectionCount,"net.minecraft.world.level.LevelHeightAccessor","m_151559_","()I")
NOVA_FREECAM(setUuid,"net.minecraft.world.entity.Entity","m_20084_","(Ljava/util/UUID;)V")
NOVA_FREECAM(turn,"net.minecraft.world.entity.Entity","m_19884_","(DD)V")
#elif defined(NOVA_MLG_MAPPING)
NOVA_MLG(burning,"net.minecraft.world.entity.Entity","m_6060_","()Z")
NOVA_MLG(water,"net.minecraft.world.entity.Entity","m_20069_","()Z")
NOVA_MLG(lava,"net.minecraft.world.entity.Entity","m_20077_","()Z")
NOVA_MLG(passenger,"net.minecraft.world.entity.Entity","m_20159_","()Z")
NOVA_MLG(abilities,"net.minecraft.world.entity.player.Player","m_150110_","()Lnet/minecraft/world/entity/player/Abilities;")
NOVA_MLG(flying,"net.minecraft.world.entity.player.Abilities","f_35935_","Z")
NOVA_MLG(item,"net.minecraft.world.item.ItemStack","m_41720_","()Lnet/minecraft/world/item/Item;")
NOVA_MLG(emptyBucket,"net.minecraft.world.item.Items","f_42446_","Lnet/minecraft/world/item/Item;")
NOVA_MLG(provider,"net.minecraft.world.level.Level","m_6042_","()Lnet/minecraft/world/level/dimension/DimensionType;")
NOVA_MLG(evaporates,"net.minecraft.world.level.dimension.DimensionType","f_63857_","()Z")
NOVA_MLG(pos,"net.minecraft.core.BlockPos","<init>","(III)V")
NOVA_MLG(state,"net.minecraft.world.level.BlockGetter","m_8055_","(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;")
#elif defined(NOVA_VICTORY_MAPPING)
NOVA_VICTORY(chat, "net.minecraft.network.protocol.game.ClientboundSystemChatPacket", "f_237849_", "()Lnet/minecraft/network/chat/Component;")
NOVA_VICTORY(title, "net.minecraft.network.protocol.game.ClientboundSetTitleTextPacket", "m_179399_", "()Lnet/minecraft/network/chat/Component;")
NOVA_VICTORY(subtitle, "net.minecraft.network.protocol.game.ClientboundSetSubtitleTextPacket", "m_179385_", "()Lnet/minecraft/network/chat/Component;")
NOVA_VICTORY(actionbar, "net.minecraft.network.protocol.game.ClientboundSetActionBarTextPacket", "m_179210_", "()Lnet/minecraft/network/chat/Component;")
NOVA_VICTORY(overlay, "net.minecraft.network.protocol.game.ClientboundSystemChatPacket", "f_240374_", "()Z")
NOVA_VICTORY(text, "net.minecraft.network.chat.Component", "getString", "()Ljava/lang/String;")
#elif defined(NOVA_CRITICAL_MAPPING)
NOVA_CRITICAL(fall, "net.minecraft.world.entity.Entity", "f_19789_", "F")
NOVA_CRITICAL(ground, "net.minecraft.world.entity.Entity", "m_20096_", "()Z")
NOVA_CRITICAL(water, "net.minecraft.world.entity.Entity", "m_20069_", "()Z")
NOVA_CRITICAL(climbing, "net.minecraft.world.entity.LivingEntity", "m_6147_", "()Z")
NOVA_CRITICAL(passenger, "net.minecraft.world.entity.Entity", "m_20159_", "()Z")
NOVA_CRITICAL(sprinting, "net.minecraft.world.entity.Entity", "m_20142_", "()Z")
NOVA_CRITICAL(blind, "net.minecraft.world.effect.MobEffects", "f_19610_", "Lnet/minecraft/world/effect/MobEffect;")
NOVA_CRITICAL(hasEffect, "net.minecraft.world.entity.LivingEntity", "m_21023_", "(Lnet/minecraft/world/effect/MobEffect;)Z")
NOVA_CRITICAL(world, "net.minecraft.client.Minecraft", "f_91073_", "Lnet/minecraft/client/multiplayer/ClientLevel;")
NOVA_CRITICAL(pos, "net.minecraft.core.BlockPos", "<init>", "(III)V")
NOVA_CRITICAL(state, "net.minecraft.world.level.BlockGetter", "m_8055_", "(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;")
NOVA_CRITICAL(block, "net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase", "m_60734_", "()Lnet/minecraft/world/level/block/Block;")
NOVA_CRITICAL(sound, "net.minecraft.sounds.SoundEvents", "f_11871_", "Lnet/minecraft/sounds/SoundEvent;")
NOVA_CRITICAL(play, "net.minecraft.client.player.LocalPlayer", "m_5496_", "(Lnet/minecraft/sounds/SoundEvent;FF)V")
#elif defined(NOVA_ITEMESP_MAPPING)
NOVA_ITEMESP(stack, "net.minecraft.world.entity.item.ItemEntity", "m_32055_", "()Lnet/minecraft/world/item/ItemStack;")
NOVA_ITEMESP(item, "net.minecraft.world.item.ItemStack", "m_41720_", "()Lnet/minecraft/world/item/Item;")
NOVA_ITEMESP(count, "net.minecraft.world.item.ItemStack", "m_41613_", "()I")
NOVA_ITEMESP(iron, "net.minecraft.world.item.Items", "f_42416_", "Lnet/minecraft/world/item/Item;")
NOVA_ITEMESP(gold, "net.minecraft.world.item.Items", "f_42417_", "Lnet/minecraft/world/item/Item;")
NOVA_ITEMESP(diamond, "net.minecraft.world.item.Items", "f_42415_", "Lnet/minecraft/world/item/Item;")
NOVA_ITEMESP(emerald, "net.minecraft.world.item.Items", "f_42616_", "Lnet/minecraft/world/item/Item;")
#elif defined(NOVA_NAMETAGS_MAPPING)
NOVA_NAMETAGS(options, "net.minecraft.client.Minecraft", "f_91066_", "Lnet/minecraft/client/Options;")
NOVA_NAMETAGS(perspective, "net.minecraft.client.Options", "m_92176_", "()Lnet/minecraft/client/CameraType;")
NOVA_NAMETAGS(window, "net.minecraft.client.Minecraft", "m_91268_", "()Lcom/mojang/blaze3d/platform/Window;")
NOVA_NAMETAGS(scale, "com.mojang.blaze3d.platform.Window", "m_85449_", "()D")
NOVA_NAMETAGS(width, "net.minecraft.world.entity.Entity", "m_20205_", "()F")
NOVA_NAMETAGS(height, "net.minecraft.world.entity.Entity", "m_20206_", "()F")
NOVA_NAMETAGS(sneaking, "net.minecraft.world.entity.Entity", "m_6144_", "()Z")
NOVA_NAMETAGS(displayName, "net.minecraft.world.entity.Entity", "m_5446_", "()Lnet/minecraft/network/chat/Component;")
NOVA_NAMETAGS(flat, "net.minecraft.network.chat.Component", "m_240407_", "()Ljava/util/List;")
NOVA_NAMETAGS(text, "net.minecraft.network.chat.Component", "getString", "()Ljava/lang/String;")
NOVA_NAMETAGS(style, "net.minecraft.network.chat.Component", "m_7383_", "()Lnet/minecraft/network/chat/Style;")
NOVA_NAMETAGS(color, "net.minecraft.network.chat.Style", "m_131135_", "()Lnet/minecraft/network/chat/TextColor;")
NOVA_NAMETAGS(rgb, "net.minecraft.network.chat.TextColor", "m_131265_", "()I")
NOVA_NAMETAGS(bold, "net.minecraft.network.chat.Style", "m_131154_", "()Z")
NOVA_NAMETAGS(italic, "net.minecraft.network.chat.Style", "m_131161_", "()Z")
NOVA_NAMETAGS(strike, "net.minecraft.network.chat.Style", "m_131168_", "()Z")
NOVA_NAMETAGS(underline, "net.minecraft.network.chat.Style", "m_131171_", "()Z")
#elif defined(NOVA_AUTOPLAY_MAPPING)
NOVA_AUTOPLAY_CLASS(packet, "net.minecraft.network.protocol.game.ClientboundSystemChatPacket")
NOVA_AUTOPLAY_CLASS(join, "net.minecraft.network.protocol.game.ClientboundLoginPacket")
NOVA_AUTOPLAY(overlay, "net.minecraft.network.protocol.game.ClientboundSystemChatPacket", "f_240374_", "()Z")
NOVA_AUTOPLAY(content, "net.minecraft.network.protocol.game.ClientboundSystemChatPacket", "f_237849_", "()Lnet/minecraft/network/chat/Component;")
NOVA_AUTOPLAY(text, "net.minecraft.network.chat.Component", "getString", "()Ljava/lang/String;")
NOVA_AUTOPLAY(siblings, "net.minecraft.network.chat.Component", "m_7360_", "()Ljava/util/List;")
NOVA_AUTOPLAY(style, "net.minecraft.network.chat.Component", "m_7383_", "()Lnet/minecraft/network/chat/Style;")
NOVA_AUTOPLAY(click, "net.minecraft.network.chat.Style", "m_131182_", "()Lnet/minecraft/network/chat/ClickEvent;")
NOVA_AUTOPLAY(action, "net.minecraft.network.chat.ClickEvent", "m_130622_", "()Lnet/minecraft/network/chat/ClickEvent$Action;")
NOVA_AUTOPLAY(value, "net.minecraft.network.chat.ClickEvent", "m_130623_", "()Ljava/lang/String;")
NOVA_AUTOPLAY(connection, "net.minecraft.client.Minecraft", "m_91403_", "()Lnet/minecraft/client/multiplayer/ClientPacketListener;")
NOVA_AUTOPLAY(send, "net.minecraft.client.multiplayer.ClientPacketListener", "m_246623_", "(Ljava/lang/String;)V")
#elif defined(NOVA_ESP2D_MAPPING)
NOVA_ESP2D_CLASS("net.minecraft.world.entity.decoration.ArmorStand")
NOVA_ESP2D(armor, "net.minecraft.world.entity.LivingEntity", "m_21230_", "()I")
NOVA_ESP2D(width, "net.minecraft.world.entity.Entity", "m_20205_", "()F")
NOVA_ESP2D(height, "net.minecraft.world.entity.Entity", "m_20206_", "()F")
NOVA_ESP2D(sneaking, "net.minecraft.world.entity.Entity", "m_6144_", "()Z")
NOVA_ESP2D(view, "net.minecraft.client.Minecraft", "m_91288_", "()Lnet/minecraft/world/entity/Entity;")
NOVA_ESP2D(options, "net.minecraft.client.Minecraft", "f_91066_", "Lnet/minecraft/client/Options;")
NOVA_ESP2D(perspective, "net.minecraft.client.Options", "m_92176_", "()Lnet/minecraft/client/CameraType;")
NOVA_ESP2D(window, "net.minecraft.client.Minecraft", "m_91268_", "()Lcom/mojang/blaze3d/platform/Window;")
NOVA_ESP2D(scale, "com.mojang.blaze3d.platform.Window", "m_85449_", "()D")
#elif defined(NOVA_MOVEMENT_KEYS_MAPPING)
NOVA_MOVEMENT_KEY(sprint,"net.minecraft.client.Options","f_92091_","Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(drop,"net.minecraft.client.Options","f_92094_","Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(swap,"net.minecraft.client.Options","f_92093_","Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(sneak,"net.minecraft.client.Options","f_92090_","Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(attack,"net.minecraft.client.Options","f_92096_","Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(use,"net.minecraft.client.Options","f_92095_","Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(clicks,"net.minecraft.client.KeyMapping","f_90818_","I")
NOVA_MOVEMENT_KEY(options, "net.minecraft.client.Minecraft", "f_91066_", "Lnet/minecraft/client/Options;")
NOVA_MOVEMENT_KEY(jump, "net.minecraft.client.Options", "f_92089_", "Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(forward, "net.minecraft.client.Options", "f_92085_", "Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(back, "net.minecraft.client.Options", "f_92087_", "Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(left, "net.minecraft.client.Options", "f_92086_", "Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(right, "net.minecraft.client.Options", "f_92088_", "Lnet/minecraft/client/KeyMapping;")
NOVA_MOVEMENT_KEY(key, "net.minecraft.client.KeyMapping", "f_90816_", "Lcom/mojang/blaze3d/platform/InputConstants$Key;")
NOVA_MOVEMENT_KEY(keyCode, "com.mojang.blaze3d.platform.InputConstants$Key", "m_84873_", "()I")
NOVA_MOVEMENT_KEY(keyType, "com.mojang.blaze3d.platform.InputConstants$Key", "m_84868_", "()Lcom/mojang/blaze3d/platform/InputConstants$Type;")
NOVA_MOVEMENT_KEY(pressed, "net.minecraft.client.KeyMapping", "f_90817_", "Z")
NOVA_MOVEMENT_KEY(window, "net.minecraft.client.Minecraft", "m_91268_", "()Lcom/mojang/blaze3d/platform/Window;")
NOVA_MOVEMENT_KEY(windowHandle, "com.mojang.blaze3d.platform.Window", "m_85439_", "()J")
#elif defined(NOVA_INVENTORY_SCREEN_MAPPING)
NOVA_INVENTORY_SCREEN(show, "net.minecraft.client.Minecraft", "m_91152_", "(Lnet/minecraft/client/gui/screens/Screen;)V")
NOVA_INVENTORY_SCREEN(inventory, "net.minecraft.client.gui.screens.inventory.InventoryScreen", "<init>", "(Lnet/minecraft/world/entity/player/Player;)V")
NOVA_INVENTORY_SCREEN(carried, "net.minecraft.world.inventory.AbstractContainerMenu", "m_142621_", "()Lnet/minecraft/world/item/ItemStack;")
#elif defined(NOVA_AUTOPOT_MAPPING)
NOVA_AUTOPOT_CLASS("net.minecraft.world.item.SplashPotionItem")
NOVA_AUTOPOT(stackItem, "net.minecraft.world.item.ItemStack", "m_41720_", "()Lnet/minecraft/world/item/Item;")
NOVA_AUTOPOT(stackCount, "net.minecraft.world.item.ItemStack", "m_41613_", "()I")
NOVA_AUTOPOT(effects, "net.minecraft.world.item.alchemy.PotionUtils", "m_43547_", "(Lnet/minecraft/world/item/ItemStack;)Ljava/util/List;")
NOVA_AUTOPOT(effectKind, "net.minecraft.world.effect.MobEffectInstance", "m_19576_", "()Ljava/lang/String;")
NOVA_AUTOPOT(controller, "net.minecraft.client.Minecraft", "f_91072_", "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;")
NOVA_AUTOPOT(sync, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "m_105297_", "()V")
NOVA_AUTOPOT(use, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "m_233721_", "(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/InteractionHand;)Lnet/minecraft/world/InteractionResult;")
NOVA_AUTOPOT(hand, "net.minecraft.world.InteractionHand", "MAIN_HAND", "Lnet/minecraft/world/InteractionHand;")
NOVA_AUTOPOT(consumesAction, "net.minecraft.world.InteractionResult", "m_19077_", "()Z")
#elif defined(NOVA_KEEP_SPRINT_MAPPING)
{"net.minecraft.world.entity.Entity", "m_20096_", "()Z"},
{"net.minecraft.world.entity.LivingEntity", "f_20916_", "I"},
{"net.minecraft.world.entity.Entity", "m_20184_", "()Lnet/minecraft/world/phys/Vec3;"},
{"net.minecraft.world.phys.Vec3", "m_82542_", "(DDD)Lnet/minecraft/world/phys/Vec3;"},
{"net.minecraft.world.entity.Entity", "m_20256_", "(Lnet/minecraft/world/phys/Vec3;)V"},
{"net.minecraft.world.entity.Entity", "m_6858_", "(Z)V"}
#elif defined(NOVA_FREELOOK_MAPPING)
NOVA_FREELOOK(options, "net.minecraft.client.Minecraft", "f_91066_", "Lnet/minecraft/client/Options;")
NOVA_FREELOOK(view, "net.minecraft.client.Minecraft", "m_91288_", "()Lnet/minecraft/world/entity/Entity;")
NOVA_FREELOOK(perspective, "net.minecraft.client.Options", "m_92176_", "()Lnet/minecraft/client/CameraType;")
NOVA_FREELOOK(setPerspective, "net.minecraft.client.Options", "m_92157_", "(Lnet/minecraft/client/CameraType;)V")
NOVA_FREELOOK(cameraValues, "net.minecraft.client.CameraType", "values", "()[Lnet/minecraft/client/CameraType;")
NOVA_FREELOOK(fov, "net.minecraft.client.Options", "m_231837_", "()Lnet/minecraft/client/OptionInstance;")
NOVA_FREELOOK(optionValue, "net.minecraft.client.OptionInstance", "f_231481_", "Ljava/lang/Object;")
NOVA_FREELOOK(invertY, "net.minecraft.client.Options", "m_231820_", "()Lnet/minecraft/client/OptionInstance;")
NOVA_FREELOOK(yaw, "net.minecraft.world.entity.Entity", "m_146908_", "()F")
NOVA_FREELOOK(pitch, "net.minecraft.world.entity.Entity", "m_146909_", "()F")
NOVA_FREELOOK(previousYaw, "net.minecraft.world.entity.Entity", "f_19859_", "F")
NOVA_FREELOOK(previousPitch, "net.minecraft.world.entity.Entity", "f_19860_", "F")
NOVA_FREELOOK(turn, "net.minecraft.world.entity.Entity", "m_19884_", "(DD)V")
NOVA_FREELOOK(viewYaw, "net.minecraft.world.entity.Entity", "m_5675_", "(F)F")
NOVA_FREELOOK(viewPitch, "net.minecraft.world.entity.Entity", "m_5686_", "(F)F")
NOVA_FREELOOK(armYaw, "net.minecraft.client.player.LocalPlayer", "f_108585_", "F")
NOVA_FREELOOK(previousArmYaw, "net.minecraft.client.player.LocalPlayer", "f_108587_", "F")
#elif defined(NOVA_BREAKER_MAPPING)
NOVA_BREAKER(continueMining, "net.minecraft.client.Minecraft", "m_91386_", "(Z)V")
NOVA_BREAKER(controller, "net.minecraft.client.Minecraft", "f_91072_", "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;")
NOVA_BREAKER(level, "net.minecraft.client.Minecraft", "f_91073_", "Lnet/minecraft/client/multiplayer/ClientLevel;")
NOVA_BREAKER(progress, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "f_105193_", "F")
NOVA_BREAKER(pos, "net.minecraft.core.BlockPos", "<init>", "(III)V")
NOVA_BREAKER(state, "net.minecraft.world.level.Level", "m_8055_", "(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;")
NOVA_BREAKER(block, "net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase", "m_60734_", "()Lnet/minecraft/world/level/block/Block;")
NOVA_BREAKER(hardness, "net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase", "m_60800_", "(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/core/BlockPos;)F")
NOVA_BREAKER(relative, "net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase", "m_60625_", "(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/core/BlockPos;)F")
NOVA_BREAKER(air, "net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase", "m_60795_", "()Z")
NOVA_BREAKER(collision, "net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase", "m_60812_", "(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/phys/shapes/VoxelShape;")
NOVA_BREAKER(empty, "net.minecraft.world.phys.shapes.VoxelShape", "m_83281_", "()Z")
NOVA_BREAKER(damage, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "m_105283_", "(Lnet/minecraft/core/BlockPos;Lnet/minecraft/core/Direction;)Z")
NOVA_BREAKER(destroy, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "m_105267_", "(Lnet/minecraft/core/BlockPos;)Z")
NOVA_BREAKER(stop, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "m_105276_", "()V")
NOVA_BREAKER(ground, "net.minecraft.world.entity.Entity", "f_19861_", "Z")
NOVA_BREAKER(facing, "net.minecraft.core.Direction", "values", "()[Lnet/minecraft/core/Direction;")
NOVA_BREAKER(vector, "net.minecraft.world.phys.Vec3", "<init>", "(DDD)V")
NOVA_BREAKER(clip, "net.minecraft.world.phys.shapes.VoxelShape", "m_83220_", "(Lnet/minecraft/world/phys/Vec3;Lnet/minecraft/world/phys/Vec3;Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/phys/BlockHitResult;")
NOVA_BREAKER(shape, "net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase", "m_60808_", "(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/phys/shapes/VoxelShape;")
NOVA_BREAKER(hit, "net.minecraft.client.Minecraft", "f_91077_", "Lnet/minecraft/world/phys/HitResult;")
NOVA_BREAKER(hitFace, "net.minecraft.world.phys.BlockHitResult", "m_82434_", "()Lnet/minecraft/core/Direction;")
NOVA_BREAKER(dig, "net.minecraft.network.protocol.game.ServerboundPlayerActionPacket", "<init>", "(Lnet/minecraft/network/protocol/game/ServerboundPlayerActionPacket$Action;Lnet/minecraft/core/BlockPos;Lnet/minecraft/core/Direction;)V")
NOVA_BREAKER(digAction, "net.minecraft.network.protocol.game.ServerboundPlayerActionPacket", "m_134285_", "()Lnet/minecraft/network/protocol/game/ServerboundPlayerActionPacket$Action;")
NOVA_BREAKER(actions, "net.minecraft.network.protocol.game.ServerboundPlayerActionPacket$Action", "values", "()[Lnet/minecraft/network/protocol/game/ServerboundPlayerActionPacket$Action;")
NOVA_BREAKER(updatePos, "net.minecraft.network.protocol.game.ClientboundBlockUpdatePacket", "m_131749_", "()Lnet/minecraft/core/BlockPos;")
NOVA_BREAKER(updateState, "net.minecraft.network.protocol.game.ClientboundBlockUpdatePacket", "m_131746_", "()Lnet/minecraft/world/level/block/state/BlockState;")
NOVA_BREAKER(posX, "net.minecraft.core.Vec3i", "m_123341_", "()I")
NOVA_BREAKER(posY, "net.minecraft.core.Vec3i", "m_123342_", "()I")
NOVA_BREAKER(posZ, "net.minecraft.core.Vec3i", "m_123343_", "()I")
NOVA_BREAKER(teleX, "net.minecraft.network.protocol.game.ClientboundPlayerPositionPacket", "m_132818_", "()D")
NOVA_BREAKER(teleY, "net.minecraft.network.protocol.game.ClientboundPlayerPositionPacket", "m_132821_", "()D")
NOVA_BREAKER(teleZ, "net.minecraft.network.protocol.game.ClientboundPlayerPositionPacket", "m_132822_", "()D")
NOVA_BREAKER(teleFlags, "net.minecraft.network.protocol.game.ClientboundPlayerPositionPacket", "m_132826_", "()Ljava/util/Set;")
NOVA_BREAKER_CLASSES("net.minecraft.world.level.block.BedBlock", "net.minecraft.world.level.block.AirBlock", "net.minecraft.world.level.block.LiquidBlock")
#elif defined(NOVA_JUMP_DELAY_MAPPING)
{"net.minecraft.world.entity.LivingEntity", "f_20954_", "I"}
#elif defined(NOVA_SLOT_SPOOF_MAPPING)
"m_150109_", "()Lnet/minecraft/world/entity/player/Inventory;", "f_35977_",
"m_8020_", "(I)Lnet/minecraft/world/item/ItemStack;",
"f_91072_", "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;", "m_105297_"
#elif defined(NOVA_BEDPLATES_MAPPING)
#include "minecraft/compat/BedMapping.h"
namespace nova::compat {
inline constexpr BedMapping Bed1201Mapping{
    "net.minecraft.core.BlockPos", "net.minecraft.world.level.block.BedBlock",
    "Lnet/minecraft/core/BlockPos;", "Lnet/minecraft/world/level/block/state/BlockState;", "Lnet/minecraft/world/level/block/Block;", "Lnet/minecraft/world/level/chunk/LevelChunk;",
    "m_8055_", "m_60734_", "m_6325_", "m_62954_", "m_123341_", "m_123342_", "m_123343_",
    "m_60800_", "(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/core/BlockPos;)F", "m_280296_"
};
}
#elif defined(NOVA_BRIDGEASSIST_MAPPING)
NOVA_BRIDGE(abilities, "net.minecraft.world.entity.player.Player", "m_150110_", "()Lnet/minecraft/world/entity/player/Abilities;")
NOVA_BRIDGE(block, "net.minecraft.world.level.block.state.BlockBehaviour$BlockStateBase", "m_60734_", "()Lnet/minecraft/world/level/block/Block;")
NOVA_BRIDGE(boxCtor, "net.minecraft.world.phys.AABB", "<init>", "(DDDDDD)V")
NOVA_BRIDGE(collisions, "net.minecraft.world.level.CollisionGetter", "m_186431_", "(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/world/phys/AABB;)Ljava/lang/Iterable;")
NOVA_BRIDGE(controller, "net.minecraft.client.Minecraft", "f_91072_", "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;")
NOVA_BRIDGE(flying, "net.minecraft.world.entity.player.Abilities", "f_35935_", "Z")
NOVA_BRIDGE(food, "net.minecraft.world.entity.player.Player", "m_36324_", "()Lnet/minecraft/world/food/FoodData;")
NOVA_BRIDGE(foodLevel, "net.minecraft.world.food.FoodData", "m_38702_", "()I")
NOVA_BRIDGE(forward, "net.minecraft.client.player.Input", "f_108567_", "F")
NOVA_BRIDGE(friction, "net.minecraft.world.level.block.Block", "m_49958_", "()F")
NOVA_BRIDGE(horizontal, "net.minecraft.world.entity.Entity", "f_19862_", "Z")
NOVA_BRIDGE(inLava, "net.minecraft.world.entity.Entity", "m_20077_", "()Z")
NOVA_BRIDGE(inWater, "net.minecraft.world.entity.Entity", "m_20069_", "()Z")
NOVA_BRIDGE(input, "net.minecraft.client.player.LocalPlayer", "f_108618_", "Lnet/minecraft/client/player/Input;")
NOVA_BRIDGE(jumpBoost, "net.minecraft.world.entity.LivingEntity", "m_285755_", "()F")
NOVA_BRIDGE(jumpCooldown, "net.minecraft.world.entity.LivingEntity", "f_20954_", "I")
NOVA_BRIDGE(jumpPower, "net.minecraft.world.entity.LivingEntity", "m_6118_", "()F")
NOVA_BRIDGE(key, "net.minecraft.client.KeyMapping", "f_90816_", "Lcom/mojang/blaze3d/platform/InputConstants$Key;")
NOVA_BRIDGE(keyCode, "com.mojang.blaze3d.platform.InputConstants$Key", "m_84873_", "()I")
NOVA_BRIDGE(keyType, "com.mojang.blaze3d.platform.InputConstants$Key", "m_84868_", "()Lcom/mojang/blaze3d/platform/InputConstants$Type;")
NOVA_BRIDGE(ladder, "net.minecraft.world.entity.LivingEntity", "m_6147_", "()Z")
NOVA_BRIDGE(maxX, "net.minecraft.world.phys.AABB", "f_82291_", "D")
NOVA_BRIDGE(maxY, "net.minecraft.world.phys.AABB", "f_82292_", "D")
NOVA_BRIDGE(maxZ, "net.minecraft.world.phys.AABB", "f_82293_", "D")
NOVA_BRIDGE(minX, "net.minecraft.world.phys.AABB", "f_82288_", "D")
NOVA_BRIDGE(minY, "net.minecraft.world.phys.AABB", "f_82289_", "D")
NOVA_BRIDGE(minZ, "net.minecraft.world.phys.AABB", "f_82290_", "D")
NOVA_BRIDGE(options, "net.minecraft.client.Minecraft", "f_91066_", "Lnet/minecraft/client/Options;")
NOVA_BRIDGE(posCtor, "net.minecraft.core.BlockPos", "<init>", "(III)V")
NOVA_BRIDGE(reach, "net.minecraft.client.multiplayer.MultiPlayerGameMode", "m_105286_", "()F")
NOVA_BRIDGE(riding, "net.minecraft.world.entity.Entity", "m_20159_", "()Z")
NOVA_BRIDGE(shapeBoxes, "net.minecraft.world.phys.shapes.VoxelShape", "m_83299_", "()Ljava/util/List;")
NOVA_BRIDGE(sleeping, "net.minecraft.world.entity.LivingEntity", "m_5803_", "()Z")
NOVA_BRIDGE(sneakKey, "net.minecraft.client.Options", "f_92090_", "Lnet/minecraft/client/KeyMapping;")
NOVA_BRIDGE(state, "net.minecraft.world.level.BlockGetter", "m_8055_", "(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;")
NOVA_BRIDGE(step, "net.minecraft.world.entity.Entity", "m_274421_", "()F")
NOVA_BRIDGE(window, "net.minecraft.client.Minecraft", "m_91268_", "()Lcom/mojang/blaze3d/platform/Window;")
NOVA_BRIDGE(windowHandle, "com.mojang.blaze3d.platform.Window", "m_85439_", "()J")
NOVA_BRIDGE(world, "net.minecraft.client.Minecraft", "f_91073_", "Lnet/minecraft/client/multiplayer/ClientLevel;")
#elif defined(NOVA_PLAYER_LIST_MAPPING)
NOVA_PLAYER_LIST(Uuid, "net.minecraft.world.entity.Entity", "m_20148_", "()Ljava/util/UUID;")
NOVA_PLAYER_LIST(Connection, "net.minecraft.client.Minecraft", "m_91403_", "()Lnet/minecraft/client/multiplayer/ClientPacketListener;")
NOVA_PLAYER_LIST(OnlineIds, "net.minecraft.client.multiplayer.ClientPacketListener", "m_105143_", "()Ljava/util/Collection;")
NOVA_PLAYER_LIST(Contains, "java.util.Collection", "contains", "(Ljava/lang/Object;)Z")
#elif defined(NOVA_TEAMS_MAPPING)
// UUID, connection, player info, team, name, color container, color, value.
NOVA_TEAM(Uuid, "net.minecraft.world.entity.Entity", "m_20148_", "()Ljava/util/UUID;")
NOVA_TEAM(Connection, "net.minecraft.client.Minecraft", "m_91403_", "()Lnet/minecraft/client/multiplayer/ClientPacketListener;")
NOVA_TEAM(Info, "net.minecraft.client.multiplayer.ClientPacketListener", "m_104949_", "(Ljava/util/UUID;)Lnet/minecraft/client/multiplayer/PlayerInfo;")
NOVA_TEAM(Team, "net.minecraft.client.multiplayer.PlayerInfo", "m_105340_", "()Lnet/minecraft/world/scores/PlayerTeam;")
NOVA_TEAM(EntityTeam, "net.minecraft.world.entity.Entity", "m_5647_", "()Lnet/minecraft/world/scores/Team;")
NOVA_TEAM(Name, "net.minecraft.world.scores.PlayerTeam", "m_5758_", "()Ljava/lang/String;")
NOVA_TEAM(Format, "net.minecraft.world.scores.PlayerTeam", "m_7414_", "()Lnet/minecraft/ChatFormatting;")
NOVA_TEAM(Color, "net.minecraft.ChatFormatting", "m_126665_", "()Ljava/lang/Integer;")
NOVA_TEAM(Value, "java.lang.Integer", "intValue", "()I")
// Armor team detection; only members used by this version are resolved.
NOVA_TEAM(Equipped, "net.minecraft.world.entity.LivingEntity", "m_6844_", "(Lnet/minecraft/world/entity/EquipmentSlot;)Lnet/minecraft/world/item/ItemStack;")
NOVA_TEAM(StackItem, "net.minecraft.world.item.ItemStack", "m_41720_", "()Lnet/minecraft/world/item/Item;")
NOVA_TEAM(HasCustomColor, "net.minecraft.world.item.DyeableLeatherItem", "m_41113_", "(Lnet/minecraft/world/item/ItemStack;)Z")
NOVA_TEAM(ArmorColor, "net.minecraft.world.item.DyeableLeatherItem", "m_41121_", "(Lnet/minecraft/world/item/ItemStack;)I")
NOVA_TEAM(HeadSlot, "net.minecraft.world.entity.EquipmentSlot", "HEAD", "Lnet/minecraft/world/entity/EquipmentSlot;")
NOVA_TEAM(ChestSlot, "net.minecraft.world.entity.EquipmentSlot", "CHEST", "Lnet/minecraft/world/entity/EquipmentSlot;")
#elif defined(NOVA_HUD_MAPPING)
NOVA_HUD("server", "net.minecraft.client.Minecraft", "m_91089_", "()Lnet/minecraft/client/multiplayer/ServerData;")
NOVA_HUD("address", "net.minecraft.client.multiplayer.ServerData", "f_105363_", "Ljava/lang/String;")
NOVA_HUD("local", "net.minecraft.client.Minecraft", "m_91090_", "()Z")
NOVA_HUD("uuid", "net.minecraft.world.entity.Entity", "m_20148_", "()Ljava/util/UUID;")
NOVA_HUD("connection", "net.minecraft.client.Minecraft", "m_91403_", "()Lnet/minecraft/client/multiplayer/ClientPacketListener;")
NOVA_HUD("info", "net.minecraft.client.multiplayer.ClientPacketListener", "m_104949_", "(Ljava/util/UUID;)Lnet/minecraft/client/multiplayer/PlayerInfo;")
NOVA_HUD("latency", "net.minecraft.client.multiplayer.PlayerInfo", "m_105330_", "()I")
NOVA_HUD("time", "net.minecraft.network.protocol.game.ClientboundSetTimePacket", "m_133358_", "()J")
#elif defined(NOVA_BACKTRACK_MAPPING)
NOVA_BT("Move", "net.minecraft.network.protocol.game.ClientboundMoveEntityPacket", "", "")
NOVA_BT("Teleport", "net.minecraft.network.protocol.game.ClientboundTeleportEntityPacket", "", "")
NOVA_BT("Health", "net.minecraft.network.protocol.game.ClientboundSetHealthPacket", "", "")
NOVA_BT("Bundle", "net.minecraft.network.protocol.BundlePacket", "", "")
NOVA_BT("Codec", "net.minecraft.network.protocol.game.VecDeltaCodec", "", "")
NOVA_BT("Vec", "net.minecraft.world.phys.Vec3", "", "")
NOVA_BT("moveId", "net.minecraft.network.protocol.game.ClientboundMoveEntityPacket", "f_132499_", "I")
NOVA_BT("teleId", "net.minecraft.network.protocol.game.ClientboundTeleportEntityPacket", "f_133529_", "I")
NOVA_BT("health", "net.minecraft.network.protocol.game.ClientboundSetHealthPacket", "f_133233_", "F")
NOVA_BT("bundlePackets", "net.minecraft.network.protocol.BundlePacket", "m_264216_", "()Ljava/lang/Iterable;")
NOVA_BT("codec", "net.minecraft.world.entity.Entity", "f_216985_", "Lnet/minecraft/network/protocol/game/VecDeltaCodec;")
NOVA_BT("base", "net.minecraft.network.protocol.game.VecDeltaCodec", "f_238015_", "Lnet/minecraft/world/phys/Vec3;")
NOVA_BT("x", "net.minecraft.world.phys.Vec3", "f_82479_", "D")
NOVA_BT("y", "net.minecraft.world.phys.Vec3", "f_82480_", "D")
NOVA_BT("z", "net.minecraft.world.phys.Vec3", "f_82481_", "D")
NOVA_BT("dx", "net.minecraft.network.protocol.game.ClientboundMoveEntityPacket", "f_132500_", "S")
NOVA_BT("dy", "net.minecraft.network.protocol.game.ClientboundMoveEntityPacket", "f_132501_", "S")
NOVA_BT("dz", "net.minecraft.network.protocol.game.ClientboundMoveEntityPacket", "f_132502_", "S")
NOVA_BT("tx", "net.minecraft.network.protocol.game.ClientboundTeleportEntityPacket", "f_133530_", "D")
NOVA_BT("ty", "net.minecraft.network.protocol.game.ClientboundTeleportEntityPacket", "f_133531_", "D")
NOVA_BT("tz", "net.minecraft.network.protocol.game.ClientboundTeleportEntityPacket", "f_133532_", "D")
#elif defined(NOVA_FORGE1201_PROFILE)
#include "minecraft/compat/Profile.h"

namespace nova::compat {
inline constexpr const char* Forge1201ChatScreenDot = "net.minecraft.client.gui.screens.ChatScreen";
inline constexpr const char* Forge1201ChatScreenSig = "Lnet/minecraft/client/gui/screens/ChatScreen;";
inline constexpr const char* Forge1201LocalServer = "m_91090_";
inline constexpr const char* Forge1201Connection = "m_91403_";
inline constexpr const char* Forge1201ConnectionDesc = "()Lnet/minecraft/client/multiplayer/ClientPacketListener;";
inline constexpr GameMapping Forge1201Mapping{
    "m_91087_", "f_91074_", "Lnet/minecraft/client/player/LocalPlayer;",
    "f_91073_", "Lnet/minecraft/client/multiplayer/ClientLevel;",
    "f_91080_", "Lnet/minecraft/client/gui/screens/Screen;"
};
}
#elif defined(NOVA_FORGE1201_CHEST)
// CHEST
constexpr const char* kMinecraftDot    = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig    = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kGameRendererDot = "net.minecraft.client.renderer.GameRenderer";
constexpr const char* kGameRendererSig = "Lnet/minecraft/client/renderer/GameRenderer;";
constexpr const char* kCameraDot       = "net.minecraft.client.Camera";
constexpr const char* kCameraSig       = "Lnet/minecraft/client/Camera;";
constexpr const char* kLevelDot        = "net.minecraft.world.level.Level";
constexpr const char* kLevelSig        = "Lnet/minecraft/world/level/Level;";
constexpr const char* kClientLevelSig  = "Lnet/minecraft/client/multiplayer/ClientLevel;";
constexpr const char* kChunkDot        = "net.minecraft.world.level.chunk.LevelChunk";
constexpr const char* kChunkSig        = "Lnet/minecraft/world/level/chunk/LevelChunk;";
constexpr const char* kBlockEntityDot  = "net.minecraft.world.level.block.entity.BlockEntity";
constexpr const char* kBlockEntitySig  = "Lnet/minecraft/world/level/block/entity/BlockEntity;";
constexpr const char* kChestEntityDot  = "net.minecraft.world.level.block.entity.ChestBlockEntity";
constexpr const char* kChestEntitySig =
    "Lnet/minecraft/world/level/block/entity/ChestBlockEntity;";
constexpr const char* kBlockStateDot   = "net.minecraft.world.level.block.state.BlockState";
constexpr const char* kBlockStateSig   = "Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kShapeDot        = "net.minecraft.world.phys.shapes.VoxelShape";
constexpr const char* kShapeSig        = "Lnet/minecraft/world/phys/shapes/VoxelShape;";
constexpr const char* kChestBlockDot   = "net.minecraft.world.level.block.ChestBlock";
constexpr const char* kChestBlockSig   = "Lnet/minecraft/world/level/block/ChestBlock;";
constexpr const char* kBlocksDot       = "net.minecraft.world.level.block.Blocks";
constexpr const char* kBlocksSig       = "Lnet/minecraft/world/level/block/Blocks;";
constexpr const char* kBlockSig        = "Lnet/minecraft/world/level/block/Block;";
constexpr const char* kPosDot          = "net.minecraft.core.BlockPos";
constexpr const char* kPosSig          = "Lnet/minecraft/core/BlockPos;";
constexpr const char* kDirectionSig    = "Lnet/minecraft/core/Direction;";
constexpr const char* kVecDot          = "net.minecraft.world.phys.Vec3";
constexpr const char* kVecSig          = "Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kAabbDot         = "net.minecraft.world.phys.AABB";
constexpr const char* kAabbSig         = "Lnet/minecraft/world/phys/AABB;";
constexpr const char* kMapSig          = "Ljava/util/Map;";
constexpr const char* kCollectionSig   = "Ljava/util/Collection;";
constexpr const char* kIteratorSig     = "Ljava/util/Iterator;";
constexpr const char* kBlockEventDot =
    "net.minecraft.network.protocol.game.ClientboundBlockEventPacket";
constexpr const char* kBlockEventSig =
    "Lnet/minecraft/network/protocol/game/ClientboundBlockEventPacket;";
constexpr const char* kGetInstance     = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kGameRendererField = "f_91063_";
constexpr const char* kOptionsField    = "f_91066_";
constexpr const char* kOptionsSig      = "Lnet/minecraft/client/Options;";
constexpr const char* kRenderDistance  = "m_193772_";
constexpr const char* kLevelField      = "f_91073_";
constexpr const char* kGetMainCamera   = "m_109153_";
constexpr const char* kGetMainCameraDesc = "()Lnet/minecraft/client/Camera;";
constexpr const char* kGetFov          = "m_109141_";
constexpr const char* kGetFovDesc      = "(Lnet/minecraft/client/Camera;FZ)D";
constexpr const char* kGetProjection   = "m_253088_";
constexpr const char* kGetProjectionDesc = "(D)Lorg/joml/Matrix4f;";
constexpr const char* kGetWindow       = "m_91268_";
constexpr const char* kGetWindowDesc   = "()Lcom/mojang/blaze3d/platform/Window;";
constexpr const char* kWindowWidth     = "m_85441_";
constexpr const char* kWindowHeight    = "m_85442_";
constexpr const char* kBooleanValue    = "booleanValue";
constexpr const char* kMatrixDot       = "org.joml.Matrix4f";
constexpr const char* kMatrixSig       = "Lorg/joml/Matrix4f;";
constexpr const char* kBobViewField    = "f_92080_";
constexpr const char* kPlayerField     = "f_91074_";
constexpr const char* kLocalPlayerSig  = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kWalkDist        = "f_19787_";
constexpr const char* kWalkDistO       = "f_19867_";
constexpr const char* kBob             = "f_36100_";
constexpr const char* kOBob            = "f_36099_";
constexpr const char* kFloatField      = "F";
constexpr const char* kOptionInstanceSig = "Lnet/minecraft/client/OptionInstance;";
constexpr const char* kOptionGet       = "m_231551_";
constexpr const char* kOptionGetDesc   = "()Ljava/lang/Object;";
constexpr const char* kCameraPosition  = "m_90583_";
constexpr const char* kCameraPositionDesc = "()Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kCameraYRot      = "m_90590_";
constexpr const char* kCameraXRot      = "m_90589_";
constexpr const char* kFloatDesc       = "()F";
constexpr const char* kBoolDesc        = "()Z";
constexpr const char* kIntDesc         = "()I";
constexpr const char* kGetChunk        = "m_6325_";
constexpr const char* kGetChunkDesc    = "(II)Lnet/minecraft/world/level/chunk/LevelChunk;";
constexpr const char* kGetBlockEntities = "m_62954_";
constexpr const char* kGetBlockEntitiesDesc = "()Ljava/util/Map;";
constexpr const char* kMapValues       = "values";
constexpr const char* kMapValuesDesc   = "()Ljava/util/Collection;";
constexpr const char* kIterator        = "iterator";
constexpr const char* kIteratorDesc    = "()Ljava/util/Iterator;";
constexpr const char* kHasNext         = "hasNext";
constexpr const char* kHasNextDesc     = "()Z";
constexpr const char* kNext            = "next";
constexpr const char* kNextDesc        = "()Ljava/lang/Object;";
constexpr const char* kGetBlockState   = "m_58900_";
constexpr const char* kGetBlockStateDesc = "()Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kGetBlockPos     = "m_58899_";
constexpr const char* kGetBlockPosDesc = "()Lnet/minecraft/core/BlockPos;";
constexpr const char* kGetValue        = "m_61143_";
constexpr const char* kGetValueDesc =
    "(Lnet/minecraft/world/level/block/state/properties/Property;)Ljava/lang/Comparable;";
constexpr const char* kHasProperty     = "m_61138_";
constexpr const char* kHasPropertyDesc =
    "(Lnet/minecraft/world/level/block/state/properties/Property;)Z";
constexpr const char* kChestType       = "f_51479_";
constexpr const char* kChestTypeSig =
    "Lnet/minecraft/world/level/block/state/properties/EnumProperty;";
constexpr const char* kConnectedDir    = "m_51584_";
constexpr const char* kConnectedDirDesc =
    "(Lnet/minecraft/world/level/block/state/BlockState;)Lnet/minecraft/core/Direction;";
constexpr const char* kGetShape        = "m_60812_";
constexpr const char* kGetShapeDesc =
    "(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/core/BlockPos;)"
    "Lnet/minecraft/world/phys/shapes/VoxelShape;";
constexpr const char* kShapeBounds     = "m_83215_";
constexpr const char* kShapeBoundsDesc = "()Lnet/minecraft/world/phys/AABB;";
constexpr const char* kShapeEmpty      = "m_83281_";
constexpr const char* kLevelGetState   = "m_8055_";
constexpr const char* kLevelGetStateDesc =
    "(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kPosCtor         = "<init>";
constexpr const char* kPosCtorDesc     = "(III)V";
constexpr const char* kPosX            = "m_123341_";
constexpr const char* kPosY            = "m_123342_";
constexpr const char* kPosZ            = "m_123343_";
constexpr const char* kRelative        = "m_121945_";
constexpr const char* kRelativeDesc    = "(Lnet/minecraft/core/Direction;)"
                                          "Lnet/minecraft/core/BlockPos;";
constexpr const char* kOrdinal         = "ordinal";
constexpr const char* kOrdinalDesc     = "()I";
constexpr const char* kVecX            = "f_82479_";
constexpr const char* kVecY            = "f_82480_";
constexpr const char* kVecZ            = "f_82481_";
constexpr const char* kAabbMinX        = "f_82288_";
constexpr const char* kAabbMinY        = "f_82289_";
constexpr const char* kAabbMinZ        = "f_82290_";
constexpr const char* kAabbMaxX        = "f_82291_";
constexpr const char* kAabbMaxY        = "f_82292_";
constexpr const char* kAabbMaxZ        = "f_82293_";
constexpr const char* kChestBlockField = "f_50087_";
constexpr const char* kTrappedChestField = "f_50325_";
constexpr const char* kEventGetBlock   = "m_131730_";
constexpr const char* kEventGetBlockDesc = "()Lnet/minecraft/world/level/block/Block;";
constexpr const char* kEventGetB0      = "m_131728_";
constexpr const char* kEventGetB1      = "m_131729_";
constexpr const char* kEventGetPos     = "m_131725_";
constexpr const char* kEventGetPosDesc = "()Lnet/minecraft/core/BlockPos;";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedOptionsDot = "net.minecraft.client.Options";
constexpr const char* kMappedMapDot = "java/util/Map";
constexpr const char* kMappedCollectionDot = "java/util/Collection";
constexpr const char* kMappedIteratorDot = "java/util/Iterator";
constexpr const char* kMappedEnumDot = "java/lang/Enum";
constexpr const char* kMappedEnumSig = "Ljava/lang/Enum;";
constexpr const char* kMappedOptionInstanceDot = "net.minecraft.client.OptionInstance";
constexpr const char* kMappedWindowDot = "com.mojang.blaze3d.platform.Window";
constexpr const char* kMappedWindowSig = "Lcom/mojang/blaze3d/platform/Window;";
constexpr const char* kMappedBooleanDot = "java/lang/Boolean";
constexpr const char* kMappedBooleanSig = "Ljava/lang/Boolean;";
constexpr const char* kMappedLocalPlayerDot = "net.minecraft.client.player.LocalPlayer";
constexpr const char* kMappedPrimitiveDoubleSig = "D";
#elif defined(NOVA_FORGE1201_CLICK)
// CLICK
constexpr const char* kMinecraftDot    = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig    = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kLevelDot        = "net.minecraft.world.level.Level";
constexpr const char* kLevelSig        = "Lnet/minecraft/world/level/Level;";
constexpr const char* kLevelFieldSig   = "Lnet/minecraft/client/multiplayer/ClientLevel;";
constexpr const char* kGameModeDot     = "net.minecraft.client.multiplayer.MultiPlayerGameMode";
constexpr const char* kGameModeSig     = "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;";
constexpr const char* kLocalPlayerSig  = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kScreenDot       = "net.minecraft.client.gui.screens.Screen";
constexpr const char* kScreenSig       = "Lnet/minecraft/client/gui/screens/Screen;";
constexpr const char* kContainerDot    = "net.minecraft.client.gui.screens.inventory.AbstractContainerScreen";
constexpr const char* kContainerSig    = "Lnet/minecraft/client/gui/screens/inventory/AbstractContainerScreen;";
constexpr const char* kMenuDot         = "net.minecraft.world.inventory.AbstractContainerMenu";
constexpr const char* kMenuSig         = "Lnet/minecraft/world/inventory/AbstractContainerMenu;";
constexpr const char* kSlotSig         = "Lnet/minecraft/world/inventory/Slot;";
constexpr const char* kSlotListSig     = "Lnet/minecraft/core/NonNullList;";
constexpr const char* kClickTypeDot    = "net.minecraft.world.inventory.ClickType";
constexpr const char* kClickTypeSig    = "Lnet/minecraft/world/inventory/ClickType;";
constexpr const char* kHitResultDot    = "net.minecraft.world.phys.HitResult";
constexpr const char* kHitResultSig    = "Lnet/minecraft/world/phys/HitResult;";
constexpr const char* kHitTypeDot      = "net.minecraft.world.phys.HitResult$Type";
constexpr const char* kHitTypeSig      = "Lnet/minecraft/world/phys/HitResult$Type;";
constexpr const char* kBlockHitDot     = "net.minecraft.world.phys.BlockHitResult";
constexpr const char* kBlockHitSig     = "Lnet/minecraft/world/phys/BlockHitResult;";
constexpr const char* kStateDot        = "net.minecraft.world.level.block.state.BlockState";
constexpr const char* kStateSig        = "Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kLiquidDot       = "net.minecraft.world.level.block.LiquidBlock";
constexpr const char* kLiquidSig       = "Lnet/minecraft/world/level/block/LiquidBlock;";
constexpr const char* kListDot         = "java/util/List";
constexpr const char* kEntityHitDot    = "net.minecraft.world.phys.EntityHitResult";
constexpr const char* kEntityHitSig    = "Lnet/minecraft/world/phys/EntityHitResult;";
constexpr const char* kLivingDot       = "net.minecraft.world.entity.LivingEntity";
constexpr const char* kLivingSig       = "Lnet/minecraft/world/entity/LivingEntity;";
constexpr const char* kHandDot         = "net.minecraft.world.InteractionHand";
constexpr const char* kHandSig         = "Lnet/minecraft/world/InteractionHand;";
constexpr const char* kGetEntity       = "m_82443_";
constexpr const char* kGetEntityDesc   = "()Lnet/minecraft/world/entity/Entity;";
constexpr const char* kAttack          = "m_105223_";
constexpr const char* kAttackDesc =
    "(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/entity/Entity;)V";
constexpr const char* kSwing           = "m_6674_";
constexpr const char* kSwingDesc       = "(Lnet/minecraft/world/InteractionHand;)V";
constexpr const char* kMainHand        = "MAIN_HAND";
constexpr const char* kGetInstance     = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kLevelField      = "f_91073_";
constexpr const char* kGameModeField   = "f_91072_";
constexpr const char* kPlayerField     = "f_91074_";
constexpr const char* kScreenField     = "f_91080_";
constexpr const char* kHitResultField  = "f_91077_";
constexpr const char* kMissTimeField   = "f_91078_";
constexpr const char* kGetType         = "m_6662_";
constexpr const char* kGetTypeDesc     = "()Lnet/minecraft/world/phys/HitResult$Type;";
constexpr const char* kBlockType       = "BLOCK";
constexpr const char* kGetBlockPos     = "m_82425_";
constexpr const char* kGetBlockPosDesc = "()Lnet/minecraft/core/BlockPos;";
constexpr const char* kGetBlockState   = "m_8055_";
constexpr const char* kGetBlockStateDesc =
    "(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kIsAir           = "m_60795_";
constexpr const char* kIsAirDesc       = "()Z";
constexpr const char* kGetBlock        = "m_60734_";
constexpr const char* kGetBlockDesc    = "()Lnet/minecraft/world/level/block/Block;";
constexpr const char* kHoveredSlot     = "f_97734_";
constexpr const char* kMenuField       = "f_97732_";
constexpr const char* kContainerId     = "f_38840_";
constexpr const char* kSlotsField      = "f_38839_";
constexpr const char* kIndexOf         = "indexOf";
constexpr const char* kIndexOfDesc     = "(Ljava/lang/Object;)I";
constexpr const char* kPickup          = "PICKUP";
constexpr const char* kQuickMove       = "QUICK_MOVE";
constexpr const char* kHandleClick     = "m_171799_";
constexpr const char* kHandleClickDesc =
    "(IIILnet/minecraft/world/inventory/ClickType;Lnet/minecraft/world/entity/player/Player;)V";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedPrimitiveIntSig = "I";
#elif defined(NOVA_FORGE1201_COMPASS)
// COMPASS
constexpr const char* kMinecraftDot   = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig   = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kWindowDot      = "com.mojang.blaze3d.platform.Window";
constexpr const char* kWindowSig      = "Lcom/mojang/blaze3d/platform/Window;";
constexpr const char* kLevelDot       = "net.minecraft.world.level.Level";
constexpr const char* kLevelSig       = "Lnet/minecraft/world/level/Level;";
constexpr const char* kClientLevelSig = "Lnet/minecraft/client/multiplayer/ClientLevel;";
constexpr const char* kEntityDot      = "net.minecraft.world.entity.Entity";
constexpr const char* kEntitySig      = "Lnet/minecraft/world/entity/Entity;";
constexpr const char* kLocalPlayerSig = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kDimensionDot   = "net.minecraft.world.level.dimension.DimensionType";
constexpr const char* kDimensionSig   = "Lnet/minecraft/world/level/dimension/DimensionType;";
constexpr const char* kPosSig         = "Lnet/minecraft/core/BlockPos;";
constexpr const char* kGetInstance    = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kGetWindow      = "m_91268_";
constexpr const char* kGetWindowDesc  = "()Lcom/mojang/blaze3d/platform/Window;";
constexpr const char* kLevelField     = "f_91073_";
constexpr const char* kPlayerField    = "f_91074_";
constexpr const char* kWindowWidth    = "m_85441_";
constexpr const char* kWindowHeight   = "m_85442_";
constexpr const char* kWindowGuiScale = "m_85449_";
constexpr const char* kIntDesc        = "()I";
constexpr const char* kDoubleDesc     = "()D";
constexpr const char* kXOld           = "f_19790_";
constexpr const char* kZOld           = "f_19792_";
constexpr const char* kYRotO          = "f_19859_";
constexpr const char* kDoubleField    = "D";
constexpr const char* kFloatField     = "F";
constexpr const char* kGetX           = "m_20185_";
constexpr const char* kGetZ           = "m_20189_";
constexpr const char* kGetYRot        = "m_146908_";
constexpr const char* kFloatDesc      = "()F";
constexpr const char* kGetSpawnPos    = "m_220360_";
constexpr const char* kGetSpawnPosDesc = "()Lnet/minecraft/core/BlockPos;";
constexpr const char* kDimensionType  = "m_6042_";
constexpr const char* kDimensionTypeDesc =
    "()Lnet/minecraft/world/level/dimension/DimensionType;";
constexpr const char* kNatural        = "f_63858_";
constexpr const char* kBoolDesc       = "()Z";
constexpr const char* kVec3iGetX      = "m_123341_";
constexpr const char* kVec3iGetZ      = "m_123343_";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedBlockPosDot = "net.minecraft.core.BlockPos";
#elif defined(NOVA_FORGE1201_ENTITY)
// ENTITY
constexpr const char* kMinecraftDot = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kLevelDot     = "net.minecraft.client.multiplayer.ClientLevel";
constexpr const char* kLevelSig     = "Lnet/minecraft/client/multiplayer/ClientLevel;";
constexpr const char* kEntityDot    = "net.minecraft.world.entity.Entity";
constexpr const char* kEntitySig    = "Lnet/minecraft/world/entity/Entity;";
constexpr const char* kPlayerDot    = "net.minecraft.world.entity.player.Player";
constexpr const char* kPlayerSig    = "Lnet/minecraft/world/entity/player/Player;";
constexpr const char* kAabbDot      = "net.minecraft.world.phys.AABB";
constexpr const char* kAabbSig      = "Lnet/minecraft/world/phys/AABB;";
constexpr const char* kVec3Dot      = "net.minecraft.world.phys.Vec3";
constexpr const char* kVec3Sig      = "Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kHitDot       = "net.minecraft.world.phys.HitResult";
constexpr const char* kHitSig       = "Lnet/minecraft/world/phys/HitResult;";
constexpr const char* kBlockHitDot  = "net.minecraft.world.phys.BlockHitResult";
constexpr const char* kBlockHitSig  = "Lnet/minecraft/world/phys/BlockHitResult;";
constexpr const char* kHitTypeSig   = "Lnet/minecraft/world/phys/HitResult$Type;";
constexpr const char* kEntityHitDot = "net.minecraft.world.phys.EntityHitResult";
constexpr const char* kEntityHitSig = "Lnet/minecraft/world/phys/EntityHitResult;";
constexpr const char* kClipCtxDot   = "net.minecraft.world.level.ClipContext";
constexpr const char* kClipCtxSig   = "Lnet/minecraft/world/level/ClipContext;";
constexpr const char* kClipBlockSig = "Lnet/minecraft/world/level/ClipContext$Block;";
constexpr const char* kClipFluidSig = "Lnet/minecraft/world/level/ClipContext$Fluid;";
constexpr const char* kListDot      = "java/util/List";
constexpr const char* kListSig      = "Ljava/util/List;";
constexpr const char* kOptionalDot  = "java/util/Optional";
constexpr const char* kOptionalSig  = "Ljava/util/Optional;";
constexpr const char* kGetInstance  = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kLevelField   = "f_91073_";
constexpr const char* kPlayerField  = "f_91074_";
constexpr const char* kFrameTime    = "m_91296_";
constexpr const char* kFrameTimeDesc = "()F";
constexpr const char* kGetEntities  = "m_45933_";
constexpr const char* kGetEntitiesDesc =
    "(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/world/phys/AABB;)Ljava/util/List;";
constexpr const char* kPlayers      = "m_6907_";
constexpr const char* kPlayersDesc  = "()Ljava/util/List;";
constexpr const char* kClip         = "m_45547_";
constexpr const char* kClipDesc =
    "(Lnet/minecraft/world/level/ClipContext;)Lnet/minecraft/world/phys/BlockHitResult;";
constexpr const char* kGetX         = "m_20185_";
constexpr const char* kGetY         = "m_20186_";
constexpr const char* kGetZ         = "m_20189_";
constexpr const char* kGetDouble    = "()D";
constexpr const char* kPosition     = "m_20182_";
constexpr const char* kPositionDesc = "()Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kGetEyePos    = "m_20299_";
constexpr const char* kGetEyePosDesc = "(F)Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kGetEyeHeight = "m_20192_";
constexpr const char* kGetEyeHeightDesc = "()F";
constexpr const char* kBoundingBox  = "m_20191_";
constexpr const char* kBoundingBoxDesc = "()Lnet/minecraft/world/phys/AABB;";
constexpr const char* kDistanceTo   = "m_20270_";
constexpr const char* kDistanceToDesc = "(Lnet/minecraft/world/entity/Entity;)F";
constexpr const char* kPickRadius   = "m_6143_";
constexpr const char* kPickRadiusDesc = "()F";
constexpr const char* kIsSpectator  = "m_5833_";
constexpr const char* kIsPickable   = "m_6087_";
constexpr const char* kBoolDesc     = "()Z";
constexpr const char* kRootVehicle  = "m_20201_";
constexpr const char* kRootVehicleDesc = "()Lnet/minecraft/world/entity/Entity;";
constexpr const char* kTickCount    = "f_19797_";
constexpr const char* kOnGround     = "f_19861_";
constexpr const char* kHurtTime     = "f_20916_";
constexpr const char* kDeathTime    = "f_20919_";
constexpr const char* kIntDesc      = "I";
constexpr const char* kFloatDesc    = "()F";
constexpr const char* kIsInvisible  = "m_20145_";
constexpr const char* kIsAlliedTo   = "m_7307_";
constexpr const char* kGetDisplayName = "m_5446_";
constexpr const char* kGetHealth    = "m_21223_";
constexpr const char* kGetAbsorption = "m_6103_";
constexpr const char* kGetMaxHealth = "m_21233_";
constexpr const char* kSetHealth    = "m_21153_";
constexpr const char* kSetHealthDesc = "(F)V";
constexpr const char* kScoreboardName = "m_6302_";
constexpr const char* kStringDesc   = "()Ljava/lang/String;";
constexpr const char* kXOld         = "f_19790_";
constexpr const char* kYOld         = "f_19791_";
constexpr const char* kZOld         = "f_19792_";
constexpr const char* kDoubleField  = "D";
constexpr const char* kLevelGetEntity = "m_6815_";
constexpr const char* kLevelGetEntityDesc =
    "(I)Lnet/minecraft/world/entity/Entity;";
constexpr const char* kDistanceSqr  = "m_20280_";
constexpr const char* kIsAlive      = "m_6084_";
constexpr const char* kMonsterDot   = "net.minecraft.world.entity.monster.Monster";
constexpr const char* kSlimeDot     = "net.minecraft.world.entity.monster.Slime";
constexpr const char* kDragonDot    = "net.minecraft.world.entity.boss.enderdragon.EnderDragon";
constexpr const char* kDragonPartDot = "net.minecraft.world.entity.boss.EnderDragonPart";
constexpr const char* kAnimalDot    = "net.minecraft.world.entity.animal.Animal";
constexpr const char* kAmbientDot   = "net.minecraft.world.entity.ambient.AmbientCreature";
constexpr const char* kWaterDot     = "net.minecraft.world.entity.animal.WaterAnimal";
constexpr const char* kHorseDot     = "net.minecraft.world.entity.animal.horse.AbstractHorse";
constexpr const char* kVillagerDot  = "net.minecraft.world.entity.npc.AbstractVillager";
constexpr const char* kAabbCtor     = "<init>";
constexpr const char* kAabbCtorDesc = "(DDDDDD)V";
constexpr const char* kAabbClip     = "m_82371_";
constexpr const char* kAabbClipDesc =
    "(Lnet/minecraft/world/phys/Vec3;Lnet/minecraft/world/phys/Vec3;)Ljava/util/Optional;";
constexpr const char* kAabbContains = "m_82390_";
constexpr const char* kAabbContainsDesc = "(Lnet/minecraft/world/phys/Vec3;)Z";
constexpr const char* kAabbInflate  = "m_82400_";
constexpr const char* kAabbInflateDesc = "(D)Lnet/minecraft/world/phys/AABB;";
constexpr const char* kAabbExpand   = "m_82363_";
constexpr const char* kAabbExpandDesc = "(DDD)Lnet/minecraft/world/phys/AABB;";
constexpr const char* kAabbMinX     = "f_82288_";
constexpr const char* kAabbMinY     = "f_82289_";
constexpr const char* kAabbMinZ     = "f_82290_";
constexpr const char* kAabbMaxX     = "f_82291_";
constexpr const char* kAabbMaxY     = "f_82292_";
constexpr const char* kAabbMaxZ     = "f_82293_";
constexpr const char* kVecCtor      = "<init>";
constexpr const char* kVecCtorDesc  = "(DDD)V";
constexpr const char* kVecDirFromRot = "m_82498_";
constexpr const char* kVecDirFromRotDesc = "(FF)Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kVecX         = "f_82479_";
constexpr const char* kVecY         = "f_82480_";
constexpr const char* kVecZ         = "f_82481_";
constexpr const char* kHitGetType   = "m_6662_";
constexpr const char* kHitGetTypeDesc = "()Lnet/minecraft/world/phys/HitResult$Type;";
constexpr const char* kHitGetLocation = "m_82450_";
constexpr const char* kHitGetLocationDesc = "()Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kHitGetBlockPos = "m_82425_";
constexpr const char* kHitGetBlockPosDesc = "()Lnet/minecraft/core/BlockPos;";
constexpr const char* kHitGetDirection = "m_82434_";
constexpr const char* kHitGetDirectionDesc = "()Lnet/minecraft/core/Direction;";
constexpr const char* kBlockPosDot  = "net.minecraft.core.BlockPos";
constexpr const char* kBlockPosSig  = "Lnet/minecraft/core/BlockPos;";
constexpr const char* kVec3iGetX    = "m_123341_";
constexpr const char* kVec3iGetY    = "m_123342_";
constexpr const char* kVec3iGetZ    = "m_123343_";
constexpr const char* kClipCtxCtor  = "<init>";
constexpr const char* kClipCtxCtorDesc =
    "(Lnet/minecraft/world/phys/Vec3;Lnet/minecraft/world/phys/Vec3;"
    "Lnet/minecraft/world/level/ClipContext$Block;Lnet/minecraft/world/level/ClipContext$Fluid;"
    "Lnet/minecraft/world/entity/Entity;)V";
constexpr const char* kEntityHitCtor = "<init>";
constexpr const char* kEntityHitCtorDesc =
    "(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/world/phys/Vec3;)V";
constexpr const char* kListSize     = "size";
constexpr const char* kListSizeDesc = "()I";
constexpr const char* kListGet      = "get";
constexpr const char* kListGetDesc  = "(I)Ljava/lang/Object;";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedHitResultTypeDot = "net.minecraft.world.phys.HitResult$Type";
constexpr const char* kMappedClipContextBlockDot = "net.minecraft.world.level.ClipContext$Block";
constexpr const char* kMappedClipContextFluidDot = "net.minecraft.world.level.ClipContext$Fluid";
constexpr const char* kMappedLivingEntityDot = "net.minecraft.world.entity.LivingEntity";
constexpr const char* kMappedLivingEntitySig = "Lnet/minecraft/world/entity/LivingEntity;";
constexpr const char* kMappedComponentDot = "net.minecraft.network.chat.Component";
constexpr const char* kMappedComponentSig = "Lnet/minecraft/network/chat/Component;";
constexpr const char* kMappedEnumDot = "java/lang/Enum";
constexpr const char* kMappedEnumSig = "Ljava/lang/Enum;";
constexpr const char* kMappedLocalPlayerSig = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kMappedEntityToBooleanDesc = "(Lnet/minecraft/world/entity/Entity;)Z";
constexpr const char* kMappedNoArgsToComponentDesc = "()Lnet/minecraft/network/chat/Component;";
constexpr const char* kMappedGetString = "getString";
constexpr const char* kMappedEntityToDoubleDesc = "(Lnet/minecraft/world/entity/Entity;)D";
constexpr const char* kMappedPrimitiveBooleanSig = "Z";
constexpr const char* kMappedOrdinal = "ordinal";
constexpr const char* kMappedOUTLINE = "OUTLINE";
constexpr const char* kMappedNONE = "NONE";
constexpr const char* kMappedIsPresent = "isPresent";
constexpr const char* kMappedNoArgsToObjectDesc = "()Ljava/lang/Object;";
constexpr const char* kMappedMonsterSig = "Lnet/minecraft/world/entity/monster/Monster;";
constexpr const char* kMappedSlimeSig = "Lnet/minecraft/world/entity/monster/Slime;";
constexpr const char* kMappedEnderDragonSig = "Lnet/minecraft/world/entity/boss/enderdragon/EnderDragon;";
constexpr const char* kMappedEnderDragonPartSig = "Lnet/minecraft/world/entity/boss/EnderDragonPart;";
constexpr const char* kMappedAnimalSig = "Lnet/minecraft/world/entity/animal/Animal;";
constexpr const char* kMappedAmbientCreatureSig = "Lnet/minecraft/world/entity/ambient/AmbientCreature;";
constexpr const char* kMappedWaterAnimalSig = "Lnet/minecraft/world/entity/animal/WaterAnimal;";
constexpr const char* kMappedAbstractHorseSig = "Lnet/minecraft/world/entity/animal/horse/AbstractHorse;";
constexpr const char* kMappedAbstractVillagerSig = "Lnet/minecraft/world/entity/npc/AbstractVillager;";
constexpr const char* kMappedEntityId = "m_19879_";
constexpr const char* kMappedFallDistance = "f_19789_";
constexpr const char* kMappedPrimitiveFloatSig = "F";
#elif defined(NOVA_FORGE1201_INPUT)
// INPUT
constexpr const char* kMinecraftDot = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kGetInstance = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kMouseHandlerDot = "net.minecraft.client.MouseHandler";
constexpr const char* kMouseHandler = "f_91067_";
constexpr const char* kMouseHandlerSig = "Lnet/minecraft/client/MouseHandler;";
constexpr const char* kGrabMouse = "m_91601_";
constexpr const char* kReleaseMouse = "m_91602_";
constexpr const char* kVoidDesc = "()V";
constexpr const char* kKeyMappingDot  = "net.minecraft.client.KeyMapping";
constexpr const char* kKeyMappingSig  = "Lnet/minecraft/client/KeyMapping;";
constexpr const char* kInputTypeDot   = "com.mojang.blaze3d.platform.InputConstants$Type";
constexpr const char* kInputTypeSig   = "Lcom/mojang/blaze3d/platform/InputConstants$Type;";
constexpr const char* kInputKeySig    = "Lcom/mojang/blaze3d/platform/InputConstants$Key;";
constexpr const char* kMouseGrabbed   = "f_91520_";
constexpr const char* kMouseType      = "MOUSE";
constexpr const char* kGetOrCreate    = "m_84895_";
constexpr const char* kGetOrCreateDesc = "(I)Lcom/mojang/blaze3d/platform/InputConstants$Key;";
constexpr const char* kClickKey       = "m_90835_";
constexpr const char* kClickKeyDesc   = "(Lcom/mojang/blaze3d/platform/InputConstants$Key;)V";
constexpr const char* kSetKey         = "m_90837_";
constexpr const char* kSetKeyDesc     = "(Lcom/mojang/blaze3d/platform/InputConstants$Key;Z)V";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedPrimitiveBooleanSig = "Z";
#elif defined(NOVA_FORGE1201_INVENTORY)
// INVENTORY
constexpr const char* kMinecraftDot    = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig    = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kScreenDot       = "net.minecraft.client.gui.screens.Screen";
constexpr const char* kScreenSig       = "Lnet/minecraft/client/gui/screens/Screen;";
constexpr const char* kContainerDot    = "net.minecraft.client.gui.screens.inventory.ContainerScreen";
constexpr const char* kContainerSig    = "Lnet/minecraft/client/gui/screens/inventory/ContainerScreen;";
constexpr const char* kInvScreenDot    = "net.minecraft.client.gui.screens.inventory.InventoryScreen";
constexpr const char* kInvScreenSig    = "Lnet/minecraft/client/gui/screens/inventory/InventoryScreen;";
constexpr const char* kAbsScreenDot    =
    "net.minecraft.client.gui.screens.inventory.AbstractContainerScreen";
constexpr const char* kAbsScreenSig    =
    "Lnet/minecraft/client/gui/screens/inventory/AbstractContainerScreen;";
constexpr const char* kMenuDot         = "net.minecraft.world.inventory.AbstractContainerMenu";
constexpr const char* kMenuSig         = "Lnet/minecraft/world/inventory/AbstractContainerMenu;";
constexpr const char* kChestMenuDot    = "net.minecraft.world.inventory.ChestMenu";
constexpr const char* kChestMenuSig    = "Lnet/minecraft/world/inventory/ChestMenu;";
constexpr const char* kSlotDot         = "net.minecraft.world.inventory.Slot";
constexpr const char* kSlotSig         = "Lnet/minecraft/world/inventory/Slot;";
constexpr const char* kClickTypeDot    = "net.minecraft.world.inventory.ClickType";
constexpr const char* kClickTypeSig    = "Lnet/minecraft/world/inventory/ClickType;";
constexpr const char* kGameModeDot     = "net.minecraft.client.multiplayer.MultiPlayerGameMode";
constexpr const char* kGameModeSig     = "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;";
constexpr const char* kPlayerDot       = "net.minecraft.world.entity.player.Player";
constexpr const char* kPlayerSig       = "Lnet/minecraft/world/entity/player/Player;";
constexpr const char* kInventoryDot    = "net.minecraft.world.entity.player.Inventory";
constexpr const char* kInventorySig    = "Lnet/minecraft/world/entity/player/Inventory;";
constexpr const char* kListDot         = "net.minecraft.core.NonNullList";
constexpr const char* kListSig         = "Lnet/minecraft/core/NonNullList;";
constexpr const char* kStackDot        = "net.minecraft.world.item.ItemStack";
constexpr const char* kStackSig        = "Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kItemDot         = "net.minecraft.world.item.Item";
constexpr const char* kItemSig         = "Lnet/minecraft/world/item/Item;";
constexpr const char* kBlockDot        = "net.minecraft.world.level.block.Block";
constexpr const char* kBlockSig        = "Lnet/minecraft/world/level/block/Block;";
constexpr const char* kStateDot        = "net.minecraft.world.level.block.state.BlockState";
constexpr const char* kStateSig        = "Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kItemsDot        = "net.minecraft.world.item.Items";
constexpr const char* kItemsSig        = "Lnet/minecraft/world/item/Items;";
constexpr const char* kBlocksDot       = "net.minecraft.world.level.block.Blocks";
constexpr const char* kBlocksSig       = "Lnet/minecraft/world/level/block/Blocks;";
constexpr const char* kEnchantsDot     = "net.minecraft.world.item.enchantment.Enchantments";
constexpr const char* kEnchantsSig     = "Lnet/minecraft/world/item/enchantment/Enchantments;";
constexpr const char* kEnchDot         = "net.minecraft.world.item.enchantment.Enchantment";
constexpr const char* kEnchSig         = "Lnet/minecraft/world/item/enchantment/Enchantment;";
constexpr const char* kHelperDot       = "net.minecraft.world.item.enchantment.EnchantmentHelper";
constexpr const char* kHelperSig       = "Lnet/minecraft/world/item/enchantment/EnchantmentHelper;";
constexpr const char* kMobTypeDot      = "net.minecraft.world.entity.MobType";
constexpr const char* kMobTypeSig      = "Lnet/minecraft/world/entity/MobType;";
constexpr const char* kMaterialsDot    = "net.minecraft.world.item.ArmorMaterials";
constexpr const char* kMaterialsSig    = "Lnet/minecraft/world/item/ArmorMaterials;";
constexpr const char* kEquipSlotDot    = "net.minecraft.world.entity.EquipmentSlot";
constexpr const char* kEquipSlotSig    = "Lnet/minecraft/world/entity/EquipmentSlot;";
constexpr const char* kComponentDot    = "net.minecraft.network.chat.Component";
constexpr const char* kComponentSig    = "Lnet/minecraft/network/chat/Component;";
constexpr const char* kClosePacketDot  =
    "net.minecraft.network.protocol.game.ServerboundContainerClosePacket";
constexpr const char* kClosePacketSig  =
    "Lnet/minecraft/network/protocol/game/ServerboundContainerClosePacket;";
constexpr const char* kGetInstance      = "m_91087_";
constexpr const char* kGetInstanceDesc  = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kScreenField      = "f_91080_";
constexpr const char* kPlayerField      = "f_91074_";
constexpr const char* kPlayerFieldSig   = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kGameModeField    = "f_91072_";
constexpr const char* kAbsGetMenu       = "m_6262_";
constexpr const char* kAbsGetMenuDesc   = "()Lnet/minecraft/world/inventory/AbstractContainerMenu;";
constexpr const char* kScreenGetTitle   = "m_96636_";
constexpr const char* kScreenGetTitleDesc = "()Lnet/minecraft/network/chat/Component;";
constexpr const char* kComponentGetString = "getString";
constexpr const char* kComponentGetStringDesc = "()Ljava/lang/String;";
constexpr const char* kComponentTranslatable = "m_237115_";
constexpr const char* kComponentTranslatableDesc =
    "(Ljava/lang/String;)Lnet/minecraft/network/chat/MutableComponent;";
constexpr const char* kMenuContainerId  = "f_38840_";
constexpr const char* kMenuGetSlot      = "m_38853_";
constexpr const char* kMenuGetSlotDesc  = "(I)Lnet/minecraft/world/inventory/Slot;";
constexpr const char* kSlotGetItem      = "m_7993_";
constexpr const char* kSlotGetItemDesc  = "()Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kChestRowCount    = "m_39265_";
constexpr const char* kChestRowCountDesc = "()I";
constexpr const char* kListGet          = "get";
constexpr const char* kListGetDesc      = "(I)Ljava/lang/Object;";
constexpr const char* kListSize         = "size";
constexpr const char* kListSizeDesc     = "()I";
constexpr const char* kInvItems         = "f_35974_";
constexpr const char* kInvArmor         = "f_35975_";
constexpr const char* kInvOffhand       = "f_35976_";
constexpr const char* kPlayerMenuField  = "f_36095_";
constexpr const char* kPlayerMenuSig    = "Lnet/minecraft/world/inventory/InventoryMenu;";
constexpr const char* kPlayerClose      = "m_6915_";
constexpr const char* kPlayerCloseDesc  = "()V";
constexpr const char* kPlayerGetInventory = "m_150109_";
constexpr const char* kPlayerGetInventoryDesc =
    "()Lnet/minecraft/world/entity/player/Inventory;";
constexpr const char* kHandleClick      = "m_171799_";
constexpr const char* kHandleClickDesc =
    "(IIILnet/minecraft/world/inventory/ClickType;"
    "Lnet/minecraft/world/entity/player/Player;)V";
constexpr const char* kStackIsEmpty     = "m_41619_";
constexpr const char* kStackIsEmptyDesc = "()Z";
constexpr const char* kStackGetItem     = "m_41720_";
constexpr const char* kStackGetItemDesc = "()Lnet/minecraft/world/item/Item;";
constexpr const char* kStackGetCount    = "m_41613_";
constexpr const char* kStackGetCountDesc = "()I";
constexpr const char* kStackGetName     = "m_41611_";
constexpr const char* kStackGetNameDesc = "()Lnet/minecraft/network/chat/Component;";
constexpr const char* kStackDamageValue = "m_41773_";
constexpr const char* kStackMaxDamage   = "m_41776_";
constexpr const char* kStackMaxStack    = "m_41741_";
constexpr const char* kStackDestroySpeed = "m_41691_";
constexpr const char* kStackDestroySpeedDesc =
    "(Lnet/minecraft/world/level/block/state/BlockState;)F";
constexpr const char* kIntGetterDesc    = "()I";
constexpr const char* kFloatGetterDesc  = "()F";
constexpr const char* kBlockItemGetBlock = "m_40614_";
constexpr const char* kBlockItemGetBlockDesc = "()Lnet/minecraft/world/level/block/Block;";
constexpr const char* kBlockDefaultState = "m_49966_";
constexpr const char* kBlockDefaultStateDesc =
    "()Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kSwordGetDamage   = "m_43299_";
constexpr const char* kArmorGetMaterial = "m_40401_";
constexpr const char* kArmorGetMaterialDesc = "()Lnet/minecraft/world/item/ArmorMaterial;";
constexpr const char* kArmorGetSlot     = "m_40402_";
constexpr const char* kArmorGetSlotDesc =
    "()Lnet/minecraft/world/entity/EquipmentSlot;";
constexpr const char* kHelperGetLevel   = "m_44843_";
constexpr const char* kHelperGetLevelDesc =
    "(Lnet/minecraft/world/item/enchantment/Enchantment;"
    "Lnet/minecraft/world/item/ItemStack;)I";
constexpr const char* kEnchantDamageBonus = "m_7335_";
constexpr const char* kEnchantDamageBonusDesc = "(ILnet/minecraft/world/entity/MobType;)F";
constexpr const char* kMobTypeUndefined = "f_21640_";
constexpr const char* kCloseCtorDesc    = "(I)V";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedConstructor = "<init>";
constexpr const char* kMappedPrimitiveIntSig = "I";
constexpr const char* kMappedPICKUP = "PICKUP";
constexpr const char* kMappedQUICKMOVE = "QUICK_MOVE";
constexpr const char* kMappedSWAP = "SWAP";
constexpr const char* kMappedTHROW = "THROW";
constexpr const char* kMappedStone = "f_50069_";
constexpr const char* kMappedOakLog = "f_49999_";
constexpr const char* kMappedDirt = "f_50493_";
constexpr const char* kMappedSkullBlockDot = "net.minecraft.world.level.block.SkullBlock";
constexpr const char* kMappedSkullBlockSig = "Lnet/minecraft/world/level/block/SkullBlock;";
constexpr const char* kMappedServerboundMovePlayerPacketDot = "net.minecraft.network.protocol.game.ServerboundMovePlayerPacket";
constexpr const char* kMappedServerboundMovePlayerPacketSig = "Lnet/minecraft/network/protocol/game/ServerboundMovePlayerPacket;";
constexpr const char* kMappedServerboundUseItemPacketDot = "net.minecraft.network.protocol.game.ServerboundUseItemPacket";
constexpr const char* kMappedServerboundUseItemPacketSig = "Lnet/minecraft/network/protocol/game/ServerboundUseItemPacket;";
constexpr const char* kMappedServerboundUseItemOnPacketDot = "net.minecraft.network.protocol.game.ServerboundUseItemOnPacket";
constexpr const char* kMappedServerboundUseItemOnPacketSig = "Lnet/minecraft/network/protocol/game/ServerboundUseItemOnPacket;";
constexpr const char* kMappedServerboundInteractPacketDot = "net.minecraft.network.protocol.game.ServerboundInteractPacket";
constexpr const char* kMappedServerboundInteractPacketSig = "Lnet/minecraft/network/protocol/game/ServerboundInteractPacket;";
constexpr const char* kMappedServerboundPlayerActionPacketDot = "net.minecraft.network.protocol.game.ServerboundPlayerActionPacket";
constexpr const char* kMappedServerboundPlayerActionPacketSig = "Lnet/minecraft/network/protocol/game/ServerboundPlayerActionPacket;";
#elif defined(NOVA_FORGE1201_INVENTORY_TABLES)
// INVENTORY_TABLES
struct ItemClassSpec {
    const char* dot;
    const char* sig;
    unsigned    bit;
};
constexpr ItemClassSpec kItemClasses[] = {
    {"net.minecraft.world.item.BlockItem", "Lnet/minecraft/world/item/BlockItem;", kItemBlock},
    {"net.minecraft.world.item.SwordItem", "Lnet/minecraft/world/item/SwordItem;", kItemSword},
    {"net.minecraft.world.item.AxeItem", "Lnet/minecraft/world/item/AxeItem;", kItemAxe},
    {"net.minecraft.world.item.PickaxeItem", "Lnet/minecraft/world/item/PickaxeItem;", kItemPickaxe},
    {"net.minecraft.world.item.ShovelItem", "Lnet/minecraft/world/item/ShovelItem;", kItemShovel},
    {"net.minecraft.world.item.BowItem", "Lnet/minecraft/world/item/BowItem;", kItemBow},
    {"net.minecraft.world.item.CrossbowItem", "Lnet/minecraft/world/item/CrossbowItem;",
     kItemCrossbow},
    {"net.minecraft.world.item.ArrowItem", "Lnet/minecraft/world/item/ArrowItem;", kItemArrow},
    {"net.minecraft.world.item.FishingRodItem", "Lnet/minecraft/world/item/FishingRodItem;",
     kItemFishingRod},
    {"net.minecraft.world.item.ArmorItem", "Lnet/minecraft/world/item/ArmorItem;", kItemArmor},
    {"net.minecraft.world.item.ElytraItem", "Lnet/minecraft/world/item/ElytraItem;", kItemElytra},
    {"net.minecraft.world.item.PlayerHeadItem", "Lnet/minecraft/world/item/PlayerHeadItem;",
     kItemHead},
    {"net.minecraft.world.item.ItemNameBlockItem", "Lnet/minecraft/world/item/ItemNameBlockItem;",
     kItemNameBlock},
    {"net.minecraft.world.item.BookItem", "Lnet/minecraft/world/item/BookItem;", kItemBook},
    {"net.minecraft.world.item.ExperienceBottleItem",
     "Lnet/minecraft/world/item/ExperienceBottleItem;", kItemBottle},
    {"net.minecraft.world.item.FireworkRocketItem", "Lnet/minecraft/world/item/FireworkRocketItem;",
     kItemFirework},
};
constexpr int kItemClassCount =
    static_cast<int>(sizeof(kItemClasses) / sizeof(kItemClasses[0]));

// The block classes the building-block rule rejects. These are blocks, not items,
// so they are tested against the block a BlockItem places.
struct BlockClassSpec {
    const char* dot;
    const char* sig;
    unsigned    bit;
};
constexpr BlockClassSpec kBlockClasses[] = {
    {"net.minecraft.world.level.block.FlowerBlock", "Lnet/minecraft/world/level/block/FlowerBlock;",
     kBlockFlower},
    {"net.minecraft.world.level.block.BushBlock", "Lnet/minecraft/world/level/block/BushBlock;",
     kBlockBush},
    {"net.minecraft.world.level.block.FungusBlock", "Lnet/minecraft/world/level/block/FungusBlock;",
     kBlockFungus},
    {"net.minecraft.world.level.block.CropBlock", "Lnet/minecraft/world/level/block/CropBlock;",
     kBlockCrop},
    {"net.minecraft.world.level.block.SlabBlock", "Lnet/minecraft/world/level/block/SlabBlock;",
     kBlockSlab},
};
constexpr int kBlockClassCount =
    static_cast<int>(sizeof(kBlockClasses) / sizeof(kBlockClasses[0]));

// The identity lists. Starain compares an item against Items.* for most things and
// a block against Blocks.* for the two placeable special cases, so they are two
// separate lists rather than one.
struct IdentitySpec {
    const char* field;
    KnownItem   value;
};
constexpr IdentitySpec kItemIdentities[] = {
    {"f_42433_", KnownItem::GoldenAxe},
    {"f_42518_", KnownItem::SlimeBall},
    {"f_42398_", KnownItem::Stick},
    {"f_42437_", KnownItem::EnchantedGoldenApple},
    {"f_42436_", KnownItem::GoldenApple},
    {"f_42729_", KnownItem::EndCrystal},
    {"f_42747_", KnownItem::TotemOfUndying},
    {"f_42522_", KnownItem::Compass},
    {"f_42447_", KnownItem::WaterBucket},
    {"f_42448_", KnownItem::LavaBucket},
    {"f_42412_", KnownItem::Arrow},
    {"f_42452_", KnownItem::Snowball},
    {"f_42521_", KnownItem::Egg},
    {"f_42523_", KnownItem::FishingRod},
    {"f_42423_", KnownItem::WoodenAxe},
    {"f_42428_", KnownItem::StoneAxe},
    {"f_42386_", KnownItem::IronAxe},
    {"f_42391_", KnownItem::DiamondAxe},
    {"f_42404_", KnownItem::WheatSeeds},
    {"f_42733_", KnownItem::BeetrootSeeds},
    {"f_42578_", KnownItem::MelonSeeds},
    {"f_42577_", KnownItem::PumpkinSeeds},
    {"f_42409_", KnownItem::FlintAndSteel},
    {"f_42411_", KnownItem::Bow},
    {"f_42584_", KnownItem::EnderPearl},
    {"f_42613_", KnownItem::FireCharge},
};
constexpr int kItemIdentityCount =
    static_cast<int>(sizeof(kItemIdentities) / sizeof(kItemIdentities[0]));

constexpr IdentitySpec kBlockIdentities[] = {
    {"f_50201_", KnownItem::EnchantingTableBlock},
    {"f_50033_", KnownItem::CobwebBlock},
    {"f_50069_", KnownItem::StoneBlock},
    {"f_49999_", KnownItem::OakLogBlock},
    {"f_50493_", KnownItem::DirtBlock},
};
constexpr int kBlockIdentityCount =
    static_cast<int>(sizeof(kBlockIdentities) / sizeof(kBlockIdentities[0]));

struct EnchantSpec {
    const char* field;
    int ItemFacts::* slot;
};
constexpr EnchantSpec kEnchantments[] = {
    {"f_44977_", &ItemFacts::sharpness},
    {"f_44980_", &ItemFacts::knockback},
    {"f_44989_", &ItemFacts::punch},
    {"f_44988_", &ItemFacts::power},
    {"f_44952_", &ItemFacts::infinity},
    {"f_44990_", &ItemFacts::flame},
    {"f_44984_", &ItemFacts::efficiency},
    {"f_44965_", &ItemFacts::protection},
    {"f_44960_", &ItemFacts::quickCharge},
    {"f_44959_", &ItemFacts::multishot},
    {"f_44961_", &ItemFacts::piercing},
};
constexpr int kEnchantmentCount =
    static_cast<int>(sizeof(kEnchantments) / sizeof(kEnchantments[0]));

struct MaterialSpec {
    const char*   field;
    KnownMaterial value;
};
constexpr MaterialSpec kMaterials[] = {
    {"LEATHER", KnownMaterial::Leather},
    {"CHAIN", KnownMaterial::Chain},
    {"IRON", KnownMaterial::Iron},
    {"GOLD", KnownMaterial::Gold},
    {"DIAMOND", KnownMaterial::Diamond},
    {"NETHERITE", KnownMaterial::Netherite},
};
constexpr int kMaterialCount = static_cast<int>(sizeof(kMaterials) / sizeof(kMaterials[0]));

constexpr const char* kArmorSlotFields[4] = {"HEAD", "CHEST", "LEGS", "FEET"};

// The block blacklist the building-block rules share with the scaffold. Compared
// by identity, so each entry is held once.
constexpr const char* kBlockBlacklist[] = {
    "f_50016_", "f_49990_", "f_49991_", "f_50201_", "f_50185_", "f_50183_", "f_50125_",
    "f_49997_", "f_50089_", "f_50264_", "f_50087_", "f_50325_", "f_50081_", "f_50322_",
    "f_50065_", "f_50131_", "f_50077_", "f_49995_", "f_49996_", "f_50059_", "f_50165_",
    "f_50326_", "f_50327_", "f_50124_", "f_50164_", "f_50359_", "f_50267_", "f_50266_",
    "f_50156_", "f_50121_", "f_50073_", "f_50072_", "f_50191_", "f_50355_", "f_50155_",
    "f_50094_", "f_49992_", "f_50128_", "f_50061_", "f_50286_", "f_50091_", "f_50033_",
    "f_50133_", "f_50274_", "f_50132_", "f_50174_", "f_50276_",
};
constexpr int kBlockBlacklistCount =
    static_cast<int>(sizeof(kBlockBlacklist) / sizeof(kBlockBlacklist[0]));
#elif defined(NOVA_FORGE1201_NETWORK)
// NETWORK
constexpr const char* kMinecraftDot    = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig    = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kListenerDot     = "net.minecraft.client.multiplayer.ClientPacketListener";
constexpr const char* kListenerSig     =
    "Lnet/minecraft/client/multiplayer/ClientPacketListener;";
constexpr const char* kUseItemDot      =
    "net.minecraft.network.protocol.game.ServerboundUseItemPacket";
constexpr const char* kUseItemSig      =
    "Lnet/minecraft/network/protocol/game/ServerboundUseItemPacket;";
constexpr const char* kUseItemOnDot    =
    "net.minecraft.network.protocol.game.ServerboundUseItemOnPacket";
constexpr const char* kUseItemOnSig    =
    "Lnet/minecraft/network/protocol/game/ServerboundUseItemOnPacket;";
constexpr const char* kStatusOnlyDot   =
    "net.minecraft.network.protocol.game.ServerboundMovePlayerPacket$StatusOnly";
constexpr const char* kStatusOnlySig   =
    "Lnet/minecraft/network/protocol/game/ServerboundMovePlayerPacket$StatusOnly;";
constexpr const char* kGetInstance     = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kGetConnection   = "m_91403_";
constexpr const char* kGetConnectionDesc =
    "()Lnet/minecraft/client/multiplayer/ClientPacketListener;";
constexpr const char* kSend            = "m_104955_";
constexpr const char* kSendDesc        = "(Lnet/minecraft/network/protocol/Packet;)V";
constexpr const char* kStatusCtor      = "<init>";
constexpr const char* kStatusCtorDesc  = "(Z)V";
constexpr const char* kConnectionDot   = "net.minecraft.network.Connection";
constexpr const char* kConnectionSig   = "Lnet/minecraft/network/Connection;";
constexpr const char* kPacketDot       = "net.minecraft.network.protocol.Packet";
constexpr const char* kPacketSig       = "Lnet/minecraft/network/protocol/Packet;";
constexpr const char* kGamePacketDir   = "net.minecraft.network.protocol.game.";
constexpr const char* kGamePacketSigPrefix =
    "Lnet/minecraft/network/protocol/game/";
constexpr const char* kGenericsFtw     = "m_129517_";
constexpr const char* kGenericsFtwDesc =
    "(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketListener;)V";
constexpr const char* kPacketHandle    = "m_5797_";
constexpr const char* kPacketHandleDesc = "(Lnet/minecraft/network/PacketListener;)V";
constexpr const char* kMotionName      = "ClientboundSetEntityMotionPacket";
constexpr const char* kMoveEntityName  = "ClientboundMoveEntityPacket";
constexpr const char* kTeleportName    = "ClientboundTeleportEntityPacket";
constexpr const char* kPingName        = "ClientboundPingPacket";
constexpr const char* kLookAtName      = "ClientboundPlayerLookAtPacket";
constexpr const char* kPositionName    = "ClientboundPlayerPositionPacket";
constexpr const char* kExplodeName     = "ClientboundExplodePacket";
constexpr const char* kDisconnectName  = "ClientboundDisconnectPacket";
constexpr const char* kRespawnName     = "ClientboundRespawnPacket";
constexpr const char* kMotionId        = "m_133192_";
constexpr const char* kMotionXa        = "m_133195_";
constexpr const char* kMotionYa        = "m_133196_";
constexpr const char* kMotionZa        = "m_133197_";
constexpr const char* kIntGetterDesc   = "()I";
#elif defined(NOVA_FORGE1201_OPTIONS)
// OPTIONS
constexpr const char* kMinecraftDot    = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig    = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kOptionsDot      = "net.minecraft.client.Options";
constexpr const char* kOptionsSig      = "Lnet/minecraft/client/Options;";
constexpr const char* kKeyMappingDot   = "net.minecraft.client.KeyMapping";
constexpr const char* kKeyMappingSig   = "Lnet/minecraft/client/KeyMapping;";
constexpr const char* kOptionInstanceDot = "net.minecraft.client.OptionInstance";
constexpr const char* kOptionInstanceSig = "Lnet/minecraft/client/OptionInstance;";
constexpr const char* kGetInstance     = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kOptionsField    = "f_91066_";
constexpr const char* kSprintField     = "f_92091_";
constexpr const char* kSetDown         = "m_7249_";
constexpr const char* kSetDownDesc     = "(Z)V";
constexpr const char* kGamma           = "m_231927_";
constexpr const char* kGammaDesc       = "()Lnet/minecraft/client/OptionInstance;";
constexpr const char* kValueField      = "f_231481_";
constexpr const char* kValueDesc       = "Ljava/lang/Object;";
constexpr const char* kDoubleDot       = "java/lang/Double";
constexpr const char* kValueOf         = "valueOf";
constexpr const char* kValueOfDesc     = "(D)Ljava/lang/Double;";
constexpr const char* kDoubleValue     = "doubleValue";
constexpr const char* kDoubleValueDesc = "()D";
#elif defined(NOVA_FORGE1201_PLACE)
// PLACE
constexpr const char* kMinecraftDot     = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig     = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kOptionsDot       = "net.minecraft.client.Options";
constexpr const char* kOptionsSig       = "Lnet/minecraft/client/Options;";
constexpr const char* kKeyMappingDot    = "net.minecraft.client.KeyMapping";
constexpr const char* kKeyMappingSig    = "Lnet/minecraft/client/KeyMapping;";
constexpr const char* kBlockItemDot     = "net.minecraft.world.item.BlockItem";
constexpr const char* kBlockItemSig     = "Lnet/minecraft/world/item/BlockItem;";
constexpr const char* kLivingDot        = "net.minecraft.world.entity.LivingEntity";
constexpr const char* kLivingSig        = "Lnet/minecraft/world/entity/LivingEntity;";
constexpr const char* kEntityDot        = "net.minecraft.world.entity.Entity";
constexpr const char* kEntitySig        = "Lnet/minecraft/world/entity/Entity;";
constexpr const char* kStackDot         = "net.minecraft.world.item.ItemStack";
constexpr const char* kStackSig         = "Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kGetInstance      = "m_91087_";
constexpr const char* kGetInstanceDesc  = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kOptionsField     = "f_91066_";
constexpr const char* kUseKeyField      = "f_92095_";
constexpr const char* kIsDown           = "m_90857_";
constexpr const char* kIsDownDesc       = "()Z";
constexpr const char* kDelayField       = "f_91011_";
constexpr const char* kDelayDesc        = "I";
constexpr const char* kPlayerField      = "f_91074_";
constexpr const char* kPlayerSig        = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kMainHand         = "m_21205_";
constexpr const char* kMainHandDesc     = "()Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kGetItem          = "m_41720_";
constexpr const char* kGetItemDesc      = "()Lnet/minecraft/world/item/Item;";
constexpr const char* kIsEmpty          = "m_41619_";
constexpr const char* kIsEmptyDesc      = "()Z";
constexpr const char* kGetXRot          = "m_146909_";
constexpr const char* kGetXRotDesc      = "()F";
constexpr const char* kLevelDot        = "net.minecraft.world.level.Level";
constexpr const char* kLevelSig        = "Lnet/minecraft/world/level/Level;";
constexpr const char* kClientLevelSig  = "Lnet/minecraft/client/multiplayer/ClientLevel;";
constexpr const char* kGameModeDot     = "net.minecraft.client.multiplayer.MultiPlayerGameMode";
constexpr const char* kGameModeSig     = "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;";
constexpr const char* kInventoryDot    = "net.minecraft.world.entity.player.Inventory";
constexpr const char* kInventorySig    = "Lnet/minecraft/world/entity/player/Inventory;";
constexpr const char* kPlayerDot       = "net.minecraft.world.entity.player.Player";
constexpr const char* kPlayerEntitySig = "Lnet/minecraft/world/entity/player/Player;";
constexpr const char* kBlockStateDot   = "net.minecraft.world.level.block.state.BlockState";
constexpr const char* kBlockStateSig   = "Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kShapeDot        = "net.minecraft.world.phys.shapes.VoxelShape";
constexpr const char* kShapeSig        = "Lnet/minecraft/world/phys/shapes/VoxelShape;";
constexpr const char* kBlocksDot       = "net.minecraft.world.level.block.Blocks";
constexpr const char* kBlocksSig       = "Lnet/minecraft/world/level/block/Blocks;";
constexpr const char* kPosDot          = "net.minecraft.core.BlockPos";
constexpr const char* kPosSig          = "Lnet/minecraft/core/BlockPos;";
constexpr const char* kDirectionDot    = "net.minecraft.core.Direction";
constexpr const char* kDirectionSig    = "Lnet/minecraft/core/Direction;";
constexpr const char* kVecDot          = "net.minecraft.world.phys.Vec3";
constexpr const char* kVecSig          = "Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kBlockHitDot     = "net.minecraft.world.phys.BlockHitResult";
constexpr const char* kBlockHitSig     = "Lnet/minecraft/world/phys/BlockHitResult;";
constexpr const char* kHandDot         = "net.minecraft.world.InteractionHand";
constexpr const char* kHandSig         = "Lnet/minecraft/world/InteractionHand;";
constexpr const char* kResultDot       = "net.minecraft.world.InteractionResult";
constexpr const char* kResultSig       = "Lnet/minecraft/world/InteractionResult;";
constexpr const char* kRotPacketDot    =
    "net.minecraft.network.protocol.game.ServerboundMovePlayerPacket$Rot";
constexpr const char* kRotPacketSig    =
    "Lnet/minecraft/network/protocol/game/ServerboundMovePlayerPacket$Rot;";
constexpr const char* kSwingPacketDot  =
    "net.minecraft.network.protocol.game.ServerboundSwingPacket";
constexpr const char* kSwingPacketSig  =
    "Lnet/minecraft/network/protocol/game/ServerboundSwingPacket;";
constexpr const char* kLocalPlayerDot   = "net.minecraft.client.player.LocalPlayer";
constexpr const char* kLocalPlayerSig   = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kSwingHand        = "m_6674_";
constexpr const char* kSwingHandDesc    = "(Lnet/minecraft/world/InteractionHand;)V";
constexpr const char* kStandWallDot    = "net.minecraft.world.item.StandingAndWallBlockItem";
constexpr const char* kStandWallSig    = "Lnet/minecraft/world/item/StandingAndWallBlockItem;";
constexpr const char* kGetInventory     = "m_150109_";
constexpr const char* kInventoryDesc    = "()Lnet/minecraft/world/entity/player/Inventory;";
constexpr const char* kInvSelected      = "f_35977_";
constexpr const char* kInvSelectedDesc  = "I";
constexpr const char* kInvGetItem       = "m_8020_";
constexpr const char* kInvGetItemDesc   = "(I)Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kStackCount       = "m_41613_";
constexpr const char* kStackName        = "m_41611_";
constexpr const char* kStackNameDesc    = "()Lnet/minecraft/network/chat/Component;";
constexpr const char* kComponentString  = "getString";
constexpr const char* kComponentStringDesc = "()Ljava/lang/String;";
constexpr const char* kBlockItemBlock   = "m_40614_";
constexpr const char* kBlockItemBlockDesc = "()Lnet/minecraft/world/level/block/Block;";
constexpr const char* kLevelGetState    = "m_8055_";
constexpr const char* kLevelGetStateDesc =
    "(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kStateCollision   = "m_60812_";
constexpr const char* kStateCollisionDesc =
    "(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/core/BlockPos;)"
    "Lnet/minecraft/world/phys/shapes/VoxelShape;";
constexpr const char* kStateMenu        = "m_60750_";
constexpr const char* kStateMenuDesc =
    "(Lnet/minecraft/world/level/Level;Lnet/minecraft/core/BlockPos;)"
    "Lnet/minecraft/world/MenuProvider;";
constexpr const char* kStateReplaceable = "m_247087_";
constexpr const char* kShapeEmpty       = "m_83281_";
constexpr const char* kShapeEmptyDesc   = "()Z";
constexpr const char* kUseItemOn        = "m_233732_";
constexpr const char* kUseItemOnDesc =
    "(Lnet/minecraft/client/player/LocalPlayer;Lnet/minecraft/world/InteractionHand;"
    "Lnet/minecraft/world/phys/BlockHitResult;)Lnet/minecraft/world/InteractionResult;";
constexpr const char* kConsumesAction   = "m_19077_";
constexpr const char* kConsumesActionDesc = "()Z";
constexpr const char* kKeyJumpField     = "f_92089_";
constexpr const char* kKeyShiftField    = "f_92090_";
constexpr const char* kKeySetDown       = "m_7249_";
constexpr const char* kKeySetDownDesc   = "(Z)V";
constexpr const char* kDeltaMovement    = "m_20184_";
constexpr const char* kDeltaMovementDesc = "()Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kSetDeltaMovement = "m_20334_";
constexpr const char* kSetDeltaMovementDesc = "(DDD)V";
constexpr const char* kSetSprinting     = "m_6858_";
constexpr const char* kSetSprintingDesc = "(Z)V";
constexpr const char* kGetSpeed         = "m_6113_";
constexpr const char* kGetSpeedDesc     = "()F";
constexpr const char* kIsSprinting      = "m_20142_";
constexpr const char* kIsSprintingDesc  = "()Z";
constexpr const char* kGetStepX         = "m_122429_";
constexpr const char* kGetStepY         = "m_122430_";
constexpr const char* kGetStepZ         = "m_122431_";
constexpr const char* kGetStepDesc      = "()I";
constexpr const char* kGetOpposite      = "m_122424_";
constexpr const char* kGetOppositeDesc  = "()Lnet/minecraft/core/Direction;";
constexpr const char* kVecCtor          = "<init>";
constexpr const char* kVecCtorDesc      = "(DDD)V";
constexpr const char* kPosCtor          = "<init>";
constexpr const char* kPosCtorDesc      = "(III)V";
constexpr const char* kBlockHitCtor     = "<init>";
constexpr const char* kBlockHitCtorDesc =
    "(Lnet/minecraft/world/phys/Vec3;Lnet/minecraft/core/Direction;"
    "Lnet/minecraft/core/BlockPos;Z)V";
constexpr const char* kRotCtorDesc      = "(FFZ)V";
constexpr const char* kSwingCtorDesc    = "(Lnet/minecraft/world/InteractionHand;)V";
constexpr const char* kLevelField       = "f_91073_";
constexpr const char* kGameModeField    = "f_91072_";
constexpr const char* kVecX             = "f_82479_";
constexpr const char* kVecY             = "f_82480_";
constexpr const char* kVecZ             = "f_82481_";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedFlowerBlockDot = "net.minecraft.world.level.block.FlowerBlock";
constexpr const char* kMappedFlowerBlockSig = "Lnet/minecraft/world/level/block/FlowerBlock;";
constexpr const char* kMappedBushBlockDot = "net.minecraft.world.level.block.BushBlock";
constexpr const char* kMappedBushBlockSig = "Lnet/minecraft/world/level/block/BushBlock;";
constexpr const char* kMappedFungusBlockDot = "net.minecraft.world.level.block.FungusBlock";
constexpr const char* kMappedFungusBlockSig = "Lnet/minecraft/world/level/block/FungusBlock;";
constexpr const char* kMappedCropBlockDot = "net.minecraft.world.level.block.CropBlock";
constexpr const char* kMappedCropBlockSig = "Lnet/minecraft/world/level/block/CropBlock;";
constexpr const char* kMappedSlabBlockDot = "net.minecraft.world.level.block.SlabBlock";
constexpr const char* kMappedSlabBlockSig = "Lnet/minecraft/world/level/block/SlabBlock;";
constexpr const char* kMappedPrimitiveDoubleSig = "D";
constexpr const char* kMappedMAINHAND = "MAIN_HAND";
constexpr const char* kMappedOrdinal = "ordinal";
constexpr const char* kMappedBlockSig = "Lnet/minecraft/world/level/block/Block;";
#elif defined(NOVA_FORGE1201_PLACE_TABLES)
// PLACE_TABLES
// net.minecraft.core.Direction's declaration order, which is the order
// rotation::BlockFace mirrors and the ordinal the block hit result wants.
constexpr const char* kFacingNames[6] = {"DOWN", "UP", "NORTH", "SOUTH", "WEST", "EAST"};

// Starain's blacklist, by the static field each entry names on Blocks. They are
// compared by identity, exactly as List.of(Blocks.A, Blocks.B, ...) does, so the
// field has to be read once and then held. Each comment is the name the field
// carries in the game's own mappings.
constexpr const char* kBlacklistFields[] = {
    "f_50016_", // AIR
    "f_49990_", // WATER
    "f_49991_", // LAVA
    "f_50201_", // ENCHANTING_TABLE
    "f_50185_", // GLASS_PANE
    "f_50183_", // IRON_BARS
    "f_50125_", // SNOW
    "f_49997_", // COAL_ORE
    "f_50089_", // DIAMOND_ORE
    "f_50264_", // EMERALD_ORE
    "f_50087_", // CHEST
    "f_50325_", // TRAPPED_CHEST
    "f_50081_", // TORCH
    "f_50322_", // ANVIL
    "f_50065_", // NOTE_BLOCK
    "f_50131_", // JUKEBOX
    "f_50077_", // TNT
    "f_49995_", // GOLD_ORE
    "f_49996_", // IRON_ORE
    "f_50059_", // LAPIS_ORE
    "f_50165_", // STONE_PRESSURE_PLATE
    "f_50326_", // LIGHT_WEIGHTED_PRESSURE_PLATE
    "f_50327_", // HEAVY_WEIGHTED_PRESSURE_PLATE
    "f_50124_", // STONE_BUTTON
    "f_50164_", // LEVER
    "f_50359_", // TALL_GRASS
    "f_50267_", // TRIPWIRE
    "f_50266_", // TRIPWIRE_HOOK
    "f_50156_", // RAIL
    "f_50121_", // CORNFLOWER
    "f_50073_", // RED_MUSHROOM
    "f_50072_", // BROWN_MUSHROOM
    "f_50191_", // VINE
    "f_50355_", // SUNFLOWER
    "f_50155_", // LADDER
    "f_50094_", // FURNACE
    "f_49992_", // SAND
    "f_50128_", // CACTUS
    "f_50061_", // DISPENSER
    "f_50286_", // DROPPER
    "f_50091_", // CRAFTING_TABLE
    "f_50033_", // COBWEB
    "f_50133_", // PUMPKIN
    "f_50274_", // COBBLESTONE_WALL
    "f_50132_", // OAK_FENCE
    "f_50174_", // REDSTONE_TORCH
    "f_50276_", // FLOWER_POT
};

constexpr int kBlacklistCount =
    static_cast<int>(sizeof(kBlacklistFields) / sizeof(kBlacklistFields[0]));
#elif defined(NOVA_FORGE1201_PLAYER)
// PLAYER
constexpr const char* kAttackStrength = "m_36403_";
constexpr const char* kAttackStrengthDesc = "(F)F";
constexpr const char* kMinecraftDot    = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig    = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kLivingDot       = "net.minecraft.world.entity.LivingEntity";
constexpr const char* kLivingSig       = "Lnet/minecraft/world/entity/LivingEntity;";
constexpr const char* kEffectsDot      = "net.minecraft.world.effect.MobEffects";
constexpr const char* kEffectsSig      = "Lnet/minecraft/world/effect/MobEffects;";
constexpr const char* kEffectDot       = "net.minecraft.world.effect.MobEffect";
constexpr const char* kEffectSig       = "Lnet/minecraft/world/effect/MobEffect;";
constexpr const char* kInstanceDot     = "net.minecraft.world.effect.MobEffectInstance";
constexpr const char* kInstanceSig     = "Lnet/minecraft/world/effect/MobEffectInstance;";
constexpr const char* kPlayerSig       = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kGetInstance     = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kPlayerField     = "f_91074_";
constexpr const char* kNightVision     = "f_19611_";
constexpr const char* kAddEffect       = "m_7292_";
constexpr const char* kAddEffectDesc   = "(Lnet/minecraft/world/effect/MobEffectInstance;)Z";
constexpr const char* kRemoveEffect    = "m_21195_";
constexpr const char* kRemoveEffectDesc = "(Lnet/minecraft/world/effect/MobEffect;)Z";
constexpr const char* kOnGround        = "m_20096_";
constexpr const char* kOnGroundDesc    = "()Z";
constexpr const char* kInstanceCtor    = "<init>";
constexpr const char* kInstanceCtorDesc = "(Lnet/minecraft/world/effect/MobEffect;IIZZZ)V";
constexpr const char* kPlayerDot       = "net.minecraft.world.entity.player.Player";
constexpr const char* kPlayerTypeSig   = "Lnet/minecraft/world/entity/player/Player;";
constexpr const char* kAbilitiesDot    = "net.minecraft.world.entity.player.Abilities";
constexpr const char* kAbilitiesSig    = "Lnet/minecraft/world/entity/player/Abilities;";
constexpr const char* kItemStackDot    = "net.minecraft.world.item.ItemStack";
constexpr const char* kItemStackSig    = "Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kItemSig         = "Lnet/minecraft/world/item/Item;";
constexpr const char* kSwordDot        = "net.minecraft.world.item.SwordItem";
constexpr const char* kSwordSig        = "Lnet/minecraft/world/item/SwordItem;";
constexpr const char* kAxeDot          = "net.minecraft.world.item.AxeItem";
constexpr const char* kAxeSig          = "Lnet/minecraft/world/item/AxeItem;";
constexpr const char* kTridentDot      = "net.minecraft.world.item.TridentItem";
constexpr const char* kTridentSig      = "Lnet/minecraft/world/item/TridentItem;";
constexpr const char* kIsUsingItem     = "m_6117_";
constexpr const char* kGetAbilities    = "m_150110_";
constexpr const char* kGetAbilitiesDesc = "()Lnet/minecraft/world/entity/player/Abilities;";
constexpr const char* kGetMainHandItem = "m_21205_";
constexpr const char* kGetMainHandDesc = "()Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kGetItem         = "m_41720_";
constexpr const char* kGetItemDesc     = "()Lnet/minecraft/world/item/Item;";
constexpr const char* kInstabuild      = "f_35937_";
constexpr const char* kMayBuild        = "f_35938_";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedPrimitiveBooleanSig = "Z";
#elif defined(NOVA_FORGE1201_ROTATION)
// ROTATION
constexpr const char* kMinecraftDot   = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig   = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kEntityDot      = "net.minecraft.world.entity.Entity";
constexpr const char* kEntitySig      = "Lnet/minecraft/world/entity/Entity;";
constexpr const char* kOptionsDot     = "net.minecraft.client.Options";
constexpr const char* kOptionsSig     = "Lnet/minecraft/client/Options;";
constexpr const char* kOptionDot      = "net.minecraft.client.OptionInstance";
constexpr const char* kOptionSig      = "Lnet/minecraft/client/OptionInstance;";
constexpr const char* kVec3Sig        = "Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kGetInstance    = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kPlayerField    = "f_91074_";
constexpr const char* kPlayerSig      = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kOptionsField   = "f_91066_";
constexpr const char* kGetFps         = "m_260875_";
constexpr const char* kGetFpsDesc     = "()I";
constexpr const char* kGameRenderer   = "f_91063_";
constexpr const char* kGameRendererSig = "Lnet/minecraft/client/renderer/GameRenderer;";
constexpr const char* kPick           = "m_109087_";
constexpr const char* kPickDesc       = "(F)V";
constexpr const char* kGetYRot        = "m_146908_";
constexpr const char* kGetYRotDesc    = "()F";
constexpr const char* kGetXRot        = "m_146909_";
constexpr const char* kGetXRotDesc    = "()F";
constexpr const char* kSetYRot        = "m_146922_";
constexpr const char* kSetYRotDesc    = "(F)V";
constexpr const char* kSetXRot        = "m_146926_";
constexpr const char* kSetXRotDesc    = "(F)V";
constexpr const char* kGetEye         = "m_146892_";
constexpr const char* kGetEyeDesc     = "()Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kYRotO          = "f_19859_";
constexpr const char* kXRotO          = "f_19860_";
constexpr const char* kFloatDesc      = "F";
constexpr const char* kSensitivity    = "m_231964_";
constexpr const char* kSensitivityDesc = "()Lnet/minecraft/client/OptionInstance;";
constexpr const char* kValueField     = "f_231481_";
constexpr const char* kValueDesc      = "Ljava/lang/Object;";
constexpr const char* kDoubleDot      = "java/lang/Double";
constexpr const char* kDoubleValue    = "doubleValue";
constexpr const char* kDoubleValueDesc = "()D";
constexpr const char* kRenderDistance = "m_231984_";
constexpr const char* kFov            = "m_231837_";
constexpr const char* kIntDot         = "java/lang/Integer";
constexpr const char* kIntValue       = "intValue";
constexpr const char* kIntValueDesc   = "()I";
constexpr const char* kVecX           = "f_82479_";
constexpr const char* kVecY           = "f_82480_";
constexpr const char* kVecZ           = "f_82481_";
constexpr const char* kPosRotDot      = "net.minecraft.network.protocol.game.ServerboundMovePlayerPacket$PosRot";
constexpr const char* kPosRotSig      = "Lnet/minecraft/network/protocol/game/ServerboundMovePlayerPacket$PosRot;";
constexpr const char* kPosRotCtor     = "<init>";
constexpr const char* kPosRotCtorDesc = "(DDDFFZ)V";
constexpr const char* kPosRotGetX     = "m_134129_";
constexpr const char* kPosRotGetY     = "m_134140_";
constexpr const char* kPosRotGetZ     = "m_134146_";
constexpr const char* kPosRotGetYaw   = "m_134131_";
constexpr const char* kPosRotGetPitch = "m_134142_";
constexpr const char* kPosRotGetD     = "(D)D";
constexpr const char* kPosRotGround   = "m_134139_";
constexpr const char* kPosRotGetF     = "(F)F";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedVec3Dot = "net.minecraft.world.phys.Vec3";
constexpr const char* kMappedDoubleSig = "Ljava/lang/Double;";
constexpr const char* kMappedGameRendererDot = "net.minecraft.client.renderer.GameRenderer";
constexpr const char* kMappedIntegerSig = "Ljava/lang/Integer;";
constexpr const char* kMappedPrimitiveDoubleSig = "D";
constexpr const char* kMappedNoArgsToBooleanDesc = "()Z";
#elif defined(NOVA_FORGE1201_SCORE)
// SCORE
constexpr const char* kPacketDot =
    "net.minecraft.network.protocol.game.ClientboundSetScorePacket";
constexpr const char* kPacketSig =
    "Lnet/minecraft/network/protocol/game/ClientboundSetScorePacket;";
constexpr const char* kMethodDot = "net.minecraft.server.ServerScoreboard$Method";
constexpr const char* kMethodSig = "Lnet/minecraft/server/ServerScoreboard$Method;";
constexpr const char* kEnumDot = "java/lang/Enum";
constexpr const char* kEnumSig = "Ljava/lang/Enum;";
constexpr const char* kGetOwner     = "m_133339_";
constexpr const char* kGetObjective = "m_133342_";
constexpr const char* kGetScore     = "m_133343_";
constexpr const char* kGetMethod    = "m_133344_";
constexpr const char* kMethodDesc = "()Lnet/minecraft/server/ServerScoreboard$Method;";
constexpr const char* kStringDesc = "()Ljava/lang/String;";
constexpr const char* kIntDesc    = "()I";
constexpr const char* kOrdinalDesc = "()I";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedOrdinal = "ordinal";
#elif defined(NOVA_FORGE1201_SKIN)
// SKIN
constexpr const char* kMinecraftDot     = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig     = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kPlayerDot        = "net.minecraft.client.player.AbstractClientPlayer";
constexpr const char* kPlayerSig        = "Lnet/minecraft/client/player/AbstractClientPlayer;";
constexpr const char* kManagerDot       = "net.minecraft.client.renderer.texture.TextureManager";
constexpr const char* kManagerSig       = "Lnet/minecraft/client/renderer/texture/TextureManager;";
constexpr const char* kTextureDot       = "net.minecraft.client.renderer.texture.AbstractTexture";
constexpr const char* kTextureSig       = "Lnet/minecraft/client/renderer/texture/AbstractTexture;";
constexpr const char* kLocationDot      = "net.minecraft.resources.ResourceLocation";
constexpr const char* kLocationSig      = "Lnet/minecraft/resources/ResourceLocation;";
constexpr const char* kGetInstance      = "m_91087_";
constexpr const char* kGetInstanceDesc  = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kGetSkin          = "m_108560_";
constexpr const char* kGetSkinDesc      = "()Lnet/minecraft/resources/ResourceLocation;";
constexpr const char* kGetManager       = "m_91097_";
constexpr const char* kGetManagerDesc =
    "()Lnet/minecraft/client/renderer/texture/TextureManager;";
constexpr const char* kGetTexture       = "m_118506_";
constexpr const char* kGetTextureDesc =
    "(Lnet/minecraft/resources/ResourceLocation;)"
    "Lnet/minecraft/client/renderer/texture/AbstractTexture;";
constexpr const char* kGetId            = "m_117963_";
constexpr const char* kGetIdDesc        = "()I";
#elif defined(NOVA_FORGE1201_TOOL)
// TOOL
constexpr const char* kMinecraftDot    = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig    = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kLevelDot        = "net.minecraft.world.level.Level";
constexpr const char* kLevelSig        = "Lnet/minecraft/world/level/Level;";
constexpr const char* kClientLevelSig  = "Lnet/minecraft/client/multiplayer/ClientLevel;";
constexpr const char* kGameModeDot     = "net.minecraft.client.multiplayer.MultiPlayerGameMode";
constexpr const char* kGameModeSig     = "Lnet/minecraft/client/multiplayer/MultiPlayerGameMode;";
constexpr const char* kPlayerDot       = "net.minecraft.world.entity.player.Player";
constexpr const char* kPlayerEntitySig = "Lnet/minecraft/world/entity/player/Player;";
constexpr const char* kInventoryDot    = "net.minecraft.world.entity.player.Inventory";
constexpr const char* kInventorySig    = "Lnet/minecraft/world/entity/player/Inventory;";
constexpr const char* kStackDot        = "net.minecraft.world.item.ItemStack";
constexpr const char* kStackSig        = "Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kBlockStateDot   = "net.minecraft.world.level.block.state.BlockState";
constexpr const char* kBlockStateSig   = "Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kPosDot          = "net.minecraft.core.BlockPos";
constexpr const char* kPosSig          = "Lnet/minecraft/core/BlockPos;";
constexpr const char* kHitDot          = "net.minecraft.world.phys.HitResult";
constexpr const char* kHitSig          = "Lnet/minecraft/world/phys/HitResult;";
constexpr const char* kBlockHitDot     = "net.minecraft.world.phys.BlockHitResult";
constexpr const char* kBlockHitSig     = "Lnet/minecraft/world/phys/BlockHitResult;";
constexpr const char* kWebDot          = "net.minecraft.world.level.block.WebBlock";
constexpr const char* kWebSig          = "Lnet/minecraft/world/level/block/WebBlock;";
constexpr const char* kDropXpDot       = "net.minecraft.world.level.block.DropExperienceBlock";
constexpr const char* kDropXpSig       = "Lnet/minecraft/world/level/block/DropExperienceBlock;";
constexpr const char* kOreDot          = "net.minecraft.world.level.block.RedStoneOreBlock";
constexpr const char* kOreSig          = "Lnet/minecraft/world/level/block/RedStoneOreBlock;";
constexpr const char* kGetInstance     = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kLevelField      = "f_91073_";
constexpr const char* kGameModeField   = "f_91072_";
constexpr const char* kPlayerField     = "f_91074_";
constexpr const char* kLocalPlayerSig  = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kHitResultField  = "f_91077_";
constexpr const char* kIsDestroying    = "m_105296_";
constexpr const char* kBoolDesc        = "()Z";
constexpr const char* kLevelGetState   = "m_8055_";
constexpr const char* kLevelGetStateDesc =
    "(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;";
constexpr const char* kStateIsAir      = "m_60795_";
constexpr const char* kStateGetBlock   = "m_60734_";
constexpr const char* kStateGetBlockDesc = "()Lnet/minecraft/world/level/block/Block;";
constexpr const char* kStackDestroySpeed = "m_41691_";
constexpr const char* kStackDestroySpeedDesc =
    "(Lnet/minecraft/world/level/block/state/BlockState;)F";
constexpr const char* kGetInventory    = "m_150109_";
constexpr const char* kGetInventoryDesc = "()Lnet/minecraft/world/entity/player/Inventory;";
constexpr const char* kInvGetItem      = "m_8020_";
constexpr const char* kInvGetItemDesc  = "(I)Lnet/minecraft/world/item/ItemStack;";
constexpr const char* kInvSelected     = "f_35977_";
constexpr const char* kInvSelectedDesc = "I";
constexpr const char* kHitGetType      = "m_6662_";
constexpr const char* kHitGetTypeDesc  = "()Lnet/minecraft/world/phys/HitResult$Type;";
constexpr const char* kHitGetBlockPos  = "m_82425_";
constexpr const char* kHitGetBlockPosDesc = "()Lnet/minecraft/core/BlockPos;";
constexpr const char* kOrdinal         = "ordinal";
constexpr const char* kOrdinalDesc     = "()I";
constexpr const char* kVec3iGetX       = "m_123341_";
constexpr const char* kVec3iGetY       = "m_123342_";
constexpr const char* kVec3iGetZ       = "m_123343_";
constexpr const char* kIntDesc         = "()I";
constexpr const char* kPosCtor         = "<init>";
constexpr const char* kPosCtorDesc     = "(III)V";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedEnumDot = "java/lang/Enum";
constexpr const char* kMappedEnumSig = "Ljava/lang/Enum;";
#elif defined(NOVA_FORGE1201_HOOKS)
// HOOKS
constexpr const char* kEntityDot     = "net.minecraft.world.entity.Entity";
constexpr const char* kEntitySig     = "Lnet/minecraft/world/entity/Entity;";
constexpr const char* kLivingDot     = "net.minecraft.world.entity.LivingEntity";
constexpr const char* kLivingSig     = "Lnet/minecraft/world/entity/LivingEntity;";
constexpr const char* kVec3Dot       = "net.minecraft.world.phys.Vec3";
constexpr const char* kVec3Sig       = "Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kConnectionDot = "net.minecraft.network.Connection";
constexpr const char* kConnectionSig = "Lnet/minecraft/network/Connection;";
constexpr const char* kInputDot     = "net.minecraft.client.player.Input";
constexpr const char* kInputSig     = "Lnet/minecraft/client/player/Input;";
constexpr const char* kListenerDot  = "net.minecraft.client.multiplayer.ClientPacketListener";
constexpr const char* kListenerSig  = "Lnet/minecraft/client/multiplayer/ClientPacketListener;";
constexpr const char* kMinecraftDot = "net.minecraft.client.Minecraft";
constexpr const char* kMinecraftSig = "Lnet/minecraft/client/Minecraft;";
constexpr const char* kLocalDot     = "net.minecraft.client.player.LocalPlayer";
constexpr const char* kLocalSig     = "Lnet/minecraft/client/player/LocalPlayer;";
constexpr const char* kGetInstance  = "m_91087_";
constexpr const char* kGetInstanceDesc = "()Lnet/minecraft/client/Minecraft;";
constexpr const char* kPlayerField  = "f_91074_";
constexpr const char* kPlayerInput  = "f_108618_";
constexpr const char* kIsMovingSlowly = "m_108635_";
constexpr const char* kGetYRot       = "m_146908_";
constexpr const char* kGetXRot       = "m_146909_";
constexpr const char* kViewVector    = "m_20171_";
constexpr const char* kIsUsingItem   = "m_6117_";
constexpr const char* kSendPacket    = "m_129520_";
constexpr const char* kSendPacketDesc =
    "(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketSendListener;)V";
constexpr const char* kListenerSend  = "m_104955_";
constexpr const char* kListenerSendDesc = "(Lnet/minecraft/network/protocol/Packet;)V";
constexpr const char* kGetX          = "m_20185_";
constexpr const char* kGetY          = "m_20186_";
constexpr const char* kGetZ          = "m_20189_";

// Additional JNI lookup names and descriptors.
constexpr const char* kMappedNoArgsToFloatDesc = "()F";
constexpr const char* kMappedFloatFloatToVec3Desc = "(FF)Lnet/minecraft/world/phys/Vec3;";
constexpr const char* kMappedNoArgsToDoubleDesc = "()D";
constexpr const char* kMappedPreviousYaw = "f_19859_";
constexpr const char* kMappedPrimitiveFloatSig = "F";
constexpr const char* kMappedPreviousPitch = "f_19860_";
constexpr const char* kMappedTickCount = "f_19797_";
constexpr const char* kMappedPrimitiveIntSig = "I";
constexpr const char* kMappedOnGround = "f_19861_";
constexpr const char* kMappedPrimitiveBooleanSig = "Z";
constexpr const char* kMappedNoArgsToBooleanDesc = "()Z";
constexpr const char* kMappedTick = "m_8119_";
constexpr const char* kMappedNoArgsToVoidDesc = "()V";
constexpr const char* kMappedInputForward = "f_108567_";
constexpr const char* kMappedInputStrafe = "f_108566_";
constexpr const char* kMappedInputJumping = "f_108572_";
constexpr const char* kMappedInputSneak = "f_108573_";
constexpr const char* kMappedInputUp = "f_108568_";
constexpr const char* kMappedInputDown = "f_108569_";
constexpr const char* kMappedInputLeft = "f_108570_";
constexpr const char* kMappedInputRight = "f_108571_";
constexpr const char* kMappedVec3Zero = "f_82478_";
#elif defined(NOVA_FORGE1201_EDITS)
// EDITS
const Redirect kEntityRedirects[] = {
    {"m_19920_", "(FLnet/minecraft/world/phys/Vec3;)V",
     "net/minecraft/world/entity/Entity", "m_146908_", "()F",
     "strafeYaw", "(Lnet/minecraft/world/entity/Entity;)F", -1},

    {"m_20252_", "(F)Lnet/minecraft/world/phys/Vec3;",
     "net/minecraft/world/entity/Entity", "m_20171_",
     "(FF)Lnet/minecraft/world/phys/Vec3;",
     "viewVector",
     "(Lnet/minecraft/world/entity/Entity;FF)Lnet/minecraft/world/phys/Vec3;", -1},
};

const Insertion kEntityInsertions[] = {
    {"m_7601_",
     "(Lnet/minecraft/world/level/block/state/BlockState;"
     "Lnet/minecraft/world/phys/Vec3;)V",
     "net/minecraft/world/entity/Entity", "m_183634_", "()V",
false, 0,
     "stuckInBlock",
     "(Lnet/minecraft/world/entity/Entity;"
     "Lnet/minecraft/world/level/block/state/BlockState;"
     "Lnet/minecraft/world/phys/Vec3;)Lnet/minecraft/world/phys/Vec3;",
3, 2},
};

const Redirect kLivingEntityRedirects[] = {
    {"m_6135_", "()V",
     "net/minecraft/world/entity/LivingEntity", "m_146908_", "()F",
     "jumpYaw", "(Lnet/minecraft/world/entity/Entity;)F", -1},

    {"m_5632_", "(FF)F",
     "net/minecraft/world/entity/LivingEntity", "m_146908_", "()F",
     "bodyYaw", "(Lnet/minecraft/world/entity/Entity;)F", -1},

    {"m_7023_", "(Lnet/minecraft/world/phys/Vec3;)V",
     "net/minecraft/world/entity/LivingEntity", "m_146909_", "()F",
     "fallFlyingPitch", "(Lnet/minecraft/world/entity/Entity;)F", -1},
};

const Redirect kLocalPlayerRedirects[] = {
    {"m_108640_","()V","net/minecraft/client/player/LocalPlayer","m_108636_","()Z",
     "freeCamControlled","(Lnet/minecraft/client/player/LocalPlayer;)Z",-1},
    {"m_8107_", "()V",
     "net/minecraft/client/player/LocalPlayer", "m_6117_", "()Z",
     "usingItem", "(Lnet/minecraft/world/entity/Entity;)Z", -1},

    {"m_108640_", "()V",
     "net/minecraft/client/player/LocalPlayer", "m_146908_", "()F",
     "motionYaw", "(Lnet/minecraft/world/entity/Entity;)F", -1},

    {"m_108640_", "()V",
     "net/minecraft/client/player/LocalPlayer", "m_146909_", "()F",
     "motionPitch", "(Lnet/minecraft/world/entity/Entity;)F", -1},

    {"m_108640_", "()V",
     "net/minecraft/client/player/LocalPlayer", "m_20096_", "()Z",
     "motionOnGround", "(Lnet/minecraft/world/entity/Entity;)Z", -1},
};

const StackLoad kBeginMotionLoads[] = {
    {0x19, 0},
};

const StackLoad kEndMotionLoads[] = {
    {0x19, 0},
};

const Append kLocalPlayerAppends[] = {
    {"m_5704_", "(Lnet/minecraft/world/entity/Entity;)V", "criticalHit",
     "(Lnet/minecraft/client/player/LocalPlayer;)V", kBeginMotionLoads, 1, true},
    {"m_108640_", "()V", "beginMotion",
     "(Lnet/minecraft/client/player/LocalPlayer;)V", kBeginMotionLoads, 1, true},

    {"m_108640_", "()V", "endMotion",
     "(Lnet/minecraft/client/player/LocalPlayer;)V", kEndMotionLoads, 1, false},
};

const Redirect kPlayerRedirects[] = {
    {"m_5706_", "(Lnet/minecraft/world/entity/Entity;)V", "net/minecraft/world/entity/player/Player", "m_20256_", "(Lnet/minecraft/world/phys/Vec3;)V", "attackVelocity", "(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/phys/Vec3;)V", -1},
    {"m_5706_", "(Lnet/minecraft/world/entity/Entity;)V", "net/minecraft/world/entity/player/Player", "m_6858_", "(Z)V", "attackSprint", "(Lnet/minecraft/world/entity/player/Player;Z)V", -1},
    {"m_8061_", "(Lnet/minecraft/world/entity/EquipmentSlot;Lnet/minecraft/world/item/ItemStack;)V",
     "net/minecraft/world/entity/player/Inventory", "f_35977_", "I", "spoofSlot", "(Lnet/minecraft/world/entity/player/Inventory;)I", -1, -1, 0xB4},
    {"m_5706_", "(Lnet/minecraft/world/entity/Entity;)V",
     "net/minecraft/world/entity/player/Player", "m_146908_", "()F",
     "attackYaw", "(Lnet/minecraft/world/entity/Entity;)F", -1},
};

const Redirect kItemRedirects[] = {
    {"m_41435_",
     "(Lnet/minecraft/world/level/Level;Lnet/minecraft/world/entity/player/Player;"
     "Lnet/minecraft/world/level/ClipContext$Fluid;)Lnet/minecraft/world/phys/BlockHitResult;",
     "net/minecraft/world/entity/player/Player", "m_146908_", "()F",
     "useItemRayTraceYaw", "(Lnet/minecraft/world/entity/player/Player;)F", -1},

    {"m_41435_",
     "(Lnet/minecraft/world/level/Level;Lnet/minecraft/world/entity/player/Player;"
     "Lnet/minecraft/world/level/ClipContext$Fluid;)Lnet/minecraft/world/phys/BlockHitResult;",
     "net/minecraft/world/entity/player/Player", "m_146909_", "()F",
     "useItemRayTracePitch", "(Lnet/minecraft/world/entity/player/Player;)F", -1},
};

const Redirect kGameModeRedirects[] = {
    {"m_105297_", "()V", "net/minecraft/world/entity/player/Inventory", "f_35977_", "I",
     "spoofSlot", "(Lnet/minecraft/world/entity/player/Inventory;)I", -1, -1, 0xB4},
    {"m_233721_",
     "(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/InteractionHand;)"
     "Lnet/minecraft/world/InteractionResult;",
     "net/minecraft/client/multiplayer/ClientPacketListener", "m_104955_",
     "(Lnet/minecraft/network/protocol/Packet;)V",
     "positionItem",
     "(Lnet/minecraft/client/multiplayer/ClientPacketListener;"
     "Lnet/minecraft/network/protocol/Packet;)V", 1},
};

const StackLoad kKeyboardInputHeadLoads[] = {
    {0x17, 2},
};

const Append kKeyboardInputAppends[] = {
    {"m_214106_", "(ZF)V", "moveInputCapture", "(F)V", kKeyboardInputHeadLoads, 1, true},
    {"m_214106_", "(ZF)V", "moveInput", "()V", nullptr, 0, false},
};

const Append kMinecraftAppends[] = {
    {"m_91398_", "()V", "gameTick", "()V", nullptr, 0, true},
    {"m_91398_", "()V", "postGameTick", "()V", nullptr, 0, false, true},
    {"m_91279_", "()V", "preInteract", "()V", nullptr, 0, true},
};
const Redirect kSlotRedirects[] = {
    {"m_36056_", "()Lnet/minecraft/world/item/ItemStack;", "net/minecraft/world/entity/player/Inventory", "f_35977_", "I",
     "spoofSlot", "(Lnet/minecraft/world/entity/player/Inventory;)I", -1, -1, 0xB4}
};
const Redirect kVisualHandRedirects[] = {
    {"m_172914_", "(Lnet/minecraft/client/player/LocalPlayer;)Lnet/minecraft/client/renderer/ItemInHandRenderer$HandRenderSelection;",
     "net/minecraft/client/player/LocalPlayer", "m_21205_", "()Lnet/minecraft/world/item/ItemStack;",
     "visualPlayerHand", "(Lnet/minecraft/client/player/LocalPlayer;)Lnet/minecraft/world/item/ItemStack;", -1},
    {"m_109311_", "()V", "net/minecraft/client/player/LocalPlayer", "m_21205_", "()Lnet/minecraft/world/item/ItemStack;",
     "visualPlayerHand", "(Lnet/minecraft/client/player/LocalPlayer;)Lnet/minecraft/world/item/ItemStack;", -1}
};
const Redirect kBreakerClientRedirects[] = {
    {"m_91279_","()V","net/minecraft/client/Minecraft","m_91278_","()Z",
     "freeCamAttack","(Lnet/minecraft/client/Minecraft;)Z",-1},
    { "m_91279_", "()V", "net/minecraft/client/Minecraft", "m_91386_", "(Z)V", "breakerContinue", "(Lnet/minecraft/client/Minecraft;Z)V", -1 }
};
const Guard kMinecraftGuards[] = {
    {"m_91277_", "()V", "click", "(Lnet/minecraft/client/Minecraft;)Z", 1},
};

const Guard kLocalPlayerGuards[] = {
    {"m_8119_", "()V",
     "playerTick", "(Lnet/minecraft/client/player/LocalPlayer;)Z", 1},
};

const Redirect kConnectionRedirects[] = {
    {"m_243124_",
     "(Lnet/minecraft/network/protocol/Packet;"
     "Lnet/minecraft/network/PacketSendListener;)V",
     "net/minecraft/network/Connection", "m_129520_",
     "(Lnet/minecraft/network/protocol/Packet;"
     "Lnet/minecraft/network/PacketSendListener;)V",
     "packetOut",
     "(Lnet/minecraft/network/Connection;Lnet/minecraft/network/protocol/Packet;"
     "Lnet/minecraft/network/PacketSendListener;)V", -1},

    // The inbound half of the same idea. channelRead0 is the netty entry point
    // every server packet walks through, and it hands the packet to the
    // listener through one static helper -- redirecting that helper is the
    // single choke point, so one rule covers every packet type.
    //
    // channelRead0 keeps its name in SRG because it overrides a netty method;
    // the helper it calls is m_129517_ (genericsFtw).
    //
    // The hook takes exactly the callee's two arguments. A replacement has to
    // leave the stack the same shape it found it, and the callee is static, so
    // there is no receiver to hand over -- unlike the outbound rule above, where
    // the Connection is already sitting on the stack.
    {"channelRead0",
     "(Lio/netty/channel/ChannelHandlerContext;"
     "Lnet/minecraft/network/protocol/Packet;)V",
     "net/minecraft/network/Connection", "m_129517_",
     "(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketListener;)V",
     "packetIn",
     "(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketListener;)V", -1},
};

const Redirect kLivingEntityRendererRedirects[] = {
    {"m_7392_",
     "(Lnet/minecraft/world/entity/LivingEntity;FF"
     "Lcom/mojang/blaze3d/vertex/PoseStack;"
     "Lnet/minecraft/client/renderer/MultiBufferSource;I)V",
     "net/minecraft/util/Mth", "m_14189_", "(FFF)F",
     "renderYaw", "(FFF)F", -1, 1},

    {"m_7392_",
     "(Lnet/minecraft/world/entity/LivingEntity;FF"
     "Lcom/mojang/blaze3d/vertex/PoseStack;"
     "Lnet/minecraft/client/renderer/MultiBufferSource;I)V",
     "net/minecraft/util/Mth", "m_14179_", "(FFF)F",
     "renderPitch", "(FFF)F", -1, 0},
};

const StackLoad kRenderEntityLoads[] = {
    {0x19, 1},
};

const Append kLivingEntityRendererAppends[] = {
    {"m_7392_",
     "(Lnet/minecraft/world/entity/LivingEntity;FF"
     "Lcom/mojang/blaze3d/vertex/PoseStack;"
     "Lnet/minecraft/client/renderer/MultiBufferSource;I)V",
     "renderEntity", "(Lnet/minecraft/world/entity/LivingEntity;)V",
     kRenderEntityLoads, 1, true},
};

// The client ticks every entity in the level through ClientLevel.tickNonPassenger,
// the local player among them. Scaffold asks for a few of the player's own ticks to
// be dropped right after it sends a placement rotation, so that the rotation it
// sent is what the next movement is built from.
//
// The callee is Entity.tick, a plain invokevirtual whose receiver is already on
// the stack, so the hook takes that one argument -- the same shape as the outbound
// packet rule.
const Redirect kClientLevelRedirects[] = {
    {"m_104639_",
     "(Lnet/minecraft/world/entity/Entity;)V",
     "net/minecraft/world/entity/Entity", "m_8119_", "()V",
     "skipTick",
     "(Lnet/minecraft/world/entity/Entity;)V", -1},
};

const StackLoad kAttackTargetLoads[] = {{0x19, 2}};
const Append kGameModeAppends[] = {
    {"m_105223_", "(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/entity/Entity;)V",
     "attackTarget", "(Ljava/lang/Object;)V", kAttackTargetLoads, 1, true}
};
const StackLoad kFovLoads[] = {{0x15, 3}};
const StackLoad kPickLoads[] = {{0x17, 1}};
const Append kFovAppends[] = {
    {"m_109093_", "(FJZ)V", "resetWorldCamera", "()V", nullptr, 0, true},
    {"m_109087_", "(F)V", "cursorPickEnd", "(F)V", kPickLoads, 1, false, true},
    {"m_109141_", "(Lnet/minecraft/client/Camera;FZ)D", "keepFov", "(DZ)D", kFovLoads, 1, false, false, true}
};
const StackLoad kWorldCameraLoads[] = {{0x19,6},{0x19,1},{0x19,9}};
const Append kWorldCameraAppends[] = {
    {"m_109599_", "(Lcom/mojang/blaze3d/vertex/PoseStack;FJZLnet/minecraft/client/Camera;Lnet/minecraft/client/renderer/GameRenderer;Lnet/minecraft/client/renderer/LightTexture;Lorg/joml/Matrix4f;)V",
     "worldCamera", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V", kWorldCameraLoads, 3, true}
};
const Redirect kFreeLookCameraRedirects[] = {
    {"m_90575_", "(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/world/entity/Entity;ZZF)V", "net/minecraft/world/entity/Entity", "m_5675_", "(F)F", "freeLookViewYaw", "(Lnet/minecraft/world/entity/Entity;F)F", -1, -1, 0},
    {"m_90575_", "(Lnet/minecraft/world/level/BlockGetter;Lnet/minecraft/world/entity/Entity;ZZF)V", "net/minecraft/world/entity/Entity", "m_5686_", "(F)F", "freeLookViewPitch", "(Lnet/minecraft/world/entity/Entity;F)F", -1, -1, 0},
};
const Redirect kFreeLookMouseRedirects[] = {
    {"m_91523_", "()V", "net/minecraft/client/player/LocalPlayer", "m_19884_", "(DD)V", "freeLookTurn", "(Lnet/minecraft/client/player/LocalPlayer;DD)V", -1, -1, 0},
};
const ClassEdits kEdits[] = {
    {"net/minecraft/client/renderer/LevelRenderer", nullptr, 0, nullptr, 0, nullptr, 0, kWorldCameraAppends, 1},
    {"net/minecraft/client/Camera", kFreeLookCameraRedirects, 2},
    {"net/minecraft/client/MouseHandler", kFreeLookMouseRedirects, 1},
    {"net/minecraft/client/renderer/GameRenderer", nullptr, 0, nullptr, 0, nullptr, 0, kFovAppends, static_cast<int>(std::size(kFovAppends))},
    {"net/minecraft/world/entity/player/Inventory", kSlotRedirects, 1},
    {"net/minecraft/client/renderer/ItemInHandRenderer", kVisualHandRedirects, 2},
    {"net/minecraft/world/entity/Entity",
     kEntityRedirects,
     static_cast<int>(sizeof(kEntityRedirects) / sizeof(kEntityRedirects[0])),
     kEntityInsertions,
     static_cast<int>(sizeof(kEntityInsertions) / sizeof(kEntityInsertions[0]))},
    {"net/minecraft/world/entity/LivingEntity",
     kLivingEntityRedirects,
     static_cast<int>(sizeof(kLivingEntityRedirects) / sizeof(kLivingEntityRedirects[0]))},
    {"net/minecraft/client/player/LocalPlayer",
     kLocalPlayerRedirects,
     static_cast<int>(sizeof(kLocalPlayerRedirects) / sizeof(kLocalPlayerRedirects[0])),
     nullptr, 0,
     kLocalPlayerGuards,
     static_cast<int>(sizeof(kLocalPlayerGuards) / sizeof(kLocalPlayerGuards[0])),
     kLocalPlayerAppends,
     static_cast<int>(sizeof(kLocalPlayerAppends) / sizeof(kLocalPlayerAppends[0]))},
    {"net/minecraft/network/Connection",
     kConnectionRedirects,
     static_cast<int>(sizeof(kConnectionRedirects) / sizeof(kConnectionRedirects[0]))},
    {"net/minecraft/client/multiplayer/ClientLevel",
     kClientLevelRedirects,
     static_cast<int>(sizeof(kClientLevelRedirects) / sizeof(kClientLevelRedirects[0]))},
    {"net/minecraft/world/entity/player/Player",
     kPlayerRedirects,
     static_cast<int>(sizeof(kPlayerRedirects) / sizeof(kPlayerRedirects[0]))},
    {"net/minecraft/world/item/Item",
     kItemRedirects,
     static_cast<int>(sizeof(kItemRedirects) / sizeof(kItemRedirects[0]))},
    {"net/minecraft/client/multiplayer/MultiPlayerGameMode",
     kGameModeRedirects,
     static_cast<int>(sizeof(kGameModeRedirects) / sizeof(kGameModeRedirects[0])),
     nullptr, 0, nullptr, 0, kGameModeAppends, 1},
    {"net/minecraft/client/player/KeyboardInput",
     nullptr, 0, nullptr, 0, nullptr, 0,
     kKeyboardInputAppends,
     static_cast<int>(sizeof(kKeyboardInputAppends) / sizeof(kKeyboardInputAppends[0]))},
    {"net/minecraft/client/Minecraft",
     kBreakerClientRedirects, static_cast<int>(std::size(kBreakerClientRedirects)), nullptr, 0, kMinecraftGuards, 1,
     kMinecraftAppends,
     static_cast<int>(sizeof(kMinecraftAppends) / sizeof(kMinecraftAppends[0]))},
    {"net/minecraft/client/renderer/entity/LivingEntityRenderer",
     kLivingEntityRendererRedirects,
     static_cast<int>(sizeof(kLivingEntityRendererRedirects) /
                      sizeof(kLivingEntityRendererRedirects[0])),
     nullptr, 0, nullptr, 0,
     kLivingEntityRendererAppends,
     static_cast<int>(sizeof(kLivingEntityRendererAppends) /
                      sizeof(kLivingEntityRendererAppends[0]))},
};

constexpr int kEditCount = static_cast<int>(sizeof(kEdits) / sizeof(kEdits[0]));
#elif defined(NOVA_FORGE1201_HOOK_BINDINGS)
// HOOK_BINDINGS
const bridge::HookDef kHooks[] = {
    {"resetWorldCamera", "()V", reinterpret_cast<void*>(&nova::java::ResetWorldCameraHook)},
    {"worldCamera", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V", reinterpret_cast<void*>(&nova::java::ForgeWorldCameraHook)},
    {"criticalHit", "(Lnet/minecraft/client/player/LocalPlayer;)V", reinterpret_cast<void*>(&nova::java::CriticalHitHook)},
    {"freeLookTurn", "(Lnet/minecraft/client/player/LocalPlayer;DD)V", reinterpret_cast<void*>(&nova::java::FreeLookTurnHook)},
    {"freeCamControlled", "(Lnet/minecraft/client/player/LocalPlayer;)Z", reinterpret_cast<void*>(&nova::java::FreeCamControlledHook)},
    {"freeCamAttack", "(Lnet/minecraft/client/Minecraft;)Z", reinterpret_cast<void*>(&nova::java::FreeCamAttackHook)},
    {"freeLookViewYaw", "(Lnet/minecraft/world/entity/Entity;F)F", reinterpret_cast<void*>(&nova::java::FreeLookViewYawHook)},
    {"freeLookViewPitch", "(Lnet/minecraft/world/entity/Entity;F)F", reinterpret_cast<void*>(&nova::java::FreeLookViewPitchHook)},
    {"attackVelocity", "(Lnet/minecraft/world/entity/player/Player;Lnet/minecraft/world/phys/Vec3;)V", reinterpret_cast<void*>(&nova::java::AttackVelocityHook)},
    {"attackSprint", "(Lnet/minecraft/world/entity/player/Player;Z)V", reinterpret_cast<void*>(&nova::java::AttackSprintHook)},
    {"keepFov", "(DZ)D", reinterpret_cast<void*>(&nova::module::scaffold::KeepFovDoubleHook)},
    {"viewVector",
     "(Lnet/minecraft/world/entity/Entity;FF)Lnet/minecraft/world/phys/Vec3;",
     reinterpret_cast<void*>(&HookViewVector)},

    {"strafeYaw", "(Lnet/minecraft/world/entity/Entity;)F",
     reinterpret_cast<void*>(&HookStrafeYaw)},

    {"attackTarget", "(Ljava/lang/Object;)V", reinterpret_cast<void*>(&nova::java::BacktrackAttackHook)},
    {"attackYaw", "(Lnet/minecraft/world/entity/Entity;)F",
     reinterpret_cast<void*>(&HookAttackYaw)},

    {"jumpYaw", "(Lnet/minecraft/world/entity/Entity;)F",
     reinterpret_cast<void*>(&HookJumpYaw)},

    {"fallFlyingPitch", "(Lnet/minecraft/world/entity/Entity;)F",
     reinterpret_cast<void*>(&HookFallFlyingPitch)},

    {"bodyYaw", "(Lnet/minecraft/world/entity/Entity;)F",
     reinterpret_cast<void*>(&HookBodyYaw)},

    {"usingItem", "(Lnet/minecraft/world/entity/Entity;)Z",
     reinterpret_cast<void*>(&HookUsingItem)},

    {"playerTick", "(Lnet/minecraft/client/player/LocalPlayer;)Z",
     reinterpret_cast<void*>(&HookPlayerTick)},

    {"packetOut",
     "(Lnet/minecraft/network/Connection;Lnet/minecraft/network/protocol/Packet;"
     "Lnet/minecraft/network/PacketSendListener;)V",
     reinterpret_cast<void*>(&HookPacketOut)},

    {"packetIn",
     "(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketListener;)V",
     reinterpret_cast<void*>(&HookPacketIn)},

    {"skipTick", "(Lnet/minecraft/world/entity/Entity;)V",
     reinterpret_cast<void*>(&HookSkipTick)},

    {"stuckInBlock",
     "(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/world/level/block/state/BlockState;"
     "Lnet/minecraft/world/phys/Vec3;)Lnet/minecraft/world/phys/Vec3;",
     reinterpret_cast<void*>(&HookStuckInBlock)},

    {"moveInputCapture", "(F)V",
     reinterpret_cast<void*>(&HookMoveInputCapture)},

    {"moveInput", "()V",
     reinterpret_cast<void*>(&HookMoveInput)},

    {"gameTick", "()V",
     reinterpret_cast<void*>(&HookGameTick)},
    {"postGameTick", "()V", reinterpret_cast<void*>(&HookPostGameTick)},
    {"preInteract", "()V", reinterpret_cast<void*>(&HookPreInteract)},
    {"cursorPickEnd", "(F)V", reinterpret_cast<void*>(&HookCursorPickEnd)},

    {"breakerContinue", "(Lnet/minecraft/client/Minecraft;Z)V", reinterpret_cast<void*>(&java::BreakerContinueHook)},
    {"click", "(Lnet/minecraft/client/Minecraft;)Z",
     reinterpret_cast<void*>(&HookClick)},
    {"spoofSlot", "(Lnet/minecraft/world/entity/player/Inventory;)I", reinterpret_cast<void*>(&java::SpoofSlotHook)},
    {"visualPlayerHand", "(Lnet/minecraft/client/player/LocalPlayer;)Lnet/minecraft/world/item/ItemStack;", reinterpret_cast<void*>(&java::VisualPlayerHandHook)},

    {"beginMotion", "(Lnet/minecraft/client/player/LocalPlayer;)V",
     reinterpret_cast<void*>(&HookBeginMotion)},

    {"endMotion", "(Lnet/minecraft/client/player/LocalPlayer;)V",
     reinterpret_cast<void*>(&HookEndMotion)},

    {"motionYaw", "(Lnet/minecraft/world/entity/Entity;)F",
     reinterpret_cast<void*>(&HookMotionYaw)},

    {"motionPitch", "(Lnet/minecraft/world/entity/Entity;)F",
     reinterpret_cast<void*>(&HookMotionPitch)},

    {"motionOnGround", "(Lnet/minecraft/world/entity/Entity;)Z",
     reinterpret_cast<void*>(&HookMotionOnGround)},

    {"useItemRayTraceYaw", "(Lnet/minecraft/world/entity/player/Player;)F",
     reinterpret_cast<void*>(&HookUseItemRayTraceYaw)},

    {"useItemRayTracePitch", "(Lnet/minecraft/world/entity/player/Player;)F",
     reinterpret_cast<void*>(&HookUseItemRayTracePitch)},

    {"positionItem",
     "(Lnet/minecraft/client/multiplayer/ClientPacketListener;"
     "Lnet/minecraft/network/protocol/Packet;)V",
     reinterpret_cast<void*>(&HookPositionItem)},

    {"renderEntity", "(Lnet/minecraft/world/entity/LivingEntity;)V",
     reinterpret_cast<void*>(&HookRenderEntity)},

    {"renderYaw", "(FFF)F",
     reinterpret_cast<void*>(&HookRenderYaw)},

    {"renderPitch", "(FFF)F",
     reinterpret_cast<void*>(&HookRenderPitch)},
};

constexpr int kHookCount = static_cast<int>(sizeof(kHooks) / sizeof(kHooks[0]));
#elif defined(NOVA_FORGE1201_ENV)
constexpr const char* kConnectionDot = "net.minecraft.network.Connection";
#else
#error "Select a Forge1201 mapping section before including this file"
#endif
