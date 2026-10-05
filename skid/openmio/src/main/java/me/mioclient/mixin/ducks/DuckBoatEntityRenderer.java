package me.mioclient.mixin.ducks;

import com.mojang.datafixers.util.Pair;
import java.util.Map;
import net.minecraft.client.render.entity.BoatEntityRenderer;
import net.minecraft.client.render.entity.model.CompositeEntityModel;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.util.Identifier;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({BoatEntityRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckBoatEntityRenderer.class */
public interface DuckBoatEntityRenderer {
    @Accessor("texturesAndModels")
    Map<BoatEntity.Type, Pair<Identifier, CompositeEntityModel<BoatEntity>>> mio$getTexturesAndModels();
}
