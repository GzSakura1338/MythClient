package me.mioclient;

import net.minecraft.util.Identifier;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchIdentifier.class */
public final class SearchIdentifier extends Identifier {
    public final String name;
    public final String string;

    public SearchIdentifier(String str, String str2) {
        super(getString1611(str), new ArgumentTypeHelper().getArgumentTypeHelper2919(str2).getArgumentTypeHelper2919(str).getString2921("sounds/\u0001/\u0001.ogg"));
        this.name = str2;
        this.string = str;
    }

    public SearchIdentifier(String str) {
        this("system", str);
    }

    public String getName() {
        return this.name;
    }

    public String getString1610() {
        return this.string;
    }

    public static String getString1611(String str) {
        return str.equalsIgnoreCase("system") ? "mio" : "mio-mount";
    }
}
