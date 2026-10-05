package today.starain.mixin;

import net.minecraft.world.entity.player.Player;
import net.minecraft.world.item.Item;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;
import today.starain.Starain;
import today.starain.event.impl.UseItemRayTraceEvent;

@Mixin({Item.class})
public class MixinItem {
    @Redirect(
            method = {"getPlayerPOVHitResult"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/player/Player;getYRot()F"
            )
    )
    private static float hookRayTraceYRot(Player instance) {
        UseItemRayTraceEvent event = new UseItemRayTraceEvent(instance.getYRot(), instance.getXRot());
        Starain.getInstance().getEventManager().call(event);
        return event.getYaw();
    }

    @Redirect(
            method = {"getPlayerPOVHitResult"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/player/Player;getXRot()F"
            )
    )
    private static float hookRayTraceXRot(Player instance) {
        UseItemRayTraceEvent event = new UseItemRayTraceEvent(instance.getYRot(), instance.getXRot());
        Starain.getInstance().getEventManager().call(event);
        return event.getPitch();
    }
}
