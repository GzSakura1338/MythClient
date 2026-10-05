package today.starain.mixin;

import net.minecraft.client.Minecraft;
import net.minecraft.client.multiplayer.ClientLevel;
import net.minecraft.world.entity.Entity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;
import today.starain.Starain;

@Mixin({ClientLevel.class})
public class MixinClientLevel {
    @Redirect(
            method = {"tickNonPassenger"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/world/entity/Entity;tick()V"
            )
    )
    public void hookSkipTicks(Entity instance) {
        if (Starain.skipTicks > 0 && instance == Minecraft.getInstance().player) {
            Starain.skipTicks--;
        } else {
            instance.tick();
        }
    }
}
