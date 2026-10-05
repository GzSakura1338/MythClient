package today.starain.mixin;

import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.client.renderer.entity.EntityRenderer;
import net.minecraft.network.chat.Component;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.player.Player;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.module.impl.render.NameTags;

@Mixin(EntityRenderer.class)
public class MixinEntityRenderer {

    @Inject(method = "renderNameTag", at = @At("HEAD"), cancellable = true)
    private void starain$hideVanillaName(Entity entity, Component displayName, PoseStack poseStack, MultiBufferSource buffer, int packedLight, CallbackInfo ci) {
        // 只隐藏 NameTags 会自绘的对象（非自己的玩家）的原版头顶名，避免和自绘名牌重叠。
        if (!(entity instanceof Player) || entity == Minecraft.getInstance().player) {
            return;
        }
        if (Starain.getInstance() == null || Starain.getInstance().getModuleManager() == null) {
            return;
        }
        NameTags nameTags = Starain.getInstance().getModuleManager().getModule(NameTags.class);
        if (nameTags != null && nameTags.isEnabled()) {
            ci.cancel();
        }
    }
}
