package me.mioclient.mixin;

import java.util.function.Supplier;
import me.mioclient.Helper_15;
import net.minecraft.text.Style;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;

/* compiled from: 0.java */
@Mixin({Style.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinStyle.class */
public class MixinStyle implements Helper_15 {
    @Override // me.mioclient.Helper_15
    @Unique
    public Style mio$withColor(Supplier<Integer> supplier) {
        Style style = (Style)(Object) this;
        if (style.getColor() != null) {
            ((me.mioclient.MixinStyleHelper)(Object) style.getColor()).setSupplier(supplier);
        }
        return style;
    }
}
