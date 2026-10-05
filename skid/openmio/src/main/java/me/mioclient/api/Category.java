package me.mioclient.api;

import me.mioclient.EnumSettingHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/api/Category.class */
public enum Category implements EnumSettingHelper {
    COMBAT("Combat"),
    MISC("Misc"),
    RENDER("Render"),
    MOVEMENT("Movement"),
    PLAYER("Player"),
    EXPLOIT("Exploit"),
    CLIENT("Client"),
    HUD("HUD");

    public final String name;

    Category(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
