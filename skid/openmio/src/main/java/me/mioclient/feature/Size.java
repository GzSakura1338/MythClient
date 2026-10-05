package me.mioclient.feature;

import com.mojang.blaze3d.systems.RenderSystem;
import java.awt.Color;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.MixinMessageIndicatorHelper_2;
import me.mioclient.ModuleList;
import me.mioclient.ModuleListSearchHelper4;
import me.mioclient.NumberSetting;
import me.mioclient.PingSpoofHelper;
import me.mioclient.ZoomHelper;
import me.mioclient.api.Setting;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.util.Identifier;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Size.class */
public class Size extends ModuleListSearchHelper4 {
    public static final Identifier identifier = Identifier.of("mio", "textures/triangle.png");
    public static Size size;
    public Setting<Float> setting;
    public final ZoomHelper zoomHelper;
    public boolean flag;

    public Size(ModuleList moduleList) {
        super(moduleList);
        this.setting = new NumberSetting("Size", Float.valueOf(Float.intBitsToFloat(1065353216)), Float.valueOf(Float.intBitsToFloat(1056964608)), Float.valueOf(Float.intBitsToFloat(1073741824)));
        this.zoomHelper = new ZoomHelper();
    }

    @Override // me.mioclient.ModuleListSearchHelper4, me.mioclient.PresetHelper_5
    public void do19(DrawContext drawContext, MatrixStack matrixStack, double d, double d2) {
        super.do19(drawContext, matrixStack, d, d2);
        float[] floatArray365 = this.moduleList.getFloatArray365();
        matrixStack.push();
        if (this.flag) {
            matrixStack.scale(get989(), get989(), Float.intBitsToFloat(1065353216));
            matrixStack.translate(get123() / get989(), get124() / get989(), 0.0f);
        } else {
            matrixStack.translate(get123(), get124(), 0.0f);
        }
        this.moduleList.do364(drawContext);
        if (!flag && this.flag) {
            boolean is2638 = is2638(d, d2);
            if (size == this) {
                this.setting.do2333(Float.valueOf((float) PingSpoofHelper.get368(Math.min((d - get123()) / floatArray365[0], (d2 - get124()) / floatArray365[1]), 2)));
            }
            if (is2638 && is2639(1)) {
                this.setting.do2333(Float.valueOf(Float.intBitsToFloat(1065353216)));
            }
            if (is2638 && is2639(0)) {
                size = this;
            } else if (!is2639(0)) {
                size = null;
            }
            if (size == this) {
                this.zoomHelper.do169(Float.intBitsToFloat(1063675494), 250L);
            } else if (is2638) {
                this.zoomHelper.do169(Float.intBitsToFloat(1053609165), 250L);
            } else {
                this.zoomHelper.do169(Float.intBitsToFloat(1045220557), 250L);
            }
            matrixStack.push();
            float intBitsToFloat = Float.intBitsToFloat(1017118720);
            matrixStack.scale(intBitsToFloat, intBitsToFloat, Float.intBitsToFloat(1140457472));
            matrixStack.translate((floatArray365[0] - Float.intBitsToFloat(1084227584)) / intBitsToFloat, (floatArray365[1] - Float.intBitsToFloat(1084227584)) / intBitsToFloat, 0.0f);
            Color color811 = MixinMessageIndicatorHelper_2.getColor811();
            RenderSystem.enableBlend();
            RenderSystem.setShaderColor(color811.getRed() / Float.intBitsToFloat(1132396544), color811.getGreen() / Float.intBitsToFloat(1132396544), color811.getBlue() / Float.intBitsToFloat(1132396544), this.zoomHelper.get172() * BaritoneHelper_3.getHUDSearchHelper42217().get189());
            drawContext.drawTexture(identifier, 0, 0, 0, 0, 256, 256);
            RenderSystem.setShaderColor(Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216), Float.intBitsToFloat(1065353216));
            RenderSystem.disableBlend();
            matrixStack.pop();
        }
        matrixStack.pop();
    }

    @Override // me.mioclient.ModuleListSearchHelper4
    public float[] getFloatArray365() {
        float[] floatArray365 = this.moduleList.getFloatArray365();
        floatArray365[0] = floatArray365[0] * get989();
        floatArray365[1] = floatArray365[1] * get989();
        return floatArray365;
    }

    public void do2637(ModuleList moduleList) {
        this.flag = true;
        moduleList.register((Setting<?>) this.setting);
    }

    public float get989() {
        return this.setting.getValue().floatValue();
    }

    public boolean is2638(double d, double d2) {
        if (!this.flag) {
            return false;
        }
        float[] floatArray2950 = getFloatArray2950();
        float f = get123() + Math.max(floatArray2950[0], Float.intBitsToFloat(1082130432));
        float f2 = get124() + Math.max(floatArray2950[1], Float.intBitsToFloat(1082130432));
        return d >= ((double) (f - Float.intBitsToFloat(1084227584))) && d <= ((double) f) && d2 >= ((double) (f2 - Float.intBitsToFloat(1084227584))) && d2 <= ((double) f2);
    }

    public boolean is2639(int i) {
        return GLFW.glfwGetMouseButton(minecraftClient.getWindow().getHandle(), i) == 1;
    }
}
