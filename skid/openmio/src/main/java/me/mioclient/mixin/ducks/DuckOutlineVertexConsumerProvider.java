package me.mioclient.mixin.ducks;

import net.minecraft.client.render.OutlineVertexConsumerProvider;
import net.minecraft.client.render.VertexConsumerProvider;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({OutlineVertexConsumerProvider.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckOutlineVertexConsumerProvider.class */
public interface DuckOutlineVertexConsumerProvider {
    @Accessor("plainDrawer")
    VertexConsumerProvider.Immediate getParent();
}
