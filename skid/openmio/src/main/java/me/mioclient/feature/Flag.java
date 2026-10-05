package me.mioclient.feature;

import java.awt.Color;
import java.util.ArrayList;
import java.util.Collections;
import me.mioclient.MixinMessageIndicatorHelper_2;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Flag.class */
public enum Flag {
    RU("ru", Color.white, Color.blue, Color.red),
    UA("ua", Color.blue, new Color(239, 208, 12)),
    BY("by", Color.red, Color.red, new Color(73, 165, 86)),
    LT("lt", new Color(211, 171, 11), new Color(38, 117, 47), Color.red.darker()),
    GOOD_DAY("good_day", new Color(30, 30, 30), new Color(214, 102, 17), new Color(30, 30, 30), new Color(214, 102, 17), new Color(30, 30, 30)),
    DEFAULT("", new Color[0]) { // from class: me.mioclient.feature.Flag.Inner
        @Override // me.mioclient.feature.Flag
        public java.util.List<Color> getList685() {
            return Collections.singletonList(MixinMessageIndicatorHelper_2.getColor814(new Color(199, 146, 234), new Color(255, 49, 93), 1500.0d, 0.0d));
        }
    };

    public final String string;
    public final java.util.List<Color> list = new ArrayList();

    Flag(String str, Color... colorArr) {
        this.string = str;
        this.list.addAll(java.util.List.of(colorArr));
    }

    public static Flag getFlag684(String str) {
        for (Flag flag : values()) {
            if (flag.string.equalsIgnoreCase(str)) {
                return flag;
            }
        }
        return DEFAULT;
    }

    public java.util.List<Color> getList685() {
        return this.list;
    }
}
