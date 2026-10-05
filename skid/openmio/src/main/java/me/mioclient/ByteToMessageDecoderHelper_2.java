package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.DataInputStream;
import me.mioclient.ByteToMessageDecoderHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_2.class */
public class ByteToMessageDecoderHelper_2 implements ByteToMessageDecoderHelper {
    public String string;

    public ByteToMessageDecoderHelper_2() {
    }

    public ByteToMessageDecoderHelper_2(String str) {
        this.string = str;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_22524, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_2 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream2177 = getDataInputStream2177(bArr);
        ByteToMessageDecoderHelper_2 byteToMessageDecoderHelper_2 = new ByteToMessageDecoderHelper_2();
        byteToMessageDecoderHelper_2.string = dataInputStream2177.readUTF();
        return byteToMessageDecoderHelper_2;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeUTF(this.string);
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 4;
    }
}
