package me.mioclient.mixin.ducks;

import net.minecraft.client.option.KeyBinding;
import net.minecraft.client.util.InputUtil;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({KeyBinding.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckKeyBinding.class */
public interface DuckKeyBinding {
    @Accessor("boundKey")
    InputUtil.Key getKey();
}
