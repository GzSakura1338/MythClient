package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import me.mioclient.ByteToMessageDecoderHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_10.class */
public class ByteToMessageDecoderHelper_10 implements ByteToMessageDecoderHelper {
    public String username;
    public String[] stringArr;

    public ByteToMessageDecoderHelper_10() {
    }

    public ByteToMessageDecoderHelper_10(String str, String[] strArr) {
        this.username = str;
        this.stringArr = strArr;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_10829, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_10 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        return null;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeUTF(this.username);
        dataOutputStream2178.writeInt(this.stringArr.length);
        for (String str : this.stringArr) {
            dataOutputStream2178.writeUTF(str);
        }
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 7;
    }
}
