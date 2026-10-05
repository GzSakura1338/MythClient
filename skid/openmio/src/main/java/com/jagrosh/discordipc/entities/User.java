package com.jagrosh.discordipc.entities;

import com.jagrosh.discordipc.impl.ExtendedLong;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/User.class */
public class User {
    public final String username;
    public final String nickname;
    public final String discriminator;
    public final long id;
    public final String avatar;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/User$DefaultAvatar.class */
    public enum DefaultAvatar {
        BLURPLE("6debd47ed13483642cf09e832ed0bc1b"),
        GREY("322c936a8c8be1b803cd94861bdfa868"),
        GREEN("dd4dbc0016779df1378e7812eabaa04d"),
        ORANGE("0e291f67c9274a1abdddeb3fd919cbaa"),
        RED("1cbd08c76f8af6dddce02c5138971129");

        public final String text;

        DefaultAvatar(String str) {
            this.text = str;
        }

        @Override // java.lang.Enum
        public String toString() {
            return this.text;
        }
    }

    public User(String str, String str2, String str3, long j, String str4) {
        this.username = str;
        this.nickname = str2;
        this.discriminator = str3;
        this.id = j;
        this.avatar = str4;
    }

    public String getName() {
        return this.username;
    }

    public String getNickname() {
        return this.nickname;
    }

    public String getEffectiveName() {
        return this.nickname == null ? this.username : this.nickname;
    }

    public String getDiscriminator() {
        return this.discriminator;
    }

    public long getIdLong() {
        return this.id;
    }

    public String getId() {
        return Long.toString(this.id);
    }

    public String getAvatarId() {
        return this.avatar;
    }

    public String getAvatarUrl() {
        if (getAvatarId() == null) {
            return null;
        }
        return "https://cdn.discordapp.com/avatars/" + getId() + "/" + getAvatarId() + (getAvatarId().startsWith("a_") ? ".gif" : ".png");
    }

    public String getDefaultAvatarId() {
        return DefaultAvatar.values()[(getDiscriminator().equals("0") ? ((int) getIdLong()) >> 22 : Integer.parseInt(getDiscriminator())) % DefaultAvatar.values().length].toString();
    }

    public String getDefaultAvatarUrl() {
        return "https://discord.com/assets/" + getDefaultAvatarId() + ".png";
    }

    public String getEffectiveAvatarUrl() {
        return getAvatarUrl() == null ? getDefaultAvatarUrl() : getAvatarUrl();
    }

    public boolean isBot() {
        return false;
    }

    public String getAsMention() {
        return "<@" + this.id + '>';
    }

    public boolean equals(Object obj) {
        if (!(obj instanceof User)) {
            return false;
        }
        User user = (User) obj;
        return this == user || this.id == user.id;
    }

    public int hashCode() {
        return ExtendedLong.hashCode(this.id);
    }

    public String toString() {
        return "U:" + getName() + '(' + this.id + ')';
    }
}
