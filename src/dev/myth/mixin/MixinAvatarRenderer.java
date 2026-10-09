package dev.myth.mixin;

import dev.myth.model.render.Animations;
import net.minecraft.client.model.HumanoidModel;
import net.minecraft.client.renderer.entity.player.AvatarRenderer;
import net.minecraft.tags.ItemTags;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.Avatar;
import net.minecraft.world.item.ItemStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/**
 * Third person block pose for everyone else. On a 1.8 server ViaVersion does set the using-item
 * entity flag when a player blocks, so {@code isUsingItem()} and {@code getUsedItemHand()} are
 * correct for remote players - they read DATA_LIVING_ENTITY_FLAGS straight off the synced data.
 *
 * <p>Vanilla still shows nothing, for two reasons: getArmPose only looks at the pose once
 * {@code getUseItemRemainingTicks() > 0}, and the client derives those ticks from
 * {@code useItem.getUseDuration()}, which is 0 for a 26.2 sword; and even past that it keys on
 * {@code ItemUseAnimation.BLOCK}, which a sword does not have either. So match on the flag instead.
 */
@Mixin(AvatarRenderer.class)
public abstract class MixinAvatarRenderer {
    @Inject(method = "getArmPose(Lnet/minecraft/world/entity/Avatar;Lnet/minecraft/world/item/ItemStack;"
            + "Lnet/minecraft/world/InteractionHand;)Lnet/minecraft/client/model/HumanoidModel$ArmPose;",
            at = @At("HEAD"), cancellable = true)
    private static void myth$swordBlockPose(Avatar avatar, ItemStack itemInHand, InteractionHand hand,
                                            CallbackInfoReturnable<HumanoidModel.ArmPose> cir) {
        if (!Animations.showsOthersBlocking() || !itemInHand.is(ItemTags.SWORDS)) return;
        if (!avatar.isUsingItem() || avatar.getUsedItemHand() != hand) return;
        cir.setReturnValue(HumanoidModel.ArmPose.BLOCK);
    }
}
