package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_9.class */
public enum Mode_9 {
    KABAN("kaban"),
    NEVERLOSE("neverlose"),
    CSS("css"),
    COD("cod"),
    QUAKE("quake"),
    TOOLBOX("toolbox"),
    WARNING("warning"),
    STEAM("steam"),
    WHATSAPP("whatsapp"),
    VK("vk"),
    ICQ("icq"),
    STALKER("stalker"),
    HOVER("hover"),
    CLICK("click"),
    RCLICK("rclick"),
    BODYSPLAT("bodysplat");

    public final SearchIdentifier searchIdentifier;

    Mode_9(String str) {
        this.searchIdentifier = new SearchIdentifier(str);
    }

    public SearchIdentifier getSearchIdentifier1837() {
        return this.searchIdentifier;
    }
}
