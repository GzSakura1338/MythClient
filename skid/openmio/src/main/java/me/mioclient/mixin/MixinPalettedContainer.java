package me.mioclient.mixin;

import me.mioclient.Helper_11;
import me.mioclient.MixinPalettedContainerHelper;
import net.minecraft.world.chunk.PalettedContainer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;

/* compiled from: 0.java */
@Mixin({PalettedContainer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinPalettedContainer.class */
public class MixinPalettedContainer<T> implements Helper_11<T> {

    @Shadow
    private volatile PalettedContainer.Data<T> field_34560;

    @Override // me.mioclient.Helper_11
    public MixinPalettedContainerHelper<T> mio$data() {
        return (MixinPalettedContainerHelper<T>)(Object) this.field_34560;
    }
}
