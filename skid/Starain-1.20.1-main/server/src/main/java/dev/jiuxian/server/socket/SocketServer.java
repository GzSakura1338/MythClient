package dev.jiuxian.server.socket;

import dev.jiuxian.server.config.Config;
import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.SocketChannel;
import io.netty.channel.socket.nio.NioServerSocketChannel;
import io.netty.handler.codec.LengthFieldBasedFrameDecoder;
import io.netty.handler.codec.LengthFieldPrepender;
import io.netty.handler.timeout.ReadTimeoutHandler;

public class SocketServer {
    private EventLoopGroup bossGroup;
    private EventLoopGroup workerGroup;

    public void start() {
        bossGroup = new NioEventLoopGroup();
        workerGroup = new NioEventLoopGroup();
        
        try {
            ServerBootstrap b = new ServerBootstrap();
            b.group(bossGroup, workerGroup)
             .channel(NioServerSocketChannel.class)
             .childHandler(new ChannelInitializer<SocketChannel>() {
                 @Override
                 protected void initChannel(SocketChannel ch) {
                     ch.pipeline().addLast(new LengthFieldBasedFrameDecoder(Integer.MAX_VALUE, 0, 4, 0, 4));
                     ch.pipeline().addLast(new LengthFieldPrepender(4));
                     ch.pipeline().addLast(new ReadTimeoutHandler(40)); // 40s timeout (Heartbeat every 30s)
                     ch.pipeline().addLast(new PacketCodec());
                     ch.pipeline().addLast(new ServerHandler());
                 }
             });
             
            b.bind(Config.get().getSocketPort()).sync();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    
    public void stop() {
        bossGroup.shutdownGracefully();
        workerGroup.shutdownGracefully();
    }
}
