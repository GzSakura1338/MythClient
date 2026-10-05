package me.mioclient.mixin;

import me.mioclient.MixinPalettedContainerHelper;
import net.minecraft.util.collection.PaletteStorage;
import net.minecraft.world.chunk.Palette;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;

/* compiled from: 0.java */
@Mixin(targets = {"net.minecraft.world.chunk.PalettedContainer$Data"})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinPalettedData.class */
public class MixinPalettedData<T> implements MixinPalettedContainerHelper<T> {

    @Shadow
    @Final
    PaletteStorage comp_118;

    @Shadow
    @Final
    Palette<T> comp_119;

    @Override // me.mioclient.MixinPalettedContainerHelper
    public PaletteStorage mio$storage() {
        return this.comp_118;
    }

    @Override // me.mioclient.MixinPalettedContainerHelper
    public Palette<T> mio$palette() {
        return this.comp_119;
    }
}
