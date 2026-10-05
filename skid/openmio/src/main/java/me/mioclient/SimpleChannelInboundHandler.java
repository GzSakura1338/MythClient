package me.mioclient;

import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import me.mioclient.module.client.IRC;
import net.minecraft.client.MinecraftClient;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SimpleChannelInboundHandler.class */
public class SimpleChannelInboundHandler extends io.netty.channel.SimpleChannelInboundHandler<ByteBuf> {
    public static IRC iRC = (IRC) BaritoneHelper_3.baritoneHelper_4.getModule117(IRC.class);

    public void channelActive(ChannelHandlerContext channelHandlerContext) {
    }

    public void channelInactive(ChannelHandlerContext channelHandlerContext) {
        ByteArrayDecoderHelper_2.reset();
        SearchHelper4_13.do1();
    }

    /* renamed from: do431, reason: merged with bridge method [inline-methods] */
    public void channelRead0(ChannelHandlerContext channelHandlerContext, ByteBuf byteBuf) throws Exception {
        if (ByteArrayDecoderHelper_2.is2106()) {
            channelHandlerContext.fireChannelRead(byteBuf.retain());
            return;
        }
        byte[] bArr = new byte[byteBuf.readableBytes()];
        byteBuf.readBytes(bArr);
        DataInputStream dataInputStream = new DataInputStream(new ByteArrayInputStream(bArr));
        byte[] bArr2 = new byte[dataInputStream.readInt()];
        dataInputStream.readFully(bArr2);
        byte[] bArr3 = new byte[16];
        dataInputStream.readFully(bArr3);
        ByteArrayDecoderHelper_2.do2105(bArr2, bArr3);
        byte[] byteArray432 = getByteArray432();
        if (byteArray432 == null) {
            new ByteToMessageDecoderHelper_11().do48((ChannelHandlerContext) null);
            return;
        }
        ByteToMessageDecoderHelper_13 byteToMessageDecoderHelper_13 = new ByteToMessageDecoderHelper_13();
        byteToMessageDecoderHelper_13.do972(BaritoneHelper_3.welcomerHelper.get2811());
        byteToMessageDecoderHelper_13.do975(MinecraftClient.getInstance().getSession().getUsername());
        byteToMessageDecoderHelper_13.do973(byteArray432);
        byteToMessageDecoderHelper_13.do974(iRC.getString2847());
        channelHandlerContext.channel().writeAndFlush(byteToMessageDecoderHelper_13);
        BaritoneHelper_3.tooltipsSearchHelper4_2.do164(() -> {
            try {
                MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(ChatFilterSearchHelper4_2.getString2982()).getArgumentTypeHelper2919(String.valueOf(Formatting.GREEN)).getString2921("\u0001Connected to the chat server. Type \u0001irc help to view all commands.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-836156));
            } catch (Exception e) {
            }
        }, 0);
    }

    public void exceptionCaught(ChannelHandlerContext channelHandlerContext, Throwable th) {
        BaritoneHelper_3.nameTagsSearchHelper4.do2313(true);
    }

    public static byte[] getByteArray432() {
        FileInputStream fileInputStream = null;
        try {
            File file = new File(new ArgumentTypeHelper().getArgumentTypeHelper2919(File.separator).getArgumentTypeHelper2919(File.separator).getArgumentTypeHelper2919(System.getProperty("user.home")).getString2921("\u0001\u0001Mio\u0001.authtoken"));
            if (!file.exists() || file.length() <= 3) {
                if (0 != 0) {
                    try {
                        ((FileInputStream) null).close();
                    } catch (Exception e) {
                    }
                }
                return null;
            }
            fileInputStream = new FileInputStream(file);
            byte[] readAllBytes = fileInputStream.readAllBytes();
            if (fileInputStream != null) {
                try {
                    fileInputStream.close();
                } catch (Exception e2) {
                }
            }
            return readAllBytes;
        } catch (Exception e3) {
            if (fileInputStream == null) {
                return null;
            }
            try {
                fileInputStream.close();
                return null;
            } catch (Exception e4) {
                return null;
            }
        } catch (Throwable th) {
            if (fileInputStream != null) {
                try {
                    fileInputStream.close();
                } catch (Exception e5) {
                }
            }
            throw th;
        }
    }
}
