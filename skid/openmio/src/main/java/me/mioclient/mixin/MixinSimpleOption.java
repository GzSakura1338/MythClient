package me.mioclient.mixin;

import me.mioclient.ZoomHelper_3;
import net.minecraft.client.option.SimpleOption;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;

/* compiled from: 0.java */
@Mixin({SimpleOption.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinSimpleOption.class */
public class MixinSimpleOption<T> implements ZoomHelper_3<T> {

    @Shadow
    T field_37868;

    @Override // me.mioclient.ZoomHelper_3
    public void forceSetValue(T t) {
        this.field_37868 = t;
    }
}
