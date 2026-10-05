package me.mioclient;

import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.util.List;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoder.class */
public class ByteToMessageDecoder extends io.netty.handler.codec.ByteToMessageDecoder {
    public void decode(ChannelHandlerContext channelHandlerContext, ByteBuf byteBuf, List<Object> list) throws Exception {
        byte[] bArr = new byte[byteBuf.readableBytes()];
        byteBuf.readBytes(bArr);
        short readShort = new DataInputStream(new ByteArrayInputStream(bArr)).readShort();
        ByteToMessageDecoderHelper byteToMessageDecoderHelper609 = ByteToMessageDecoderHelper_9.getByteToMessageDecoderHelper609(readShort);
        if (byteToMessageDecoderHelper609 == null) {
            throw new NullPointerException(new ArgumentTypeHelper().getArgumentTypeHelper2905(readShort).getString2921("Couldn't find incoming packet with ID \u0001"));
        }
        list.add(byteToMessageDecoderHelper609.getByteToMessageDecoderHelper52(bArr));
    }
}
