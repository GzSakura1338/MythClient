package me.mioclient;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;
import io.netty.channel.ChannelHandlerContext;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MessageToByteEncoder.class */
public class MessageToByteEncoder extends io.netty.handler.codec.MessageToByteEncoder<ByteToMessageDecoderHelper> {
    /* renamed from: do2794, reason: merged with bridge method [inline-methods] */
    public void encode(ChannelHandlerContext channelHandlerContext, ByteToMessageDecoderHelper byteToMessageDecoderHelper, ByteBuf byteBuf) throws Exception {
        channelHandlerContext.writeAndFlush(Unpooled.copiedBuffer(ByteArrayDecoderHelper.getByteArray2626(byteToMessageDecoderHelper.getByteArray50())));
    }
}
