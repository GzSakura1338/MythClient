package me.mioclient.mixin.ducks;

import net.minecraft.client.session.Session;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({Session.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckSession.class */
public interface DuckSession {
    @Accessor("username")
    @Mutable
    void setUsername(String str);
}
