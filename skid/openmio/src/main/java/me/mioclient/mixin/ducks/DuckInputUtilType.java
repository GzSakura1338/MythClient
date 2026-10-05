package me.mioclient.mixin.ducks;

import it.unimi.dsi.fastutil.ints.Int2ObjectMap;
import net.minecraft.client.util.InputUtil;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({InputUtil.Type.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckInputUtilType.class */
public interface DuckInputUtilType {
    @Accessor("map")
    Int2ObjectMap<InputUtil.Key> getKeyMap();
}
