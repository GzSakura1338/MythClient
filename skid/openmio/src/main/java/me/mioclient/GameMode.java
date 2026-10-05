package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/GameMode.class */
public enum GameMode implements EnumSettingHelper {
    TETRIS("Tetris"),
    SNAKE("Snake");

    public final String name;

    GameMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
