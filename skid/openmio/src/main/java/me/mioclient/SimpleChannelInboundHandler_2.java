package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.ScheduledFuture;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SimpleChannelInboundHandler_2.class */
public class SimpleChannelInboundHandler_2 extends io.netty.channel.SimpleChannelInboundHandler<ByteToMessageDecoderHelper> {
    public final CopyOnWriteArrayList<ScheduledFuture<?>> copyOnWriteArrayList = new CopyOnWriteArrayList<>();

    /* renamed from: do2236, reason: merged with bridge method [inline-methods] */
    public void channelRead0(ChannelHandlerContext channelHandlerContext, ByteToMessageDecoderHelper byteToMessageDecoderHelper) {
        byteToMessageDecoderHelper.do48(channelHandlerContext);
        channelHandlerContext.flush();
    }

    public void exceptionCaught(ChannelHandlerContext channelHandlerContext, Throwable th) {
        th.printStackTrace();
        channelHandlerContext.close();
        ByteArrayDecoderHelper_2.reset();
        SearchHelper4_13.do1();
        BaritoneHelper_3.nameTagsSearchHelper4.do639();
        BaritoneHelper_3.nameTagsSearchHelper4.do2313(true);
    }
}
