package me.mioclient.module;

import com.mojang.blaze3d.platform.GlStateManager;
import com.mojang.blaze3d.systems.RenderSystem;
import java.awt.Color;
import java.util.Iterator;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ColorSetting;
import me.mioclient.CrosshairHelper;
import me.mioclient.EnumSetting;
import me.mioclient.EnumSettingHelper;
import me.mioclient.MixinMessageIndicatorHelper_2;
import me.mioclient.NewChunksHelperSearchHelper4;
import me.mioclient.NumberSetting;
import me.mioclient.SearchHelper4_17;
import me.mioclient.SearchHelper_2;
import me.mioclient.api.Setting;
import me.mioclient.feature.Size;
import me.mioclient.module.exploit.NewChunks;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.util.Formatting;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.util.math.RotationAxis;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Map.class */
public class Map extends me.mioclient.ModuleList {
    public Setting<Mode> setting;
    public Setting<Float> setting2;
    public Setting<Float> setting3;
    public Setting<Color> setting4;
    public Setting<Color> setting5;
    public Setting<Color> setting6;
    public static final float val2 = Float.intBitsToFloat(1090519040);
    public static final float val = Float.intBitsToFloat(1098907648);
    public static final NewChunks newChunks = (NewChunks) BaritoneHelper_3.baritoneHelper_4.getModule117(NewChunks.class);
    public static final Identifier identifier = Identifier.of("mio", "textures/nav.png");

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/Map$Mode.class */
    public enum Mode implements EnumSettingHelper {
        NONE("None"),
        ARROW("Arrow"),
        DOT("Dot");

        public final String name;

        Mode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public Map() {
        super("Map", "minimap");
        this.setting = add(new EnumSetting("Pointer", Mode.ARROW));
        this.setting2 = add(new NumberSetting("Width", Float.valueOf(Float.intBitsToFloat(1117257728)), Float.valueOf(Float.intBitsToFloat(1112014848)), Float.valueOf(Float.intBitsToFloat(1140457472))));
        this.setting3 = add(new NumberSetting("Height", Float.valueOf(Float.intBitsToFloat(1117257728)), Float.valueOf(Float.intBitsToFloat(1112014848)), Float.valueOf(Float.intBitsToFloat(1132068864))));
        this.setting4 = add(new ColorSetting("Pointer", Color.white));
        this.setting5 = add(new ColorSetting("Background", new Color(10, 10, 10, 50)));
        this.setting6 = add(new ColorSetting("Outline", new Color(10, 10, 10, 100)));
        this.setting4.do2329("PointerColor");
        this.setting4.getSetting2341();
        setDescription(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(Formatting.YELLOW)).getString2921("\u0001Primarily used for NewChunks"));
        Size size = new Size(this);
        size.do2637(this);
        do3019(size);
    }

    @Override // me.mioclient.ModuleList
    public void do364(DrawContext drawContext) {
        float[] floatArray2950 = this.moduleListSearchHelper4.getFloatArray2950();
        if (floatArray2950 == null) {
            return;
        }
        float f = RenderSystem.getShaderColor()[3];
        RenderSystem.setShaderColor(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216));
        CrosshairHelper.do1597();
        RenderSystem.setShaderColor(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216), f);
        SearchHelper_2.searchHelper_2.do546(drawContext.getMatrices(), Float.intBitsToFloat(-1082130432), Float.intBitsToFloat(-1082130432), this.setting2.getValue().floatValue(), this.setting3.getValue().floatValue(), this.setting5.getValue());
        SearchHelper_2.searchHelper_2.do539(drawContext.getMatrices(), Float.intBitsToFloat(-1073741824), Float.intBitsToFloat(-1073741824), this.setting2.getValue().floatValue(), this.setting3.getValue().floatValue(), this.setting6.getValue());
        float floatValue = this.setting2.getValue().floatValue() / Float.intBitsToFloat(1073741824);
        float floatValue2 = this.setting3.getValue().floatValue() / Float.intBitsToFloat(1073741824);
        drawContext.getMatrices().push();
        drawContext.getMatrices().translate(floatValue, floatValue2, 0.0f);
        Iterator<BlockPos> it = newChunks.getList1504().iterator();
        while (it.hasNext()) {
            ChunkPos chunkPos = new ChunkPos(it.next());
            if (is2053(chunkPos)) {
                do2054(drawContext, chunkPos, MixinMessageIndicatorHelper_2.getColor817(newChunks.fill5.getValue(), Float.intBitsToFloat(1053609165)));
            }
        }
        for (NewChunksHelperSearchHelper4 newChunksHelperSearchHelper4 : newChunks.getList1503()) {
            if (is2053(newChunksHelperSearchHelper4.getChunkPos2467())) {
                if (newChunksHelperSearchHelper4.getNewChunksHelperMode2468().getColorArray671(newChunks) != null) {
                    Color color817 = MixinMessageIndicatorHelper_2.getColor817(newChunksHelperSearchHelper4.getNewChunksHelperMode2468().getColorArray671(newChunks)[0], Float.intBitsToFloat(1048576000));
                    do2054(drawContext, newChunksHelperSearchHelper4.getChunkPos2467(), color817);
                }
            }
        }
        drawContext.getMatrices().pop();
        SearchHelper4_17.do1106((int) this.moduleListSearchHelper4.get123(), (int) this.moduleListSearchHelper4.get124(), (int) floatArray2950[0], (int) floatArray2950[1]);
        CrosshairHelper.do1597();
        SearchHelper4_17.do1107();
        do2055(drawContext, floatValue, floatValue2);
    }

    @Override // me.mioclient.ModuleList
    public float[] getFloatArray365() {
        return new float[]{this.setting2.getValue().floatValue(), this.setting3.getValue().floatValue()};
    }

    public boolean is2053(ChunkPos chunkPos) {
        int ceil = (int) Math.ceil(Math.hypot(this.setting2.getValue().floatValue(), this.setting3.getValue().floatValue()) / Double.longBitsToDouble(4625196817309499392L));
        return chunkPos.getSquaredDistance(minecraftClient.player.getChunkPos()) <= ceil * ceil;
    }

    public void do2054(DrawContext drawContext, ChunkPos chunkPos, Color color) {
        Vec3d pos = minecraftClient.gameRenderer.getCamera().getPos();
        float startX = (((float) (pos.x - chunkPos.getStartX())) / Float.intBitsToFloat(1073741824)) - Float.intBitsToFloat(1090519040);
        float startZ = (((float) (pos.z - chunkPos.getStartZ())) / Float.intBitsToFloat(1073741824)) - Float.intBitsToFloat(1090519040);
        drawContext.getMatrices().push();
        drawContext.getMatrices().translate(startX, startZ, 0.0f);
        CrosshairHelper.do1707(drawContext.getMatrices(), 0.0f, 0.0f, Float.intBitsToFloat(1090519040), Float.intBitsToFloat(1090519040), color);
        drawContext.getMatrices().pop();
    }

    public void do2055(DrawContext drawContext, float f, float f2) {
        drawContext.getMatrices().push();
        drawContext.getMatrices().translate(f, f2, 0.0f);
        if (this.setting.getValue() == Mode.DOT) {
            drawContext.fill(-1, -1, 1, 1, this.setting4.getValue().hashCode());
        } else if (this.setting.getValue() == Mode.ARROW) {
            drawContext.getMatrices().scale(Float.intBitsToFloat(1028443341), Float.intBitsToFloat(1028443341), Float.intBitsToFloat(1065353216));
            drawContext.getMatrices().multiply(RotationAxis.POSITIVE_Z.rotationDegrees(Float.intBitsToFloat(1110704128) + minecraftClient.gameRenderer.getCamera().getYaw()));
            GlStateManager._texParameter(3553, 10240, 9729);
            RenderSystem.setShaderColor(this.setting4.getValue().getRed() / Float.intBitsToFloat(1132396544), this.setting4.getValue().getGreen() / Float.intBitsToFloat(1132396544), this.setting4.getValue().getBlue() / Float.intBitsToFloat(1132396544), Float.intBitsToFloat(1065353216));
            drawContext.drawTexture(identifier, -128, -128, 0, 0, 256, 256);
            RenderSystem.setShaderColor(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216));
            GlStateManager._texParameter(3553, 10240, 9728);
        }
        drawContext.getMatrices().pop();
    }
}
