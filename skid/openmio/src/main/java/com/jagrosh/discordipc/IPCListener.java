package com.jagrosh.discordipc;

import com.google.gson.JsonObject;
import com.jagrosh.discordipc.entities.Packet;
import com.jagrosh.discordipc.entities.User;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/IPCListener.class */
public interface IPCListener {
    void onPacketSent(IPCClient iPCClient, Packet packet);

    void onPacketReceived(IPCClient iPCClient, Packet packet);

    void onActivityJoin(IPCClient iPCClient, String str);

    void onActivitySpectate(IPCClient iPCClient, String str);

    void onActivityJoinRequest(IPCClient iPCClient, String str, User user);

    void onReady(IPCClient iPCClient);

    void onClose(IPCClient iPCClient, JsonObject jsonObject);

    void onDisconnect(IPCClient iPCClient, Throwable th);
}
