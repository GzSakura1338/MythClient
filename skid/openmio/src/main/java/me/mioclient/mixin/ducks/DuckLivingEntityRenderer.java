package me.mioclient.mixin.ducks;

import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.LivingEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

/* compiled from: 0.java */
@Mixin({LivingEntityRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckLivingEntityRenderer.class */
public interface DuckLivingEntityRenderer<T extends LivingEntity, M extends EntityModel<T>> {
    @Invoker("setupTransforms")
    void mio$setupTransforms(T t, MatrixStack matrixStack, float f, float f2, float f3, float f4);

    @Invoker("scale")
    void mio$scale(T t, MatrixStack matrixStack, float f);

    @Invoker("getAnimationProgress")
    float mio$getAnimationProgress(T t, float f);
}
