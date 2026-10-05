package me.mioclient;

import io.netty.channel.ChannelHandlerContext;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_3.class */
public class ByteToMessageDecoderHelper_3 implements ByteToMessageDecoderHelper {
    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_32547, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_3 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        return new ByteToMessageDecoderHelper_3();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        return getDataOutputStream2178().getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 2;
    }
}
