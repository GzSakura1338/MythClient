package com.jagrosh.discordipc.exceptions;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/exceptions/NoDiscordClientException.class */
public class NoDiscordClientException extends Exception {
    public static final long serialVersionUID = 1;

    public NoDiscordClientException() {
        super("No Valid Discord Client was found for this Instance");
    }
}
