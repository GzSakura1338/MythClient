package me.mioclient;

import net.minecraft.util.collection.PaletteStorage;
import net.minecraft.world.chunk.Palette;
import net.minecraft.world.chunk.PalettedContainer;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MixinPalettedContainerHelper.class */
public interface MixinPalettedContainerHelper<T> {
    PaletteStorage mio$storage();

    Palette<T> mio$palette();

    static PaletteStorage getPaletteStorage2721(PalettedContainer<?> palettedContainer) {
        return ((Helper_11) palettedContainer).mio$data().mio$storage();
    }

    static <T> Palette<T> getPalette2722(PalettedContainer<T> palettedContainer) {
        return ((Helper_11) palettedContainer).mio$data().mio$palette();
    }
}
