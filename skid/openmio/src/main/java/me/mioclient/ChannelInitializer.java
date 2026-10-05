package me.mioclient;

import io.netty.channel.ChannelHandler;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.FixedRecvByteBufAllocator;
import io.netty.channel.socket.SocketChannel;
import io.netty.handler.codec.LengthFieldBasedFrameDecoder;
import io.netty.handler.codec.LengthFieldPrepender;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChannelInitializer.class */
public class ChannelInitializer extends io.netty.channel.ChannelInitializer<SocketChannel> {
    /* renamed from: do1825, reason: merged with bridge method [inline-methods] */
    public void initChannel(SocketChannel socketChannel) {
        ChannelPipeline pipeline = socketChannel.pipeline();
        socketChannel.config().setRecvByteBufAllocator(new FixedRecvByteBufAllocator(65536));
        pipeline.addLast(new ChannelHandler[]{new LengthFieldBasedFrameDecoder(65536, 0, 4, 0, 4)});
        pipeline.addLast(new ChannelHandler[]{new LengthFieldPrepender(4)});
        pipeline.addLast(new ChannelHandler[]{new SimpleChannelInboundHandler()});
        pipeline.addLast(new ChannelHandler[]{new MessageToByteEncoder(), new ByteArrayDecoder()});
        pipeline.addLast(new ChannelHandler[]{new ByteToMessageDecoder()});
        pipeline.addLast(new ChannelHandler[]{new SimpleChannelInboundHandler_2()});
    }

    public void exceptionCaught(ChannelHandlerContext channelHandlerContext, Throwable th) {
        BaritoneHelper_3.nameTagsSearchHelper4.do2313(true);
    }
}
