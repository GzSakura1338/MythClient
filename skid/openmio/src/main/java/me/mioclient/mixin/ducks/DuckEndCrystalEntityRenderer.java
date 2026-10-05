package me.mioclient.mixin.ducks;

import net.minecraft.client.model.ModelPart;
import net.minecraft.client.render.entity.EndCrystalEntityRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({EndCrystalEntityRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckEndCrystalEntityRenderer.class */
public interface DuckEndCrystalEntityRenderer {
    @Accessor("core")
    ModelPart mio$getCore();

    @Accessor("bottom")
    ModelPart mio$getBottom();

    @Accessor("frame")
    ModelPart mio$getFrame();
}
