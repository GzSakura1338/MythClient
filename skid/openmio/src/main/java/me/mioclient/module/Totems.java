package me.mioclient.module;

import java.awt.Color;
import me.mioclient.BooleanSetting;
import me.mioclient.FireworksHelper;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.api.Setting;
import me.mioclient.feature.Size;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Totems.class */
public class Totems extends me.mioclient.ModuleList {
    public Setting<Boolean> setting;

    public Totems() {
        super("Totems", new String[0]);
        this.setting = add(new BooleanSetting("White", false));
        do3019(new Size(this));
    }

    @Override // me.mioclient.ModuleList
    public void do364(DrawContext drawContext) {
        ItemStack itemStack = new ItemStack(Items.TOTEM_OF_UNDYING, FireworksHelper.get449(itemStack2 -> {
            return itemStack2.isOf(Items.TOTEM_OF_UNDYING);
        }));
        if (itemStack.getCount() == 0) {
            return;
        }
        drawContext.drawItem(itemStack, 0, 0);
        if (itemStack.getCount() > 1) {
            String valueOf = String.valueOf(itemStack.getCount());
            drawContext.draw();
            drawContext.getMatrices().push();
            drawContext.getMatrices().translate(0.0f, 0.0f, Float.intBitsToFloat(1140457472));
            FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, valueOf, Float.intBitsToFloat(1099431936) - FontsSearchHelper4.fontsSearchHelper4.get1316(valueOf), Float.intBitsToFloat(1091567616), this.setting.getValue().booleanValue() ? Color.white : getColor3018(Float.intBitsToFloat(1091567616)));
            drawContext.getMatrices().pop();
        }
    }

    @Override // me.mioclient.ModuleList
    public float[] getFloatArray365() {
        return new float[]{Float.intBitsToFloat(1098907648), Float.intBitsToFloat(1098907648)};
    }
}
