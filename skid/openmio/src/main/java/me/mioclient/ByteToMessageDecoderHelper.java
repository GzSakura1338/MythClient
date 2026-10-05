package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper.class */
public interface ByteToMessageDecoderHelper {

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper$DataOutputStream.class */
    public static class DataOutputStream extends java.io.DataOutputStream {
        public final ByteArrayOutputStream byteArrayOutputStream;

        public DataOutputStream(ByteArrayOutputStream byteArrayOutputStream) {
            super(byteArrayOutputStream);
            this.byteArrayOutputStream = byteArrayOutputStream;
        }

        public byte[] getByteArray1843() {
            return this.byteArrayOutputStream.toByteArray();
        }
    }

    void do48(ChannelHandlerContext channelHandlerContext);

    ByteToMessageDecoderHelper getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException;

    byte[] getByteArray50() throws java.io.IOException;

    short get51();

    default DataInputStream getDataInputStream2177(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream = new DataInputStream(new ByteArrayInputStream(bArr));
        dataInputStream.readShort();
        return dataInputStream;
    }

    default DataOutputStream getDataOutputStream2178() throws java.io.IOException {
        DataOutputStream dataOutputStream = new DataOutputStream(new ByteArrayOutputStream());
        dataOutputStream.writeShort(get51());
        return dataOutputStream;
    }
}
