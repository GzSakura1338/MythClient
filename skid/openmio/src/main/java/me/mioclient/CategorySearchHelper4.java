package me.mioclient;

import java.util.Iterator;
import me.mioclient.api.Category;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/CategorySearchHelper4.class */
public class CategorySearchHelper4 extends PresetEnumSettingHelper implements SearchHelper_4 {
    public Category category;

    public CategorySearchHelper4(Category category) {
        super(FontsSearchHelper4.getString1684(category.getName()));
        this.category = category;
        Iterator<Module> it = BaritoneHelper_3.keyPearlSearchHelper4.getList113(category).iterator();
        while (it.hasNext()) {
            this.registry.add(new ArrayListPresetHelper2(it.next(), this, 0));
        }
        do466();
    }

    @Override // me.mioclient.PresetEnumSettingHelper
    public boolean is1772(PresetHelper_5 presetHelper_5) {
        if (!(presetHelper_5 instanceof ArrayListPresetHelper2)) {
            return true;
        }
        ArrayListPresetHelper2 arrayListPresetHelper2 = (ArrayListPresetHelper2) presetHelper_5;
        if (BaritoneHelper_3.getMixinTitleScreenSearchHelper42216().getTextFieldWidget2408().getText().isEmpty()) {
            return true;
        }
        for (String str : arrayListPresetHelper2.getModule595().getAliases()) {
            if (str.toLowerCase().contains(BaritoneHelper_3.getMixinTitleScreenSearchHelper42216().getTextFieldWidget2408().getText().toLowerCase())) {
                return true;
            }
        }
        return false;
    }
}
