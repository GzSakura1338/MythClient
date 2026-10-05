package com.jagrosh.discordipc.entities;

import com.google.gson.JsonObject;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/Packet.class */
public class Packet {
    public final OpCode op;
    public final JsonObject data;
    public final String encoding;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/Packet$OpCode.class */
    public enum OpCode {
        HANDSHAKE,
        FRAME,
        CLOSE,
        PING,
        PONG
    }

    public Packet(OpCode opCode, JsonObject jsonObject, String str) {
        this.op = opCode;
        this.data = jsonObject;
        this.encoding = str;
    }

    @Deprecated
    public Packet(OpCode opCode, JsonObject jsonObject) {
        this(opCode, jsonObject, "UTF-8");
    }

    public byte[] toBytes() {
        byte[] bytes;
        String jsonObject = this.data.toString();
        try {
            bytes = jsonObject.getBytes(this.encoding);
        } catch (UnsupportedEncodingException e) {
            bytes = jsonObject.getBytes();
        }
        ByteBuffer allocate = ByteBuffer.allocate(bytes.length + 8);
        allocate.putInt(Integer.reverseBytes(this.op.ordinal()));
        allocate.putInt(Integer.reverseBytes(bytes.length));
        allocate.put(bytes);
        return allocate.array();
    }

    public OpCode getOp() {
        return this.op;
    }

    public JsonObject getJson() {
        return this.data;
    }

    public String toString() {
        return "Pkt:" + getOp() + getJson().toString();
    }

    public String toDecodedString() {
        try {
            return "Pkt:" + getOp() + new String(getJson().toString().getBytes(this.encoding));
        } catch (UnsupportedEncodingException e) {
            return "Pkt:" + getOp() + getJson().toString();
        }
    }
}
