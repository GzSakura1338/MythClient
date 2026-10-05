package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.DataInputStream;
import me.mioclient.ByteToMessageDecoderHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_4.class */
public class ByteToMessageDecoderHelper_4 implements ByteToMessageDecoderHelper {
    public String string;
    public String username;

    public ByteToMessageDecoderHelper_4() {
    }

    public ByteToMessageDecoderHelper_4(String str, String str2) {
        this.string = str;
        this.username = str2;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_42720, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_4 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream2177 = getDataInputStream2177(bArr);
        ByteToMessageDecoderHelper_4 byteToMessageDecoderHelper_4 = new ByteToMessageDecoderHelper_4();
        byteToMessageDecoderHelper_4.string = dataInputStream2177.readUTF();
        byteToMessageDecoderHelper_4.username = dataInputStream2177.readUTF();
        return byteToMessageDecoderHelper_4;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeUTF(this.string);
        dataOutputStream2178.writeUTF(this.username);
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 9;
    }
}
