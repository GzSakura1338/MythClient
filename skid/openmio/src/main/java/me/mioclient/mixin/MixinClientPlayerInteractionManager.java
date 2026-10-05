package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.v2.WrapWithCondition;
import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.BreakingProgressHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.AttackBlockEvent;
import me.mioclient.event.BreakBlockEvent;
import me.mioclient.event.InteractBlockEvent;
import me.mioclient.event.InteractItemEvent;
import me.mioclient.event.InteractItemEvent_2;
import me.mioclient.event.StopUsingItemEvent;
import me.mioclient.module.combat.AutoClicker;
import me.mioclient.module.player.InventoryTweaks;
import me.mioclient.module.player.NoInteract;
import me.mioclient.module.player.SpeedMine;
import net.minecraft.client.network.ClientPlayNetworkHandler;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.client.network.ClientPlayerInteractionManager;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.c2s.play.ClientCommandC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractItemC2SPacket;
import net.minecraft.screen.PlayerScreenHandler;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.slot.Slot;
import net.minecraft.screen.slot.SlotActionType;
import net.minecraft.util.ActionResult;
import net.minecraft.util.Hand;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({ClientPlayerInteractionManager.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinClientPlayerInteractionManager.class */
public abstract class MixinClientPlayerInteractionManager implements SearchHelper_4, BreakingProgressHelper {
    private static final AutoClicker autoclicker = (AutoClicker) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoClicker.class);
    private static final NoInteract nointeract = (NoInteract) BaritoneHelper_3.baritoneHelper_4.getModule117(NoInteract.class);
    private static final InventoryTweaks inventorytweaks = (InventoryTweaks) BaritoneHelper_3.baritoneHelper_4.getModule117(InventoryTweaks.class);
    private static SpeedMine speedmine = (SpeedMine) BaritoneHelper_3.baritoneHelper_4.getModule117(SpeedMine.class);

    @Shadow
    private boolean field_3717;

    @Shadow
    private float field_3715;

    @Shadow
    private BlockPos field_3714;

    @Shadow
    private int field_3716;

    @Shadow
    public abstract void method_2906(int i, int i2, int i3, SlotActionType slotActionType, PlayerEntity playerEntity);

    @Inject(method = {"breakBlock"}, at = {@At("HEAD")}, cancellable = true)
    private void breakBlock(BlockPos blockPos, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        BreakBlockEvent breakBlockEvent = new BreakBlockEvent(blockPos);
        baritoneHelper.getObject1794(breakBlockEvent);
        if (breakBlockEvent.is2403()) {
            callbackInfoReturnable.setReturnValue(false);
            callbackInfoReturnable.cancel();
        }
    }

    @Inject(method = {"attackBlock"}, at = {@At("HEAD")}, cancellable = true)
    private void attackBlockHook(BlockPos blockPos, Direction direction, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        AttackBlockEvent attackBlockEvent = new AttackBlockEvent(blockPos, direction);
        baritoneHelper.getObject1794(attackBlockEvent);
        if (attackBlockEvent.is2403()) {
            callbackInfoReturnable.setReturnValue(true);
            callbackInfoReturnable.cancel();
        }
    }

    @Inject(method = {"clickSlot"}, at = {@At("HEAD")}, cancellable = true)
    public void onClickArmorSlot(int i, int i2, int i3, SlotActionType slotActionType, PlayerEntity playerEntity, CallbackInfo callbackInfo) {
        if (inventorytweaks.isToggled()) {
            ScreenHandler screenHandler = playerEntity.currentScreenHandler;
            if (inventorytweaks.fastArmor.getValue().booleanValue() && i3 == 1 && slotActionType == SlotActionType.QUICK_MOVE) {
                Slot slot = screenHandler.getSlot(i2);
                if ((slot.getStack().getItem() instanceof ArmorItem) || slot.getStack().isOf(Items.ELYTRA)) {
                    FireworksHelper.do441(i2, 8 - minecraftClient.player.getPreferredEquipmentSlot(slot.getStack()).getEntitySlotId());
                    callbackInfo.cancel();
                    return;
                }
            }
            if (inventorytweaks.xCarryTweaks.getValue().booleanValue() && i3 == 1 && slotActionType == SlotActionType.QUICK_MOVE && i2 > 4 && (screenHandler instanceof PlayerScreenHandler)) {
                for (int i4 = 0; i4 < 4; i4++) {
                    if (((ItemStack) minecraftClient.player.playerScreenHandler.getCraftingInput().getHeldStacks().get(i4)).isEmpty()) {
                        method_2906(i, i2, 0, SlotActionType.PICKUP, minecraftClient.player);
                        method_2906(i, i4 + 1, 0, SlotActionType.PICKUP, minecraftClient.player);
                        callbackInfo.cancel();
                        return;
                    }
                }
            }
        }
    }

    @Inject(method = {"interactBlock"}, at = {@At("HEAD")}, cancellable = true)
    private void interactBlockHook(ClientPlayerEntity clientPlayerEntity, Hand hand, BlockHitResult blockHitResult, CallbackInfoReturnable<ActionResult> callbackInfoReturnable) {
        InteractBlockEvent interactBlockEvent = new InteractBlockEvent(blockHitResult, hand);
        baritoneHelper.getObject1794(interactBlockEvent);
        if (interactBlockEvent.is2403()) {
            callbackInfoReturnable.setReturnValue(ActionResult.FAIL);
            callbackInfoReturnable.cancel();
        }
        Hand hand1841 = NoInteract.getHand1841();
        if (nointeract.is1842(minecraftClient.player.getStackInHand(NoInteract.getHand1841()), blockHitResult.getBlockPos())) {
            if (nointeract.mode.getValue() == NoInteract.Mode.SHIFT) {
                nointeract.flag = !BaritoneHelper_3.holeSnapSearchHelper4_4.is2625();
                AutoSignSearchHelper4.do2567(clientPlayerEntity, ClientCommandC2SPacket.Mode.PRESS_SHIFT_KEY, 0);
            } else {
                callbackInfoReturnable.setReturnValue(ActionResult.FAIL);
                callbackInfoReturnable.cancel();
            }
            minecraftClient.interactionManager.interactItem(minecraftClient.player, hand1841);
        }
    }

    @Inject(method = {"interactBlock"}, at = {@At("RETURN")})
    private void interactBlockHook2(ClientPlayerEntity clientPlayerEntity, Hand hand, BlockHitResult blockHitResult, CallbackInfoReturnable<ActionResult> callbackInfoReturnable) {
        if (nointeract.flag) {
            AutoSignSearchHelper4.do2567(clientPlayerEntity, ClientCommandC2SPacket.Mode.RELEASE_SHIFT_KEY, 0);
            nointeract.flag = false;
        }
    }

    @Inject(method = {"stopUsingItem"}, at = {@At("HEAD")}, cancellable = true)
    private void stopUsingItemHook(PlayerEntity playerEntity, CallbackInfo callbackInfo) {
        StopUsingItemEvent stopUsingItemEvent = new StopUsingItemEvent();
        baritoneHelper.getObject1794(stopUsingItemEvent);
        if (stopUsingItemEvent.is2403()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"interactItem"}, at = {@At("HEAD")}, cancellable = true)
    private void interactItemHook(PlayerEntity playerEntity, Hand hand, CallbackInfoReturnable<ActionResult> callbackInfoReturnable) {
        InteractItemEvent interactItemEvent = new InteractItemEvent(hand);
        baritoneHelper.getObject1794(interactItemEvent);
        if (interactItemEvent.is2403()) {
            callbackInfoReturnable.cancel();
            callbackInfoReturnable.setReturnValue(ActionResult.FAIL);
        }
    }

    @Inject(method = {"attackEntity"}, at = {@At("HEAD")}, cancellable = true)
    private void attackEntityHook(PlayerEntity playerEntity, Entity entity, CallbackInfo callbackInfo) {
        if (autoclicker.is1073(entity) && playerEntity == minecraftClient.player) {
            callbackInfo.cancel();
        }
    }

    @WrapWithCondition(method = {"cancelBlockBreaking"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayNetworkHandler;sendPacket(Lnet/minecraft/network/packet/Packet;)V")})
    private boolean cancelBlockBreaking(ClientPlayNetworkHandler clientPlayNetworkHandler, Packet packet) {
        return !speedmine.isToggled();
    }

    @Inject(method = {"updateBlockBreakingProgress"}, at = {@At("HEAD")})
    private void attackBlock(BlockPos blockPos, Direction direction, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (speedmine.isToggled()) {
            this.field_3716 = 0;
        }
    }

    @ModifyExpressionValue(method = {"method_41929"}, at = {@At(value = "NEW", target = "(Lnet/minecraft/util/Hand;IFF)Lnet/minecraft/network/packet/c2s/play/PlayerInteractItemC2SPacket;")})
    private PlayerInteractItemC2SPacket interactItemHook(PlayerInteractItemC2SPacket playerInteractItemC2SPacket) {
        InteractItemEvent_2 interactItemEvent_2 = new InteractItemEvent_2(playerInteractItemC2SPacket);
        baritoneHelper.getObject1794(interactItemEvent_2);
        return interactItemEvent_2.is2403() ? interactItemEvent_2.getPlayerInteractItemC2SPacket1816() : playerInteractItemC2SPacket;
    }

    @Override // me.mioclient.BreakingProgressHelper
    public boolean isBreakingBlock() {
        return this.field_3717;
    }

    @Override // me.mioclient.BreakingProgressHelper
    public void setBreakingBlock(boolean z) {
        this.field_3717 = z;
    }

    @Override // me.mioclient.BreakingProgressHelper
    public float getBreakingProgress() {
        return this.field_3715;
    }

    @Override // me.mioclient.BreakingProgressHelper
    public void setBreakingProgress(float f) {
        this.field_3715 = f;
    }

    @Override // me.mioclient.BreakingProgressHelper
    public BlockPos getCurrentBreakingBlock() {
        return this.field_3714;
    }

    @Override // me.mioclient.BreakingProgressHelper
    public void setCurrentBreakingBlock(BlockPos blockPos) {
        this.field_3714 = blockPos;
    }
}
