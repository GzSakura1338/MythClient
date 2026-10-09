package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.multiplayer.ClientLevel;
import net.minecraft.core.BlockPos;
import net.minecraft.core.particles.ExplosionParticleInfo;
import net.minecraft.core.particles.ParticleOptions;
import net.minecraft.util.random.WeightedList;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Every client particle funnels through doAddParticle, so one hook covers Mio's Potions,
 * Explosions and the ParticleTypes set at once.
 */
@Mixin(ClientLevel.class)
public abstract class MixinClientLevel {
    @Inject(method = "doAddParticle", at = @At("HEAD"), cancellable = true)
    private void myth$particles(ParticleOptions particle, boolean overrideLimiter, boolean alwaysShowParticles,
                                double x, double y, double z, double xd, double yd, double zd, CallbackInfo ci) {
        if (NoRender.hidesParticleType(particle)) ci.cancel();
    }

    /**
     * Mio only had to drop ExplosionLargeParticle and ExplosionSmokeParticle. 26.2 sends the smoke
     * and debris cloud as a server-driven weighted list instead, and ClientExplosionTracker spawns
     * it a tick later as plain block particles, so the type filter above never sees it. That cloud
     * is most of what an explosion looks like, which is why filtering only the flash looks broken.
     */
    @Inject(method = "trackExplosionEffects", at = @At("HEAD"), cancellable = true)
    private void myth$explosionCloud(Vec3 center, float radius, int blockCount,
                                     WeightedList<ExplosionParticleInfo> blockParticles, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.explosions.get()) ci.cancel();
    }

    @Inject(method = "addDestroyBlockEffect", at = @At("HEAD"), cancellable = true)
    private void myth$blockBreakingParticles(BlockPos pos, BlockState blockState, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.hidesBreakingParticles()) ci.cancel();
    }
}
