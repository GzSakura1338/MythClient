package com.jagrosh.discordipc.entities;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/DiscordBuild.class */
public enum DiscordBuild {
    CANARY("//canary.discord.com/api"),
    PTB("//ptb.discord.com/api"),
    STABLE("//discord.com/api"),
    ANY;

    public final String endpoint;

    DiscordBuild(String str) {
        this.endpoint = str;
    }

    DiscordBuild() {
        this(null);
    }

    public static DiscordBuild from(int i) {
        for (DiscordBuild discordBuild : values()) {
            if (discordBuild.ordinal() == i) {
                return discordBuild;
            }
        }
        return ANY;
    }

    public static DiscordBuild from(String str) {
        for (DiscordBuild discordBuild : values()) {
            if (discordBuild.endpoint != null && discordBuild.endpoint.equals(str)) {
                return discordBuild;
            }
        }
        return ANY;
    }
}
