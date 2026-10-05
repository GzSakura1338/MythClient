package me.mioclient.mixin.ducks;

import net.minecraft.block.AbstractBlock;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({AbstractBlock.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckAbstractBlock.class */
public interface DuckAbstractBlock {
    @Accessor("collidable")
    boolean isCollidable();
}
