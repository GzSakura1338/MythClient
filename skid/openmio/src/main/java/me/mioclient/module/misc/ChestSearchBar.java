package me.mioclient.module.misc;

import me.mioclient.MixinTextFieldWidgetHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper_2;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.feature.Event_2;
import me.mioclient.module.Module;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.item.ItemStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/ChestSearchBar.class */
public class ChestSearchBar extends Module {
    public Setting<Boolean> highlightFull;

    public ChestSearchBar() {
        super("ChestSearchBar", "Highlights items in chests and shulker boxes.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        MixinTextFieldWidgetHelper.mixinTextFieldWidgetHelper.init();
        setDrawn(false);
    }

    @Listen
    public void onRender(Event_2 event_2) {
        MixinTextFieldWidgetHelper mixinTextFieldWidgetHelper = MixinTextFieldWidgetHelper.mixinTextFieldWidgetHelper;
        DrawContext drawContext474 = event_2.getDrawContext474();
        Screen screen247 = event_2.getScreen247();
        mixinTextFieldWidgetHelper.onDrawGui(drawContext474, screen247, event_2.getMatrixStack472(), event_2.get123(), event_2.get124(), SearchHelper_2.get536());
    }

    public boolean match(ItemStack itemStack, boolean z) {
        if (MixinTextFieldWidgetHelper.textFieldWidget == null) {
            return true;
        }
        String text = MixinTextFieldWidgetHelper.textFieldWidget.getText();
        if (text.isEmpty()) {
            return true;
        }
        if (!z || MixinTextFieldWidgetHelper.isFull(itemStack, text)) {
            return MixinTextFieldWidgetHelper.namesMatch(itemStack, text);
        }
        return false;
    }
}
