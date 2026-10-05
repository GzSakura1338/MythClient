// Lunar 1.8.9 mappings. Intentionally no include guard: JNI entries are
// expanded for handle declarations, resolution, and cleanup.
#if defined(NOVA_GHOSTHAND_MAPPING)
NOVA_GHOSTHAND(view, "net.minecraft.client.Minecraft", "getRenderViewEntity", "()Lnet/minecraft/entity/Entity;")
NOVA_GHOSTHAND(look, "net.minecraft.entity.Entity", "getLook", "(F)Lnet/minecraft/util/Vec3;")
NOVA_GHOSTHAND(reach, "net.minecraft.client.multiplayer.PlayerControllerMP", "getBlockReachDistance", "()F")
NOVA_GHOSTHAND(extended, "net.minecraft.client.multiplayer.PlayerControllerMP", "extendedReach", "()Z")
NOVA_GHOSTHAND(pointed, "net.minecraft.client.Minecraft", "pointedEntity", "Lnet/minecraft/entity/Entity;")
NOVA_GHOSTHAND(renderer, "net.minecraft.client.Minecraft", "entityRenderer", "Lnet/minecraft/client/renderer/EntityRenderer;")
NOVA_GHOSTHAND(rendererPointed, "net.minecraft.client.renderer.EntityRenderer", "pointedEntity", "Lnet/minecraft/entity/Entity;")
NOVA_GHOSTHAND(stackItem, "net.minecraft.item.ItemStack", "getItem", "()Lnet/minecraft/item/Item;")
NOVA_GHOSTHAND(vx, "net.minecraft.util.Vec3", "xCoord", "D")
NOVA_GHOSTHAND(vy, "net.minecraft.util.Vec3", "yCoord", "D")
NOVA_GHOSTHAND(vz, "net.minecraft.util.Vec3", "zCoord", "D")
NOVA_GHOSTHAND(hitLocation, "net.minecraft.util.MovingObjectPosition", "hitVec", "Lnet/minecraft/util/Vec3;")
NOVA_GHOSTHAND(canCollide, "net.minecraft.block.Block", "canCollideCheck", "(Lnet/minecraft/block/state/IBlockState;Z)Z")
NOVA_GHOSTHAND(vehicle, "net.minecraft.entity.Entity", "ridingEntity", "Lnet/minecraft/entity/Entity;")
NOVA_GHOSTHAND(riderInteract, "net.minecraft.entity.Entity", "canRiderInteract", "()Z")
NOVA_GHOSTHAND_CLASSES("net.minecraft.item.ItemTool", "net.minecraft.item.ItemHoe", "net.minecraft.item.ItemShears", "net.minecraft.item.ItemBucket")
#elif defined(NOVA_FREECAM_MAPPING)
NOVA_FREECAM(attack,"net.minecraft.client.Minecraft","clickMouse","()V")
NOVA_FREECAM(controlled,"net.minecraft.client.entity.EntityPlayerSP","isCurrentViewEntity","()Z")
NOVA_FREECAM(ctor,"net.minecraft.client.entity.EntityOtherPlayerMP","<init>","(Lnet/minecraft/world/World;Lcom/mojang/authlib/GameProfile;)V")
NOVA_FREECAM(profile,"net.minecraft.entity.player.EntityPlayer","getGameProfile","()Lcom/mojang/authlib/GameProfile;")
NOVA_FREECAM(view,"net.minecraft.client.Minecraft","getRenderViewEntity","()Lnet/minecraft/entity/Entity;")
NOVA_FREECAM(setView,"net.minecraft.client.Minecraft","setRenderViewEntity","(Lnet/minecraft/entity/Entity;)V")
NOVA_FREECAM(move,"net.minecraft.entity.Entity","setPosition","(DDD)V")
NOVA_FREECAM(yaw,"net.minecraft.entity.Entity","rotationYaw","F")
NOVA_FREECAM(pitch,"net.minecraft.entity.Entity","rotationPitch","F")
NOVA_FREECAM(oldYaw,"net.minecraft.entity.Entity","prevRotationYaw","F")
NOVA_FREECAM(oldPitch,"net.minecraft.entity.Entity","prevRotationPitch","F")
NOVA_FREECAM(xo,"net.minecraft.entity.Entity","prevPosX","D")
NOVA_FREECAM(yo,"net.minecraft.entity.Entity","prevPosY","D")
NOVA_FREECAM(zo,"net.minecraft.entity.Entity","prevPosZ","D")
NOVA_FREECAM(oldX,"net.minecraft.entity.Entity","lastTickPosX","D")
NOVA_FREECAM(oldY,"net.minecraft.entity.Entity","lastTickPosY","D")
NOVA_FREECAM(oldZ,"net.minecraft.entity.Entity","lastTickPosZ","D")
NOVA_FREECAM(headYaw,"net.minecraft.entity.EntityLivingBase","rotationYawHead","F")
NOVA_FREECAM(invisible,"net.minecraft.entity.Entity","setInvisible","(Z)V")
NOVA_FREECAM(velocity,"net.minecraft.entity.Entity","setVelocity","(DDD)V")
NOVA_FREECAM(sprint,"net.minecraft.entity.Entity","setSprinting","(Z)V")
NOVA_FREECAM(sneak,"net.minecraft.entity.Entity","setSneaking","(Z)V")
NOVA_FREECAM(forward,"net.minecraft.entity.EntityLivingBase","moveForward","F")
NOVA_FREECAM(strafe,"net.minecraft.entity.EntityLivingBase","moveStrafing","F")
NOVA_FREECAM(add,"net.minecraft.client.multiplayer.WorldClient","addEntityToWorld","(ILnet/minecraft/entity/Entity;)V")
NOVA_FREECAM(remove,"net.minecraft.world.World","removeEntity","(Lnet/minecraft/entity/Entity;)V")
NOVA_FREECAM(refresh,"net.minecraft.world.World","markBlockRangeForRenderUpdate","(IIIIII)V")
NOVA_FREECAM(arm,"net.minecraft.client.entity.EntityPlayerSP","renderArmPitch","F")
NOVA_FREECAM(oldArm,"net.minecraft.client.entity.EntityPlayerSP","prevRenderArmPitch","F")
NOVA_FREECAM(turn,"net.minecraft.entity.Entity","setAngles","(FF)V")
#elif defined(NOVA_MLG_MAPPING)
NOVA_MLG(burning,"net.minecraft.entity.Entity","isBurning","()Z")
NOVA_MLG(water,"net.minecraft.entity.Entity","isInWater","()Z")
NOVA_MLG(lava,"net.minecraft.entity.Entity","isInLava","()Z")
NOVA_MLG(passenger,"net.minecraft.entity.Entity","isRiding","()Z")
NOVA_MLG(abilities,"net.minecraft.entity.player.EntityPlayer","capabilities","Lnet/minecraft/entity/player/PlayerCapabilities;")
NOVA_MLG(flying,"net.minecraft.entity.player.PlayerCapabilities","isFlying","Z")
NOVA_MLG(item,"net.minecraft.item.ItemStack","getItem","()Lnet/minecraft/item/Item;")
NOVA_MLG(emptyBucket,"net.minecraft.init.Items","bucket","Lnet/minecraft/item/Item;")
NOVA_MLG(provider,"net.minecraft.world.World","provider","Lnet/minecraft/world/WorldProvider;")
NOVA_MLG(evaporates,"net.minecraft.world.WorldProvider","doesWaterVaporize","()Z")
NOVA_MLG(pos,"net.minecraft.util.BlockPos","<init>","(III)V")
NOVA_MLG(state,"net.minecraft.world.World","getBlockState","(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/block/state/IBlockState;")
#elif defined(NOVA_VICTORY_MAPPING)
NOVA_VICTORY(chat, "net.minecraft.network.play.server.S02PacketChat", "getChatComponent", "()Lnet/minecraft/util/IChatComponent;")
NOVA_VICTORY(title, "net.minecraft.network.play.server.S45PacketTitle", "getMessage", "()Lnet/minecraft/util/IChatComponent;")
NOVA_VICTORY(kind, "net.minecraft.network.play.server.S45PacketTitle", "getType", "()Lnet/minecraft/network/play/server/S45PacketTitle$Type;")
NOVA_VICTORY(text, "net.minecraft.util.IChatComponent", "getFormattedText", "()Ljava/lang/String;")
#elif defined(NOVA_ITEMESP_MAPPING)
NOVA_ITEMESP(stack, "net.minecraft.entity.item.EntityItem", "getEntityItem", "()Lnet/minecraft/item/ItemStack;")
NOVA_ITEMESP(item, "net.minecraft.item.ItemStack", "getItem", "()Lnet/minecraft/item/Item;")
NOVA_ITEMESP(count, "net.minecraft.item.ItemStack", "stackSize", "I")
NOVA_ITEMESP(iron, "net.minecraft.init.Items", "iron_ingot", "Lnet/minecraft/item/Item;")
NOVA_ITEMESP(gold, "net.minecraft.init.Items", "gold_ingot", "Lnet/minecraft/item/Item;")
NOVA_ITEMESP(diamond, "net.minecraft.init.Items", "diamond", "Lnet/minecraft/item/Item;")
NOVA_ITEMESP(emerald, "net.minecraft.init.Items", "emerald", "Lnet/minecraft/item/Item;")
#elif defined(NOVA_NAMETAGS_MAPPING)
NOVA_NAMETAGS(options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_NAMETAGS(perspective, "net.minecraft.client.settings.GameSettings", "thirdPersonView", "I")
NOVA_NAMETAGS(scaled, "net.minecraft.client.gui.ScaledResolution", "<init>", "(Lnet/minecraft/client/Minecraft;)V")
NOVA_NAMETAGS(scale, "net.minecraft.client.gui.ScaledResolution", "getScaleFactor", "()I")
NOVA_NAMETAGS(width, "net.minecraft.entity.Entity", "width", "F")
NOVA_NAMETAGS(height, "net.minecraft.entity.Entity", "height", "F")
NOVA_NAMETAGS(sneaking, "net.minecraft.entity.Entity", "isSneaking", "()Z")
#elif defined(NOVA_AUTOPLAY_MAPPING)
NOVA_AUTOPLAY_CLASS(packet, "net.minecraft.network.play.server.S02PacketChat")
NOVA_AUTOPLAY_CLASS(join, "net.minecraft.network.play.server.S01PacketJoinGame")
NOVA_AUTOPLAY(overlay, "net.minecraft.network.play.server.S02PacketChat", "isChat", "()Z")
NOVA_AUTOPLAY(content, "net.minecraft.network.play.server.S02PacketChat", "getChatComponent", "()Lnet/minecraft/util/IChatComponent;")
NOVA_AUTOPLAY(text, "net.minecraft.util.IChatComponent", "getFormattedText", "()Ljava/lang/String;")
NOVA_AUTOPLAY(siblings, "net.minecraft.util.IChatComponent", "getSiblings", "()Ljava/util/List;")
NOVA_AUTOPLAY(style, "net.minecraft.util.IChatComponent", "getChatStyle", "()Lnet/minecraft/util/ChatStyle;")
NOVA_AUTOPLAY(click, "net.minecraft.util.ChatStyle", "getChatClickEvent", "()Lnet/minecraft/event/ClickEvent;")
NOVA_AUTOPLAY(action, "net.minecraft.event.ClickEvent", "getAction", "()Lnet/minecraft/event/ClickEvent$Action;")
NOVA_AUTOPLAY(value, "net.minecraft.event.ClickEvent", "getValue", "()Ljava/lang/String;")
NOVA_AUTOPLAY(chatPacket, "net.minecraft.network.play.client.C01PacketChatMessage", "<init>", "(Ljava/lang/String;)V")
#elif defined(NOVA_ESP2D_MAPPING)
NOVA_ESP2D_CLASS("net.minecraft.entity.item.EntityArmorStand")
NOVA_ESP2D(armor, "net.minecraft.entity.EntityLivingBase", "getTotalArmorValue", "()I")
NOVA_ESP2D(width, "net.minecraft.entity.Entity", "width", "F")
NOVA_ESP2D(height, "net.minecraft.entity.Entity", "height", "F")
NOVA_ESP2D(sneaking, "net.minecraft.entity.Entity", "isSneaking", "()Z")
NOVA_ESP2D(view, "net.minecraft.client.Minecraft", "getRenderViewEntity", "()Lnet/minecraft/entity/Entity;")
NOVA_ESP2D(options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_ESP2D(perspective, "net.minecraft.client.settings.GameSettings", "thirdPersonView", "I")
NOVA_ESP2D(scaled, "net.minecraft.client.gui.ScaledResolution", "<init>", "(Lnet/minecraft/client/Minecraft;)V")
NOVA_ESP2D(scale, "net.minecraft.client.gui.ScaledResolution", "getScaleFactor", "()I")
#elif defined(NOVA_BLOCKIN_MAPPING)
NOVA_BLOCKIN(level, "net.minecraft.client.Minecraft", "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;")
NOVA_BLOCKIN(options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_BLOCKIN(attack, "net.minecraft.client.settings.GameSettings", "keyBindAttack", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_BLOCKIN(use, "net.minecraft.client.settings.GameSettings", "keyBindUseItem", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_BLOCKIN(down, "net.minecraft.client.settings.KeyBinding", "isKeyDown", "()Z")
NOVA_BLOCKIN(posCtor, "net.minecraft.util.BlockPos", "<init>", "(III)V")
NOVA_BLOCKIN(state, "net.minecraft.world.World", "getBlockState", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/block/state/IBlockState;")
NOVA_BLOCKIN(block, "net.minecraft.block.state.IBlockState", "getBlock", "()Lnet/minecraft/block/Block;")
NOVA_BLOCKIN(replaceable, "net.minecraft.block.Block", "isReplaceable", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;)Z")
NOVA_BLOCKIN(item, "net.minecraft.item.ItemStack", "getItem", "()Lnet/minecraft/item/Item;")
NOVA_BLOCKIN(count, "net.minecraft.item.ItemStack", "stackSize", "I")
NOVA_BLOCKIN(meta, "net.minecraft.item.ItemStack", "getItemDamage", "()I")
NOVA_BLOCKIN(itemBlock, "net.minecraft.item.ItemBlock", "getBlock", "()Lnet/minecraft/block/Block;")
NOVA_BLOCKIN(hardness, "net.minecraft.block.Block", "getBlockHardness", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;)F")
NOVA_BLOCKIN(material, "net.minecraft.block.Block", "getMaterial", "()Lnet/minecraft/block/material/Material;")
NOVA_BLOCKIN(noTool, "net.minecraft.block.material.Material", "isToolNotRequired", "()Z")
NOVA_BLOCKIN(canPlace, "net.minecraft.item.ItemBlock", "canPlaceBlockOnSide", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/item/ItemStack;)Z")
NOVA_BLOCKIN(faces, "net.minecraft.util.EnumFacing", "values", "()[Lnet/minecraft/util/EnumFacing;")
NOVA_BLOCKIN(registry, "net.minecraft.item.Item", "itemRegistry", "Lnet/minecraft/util/RegistryNamespaced;")
NOVA_BLOCKIN(registryName, "net.minecraft.util.RegistryNamespaced", "getNameForObject", "(Ljava/lang/Object;)Ljava/lang/Object;")
NOVA_BLOCKIN(toString, "java.lang.Object", "toString", "()Ljava/lang/String;")
#elif defined(NOVA_INVMOVE_MAPPING)
NOVA_INVMOVE(emptyClick, "net.minecraft.network.play.client.C0EPacketClickWindow", "<init>", "()V")
NOVA_INVMOVE(entityAction, "net.minecraft.network.play.client.C0BPacketEntityAction", "getAction", "()Lnet/minecraft/network/play/client/C0BPacketEntityAction$Action;")
NOVA_INVMOVE(screen, "net.minecraft.client.Minecraft", "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;")
NOVA_INVMOVE(world, "net.minecraft.client.Minecraft", "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;")
NOVA_INVMOVE(options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_INVMOVE(jumpKey, "net.minecraft.client.settings.GameSettings", "keyBindJump", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_INVMOVE(pressed, "net.minecraft.client.settings.KeyBinding", "pressed", "Z")
NOVA_INVMOVE(jumping, "net.minecraft.entity.EntityLivingBase", "isJumping", "Z")
NOVA_INVMOVE(speed, "net.minecraft.potion.Potion", "moveSpeed", "Lnet/minecraft/potion/Potion;")
NOVA_INVMOVE(effect, "net.minecraft.entity.EntityLivingBase", "getActivePotionEffect", "(Lnet/minecraft/potion/Potion;)Lnet/minecraft/potion/PotionEffect;")
NOVA_INVMOVE(amplifier, "net.minecraft.potion.PotionEffect", "getAmplifier", "()I")
NOVA_INVMOVE(clickMode, "net.minecraft.network.play.client.C0EPacketClickWindow", "getMode", "()I")
NOVA_INVMOVE(clickItem, "net.minecraft.network.play.client.C0EPacketClickWindow", "getClickedItem", "()Lnet/minecraft/item/ItemStack;")
NOVA_INVMOVE(placePos, "net.minecraft.network.play.client.C08PacketPlayerBlockPlacement", "getPosition", "()Lnet/minecraft/util/BlockPos;")
NOVA_INVMOVE(state, "net.minecraft.world.World", "getBlockState", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/block/state/IBlockState;")
NOVA_INVMOVE(block, "net.minecraft.block.state.IBlockState", "getBlock", "()Lnet/minecraft/block/Block;")
NOVA_INVMOVE(action, "net.minecraft.network.play.client.C02PacketUseEntity", "getAction", "()Lnet/minecraft/network/play/client/C02PacketUseEntity$Action;")
NOVA_INVMOVE(entity, "net.minecraft.network.play.client.C02PacketUseEntity", "getEntityFromWorld", "(Lnet/minecraft/world/World;)Lnet/minecraft/entity/Entity;")
NOVA_INVMOVE(dead, "net.minecraft.entity.Entity", "isDead", "Z")
NOVA_INVMOVE(x, "net.minecraft.entity.Entity", "posX", "D")
NOVA_INVMOVE(y, "net.minecraft.entity.Entity", "posY", "D")
NOVA_INVMOVE(z, "net.minecraft.entity.Entity", "posZ", "D")
NOVA_INVMOVE(lastX, "net.minecraft.entity.Entity", "lastTickPosX", "D")
NOVA_INVMOVE(lastY, "net.minecraft.entity.Entity", "lastTickPosY", "D")
NOVA_INVMOVE(lastZ, "net.minecraft.entity.Entity", "lastTickPosZ", "D")
NOVA_INVMOVE(mx, "net.minecraft.entity.Entity", "motionX", "D")
NOVA_INVMOVE(my, "net.minecraft.entity.Entity", "motionY", "D")
NOVA_INVMOVE(mz, "net.minecraft.entity.Entity", "motionZ", "D")
NOVA_INVMOVE(uuid, "net.minecraft.entity.Entity", "getUniqueID", "()Ljava/util/UUID;")
NOVA_INVMOVE(connection, "net.minecraft.client.Minecraft", "getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;")
NOVA_INVMOVE(info, "net.minecraft.client.network.NetHandlerPlayClient", "getPlayerInfo", "(Ljava/util/UUID;)Lnet/minecraft/client/network/NetworkPlayerInfo;")
NOVA_INVMOVE(status, "net.minecraft.network.play.client.C16PacketClientStatus", "getStatus", "()Lnet/minecraft/network/play/client/C16PacketClientStatus$EnumState;")
#elif defined(NOVA_MOVEMENT_KEYS_MAPPING)
NOVA_MOVEMENT_KEY(sprint,"net.minecraft.client.settings.GameSettings","keyBindSprint","Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(drop,"net.minecraft.client.settings.GameSettings","keyBindDrop","Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(sneak,"net.minecraft.client.settings.GameSettings","keyBindSneak","Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(attack,"net.minecraft.client.settings.GameSettings","keyBindAttack","Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(use,"net.minecraft.client.settings.GameSettings","keyBindUseItem","Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(clicks,"net.minecraft.client.settings.KeyBinding","pressTime","I")
NOVA_MOVEMENT_KEY(options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_MOVEMENT_KEY(jump, "net.minecraft.client.settings.GameSettings", "keyBindJump", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(forward, "net.minecraft.client.settings.GameSettings", "keyBindForward", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(back, "net.minecraft.client.settings.GameSettings", "keyBindBack", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(left, "net.minecraft.client.settings.GameSettings", "keyBindLeft", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(right, "net.minecraft.client.settings.GameSettings", "keyBindRight", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_MOVEMENT_KEY(keyCode, "net.minecraft.client.settings.KeyBinding", "getKeyCode", "()I")
NOVA_MOVEMENT_KEY(pressed, "net.minecraft.client.settings.KeyBinding", "pressed", "Z")
#elif defined(NOVA_DISPLACE_MAPPING)
NOVA_DISPLACE(options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_DISPLACE(level, "net.minecraft.client.Minecraft", "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;")
NOVA_DISPLACE(attackKey, "net.minecraft.client.settings.GameSettings", "keyBindAttack", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_DISPLACE(forwardKey, "net.minecraft.client.settings.GameSettings", "keyBindForward", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_DISPLACE(backKey, "net.minecraft.client.settings.GameSettings", "keyBindBack", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_DISPLACE(leftKey, "net.minecraft.client.settings.GameSettings", "keyBindLeft", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_DISPLACE(rightKey, "net.minecraft.client.settings.GameSettings", "keyBindRight", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_DISPLACE(keyDown, "net.minecraft.client.settings.KeyBinding", "isKeyDown", "()Z")
NOVA_DISPLACE(keyCode, "net.minecraft.client.settings.KeyBinding", "getKeyCode", "()I")
NOVA_DISPLACE(keyTick, "net.minecraft.client.settings.KeyBinding", "onTick", "(I)V")
NOVA_DISPLACE(knockback, "net.minecraft.enchantment.EnchantmentHelper", "getKnockbackModifier", "(Lnet/minecraft/entity/EntityLivingBase;)I")
NOVA_DISPLACE(heldItem, "net.minecraft.entity.player.EntityPlayer", "getHeldItem", "()Lnet/minecraft/item/ItemStack;")
NOVA_DISPLACE(item, "net.minecraft.item.ItemStack", "getItem", "()Lnet/minecraft/item/Item;")
NOVA_DISPLACE(metadata, "net.minecraft.item.ItemStack", "getMetadata", "()I")
NOVA_DISPLACE(subtypes, "net.minecraft.item.Item", "getHasSubtypes", "()Z")
NOVA_DISPLACE(damageable, "net.minecraft.item.Item", "isDamageable", "()Z")
NOVA_DISPLACE(registry, "net.minecraft.item.Item", "itemRegistry", "Lnet/minecraft/util/RegistryNamespaced;")
NOVA_DISPLACE(registryName, "net.minecraft.util.RegistryNamespaced", "getNameForObject", "(Ljava/lang/Object;)Ljava/lang/Object;")
NOVA_DISPLACE(width, "net.minecraft.entity.Entity", "width", "F")
NOVA_DISPLACE(sameTeam, "net.minecraft.entity.EntityLivingBase", "isOnSameTeam", "(Lnet/minecraft/entity/EntityLivingBase;)Z")
NOVA_DISPLACE(displayName, "net.minecraft.entity.Entity", "getDisplayName", "()Lnet/minecraft/util/IChatComponent;")
NOVA_DISPLACE(unformatted, "net.minecraft.util.IChatComponent", "getUnformattedText", "()Ljava/lang/String;")
NOVA_DISPLACE(uuid, "net.minecraft.entity.Entity", "getUniqueID", "()Ljava/util/UUID;")
NOVA_DISPLACE(connection, "net.minecraft.client.Minecraft", "getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;")
NOVA_DISPLACE(playerInfo, "net.minecraft.client.network.NetHandlerPlayClient", "getPlayerInfo", "(Ljava/util/UUID;)Lnet/minecraft/client/network/NetworkPlayerInfo;")
NOVA_DISPLACE(team, "net.minecraft.client.network.NetworkPlayerInfo", "getPlayerTeam", "()Lnet/minecraft/scoreboard/ScorePlayerTeam;")
NOVA_DISPLACE(profile, "net.minecraft.client.network.NetworkPlayerInfo", "getGameProfile", "()Lcom/mojang/authlib/GameProfile;")
NOVA_DISPLACE(formatName, "net.minecraft.scoreboard.ScorePlayerTeam", "formatPlayerName", "(Lnet/minecraft/scoreboard/Team;Ljava/lang/String;)Ljava/lang/String;")
NOVA_DISPLACE(pos, "net.minecraft.util.BlockPos", "<init>", "(III)V")
NOVA_DISPLACE(air, "net.minecraft.world.World", "isAirBlock", "(Lnet/minecraft/util/BlockPos;)Z")
NOVA_DISPLACE(loaded, "net.minecraft.world.World", "isBlockLoaded", "(Lnet/minecraft/util/BlockPos;)Z")
NOVA_DISPLACE(state, "net.minecraft.world.World", "getBlockState", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/block/state/IBlockState;")
NOVA_DISPLACE(block, "net.minecraft.block.state.IBlockState", "getBlock", "()Lnet/minecraft/block/Block;")
NOVA_DISPLACE(box, "net.minecraft.util.AxisAlignedBB", "<init>", "(DDDDDD)V")
NOVA_DISPLACE(collisions, "net.minecraft.block.Block", "addCollisionBoxesToList", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;Lnet/minecraft/block/state/IBlockState;Lnet/minecraft/util/AxisAlignedBB;Ljava/util/List;Lnet/minecraft/entity/Entity;)V")
NOVA_DISPLACE_CLASS(sword, "net.minecraft.item.ItemSword")
NOVA_DISPLACE_CLASS(bow, "net.minecraft.item.ItemBow")
NOVA_DISPLACE_CLASS(tool, "net.minecraft.item.ItemTool")
NOVA_DISPLACE_CLASS(axe, "net.minecraft.item.ItemAxe")
NOVA_DISPLACE_CLASS(pickaxe, "net.minecraft.item.ItemPickaxe")
NOVA_DISPLACE_CLASS(shovel, "net.minecraft.item.ItemSpade")
NOVA_DISPLACE_CLASS(hoe, "net.minecraft.item.ItemHoe")
NOVA_DISPLACE_CLASS(shears, "net.minecraft.item.ItemShears")
#elif defined(NOVA_INVENTORY_SCREEN_MAPPING)
NOVA_INVENTORY_SCREEN(show, "net.minecraft.client.Minecraft", "displayGuiScreen", "(Lnet/minecraft/client/gui/GuiScreen;)V")
NOVA_INVENTORY_SCREEN(inventory, "net.minecraft.client.gui.inventory.GuiInventory", "<init>", "(Lnet/minecraft/entity/player/EntityPlayer;)V")
NOVA_INVENTORY_SCREEN(carried, "net.minecraft.entity.player.InventoryPlayer", "getItemStack", "()Lnet/minecraft/item/ItemStack;")
NOVA_INVENTORY_SCREEN(inventoryField, "net.minecraft.entity.player.EntityPlayer", "inventory", "Lnet/minecraft/entity/player/InventoryPlayer;")
NOVA_INVENTORY_SCREEN(status, "net.minecraft.network.play.client.C16PacketClientStatus", "<init>", "(Lnet/minecraft/network/play/client/C16PacketClientStatus$EnumState;)V")
NOVA_INVENTORY_SCREEN(openStatus, "net.minecraft.network.play.client.C16PacketClientStatus$EnumState", "OPEN_INVENTORY_ACHIEVEMENT", "Lnet/minecraft/network/play/client/C16PacketClientStatus$EnumState;")
#elif defined(NOVA_AUTOPOT_MAPPING)
NOVA_AUTOPOT_CLASS("net.minecraft.item.ItemPotion")
NOVA_AUTOPOT(stackItem, "net.minecraft.item.ItemStack", "getItem", "()Lnet/minecraft/item/Item;")
NOVA_AUTOPOT(stackCount, "net.minecraft.item.ItemStack", "stackSize", "I")
NOVA_AUTOPOT(damage, "net.minecraft.item.ItemStack", "getItemDamage", "()I")
NOVA_AUTOPOT(isSplash, "net.minecraft.item.ItemPotion", "isSplash", "(I)Z")
NOVA_AUTOPOT(effects, "net.minecraft.item.ItemPotion", "getEffects", "(Lnet/minecraft/item/ItemStack;)Ljava/util/List;")
NOVA_AUTOPOT(effectKind, "net.minecraft.potion.PotionEffect", "getPotionID", "()I")
NOVA_AUTOPOT(controller, "net.minecraft.client.Minecraft", "playerController", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;")
NOVA_AUTOPOT(world, "net.minecraft.client.Minecraft", "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;")
NOVA_AUTOPOT(sync, "net.minecraft.client.multiplayer.PlayerControllerMP", "syncCurrentPlayItem", "()V")
NOVA_AUTOPOT(use, "net.minecraft.client.multiplayer.PlayerControllerMP", "sendUseItem", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z")
#elif defined(NOVA_KEEP_SPRINT_MAPPING)
{"net.minecraft.entity.Entity", "onGround", "Z"},
{"net.minecraft.entity.EntityLivingBase", "hurtTime", "I"}, {}, {}, {},
{"net.minecraft.entity.Entity", "setSprinting", "(Z)V"}
#elif defined(NOVA_FREELOOK_MAPPING)
NOVA_FREELOOK(options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_FREELOOK(view, "net.minecraft.client.Minecraft", "getRenderViewEntity", "()Lnet/minecraft/entity/Entity;")
NOVA_FREELOOK(perspective, "net.minecraft.client.settings.GameSettings", "thirdPersonView", "I")
NOVA_FREELOOK(fov, "net.minecraft.client.settings.GameSettings", "fovSetting", "F")
NOVA_FREELOOK(yaw, "net.minecraft.entity.Entity", "rotationYaw", "F")
NOVA_FREELOOK(pitch, "net.minecraft.entity.Entity", "rotationPitch", "F")
NOVA_FREELOOK(previousYaw, "net.minecraft.entity.Entity", "prevRotationYaw", "F")
NOVA_FREELOOK(previousPitch, "net.minecraft.entity.Entity", "prevRotationPitch", "F")
NOVA_FREELOOK(focus, "net.minecraft.client.Minecraft", "inGameHasFocus", "Z")
NOVA_FREELOOK(helper, "net.minecraft.client.Minecraft", "mouseHelper", "Lnet/minecraft/util/MouseHelper;")
NOVA_FREELOOK(mouseUpdate, "net.minecraft.util.MouseHelper", "mouseXYChange", "()V")
NOVA_FREELOOK(deltaX, "net.minecraft.util.MouseHelper", "deltaX", "I")
NOVA_FREELOOK(deltaY, "net.minecraft.util.MouseHelper", "deltaY", "I")
NOVA_FREELOOK(sensitivity, "net.minecraft.client.settings.GameSettings", "mouseSensitivity", "F")
NOVA_FREELOOK(renderer, "net.minecraft.client.Minecraft", "entityRenderer", "Lnet/minecraft/client/renderer/EntityRenderer;")
NOVA_FREELOOK(loadShader, "net.minecraft.client.renderer.EntityRenderer", "loadEntityShader", "(Lnet/minecraft/entity/Entity;)V")
NOVA_FREELOOK(renderGlobal, "net.minecraft.client.Minecraft", "renderGlobal", "Lnet/minecraft/client/renderer/RenderGlobal;")
NOVA_FREELOOK(dirty, "net.minecraft.client.renderer.RenderGlobal", "setDisplayListEntitiesDirty", "()V")
NOVA_FREELOOK(viewX, "net.minecraft.client.renderer.entity.RenderManager", "playerViewX", "F")
NOVA_FREELOOK(viewY, "net.minecraft.client.renderer.entity.RenderManager", "playerViewY", "F")
NOVA_FREELOOK(rotationX, "net.minecraft.client.renderer.ActiveRenderInfo", "rotationX", "F")
NOVA_FREELOOK(rotationZ, "net.minecraft.client.renderer.ActiveRenderInfo", "rotationZ", "F")
NOVA_FREELOOK(rotationYZ, "net.minecraft.client.renderer.ActiveRenderInfo", "rotationYZ", "F")
NOVA_FREELOOK(rotationXY, "net.minecraft.client.renderer.ActiveRenderInfo", "rotationXY", "F")
NOVA_FREELOOK(rotationXZ, "net.minecraft.client.renderer.ActiveRenderInfo", "rotationXZ", "F")
NOVA_FREELOOK(armYaw, "net.minecraft.client.entity.EntityPlayerSP", "renderArmYaw", "F")
NOVA_FREELOOK(previousArmYaw, "net.minecraft.client.entity.EntityPlayerSP", "prevRenderArmYaw", "F")
#elif defined(NOVA_BREAKER_MAPPING)
NOVA_BREAKER(continueMining, "net.minecraft.client.Minecraft", "sendClickBlockToController", "(Z)V")
NOVA_BREAKER(controller, "net.minecraft.client.Minecraft", "playerController", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;")
NOVA_BREAKER(level, "net.minecraft.client.Minecraft", "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;")
NOVA_BREAKER(progress, "net.minecraft.client.multiplayer.PlayerControllerMP", "curBlockDamageMP", "F")
NOVA_BREAKER(pos, "net.minecraft.util.BlockPos", "<init>", "(III)V")
NOVA_BREAKER(state, "net.minecraft.world.World", "getBlockState", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/block/state/IBlockState;")
NOVA_BREAKER(block, "net.minecraft.block.state.IBlockState", "getBlock", "()Lnet/minecraft/block/Block;")
NOVA_BREAKER(hardness, "net.minecraft.block.Block", "getBlockHardness", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;)F")
NOVA_BREAKER(relative, "net.minecraft.block.Block", "getPlayerRelativeBlockHardness", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;)F")
NOVA_BREAKER(air, "net.minecraft.block.Block", "getMaterial", "()Lnet/minecraft/block/material/Material;")
NOVA_BREAKER(collision, "net.minecraft.block.Block", "getCollisionBoundingBox", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;Lnet/minecraft/block/state/IBlockState;)Lnet/minecraft/util/AxisAlignedBB;")
NOVA_BREAKER(empty, "net.minecraft.block.material.Material", "isLiquid", "()Z")
NOVA_BREAKER(damage, "net.minecraft.client.multiplayer.PlayerControllerMP", "onPlayerDamageBlock", "(Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;)Z")
NOVA_BREAKER(destroy, "net.minecraft.client.multiplayer.PlayerControllerMP", "onPlayerDestroyBlock", "(Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;)Z")
NOVA_BREAKER(stop, "net.minecraft.client.multiplayer.PlayerControllerMP", "resetBlockRemoving", "()V")
NOVA_BREAKER(ground, "net.minecraft.entity.Entity", "onGround", "Z")
NOVA_BREAKER(facing, "net.minecraft.util.EnumFacing", "values", "()[Lnet/minecraft/util/EnumFacing;")
NOVA_BREAKER(vector, "net.minecraft.util.Vec3", "<init>", "(DDD)V")
NOVA_BREAKER(clip, "net.minecraft.block.Block", "collisionRayTrace", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/Vec3;Lnet/minecraft/util/Vec3;)Lnet/minecraft/util/MovingObjectPosition;")
NOVA_BREAKER(shape, "net.minecraft.block.Block", "getMaterial", "()Lnet/minecraft/block/material/Material;")
NOVA_BREAKER(hit, "net.minecraft.client.Minecraft", "objectMouseOver", "Lnet/minecraft/util/MovingObjectPosition;")
NOVA_BREAKER(hitFace, "net.minecraft.util.MovingObjectPosition", "sideHit", "Lnet/minecraft/util/EnumFacing;")
NOVA_BREAKER(dig, "net.minecraft.network.play.client.C07PacketPlayerDigging", "<init>", "(Lnet/minecraft/network/play/client/C07PacketPlayerDigging$Action;Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;)V")
NOVA_BREAKER(digAction, "net.minecraft.network.play.client.C07PacketPlayerDigging", "getStatus", "()Lnet/minecraft/network/play/client/C07PacketPlayerDigging$Action;")
NOVA_BREAKER(actions, "net.minecraft.network.play.client.C07PacketPlayerDigging$Action", "values", "()[Lnet/minecraft/network/play/client/C07PacketPlayerDigging$Action;")
NOVA_BREAKER(updatePos, "net.minecraft.network.play.server.S23PacketBlockChange", "getBlockPosition", "()Lnet/minecraft/util/BlockPos;")
NOVA_BREAKER(updateState, "net.minecraft.network.play.server.S23PacketBlockChange", "getBlockState", "()Lnet/minecraft/block/state/IBlockState;")
NOVA_BREAKER(posX, "net.minecraft.util.Vec3i", "getX", "()I")
NOVA_BREAKER(posY, "net.minecraft.util.Vec3i", "getY", "()I")
NOVA_BREAKER(posZ, "net.minecraft.util.Vec3i", "getZ", "()I")
NOVA_BREAKER(teleX, "net.minecraft.network.play.server.S08PacketPlayerPosLook", "getX", "()D")
NOVA_BREAKER(teleY, "net.minecraft.network.play.server.S08PacketPlayerPosLook", "getY", "()D")
NOVA_BREAKER(teleZ, "net.minecraft.network.play.server.S08PacketPlayerPosLook", "getZ", "()D")
NOVA_BREAKER(teleFlags, "net.minecraft.network.play.server.S08PacketPlayerPosLook", "func_179834_f", "()Ljava/util/Set;")
NOVA_BREAKER_CLASSES("net.minecraft.block.BlockBed", "net.minecraft.block.BlockAir", "net.minecraft.block.BlockLiquid")
#elif defined(NOVA_JUMP_DELAY_MAPPING)
{"net.minecraft.entity.EntityLivingBase", "jumpTicks", "I"}
#elif defined(NOVA_SLOT_SPOOF_MAPPING)
"inventory", "Lnet/minecraft/entity/player/InventoryPlayer;", "currentItem",
"getStackInSlot", "(I)Lnet/minecraft/item/ItemStack;",
"playerController", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;", "syncCurrentPlayItem"
#elif defined(NOVA_BEDPLATES_MAPPING)
#include "minecraft/compat/BedMapping.h"
namespace nova::compat {
inline constexpr BedMapping Bed189Mapping{
    "net.minecraft.util.BlockPos", "net.minecraft.block.BlockBed",
    "Lnet/minecraft/util/BlockPos;", "Lnet/minecraft/block/state/IBlockState;", "Lnet/minecraft/block/Block;", "Lnet/minecraft/world/chunk/Chunk;",
    "getBlockState", "getBlock", "getChunkFromChunkCoords", "", "getX", "getY", "getZ",
    "getBlockHardness", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;)F", "isSolid",
    "getBlockStorageArray", "()[Lnet/minecraft/world/chunk/storage/ExtendedBlockStorage;", "getData", "getMaterial", "()Lnet/minecraft/block/material/Material;"
};
}
#elif defined(NOVA_BRIDGEASSIST_MAPPING)
NOVA_BRIDGE(abilities, "net.minecraft.entity.player.EntityPlayer", "capabilities", "Lnet/minecraft/entity/player/PlayerCapabilities;")
NOVA_BRIDGE(airSpeed, "net.minecraft.entity.EntityLivingBase", "jumpMovementFactor", "F")
NOVA_BRIDGE(amplifier, "net.minecraft.potion.PotionEffect", "getAmplifier", "()I")
NOVA_BRIDGE(areaLoaded, "net.minecraft.world.World", "isAreaLoaded", "(Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/BlockPos;Z)Z")
NOVA_BRIDGE(attribute, "net.minecraft.entity.EntityLivingBase", "getEntityAttribute", "(Lnet/minecraft/entity/ai/attributes/IAttribute;)Lnet/minecraft/entity/ai/attributes/IAttributeInstance;")
NOVA_BRIDGE(attributeValue, "net.minecraft.entity.ai.attributes.IAttributeInstance", "getAttributeValue", "()D")
NOVA_BRIDGE(block, "net.minecraft.block.state.IBlockState", "getBlock", "()Lnet/minecraft/block/Block;")
NOVA_BRIDGE(boxCtor, "net.minecraft.util.AxisAlignedBB", "<init>", "(DDDDDD)V")
NOVA_BRIDGE(collisions, "net.minecraft.world.World", "getCollidingBoundingBoxes", "(Lnet/minecraft/entity/Entity;Lnet/minecraft/util/AxisAlignedBB;)Ljava/util/List;")
NOVA_BRIDGE(controller, "net.minecraft.client.Minecraft", "playerController", "Lnet/minecraft/client/multiplayer/PlayerControllerMP;")
NOVA_BRIDGE(depth, "net.minecraft.enchantment.EnchantmentHelper", "getDepthStriderModifier", "(Lnet/minecraft/entity/Entity;)I")
NOVA_BRIDGE(flow, "net.minecraft.block.Block", "modifyAcceleration", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;Lnet/minecraft/entity/Entity;Lnet/minecraft/util/Vec3;)Lnet/minecraft/util/Vec3;")
NOVA_BRIDGE(flying, "net.minecraft.entity.player.PlayerCapabilities", "isFlying", "Z")
NOVA_BRIDGE(food, "net.minecraft.entity.player.EntityPlayer", "getFoodStats", "()Lnet/minecraft/util/FoodStats;")
NOVA_BRIDGE(foodLevel, "net.minecraft.util.FoodStats", "getFoodLevel", "()I")
NOVA_BRIDGE(forward, "net.minecraft.util.MovementInput", "moveForward", "F")
NOVA_BRIDGE(friction, "net.minecraft.block.Block", "slipperiness", "F")
NOVA_BRIDGE(getEffect, "net.minecraft.entity.EntityLivingBase", "getActivePotionEffect", "(Lnet/minecraft/potion/Potion;)Lnet/minecraft/potion/PotionEffect;")
NOVA_BRIDGE(height, "net.minecraft.entity.Entity", "height", "F")
NOVA_BRIDGE(horizontal, "net.minecraft.entity.Entity", "isCollidedHorizontally", "Z")
NOVA_BRIDGE(input, "net.minecraft.client.entity.EntityPlayerSP", "movementInput", "Lnet/minecraft/util/MovementInput;")
NOVA_BRIDGE(jumpCooldown, "net.minecraft.entity.EntityLivingBase", "jumpTicks", "I")
NOVA_BRIDGE(jumpEffect, "net.minecraft.potion.Potion", "jump", "Lnet/minecraft/potion/Potion;")
NOVA_BRIDGE(keyCode, "net.minecraft.client.settings.KeyBinding", "getKeyCode", "()I")
NOVA_BRIDGE(ladder, "net.minecraft.entity.EntityLivingBase", "isOnLadder", "()Z")
NOVA_BRIDGE(lava, "net.minecraft.block.material.Material", "lava", "Lnet/minecraft/block/material/Material;")
NOVA_BRIDGE(liquidHeight, "net.minecraft.block.BlockLiquid", "getLiquidHeightPercent", "(I)F")
NOVA_BRIDGE(liquidLevel, "net.minecraft.block.BlockLiquid", "LEVEL", "Lnet/minecraft/block/properties/PropertyInteger;")
NOVA_BRIDGE(material, "net.minecraft.block.Block", "getMaterial", "()Lnet/minecraft/block/material/Material;")
NOVA_BRIDGE(materialInBox, "net.minecraft.world.World", "isMaterialInBB", "(Lnet/minecraft/util/AxisAlignedBB;Lnet/minecraft/block/material/Material;)Z")
NOVA_BRIDGE(maxX, "net.minecraft.util.AxisAlignedBB", "maxX", "D")
NOVA_BRIDGE(maxY, "net.minecraft.util.AxisAlignedBB", "maxY", "D")
NOVA_BRIDGE(maxZ, "net.minecraft.util.AxisAlignedBB", "maxZ", "D")
NOVA_BRIDGE(minX, "net.minecraft.util.AxisAlignedBB", "minX", "D")
NOVA_BRIDGE(minY, "net.minecraft.util.AxisAlignedBB", "minY", "D")
NOVA_BRIDGE(minZ, "net.minecraft.util.AxisAlignedBB", "minZ", "D")
NOVA_BRIDGE(normal, "net.minecraft.block.Block", "isNormalCube", "()Z")
NOVA_BRIDGE(options, "net.minecraft.client.Minecraft", "gameSettings", "Lnet/minecraft/client/settings/GameSettings;")
NOVA_BRIDGE(posCtor, "net.minecraft.util.BlockPos", "<init>", "(III)V")
NOVA_BRIDGE(reach, "net.minecraft.client.multiplayer.PlayerControllerMP", "getBlockReachDistance", "()F")
NOVA_BRIDGE(riding, "net.minecraft.entity.Entity", "isRiding", "()Z")
NOVA_BRIDGE(sleeping, "net.minecraft.entity.player.EntityPlayer", "isPlayerSleeping", "()Z")
NOVA_BRIDGE(sneakKey, "net.minecraft.client.settings.GameSettings", "keyBindSneak", "Lnet/minecraft/client/settings/KeyBinding;")
NOVA_BRIDGE(speedAttribute, "net.minecraft.entity.SharedMonsterAttributes", "movementSpeed", "Lnet/minecraft/entity/ai/attributes/IAttribute;")
NOVA_BRIDGE(state, "net.minecraft.world.World", "getBlockState", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/block/state/IBlockState;")
NOVA_BRIDGE(stateValue, "net.minecraft.block.state.IBlockState", "getValue", "(Lnet/minecraft/block/properties/IProperty;)Ljava/lang/Comparable;")
NOVA_BRIDGE(step, "net.minecraft.entity.Entity", "stepHeight", "F")
NOVA_BRIDGE(vecCtor, "net.minecraft.util.Vec3", "<init>", "(DDD)V")
NOVA_BRIDGE(vecX, "net.minecraft.util.Vec3", "xCoord", "D")
NOVA_BRIDGE(vecY, "net.minecraft.util.Vec3", "yCoord", "D")
NOVA_BRIDGE(vecZ, "net.minecraft.util.Vec3", "zCoord", "D")
NOVA_BRIDGE(water, "net.minecraft.block.material.Material", "water", "Lnet/minecraft/block/material/Material;")
NOVA_BRIDGE(web, "net.minecraft.entity.Entity", "isInWeb", "Z")
NOVA_BRIDGE(width, "net.minecraft.entity.Entity", "width", "F")
NOVA_BRIDGE(world, "net.minecraft.client.Minecraft", "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;")
#elif defined(NOVA_PLAYER_LIST_MAPPING)
NOVA_PLAYER_LIST(Uuid, "net.minecraft.entity.Entity", "getUniqueID", "()Ljava/util/UUID;")
NOVA_PLAYER_LIST(Connection, "net.minecraft.client.Minecraft", "getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;")
NOVA_PLAYER_LIST(PlayerInfo, "net.minecraft.client.network.NetHandlerPlayClient", "getPlayerInfo", "(Ljava/util/UUID;)Lnet/minecraft/client/network/NetworkPlayerInfo;")
#elif defined(NOVA_TEAMS_MAPPING)
// UUID, connection, player info, team, name, color container, color, value.
NOVA_TEAM(Uuid, "net.minecraft.entity.Entity", "getUniqueID", "()Ljava/util/UUID;")
NOVA_TEAM(EntityTeam, "net.minecraft.entity.EntityLivingBase", "getTeam", "()Lnet/minecraft/scoreboard/Team;")
NOVA_TEAM(Connection, "net.minecraft.client.Minecraft", "getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;")
NOVA_TEAM(Info, "net.minecraft.client.network.NetHandlerPlayClient", "getPlayerInfo", "(Ljava/util/UUID;)Lnet/minecraft/client/network/NetworkPlayerInfo;")
NOVA_TEAM(Team, "net.minecraft.client.network.NetworkPlayerInfo", "getPlayerTeam", "()Lnet/minecraft/scoreboard/ScorePlayerTeam;")
NOVA_TEAM(Name, "net.minecraft.scoreboard.ScorePlayerTeam", "getRegisteredName", "()Ljava/lang/String;")
NOVA_TEAM(Format, "net.minecraft.scoreboard.ScorePlayerTeam", "getChatFormat", "()Lnet/minecraft/util/EnumChatFormatting;")
NOVA_TEAM(Color, "net.minecraft.util.EnumChatFormatting", "getColorIndex", "()I")
NOVA_TEAM(Value, "java.lang.Integer", "intValue", "()I")
// Armor team detection; only members used by this version are resolved.
NOVA_TEAM(Equipped, "net.minecraft.entity.player.EntityPlayer", "getCurrentArmor", "(I)Lnet/minecraft/item/ItemStack;")
NOVA_TEAM(StackItem, "net.minecraft.item.ItemStack", "getItem", "()Lnet/minecraft/item/Item;")
NOVA_TEAM(HasCustomColor, "net.minecraft.item.ItemArmor", "hasColor", "(Lnet/minecraft/item/ItemStack;)Z")
NOVA_TEAM(ArmorColor, "net.minecraft.item.ItemArmor", "getColor", "(Lnet/minecraft/item/ItemStack;)I")
#elif defined(NOVA_HUD_MAPPING)
NOVA_HUD("server", "net.minecraft.client.Minecraft", "getCurrentServerData", "()Lnet/minecraft/client/multiplayer/ServerData;")
NOVA_HUD("address", "net.minecraft.client.multiplayer.ServerData", "serverIP", "Ljava/lang/String;")
NOVA_HUD("local", "net.minecraft.client.Minecraft", "isSingleplayer", "()Z")
NOVA_HUD("uuid", "net.minecraft.entity.Entity", "getUniqueID", "()Ljava/util/UUID;")
NOVA_HUD("connection", "net.minecraft.client.Minecraft", "getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;")
NOVA_HUD("info", "net.minecraft.client.network.NetHandlerPlayClient", "getPlayerInfo", "(Ljava/util/UUID;)Lnet/minecraft/client/network/NetworkPlayerInfo;")
NOVA_HUD("latency", "net.minecraft.client.network.NetworkPlayerInfo", "getResponseTime", "()I")
NOVA_HUD("time", "net.minecraft.network.play.server.S03PacketTimeUpdate", "getTotalWorldTime", "()J")
#elif defined(NOVA_BACKTRACK_MAPPING)
NOVA_BT("Move", "net.minecraft.network.play.server.S14PacketEntity", "", "")
NOVA_BT("Teleport", "net.minecraft.network.play.server.S18PacketEntityTeleport", "", "")
NOVA_BT("Health", "net.minecraft.network.play.server.S06PacketUpdateHealth", "", "")
NOVA_BT("moveId", "net.minecraft.network.play.server.S14PacketEntity", "entityId", "I")
NOVA_BT("teleId", "net.minecraft.network.play.server.S18PacketEntityTeleport", "entityId", "I")
NOVA_BT("health", "net.minecraft.network.play.server.S06PacketUpdateHealth", "health", "F")
NOVA_BT("dx", "net.minecraft.network.play.server.S14PacketEntity", "posX", "B")
NOVA_BT("dy", "net.minecraft.network.play.server.S14PacketEntity", "posY", "B")
NOVA_BT("dz", "net.minecraft.network.play.server.S14PacketEntity", "posZ", "B")
NOVA_BT("tx", "net.minecraft.network.play.server.S18PacketEntityTeleport", "posX", "I")
NOVA_BT("ty", "net.minecraft.network.play.server.S18PacketEntityTeleport", "posY", "I")
NOVA_BT("tz", "net.minecraft.network.play.server.S18PacketEntityTeleport", "posZ", "I")
NOVA_BT("baseX", "net.minecraft.entity.Entity", "serverPosX", "I")
NOVA_BT("baseY", "net.minecraft.entity.Entity", "serverPosY", "I")
NOVA_BT("baseZ", "net.minecraft.entity.Entity", "serverPosZ", "I")
#elif defined(NOVA_LUNAR189_PROFILE)
#include "minecraft/compat/Profile.h"

namespace nova::compat {
inline constexpr GameMapping Lunar189Mapping{
    "getMinecraft", "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;",
    "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;",
    "currentScreen", "Lnet/minecraft/client/gui/GuiScreen;"
};
}
#elif defined(NOVA_LUNAR189_EDITS)
const Redirect lunarEntityRedirects[] = {
    {"moveFlying", "(FFF)V", "net/minecraft/entity/Entity", "rotationYaw", "F",
     "strafeYaw", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"getLook", "(F)Lnet/minecraft/util/Vec3;", "net/minecraft/entity/Entity",
     "getVectorForRotation", "(FF)Lnet/minecraft/util/Vec3;",
     "viewVector", "(Lnet/minecraft/entity/Entity;FF)Lnet/minecraft/util/Vec3;", -1}
};
const Redirect lunarLivingRedirects[] = {
    {"moveEntityWithHeading", "(FF)V", "net/minecraft/entity/EntityLivingBase", "moveEntity", "(DDD)V",
     "inventoryMove", "(Lnet/minecraft/entity/EntityLivingBase;DDD)V", -1},
    {"jump", "()V", "net/minecraft/entity/EntityLivingBase", "rotationYaw", "F",
     "jumpYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"updateDistance", "(FF)F", "net/minecraft/entity/EntityLivingBase", "rotationYaw", "F",
     "bodyYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", -1, -1, 0xB4},
    {"getLook", "(F)Lnet/minecraft/util/Vec3;", "net/minecraft/entity/EntityLivingBase",
     "getVectorForRotation", "(FF)Lnet/minecraft/util/Vec3;",
     "livingViewVector", "(Lnet/minecraft/entity/EntityLivingBase;FF)Lnet/minecraft/util/Vec3;", -1}
};
const Redirect lunarPlayerRedirects[] = {
    {"onUpdateWalkingPlayer","()V","net/minecraft/client/entity/EntityPlayerSP","isCurrentViewEntity","()Z",
     "freeCamControlled","(Lnet/minecraft/client/entity/EntityPlayerSP;)Z",-1},
    {"getLook", "(F)Lnet/minecraft/util/Vec3;", "net/minecraft/client/entity/EntityPlayerSP",
     "getVectorForRotation", "(FF)Lnet/minecraft/util/Vec3;",
     "playerViewVector", "(Lnet/minecraft/client/entity/EntityPlayerSP;FF)Lnet/minecraft/util/Vec3;", -1},
    {"onUpdate", "()V", "net/minecraft/client/entity/EntityPlayerSP", "onUpdateWalkingPlayer", "()V",
     "walking", "(Lnet/minecraft/client/entity/EntityPlayerSP;)V", -1},
    {"onLivingUpdate", "()V", "net/minecraft/util/MovementInput", "updatePlayerMoveState", "()V",
     "input", "(Lnet/minecraft/util/MovementInput;)V", -1},
    {"onUpdateWalkingPlayer", "()V", "net/minecraft/client/entity/EntityPlayerSP", "rotationYaw", "F",
     "motionYaw", "(Lnet/minecraft/client/entity/EntityPlayerSP;)F", -1, -1, 0xB4},
    {"onUpdateWalkingPlayer", "()V", "net/minecraft/client/entity/EntityPlayerSP", "rotationPitch", "F",
     "motionPitch", "(Lnet/minecraft/client/entity/EntityPlayerSP;)F", -1, -1, 0xB4},
    {"onUpdateWalkingPlayer", "()V", "net/minecraft/client/entity/EntityPlayerSP", "onGround", "Z",
     "motionGround", "(Lnet/minecraft/client/entity/EntityPlayerSP;)Z", -1, -1, 0xB4},
    {"onLivingUpdate", "()V", "net/minecraft/client/entity/AbstractClientPlayer", "onLivingUpdate", "()V",
     "livingUpdate", "(Lnet/minecraft/client/entity/AbstractClientPlayer;)V", -1}
};
const Guard lunarEntityGuards[] = {
    {"moveFlying", "(FFF)V", "inventoryAcceleration", "(Lnet/minecraft/entity/Entity;)Z", 1, true}
};
const Guard lunarLivingGuards[] = {
    {"jump", "()V", "inventoryAcceleration", "(Lnet/minecraft/entity/Entity;)Z", 1, true}
};
const Guard lunarPlayerGuards[] = {
    {"onUpdate", "()V", "playerTick", "(Lnet/minecraft/client/entity/EntityPlayerSP;)Z", 1}
};
const Append lunarPlayerAppends[] = {
    {"onUpdate", "()V", "postPlayerTick", "()V", nullptr, 0, false, true}
};
const Append lunarMinecraftAppends[] = {
    {"runTick", "()V", "gameTick", "()V", nullptr, 0, true},
    {"runTick", "()V", "postGameTick", "()V", nullptr, 0, false, true}
};
const Redirect lunarMinecraftRedirects[] = {
    {"runTick","()V","net/minecraft/client/Minecraft","clickMouse","()V",
     "freeCamClick","(Lnet/minecraft/client/Minecraft;)V",-1},
    {"runTick", "()V", "net/minecraft/client/renderer/EntityRenderer", "getMouseOver", "(F)V",
     "clientRotation", "(Lnet/minecraft/client/renderer/EntityRenderer;F)V", -1},
    {"runGameLoop", "()V", "net/minecraft/client/renderer/EntityRenderer", "updateCameraAndRender", "(FJ)V",
     "renderFrame", "(Lnet/minecraft/client/renderer/EntityRenderer;FJ)V", -1},
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
const Redirect lunarBlockRenderRedirects[] = {
    {"updateEquippedItem", "()V", "net/minecraft/entity/player/InventoryPlayer", "getCurrentItem", "()Lnet/minecraft/item/ItemStack;",
     "visualInventoryHand", "(Lnet/minecraft/entity/player/InventoryPlayer;)Lnet/minecraft/item/ItemStack;", -1},
    {"renderItemInFirstPerson", "(F)V", "net/minecraft/client/entity/AbstractClientPlayer",
     "getItemInUseCount", "()I", "blockUseCount",
     "(Lnet/minecraft/client/entity/AbstractClientPlayer;)I", -1}
};
const Redirect lunarAttackRedirects[] = {
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
const Redirect lunarItemRedirects[] = {
    {"getMovingObjectPositionFromPlayer", "(Lnet/minecraft/world/World;Lnet/minecraft/entity/player/EntityPlayer;Z)Lnet/minecraft/util/MovingObjectPosition;",
     "net/minecraft/entity/player/EntityPlayer", "rotationYaw", "F",
     "itemYaw", "(Lnet/minecraft/entity/player/EntityPlayer;)F", -1, -1, 0xB4},
    {"getMovingObjectPositionFromPlayer", "(Lnet/minecraft/world/World;Lnet/minecraft/entity/player/EntityPlayer;Z)Lnet/minecraft/util/MovingObjectPosition;",
     "net/minecraft/entity/player/EntityPlayer", "rotationPitch", "F",
     "itemPitch", "(Lnet/minecraft/entity/player/EntityPlayer;)F", -1, -1, 0xB4}
};
const Redirect lunarRenderRedirects[] = {
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
const Redirect lunarNetworkRedirects[] = {
    {"sendPacket", "(Lnet/minecraft/network/Packet;)V",
     "net/minecraft/network/NetworkManager", "dispatchPacket",
     "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V",
     "packetOut", "(Lnet/minecraft/network/NetworkManager;Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V", -1},
    {"sendPacket", "(Lnet/minecraft/network/Packet;Lio/netty/util/concurrent/GenericFutureListener;[Lio/netty/util/concurrent/GenericFutureListener;)V",
     "net/minecraft/network/NetworkManager", "dispatchPacket",
     "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V",
     "packetOut", "(Lnet/minecraft/network/NetworkManager;Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V", -1},
    {"flushOutboundQueue", "()V",
     "net/minecraft/network/NetworkManager", "dispatchPacket",
     "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V",
     "packetOut", "(Lnet/minecraft/network/NetworkManager;Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V", -1},
    {"channelRead0", "(Lio/netty/channel/ChannelHandlerContext;Lnet/minecraft/network/Packet;)V",
     "net/minecraft/network/Packet", "processPacket", "(Lnet/minecraft/network/INetHandler;)V",
     "packetIn", "(Lnet/minecraft/network/Packet;Lnet/minecraft/network/INetHandler;)V", -1}
};
const Redirect lunarVelocityRedirects[] = {
    {"handleEntityVelocity", "(Lnet/minecraft/network/play/server/S12PacketEntityVelocity;)V",
     "net/minecraft/entity/Entity", "setVelocity", "(DDD)V",
     "appliedVelocity", "(Lnet/minecraft/entity/Entity;DDD)V", -1}
};
const Guard lunarPacketTaskGuards[] = {
    {"run", "()V", "clientPacketTask", "(Ljava/lang/Object;)Z", 1}
};
const StackLoad lunarAttackTargetLoads[] = {{0x19, 2}};
const Append lunarControllerAppends[] = {
    {"attackEntity", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/entity/Entity;)V",
     "attackTarget", "(Ljava/lang/Object;)V", lunarAttackTargetLoads, 1, true}
};
const Redirect lunarSlotRedirects[] = {
    {"getCurrentItem", "()Lnet/minecraft/item/ItemStack;", "net/minecraft/entity/player/InventoryPlayer", "currentItem", "I",
     "spoofSlot", "(Lnet/minecraft/entity/player/InventoryPlayer;)I", -1, -1, 0xB4}
};
const Redirect lunarControllerSlotRedirects[] = {
    {"sendUseItem", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z", "net/minecraft/entity/player/InventoryPlayer", "currentItem", "I",
     "spoofSlot", "(Lnet/minecraft/entity/player/InventoryPlayer;)I", -1, -1, 0xB4},
    {"syncCurrentPlayItem", "()V", "net/minecraft/entity/player/InventoryPlayer", "currentItem", "I",
     "spoofSlot", "(Lnet/minecraft/entity/player/InventoryPlayer;)I", -1, -1, 0xB4}
};
const StackLoad lunarFovLoads[] = {{0x15, 2}};
const StackLoad lunarPickLoads[] = {{0x17, 1}};
const Append lunarFovAppends[] = {
    {"getMouseOver", "(F)V", "cursorPickBegin", "()V", nullptr, 0, true},
    {"getMouseOver", "(F)V", "cursorPickEnd", "(F)V", lunarPickLoads, 1, false, true},
    {"getFOVModifier", "(FZ)F", "keepFov", "(FZ)F", lunarFovLoads, 1, false, false, true}
};
const Redirect lunarFreeLookCameraRedirects[] = {
    {"*", "", "net/minecraft/client/entity/EntityPlayerSP", "setAngles", "(FF)V", "freeCamAngles", "(Lnet/minecraft/entity/Entity;FF)V", -1},
    {"orientCamera", "(F)V", "net/minecraft/entity/Entity", "rotationYaw", "F", "freeLookYaw", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"orientCamera", "(F)V", "net/minecraft/entity/Entity", "rotationPitch", "F", "freeLookPitch", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"orientCamera", "(F)V", "net/minecraft/entity/Entity", "prevRotationYaw", "F", "freeLookPreviousYaw", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"orientCamera", "(F)V", "net/minecraft/entity/Entity", "prevRotationPitch", "F", "freeLookPreviousPitch", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"updateCameraAndRender", "(FJ)V", "net/minecraft/client/Minecraft", "inGameHasFocus", "Z", "freeLookMouse", "(Lnet/minecraft/client/Minecraft;)Z", -1, 1, 0xB4},
};
const Redirect lunarFreeLookTerrainRedirects[] = {
    {"setupTerrain", "(Lnet/minecraft/entity/Entity;DLnet/minecraft/client/renderer/culling/ICamera;IZ)V", "net/minecraft/entity/Entity", "rotationYaw", "F", "freeLookYaw", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"setupTerrain", "(Lnet/minecraft/entity/Entity;DLnet/minecraft/client/renderer/culling/ICamera;IZ)V", "net/minecraft/entity/Entity", "rotationPitch", "F", "freeLookPitch", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"getViewVector", "(Lnet/minecraft/entity/Entity;D)Lorg/lwjgl/util/vector/Vector3f;", "net/minecraft/entity/Entity", "rotationYaw", "F", "freeLookYaw", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"getViewVector", "(Lnet/minecraft/entity/Entity;D)Lorg/lwjgl/util/vector/Vector3f;", "net/minecraft/entity/Entity", "rotationPitch", "F", "freeLookPitch", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"getViewVector", "(Lnet/minecraft/entity/Entity;D)Lorg/lwjgl/util/vector/Vector3f;", "net/minecraft/entity/Entity", "prevRotationYaw", "F", "freeLookPreviousYaw", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
    {"getViewVector", "(Lnet/minecraft/entity/Entity;D)Lorg/lwjgl/util/vector/Vector3f;", "net/minecraft/entity/Entity", "prevRotationPitch", "F", "freeLookPreviousPitch", "(Lnet/minecraft/entity/Entity;)F", -1, -1, 0xB4},
};
const StackLoad lunarFreeLookManagerLoads[] = {{0x19, 0}};
const Append lunarFreeLookManagerAppends[] = {
    {"cacheActiveRenderInfo", "(Lnet/minecraft/world/World;Lnet/minecraft/client/gui/FontRenderer;Lnet/minecraft/entity/Entity;Lnet/minecraft/entity/Entity;Lnet/minecraft/client/settings/GameSettings;F)V",
     "freeLookBillboard", "(Lnet/minecraft/client/renderer/entity/RenderManager;)V", lunarFreeLookManagerLoads, 1, false, true}
};
const StackLoad lunarFreeLookParticleLoads[] = {{0x15, 1}};
const Append lunarFreeLookParticleAppends[] = {
    {"updateRenderInfo", "(Lnet/minecraft/entity/player/EntityPlayer;Z)V", "freeLookParticles", "(Z)V", lunarFreeLookParticleLoads, 1, false, true}
};
const ClassEdits lunarEdits[] = {
    {"net/minecraft/client/renderer/RenderGlobal", lunarFreeLookTerrainRedirects, static_cast<int>(std::size(lunarFreeLookTerrainRedirects))},
    {"net/minecraft/client/renderer/entity/RenderManager", nullptr, 0, nullptr, 0, nullptr, 0, lunarFreeLookManagerAppends, 1},
    {"net/minecraft/client/renderer/ActiveRenderInfo", nullptr, 0, nullptr, 0, nullptr, 0, lunarFreeLookParticleAppends, 1},
    {"net/minecraft/network/PacketThreadUtil$1", nullptr, 0, nullptr, 0, lunarPacketTaskGuards, 1},
    {"net/minecraft/client/renderer/EntityRenderer", lunarFreeLookCameraRedirects, static_cast<int>(std::size(lunarFreeLookCameraRedirects)), nullptr, 0, nullptr, 0, lunarFovAppends, static_cast<int>(std::size(lunarFovAppends))},
    {"net/minecraft/entity/player/InventoryPlayer", lunarSlotRedirects, 1},
    {"net/minecraft/client/multiplayer/PlayerControllerMP", lunarControllerSlotRedirects, static_cast<int>(std::size(lunarControllerSlotRedirects)), nullptr, 0, nullptr, 0, lunarControllerAppends, 1},
    {"net/minecraft/client/network/NetHandlerPlayClient", lunarVelocityRedirects, 1},
    {"net/minecraft/entity/Entity", lunarEntityRedirects, 2, nullptr, 0, lunarEntityGuards, 1},
    {"net/minecraft/entity/EntityLivingBase", lunarLivingRedirects, 4, nullptr, 0, lunarLivingGuards, 1},
    {"net/minecraft/client/entity/EntityPlayerSP", lunarPlayerRedirects, static_cast<int>(std::size(lunarPlayerRedirects)),
     nullptr, 0, lunarPlayerGuards, 1, lunarPlayerAppends, 1},
    {"net/minecraft/client/Minecraft", lunarMinecraftRedirects, static_cast<int>(std::size(lunarMinecraftRedirects)), nullptr, 0, nullptr, 0, lunarMinecraftAppends, 2},
    {"net/minecraft/client/renderer/ItemRenderer", lunarBlockRenderRedirects, 2},
    {"net/minecraft/entity/player/EntityPlayer", lunarAttackRedirects, 4},
    {"net/minecraft/item/Item", lunarItemRedirects, 2},
    {"net/minecraft/client/renderer/entity/RendererLivingEntity", lunarRenderRedirects, 4},
    {"net/minecraft/network/NetworkManager", lunarNetworkRedirects, 4}
};
#elif defined(NOVA_CLASS)
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
NOVA_CLASS(PacketTask, "net.minecraft.network.PacketThreadUtil$1")
NOVA_CLASS(Hit, "net.minecraft.util.MovingObjectPosition")
NOVA_CLASS(HitType, "net.minecraft.util.MovingObjectPosition$MovingObjectType")
NOVA_CLASS(World, "net.minecraft.world.World")
NOVA_CLASS(BlockState, "net.minecraft.block.state.IBlockState")
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
NOVA_CLASS(VecInt, "net.minecraft.util.Vec3i")
NOVA_CLASS(Enum, "java.lang.Enum")
NOVA_CLASS(Villager, "net.minecraft.entity.passive.EntityVillager")
NOVA_CLASS(Monster, "net.minecraft.entity.monster.IMob")
NOVA_CLASS(Dragon, "net.minecraft.entity.boss.EntityDragon")
NOVA_CLASS(DragonPart, "net.minecraft.entity.boss.EntityDragonPart")
NOVA_CLASS(Animal, "net.minecraft.entity.passive.EntityAnimal")
NOVA_CLASS(Ambient, "net.minecraft.entity.passive.EntityAmbientCreature")
NOVA_CLASS(WaterAnimal, "net.minecraft.entity.passive.EntityWaterMob")
NOVA_CLASS(LocalPlayer, "net.minecraft.client.entity.EntityPlayerSP")
NOVA_CLASS(MovementInput, "net.minecraft.util.MovementInput")
NOVA_CLASS(NetworkManager, "net.minecraft.network.NetworkManager")
NOVA_CLASS(Packet, "net.minecraft.network.Packet")
NOVA_CLASS(NetHandler, "net.minecraft.client.network.NetHandlerPlayClient")
NOVA_CLASS(PacketDirection, "net.minecraft.network.EnumPacketDirection")
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
NOVA_CLASS(WorldNameable, "net.minecraft.world.IWorldNameable")
NOVA_CLASS(I18n, "net.minecraft.client.resources.I18n")
NOVA_CLASS(Object, "java.lang.Object")
NOVA_CLASS(BlockPlayerInfo, "net.minecraft.client.network.NetworkPlayerInfo")
NOVA_CLASS(BlockGameProfile, "com.mojang.authlib.GameProfile")
NOVA_CLASS(BlockScoreTeam, "net.minecraft.scoreboard.ScorePlayerTeam")
NOVA_CLASS(ClosePacket, "net.minecraft.network.play.client.C0DPacketCloseWindow")
NOVA_CLASS(WindowClickPacket, "net.minecraft.network.play.client.C0EPacketClickWindow")
NOVA_CLASS(LoginStartPacket, "net.minecraft.network.login.client.C00PacketLoginStart")
NOVA_CLASS(HandshakePacket, "net.minecraft.network.handshake.client.C00Handshake")
NOVA_CLASS(BlockNameRegistry, "net.minecraft.util.RegistryNamespaced")
NOVA_CLASS(BlockResourceLocation, "net.minecraft.util.ResourceLocation")
NOVA_METHOD(RegistryName, BlockNameRegistry, "getNameForObject", "(Ljava/lang/Object;)Ljava/lang/Object;")
NOVA_METHOD(ResourceString, BlockResourceLocation, "toString", "()Ljava/lang/String;")
NOVA_CLASS(InteractPacket, "net.minecraft.network.play.client.C02PacketUseEntity")
NOVA_CLASS(ActionPacket, "net.minecraft.network.play.client.C07PacketPlayerDigging")
NOVA_CLASS(KeepAliveReply, "net.minecraft.network.play.client.C00PacketKeepAlive")
NOVA_CLASS(TransactionReply, "net.minecraft.network.play.client.C0FPacketConfirmTransaction")
NOVA_METHOD(DigAction, ActionPacket, "getStatus", "()Lnet/minecraft/network/play/client/C07PacketPlayerDigging$Action;")

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
NOVA_METHOD(InLava, Entity, "isInLava", "()Z")
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
NOVA_FIELD(PacketOnGround, MovePacket, "onGround", "Z")
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
NOVA_FIELD(HitSide, Hit, "sideHit", "Lnet/minecraft/util/EnumFacing;")
NOVA_FIELD(MoveForward, MovementInput, "moveForward", "F")
NOVA_FIELD(MoveStrafe, MovementInput, "moveStrafe", "F")
NOVA_FIELD(MoveJump, MovementInput, "jump", "Z")
NOVA_FIELD(MoveSneak, MovementInput, "sneak", "Z")
NOVA_FIELD(HeadYaw, Living, "rotationYawHead", "F")
NOVA_FIELD(PreviousHeadYaw, Living, "prevRotationYawHead", "F")
NOVA_FIELD(NetworkDirection, NetworkManager, "direction", "Lnet/minecraft/network/EnumPacketDirection;")
NOVA_FIELD(TaskPacket, PacketTask, "val$packetIn", "Lnet/minecraft/network/Packet;")
NOVA_FIELD(TaskListener, PacketTask, "val$processor", "Lnet/minecraft/network/INetHandler;")
NOVA_STATIC_FIELD(Clientbound, PacketDirection, "CLIENTBOUND", "Lnet/minecraft/network/EnumPacketDirection;")
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
NOVA_METHOD(RenderFrame, Renderer, "updateCameraAndRender", "(FJ)V")
NOVA_METHOD(StartUseItem, Controller, "sendUseItem", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z")
NOVA_METHOD(StopUseItem, Controller, "onStoppedUsingItem", "(Lnet/minecraft/entity/player/EntityPlayer;)V")
NOVA_METHOD(InteractAction, InteractPacket, "getAction", "()Lnet/minecraft/network/play/client/C02PacketUseEntity$Action;")
NOVA_METHOD(HeldItem, Player, "getHeldItem", "()Lnet/minecraft/item/ItemStack;")
NOVA_METHOD(GetItem, Stack, "getItem", "()Lnet/minecraft/item/Item;")
NOVA_METHOD(AddEffect, Living, "addPotionEffect", "(Lnet/minecraft/potion/PotionEffect;)V")
NOVA_METHOD(RemoveEffect, Living, "removePotionEffect", "(I)V")
NOVA_METHOD(EffectCtor, Effect, "<init>", "(IIIZZ)V")
NOVA_METHOD(KeyCode, KeyBinding, "getKeyCode", "()I")
NOVA_STATIC_METHOD(KeyTick, KeyBinding, "onTick", "(I)V")
NOVA_STATIC_METHOD(KeyState, KeyBinding, "setKeyBindState", "(IZ)V")
NOVA_STATIC_METHOD(MouseButton, Mouse, "isButtonDown", "(I)Z")
NOVA_STATIC_METHOD(KeyboardKey, Keyboard, "isKeyDown", "(I)Z")
NOVA_FIELD(ChatVisibility, Settings, "chatVisibility", "Lnet/minecraft/entity/player/EntityPlayer$EnumChatVisibility;")
NOVA_STATIC_METHOD(MouseGrabbed, Mouse, "isGrabbed", "()Z")
NOVA_METHOD(HitBlockPos, Hit, "getBlockPos", "()Lnet/minecraft/util/BlockPos;")
NOVA_METHOD(GetBlockState, World, "getBlockState", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/block/state/IBlockState;")
NOVA_METHOD(GetBlock, BlockState, "getBlock", "()Lnet/minecraft/block/Block;")
NOVA_METHOD(ClickSlot, ContainerScreen, "handleMouseClick", "(Lnet/minecraft/inventory/Slot;III)V")
NOVA_METHOD(EyeHeight, Entity, "getEyeHeight", "()F")
NOVA_STATIC_METHOD(DebugFps, Minecraft, "getDebugFPS", "()I")
NOVA_METHOD(Pick, Renderer, "getMouseOver", "(F)V")
NOVA_METHOD(PosRotCtor, PosRotPacket, "<init>", "(DDDFFZ)V")
NOVA_METHOD(IsSpectator, Player, "isSpectator", "()Z")
NOVA_METHOD(IsPickable, Entity, "canBeCollidedWith", "()Z")
NOVA_METHOD(IsAlive, Entity, "isEntityAlive", "()Z")
NOVA_METHOD(IsInvisible, Entity, "isInvisible", "()Z")
NOVA_METHOD(EntityId, Entity, "getEntityId", "()I")
NOVA_METHOD(EyePosition, Entity, "getPositionEyes", "(F)Lnet/minecraft/util/Vec3;")
NOVA_METHOD(PickRadius, Entity, "getCollisionBorderSize", "()F")
NOVA_METHOD(BoundingBox, Entity, "getEntityBoundingBox", "()Lnet/minecraft/util/AxisAlignedBB;")
NOVA_METHOD(DisplayName, Entity, "getDisplayName", "()Lnet/minecraft/util/IChatComponent;")
NOVA_METHOD(BlockUuid, Entity, "getUniqueID", "()Ljava/util/UUID;")
NOVA_METHOD(BlockPlayerLookup, NetHandler, "getPlayerInfo", "(Ljava/util/UUID;)Lnet/minecraft/client/network/NetworkPlayerInfo;")
NOVA_METHOD(BlockPlayerTeam, BlockPlayerInfo, "getPlayerTeam", "()Lnet/minecraft/scoreboard/ScorePlayerTeam;")
NOVA_METHOD(BlockProfile, BlockPlayerInfo, "getGameProfile", "()Lcom/mojang/authlib/GameProfile;")
NOVA_METHOD(BlockProfileName, BlockGameProfile, "getName", "()Ljava/lang/String;")
NOVA_STATIC_METHOD(BlockFormatName, BlockScoreTeam, "formatPlayerName", "(Lnet/minecraft/scoreboard/Team;Ljava/lang/String;)Ljava/lang/String;")
NOVA_METHOD(GetName, Entity, "getName", "()Ljava/lang/String;")
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
NOVA_METHOD(BlockX, VecInt, "getX", "()I")
NOVA_METHOD(BlockY, VecInt, "getY", "()I")
NOVA_METHOD(BlockZ, VecInt, "getZ", "()I")
NOVA_METHOD(RayTrace, World, "rayTraceBlocks", "(Lnet/minecraft/util/Vec3;Lnet/minecraft/util/Vec3;ZZZ)Lnet/minecraft/util/MovingObjectPosition;")
NOVA_METHOD(EntityHitCtor, Hit, "<init>", "(Lnet/minecraft/entity/Entity;Lnet/minecraft/util/Vec3;)V")
NOVA_METHOD(BoxIntercept, Box, "calculateIntercept", "(Lnet/minecraft/util/Vec3;Lnet/minecraft/util/Vec3;)Lnet/minecraft/util/MovingObjectPosition;")
NOVA_METHOD(LookVector, Entity, "getVectorForRotation", "(FF)Lnet/minecraft/util/Vec3;")
NOVA_METHOD(UpdateInput, MovementInput, "updatePlayerMoveState", "()V")
NOVA_METHOD(WalkingUpdate, LocalPlayer, "onUpdateWalkingPlayer", "()V")
NOVA_METHOD(CurrentNetHandler, Minecraft, "getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;")
NOVA_METHOD(IsClientThread, Minecraft, "isCallingFromMinecraftThread", "()Z")
NOVA_METHOD(ScheduleTask, Minecraft, "addScheduledTask", "(Ljava/lang/Runnable;)Lcom/google/common/util/concurrent/ListenableFuture;")
NOVA_METHOD(PacketTaskCtor, PacketTask, "<init>", "(Lnet/minecraft/network/Packet;Lnet/minecraft/network/INetHandler;)V")
NOVA_METHOD(GetNetworkManager, NetHandler, "getNetworkManager", "()Lnet/minecraft/network/NetworkManager;")
NOVA_METHOD(SendPacket, NetHandler, "addToSendQueue", "(Lnet/minecraft/network/Packet;)V")
NOVA_METHOD(DispatchPacket, NetworkManager, "dispatchPacket", "(Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V")
NOVA_METHOD(ProcessPacket, Packet, "processPacket", "(Lnet/minecraft/network/INetHandler;)V")
NOVA_METHOD(StatusPacketCtor, MovePacket, "<init>", "(Z)V")
NOVA_METHOD(UseDirection, UsePacket, "getPlacedBlockDirection", "()I")
NOVA_METHOD(VelocityId, VelocityPacket, "getEntityID", "()I")
NOVA_METHOD(VelocityX, VelocityPacket, "getMotionX", "()I")
NOVA_METHOD(VelocityY, VelocityPacket, "getMotionY", "()I")
NOVA_METHOD(VelocityZ, VelocityPacket, "getMotionZ", "()I")
NOVA_METHOD(Attack, Controller, "attackEntity", "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/entity/Entity;)V")
NOVA_METHOD(Swing, Living, "swingItem", "()V")
NOVA_METHOD(SetSprinting, Entity, "setSprinting", "(Z)V")
NOVA_METHOD(IsSprinting, Entity, "isSprinting", "()Z")
NOVA_METHOD(MoveSpeed, Living, "getAIMoveSpeed", "()F")
NOVA_METHOD(KeyDown, KeyBinding, "isKeyDown", "()Z")
NOVA_METHOD(VelocityCtor, VelocityPacket, "<init>", "(IDDD)V")
NOVA_METHOD(UsePacketCtor, UsePacket, "<init>", "(Lnet/minecraft/item/ItemStack;)V")
NOVA_METHOD(MenuGetSlot, Container, "getSlot", "(I)Lnet/minecraft/inventory/Slot;")
NOVA_METHOD(SlotStack, Slot, "getStack", "()Lnet/minecraft/item/ItemStack;")
NOVA_METHOD(InventoryTitle, WorldNameable, "getDisplayName", "()Lnet/minecraft/util/IChatComponent;")
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
NOVA_STATIC_FIELD(BlockRegistry, Block, "blockRegistry", "Lnet/minecraft/util/RegistryNamespacedDefaultedByKey;")
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
NOVA_STATIC_FIELD(Block_water, Blocks, "water", "Lnet/minecraft/block/BlockStaticLiquid;")
NOVA_STATIC_FIELD(Block_lava, Blocks, "lava", "Lnet/minecraft/block/BlockStaticLiquid;")
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
NOVA_STATIC_FIELD(Block_cactus, Blocks, "cactus", "Lnet/minecraft/block/BlockCactus;")
NOVA_STATIC_FIELD(Block_dispenser, Blocks, "dispenser", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_dropper, Blocks, "dropper", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_crafting_table, Blocks, "crafting_table", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_pumpkin, Blocks, "pumpkin", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_cobblestone_wall, Blocks, "cobblestone_wall", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_oak_fence, Blocks, "oak_fence", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_redstone_torch, Blocks, "redstone_torch", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_flower_pot, Blocks, "flower_pot", "Lnet/minecraft/block/Block;")
NOVA_STATIC_FIELD(Block_flowing_water, Blocks, "flowing_water", "Lnet/minecraft/block/BlockDynamicLiquid;")
NOVA_STATIC_FIELD(Block_flowing_lava, Blocks, "flowing_lava", "Lnet/minecraft/block/BlockDynamicLiquid;")
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
NOVA_STATIC_FIELD(Material_LEATHER, ArmorMaterial, "LEATHER", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(Material_CHAIN, ArmorMaterial, "CHAIN", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(Material_IRON, ArmorMaterial, "IRON", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(Material_GOLD, ArmorMaterial, "GOLD", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(Material_DIAMOND, ArmorMaterial, "DIAMOND", "Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_STATIC_FIELD(CreatureUndefined, CreatureAttribute, "UNDEFINED", "Lnet/minecraft/entity/EnumCreatureAttribute;")
NOVA_FIELD(StackCount, Stack, "stackSize", "I")
NOVA_FIELD(SwordAttackDamage, Sword, "attackDamage", "F")
NOVA_FIELD(ArmorSlot, Armor, "armorType", "I")
NOVA_FIELD(EnchantId, Enchantment, "effectId", "I")
NOVA_METHOD(StackName, Stack, "getDisplayName", "()Ljava/lang/String;")
NOVA_METHOD(StackDamage, Stack, "getItemDamage", "()I")
NOVA_METHOD(StackMaxDamage, Stack, "getMaxDamage", "()I")
NOVA_METHOD(StackMaxCount, Stack, "getMaxStackSize", "()I")
NOVA_METHOD(StackSpeed, Stack, "getStrVsBlock", "(Lnet/minecraft/block/Block;)F")
NOVA_METHOD(GetArmorMaterial, Armor, "getArmorMaterial", "()Lnet/minecraft/item/ItemArmor$ArmorMaterial;")
NOVA_METHOD(ItemBlock, BlockItem, "getBlock", "()Lnet/minecraft/block/Block;")
NOVA_METHOD(SharpnessDamage, Enchantment, "calcDamageByCreature", "(ILnet/minecraft/entity/EnumCreatureAttribute;)F")
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
NOVA_STATIC_FIELD(ProjectionBuffer, ActiveRenderInfo, "PROJECTION", "Ljava/nio/FloatBuffer;")
NOVA_STATIC_FIELD(ModelViewBuffer, ActiveRenderInfo, "MODELVIEW", "Ljava/nio/FloatBuffer;")
NOVA_FIELD(LoadedTiles, World, "loadedTileEntityList", "Ljava/util/List;")
NOVA_METHOD(BufferFloat, FloatBuffer, "get", "(I)F")
NOVA_METHOD(ViewEntity, Minecraft, "getRenderViewEntity", "()Lnet/minecraft/entity/Entity;")
NOVA_METHOD(TilePosition, TileEntity, "getPos", "()Lnet/minecraft/util/BlockPos;")
NOVA_METHOD(BlockBounds, Block, "setBlockBoundsBasedOnState", "(Lnet/minecraft/world/IBlockAccess;Lnet/minecraft/util/BlockPos;)V")
NOVA_METHOD(SelectedBox, Block, "getSelectedBoundingBox", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;)Lnet/minecraft/util/AxisAlignedBB;")
NOVA_METHOD(BlockActionPosition, BlockActionPacket, "getBlockPosition", "()Lnet/minecraft/util/BlockPos;")
NOVA_METHOD(BlockActionType, BlockActionPacket, "getBlockType", "()Lnet/minecraft/block/Block;")
NOVA_METHOD(BlockActionId, BlockActionPacket, "getData1", "()I")
NOVA_METHOD(BlockActionParameter, BlockActionPacket, "getData2", "()I")
NOVA_FIELD(DisplayWidth, Minecraft, "displayWidth", "I")
NOVA_FIELD(DisplayHeight, Minecraft, "displayHeight", "I")
NOVA_FIELD(Provider, World, "provider", "Lnet/minecraft/world/WorldProvider;")
NOVA_METHOD(ScaledResolutionCtor, ScaledResolution, "<init>", "(Lnet/minecraft/client/Minecraft;)V")
NOVA_METHOD(ScaleFactor, ScaledResolution, "getScaleFactor", "()I")
NOVA_METHOD(SurfaceWorld, WorldProvider, "isSurfaceWorld", "()Z")
NOVA_METHOD(SpawnPoint, World, "getSpawnPoint", "()Lnet/minecraft/util/BlockPos;")
NOVA_METHOD(SkinLocation, ClientPlayer, "getLocationSkin", "()Lnet/minecraft/util/ResourceLocation;")
NOVA_METHOD(GetTextureManager, Minecraft, "getTextureManager", "()Lnet/minecraft/client/renderer/texture/TextureManager;")
NOVA_METHOD(GetTexture, TextureManager, "getTexture", "(Lnet/minecraft/util/ResourceLocation;)Lnet/minecraft/client/renderer/texture/ITextureObject;")
NOVA_METHOD(TextureId, TextureObject, "getGlTextureId", "()I")
NOVA_METHOD(ScoreOwner, ScorePacket, "getPlayerName", "()Ljava/lang/String;")
NOVA_METHOD(ScoreObjective, ScorePacket, "getObjectiveName", "()Ljava/lang/String;")
NOVA_METHOD(ScoreValue, ScorePacket, "getScoreValue", "()I")
NOVA_METHOD(ScoreAction, ScorePacket, "getScoreAction", "()Lnet/minecraft/network/play/server/S3CPacketUpdateScore$Action;")
// Placement
NOVA_CLASS(BlockPos, "net.minecraft.util.BlockPos")
NOVA_CLASS(Facing, "net.minecraft.util.EnumFacing")
NOVA_CLASS(InventoryInterface, "net.minecraft.inventory.IInventory")
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
NOVA_STATIC_FIELD(FacingWest, Facing, "WEST", "Lnet/minecraft/util/EnumFacing;")
NOVA_STATIC_FIELD(FacingEast, Facing, "EAST", "Lnet/minecraft/util/EnumFacing;")
NOVA_METHOD(BlockPosCtor, BlockPos, "<init>", "(III)V")
NOVA_METHOD(CollisionBox, Block, "getCollisionBoundingBox", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;Lnet/minecraft/block/state/IBlockState;)Lnet/minecraft/util/AxisAlignedBB;")
NOVA_METHOD(Replaceable, Block, "isReplaceable", "(Lnet/minecraft/world/World;Lnet/minecraft/util/BlockPos;)Z")
NOVA_METHOD(GetTileEntity, World, "getTileEntity", "(Lnet/minecraft/util/BlockPos;)Lnet/minecraft/tileentity/TileEntity;")
NOVA_METHOD(FacingX, Facing, "getFrontOffsetX", "()I")
NOVA_METHOD(FacingY, Facing, "getFrontOffsetY", "()I")
NOVA_METHOD(FacingZ, Facing, "getFrontOffsetZ", "()I")
NOVA_METHOD(FacingOpposite, Facing, "getOpposite", "()Lnet/minecraft/util/EnumFacing;")
NOVA_METHOD(UseBlock, Controller, "onPlayerRightClick", "(Lnet/minecraft/client/entity/EntityPlayerSP;Lnet/minecraft/client/multiplayer/WorldClient;Lnet/minecraft/item/ItemStack;Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;Lnet/minecraft/util/Vec3;)Z")
NOVA_METHOD(RotationPacketCtor, RotationPacket, "<init>", "(FFZ)V")
NOVA_METHOD(SwingPacketCtor, SwingPacket, "<init>", "()V")
// Leader Hypixel AutoBlock immediate packet / local-use operations.
NOVA_FIELD(ReportedYaw, LocalPlayer, "lastReportedYaw", "F")
NOVA_FIELD(ReportedPitch, LocalPlayer, "lastReportedPitch", "F")
NOVA_CLASS(DigActionEnum, "net.minecraft.network.play.client.C07PacketPlayerDigging$Action")
NOVA_CLASS(InteractActionEnum, "net.minecraft.network.play.client.C02PacketUseEntity$Action")
NOVA_CLASS(ChatPacket, "net.minecraft.network.play.client.C01PacketChatMessage")
NOVA_STATIC_FIELD(ReleaseItemAction, DigActionEnum, "RELEASE_USE_ITEM", "Lnet/minecraft/network/play/client/C07PacketPlayerDigging$Action;")
NOVA_STATIC_FIELD(InteractEntityAction, InteractActionEnum, "INTERACT", "Lnet/minecraft/network/play/client/C02PacketUseEntity$Action;")
NOVA_METHOD(HeldSlotCtor, HeldSlotPacket, "<init>", "(I)V")
NOVA_METHOD(DigPacketCtor, ActionPacket, "<init>", "(Lnet/minecraft/network/play/client/C07PacketPlayerDigging$Action;Lnet/minecraft/util/BlockPos;Lnet/minecraft/util/EnumFacing;)V")
NOVA_METHOD(InteractAtCtor, InteractPacket, "<init>", "(Lnet/minecraft/entity/Entity;Lnet/minecraft/util/Vec3;)V")
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
NOVA_FIELD(StatusEntityId, EntityStatusPacket, "entityId", "I")
NOVA_METHOD(StatusOpcode, EntityStatusPacket, "getOpCode", "()B")
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
#else
#error "Select a Lunar189 mapping section before including this file"
#endif
