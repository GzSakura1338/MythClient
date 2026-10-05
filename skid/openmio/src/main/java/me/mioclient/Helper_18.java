package me.mioclient;

import java.util.function.Function;
import net.minecraft.client.render.RenderLayer;
import net.minecraft.client.render.RenderPhase;
import net.minecraft.client.render.VertexFormat;
import net.minecraft.client.render.VertexFormats;
import net.minecraft.util.Identifier;
import net.minecraft.util.Util;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Helper_18.class */
public final class Helper_18 {
    public static final Function<Identifier, RenderLayer> function = Util.memoize(identifier -> {
        return getMultiPhase2030("armor_cutout_no_cull", identifier, false);
    });
    public static final Function<Identifier, RenderLayer> function2 = Util.memoize(identifier -> {
        return RenderLayer.of("entity_solid", VertexFormats.POSITION_COLOR_TEXTURE_OVERLAY_LIGHT_NORMAL, VertexFormat.DrawMode.QUADS, 1536, true, true, RenderLayer.MultiPhaseParameters.builder().program(RenderPhase.ENTITY_SOLID_PROGRAM).texture(new RenderPhase.Texture(identifier, false, false)).cull(RenderPhase.DISABLE_CULLING).transparency(RenderPhase.TRANSLUCENT_TRANSPARENCY).lightmap(RenderPhase.ENABLE_LIGHTMAP).overlay(RenderPhase.ENABLE_OVERLAY_COLOR).build(true));
    });
    public static final Function<Identifier, RenderLayer> function3 = Util.memoize(identifier -> {
        return RenderLayer.of("entity_cutout_no_cull", VertexFormats.POSITION_COLOR_TEXTURE_OVERLAY_LIGHT_NORMAL, VertexFormat.DrawMode.QUADS, 1536, true, false, RenderLayer.MultiPhaseParameters.builder().program(RenderPhase.ENTITY_CUTOUT_NONULL_PROGRAM).texture(new RenderPhase.Texture(identifier, false, false)).transparency(RenderPhase.TRANSLUCENT_TRANSPARENCY).cull(RenderPhase.DISABLE_CULLING).lightmap(RenderPhase.ENABLE_LIGHTMAP).overlay(RenderPhase.ENABLE_OVERLAY_COLOR).build(false));
    });

    public static RenderLayer.MultiPhase getMultiPhase2030(String str, Identifier identifier, boolean z) {
        return RenderLayer.of(str, VertexFormats.POSITION_COLOR_TEXTURE_OVERLAY_LIGHT_NORMAL, VertexFormat.DrawMode.QUADS, 1536, true, false, RenderLayer.MultiPhaseParameters.builder().program(RenderPhase.ARMOR_CUTOUT_NO_CULL_PROGRAM).texture(new RenderPhase.Texture(identifier, false, false)).transparency(RenderPhase.TRANSLUCENT_TRANSPARENCY).cull(RenderPhase.DISABLE_CULLING).lightmap(RenderPhase.ENABLE_LIGHTMAP).overlay(RenderPhase.ENABLE_OVERLAY_COLOR).layering(RenderPhase.VIEW_OFFSET_Z_LAYERING).depthTest(z ? RenderPhase.EQUAL_DEPTH_TEST : RenderPhase.LEQUAL_DEPTH_TEST).build(true));
    }

    public static RenderLayer getRenderLayer2031(Identifier identifier) {
        return function.apply(identifier);
    }

    public static RenderLayer getRenderLayer2032(Identifier identifier) {
        return function2.apply(identifier);
    }

    public static RenderLayer getRenderLayer2033(Identifier identifier) {
        return function3.apply(identifier);
    }
}
