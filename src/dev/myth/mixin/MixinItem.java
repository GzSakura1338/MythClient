package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.myth.Myth;
import dev.myth.event.impl.UseItemRayTraceEvent;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.item.Item;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

@Mixin(Item.class)
public abstract class MixinItem {
    @WrapOperation(method = "getPlayerPOVHitResult", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/player/Player;calculateViewVector(FF)Lnet/minecraft/world/phys/Vec3;"))
    private static Vec3 myth$useItemRayTrace(Player player, float pitch, float yaw, Operation<Vec3> original) {
        if (player == Minecraft.getInstance().player) {
            UseItemRayTraceEvent event = Myth.EVENT_MANAGER.call(new UseItemRayTraceEvent(yaw, pitch));
            return original.call(player, event.getPitch(), event.getYaw());
        }
        return original.call(player, pitch, yaw);
    }
}
