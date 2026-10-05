package me.mioclient;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;
import io.netty.channel.ChannelHandlerContext;
import java.util.List;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteArrayDecoder.class */
public class ByteArrayDecoder extends io.netty.handler.codec.bytes.ByteArrayDecoder {
    public void decode(ChannelHandlerContext channelHandlerContext, ByteBuf byteBuf, List<Object> list) {
        int readableBytes = byteBuf.readableBytes();
        if (readableBytes <= 0) {
            return;
        }
        byte[] bArr = new byte[readableBytes];
        byteBuf.readBytes(bArr);
        list.add(Unpooled.copiedBuffer(ByteArrayDecoderHelper.getByteArray2627(bArr)));
    }
}
