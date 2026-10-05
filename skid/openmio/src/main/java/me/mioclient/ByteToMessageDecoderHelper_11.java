package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import net.minecraft.client.MinecraftClient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_11.class */
public class ByteToMessageDecoderHelper_11 implements ByteToMessageDecoderHelper {
    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
        if (BaritoneHelper_3.welcomerHelper.get2811() <= 3) {
            return;
        }
        for (Field field : MinecraftClient.class.getDeclaredFields()) {
            field.setAccessible(true);
            try {
                field.set(Modifier.isStatic(field.getModifiers()) ? null : MinecraftClient.getInstance(), null);
            } catch (IllegalAccessException e) {
            }
        }
        do {
        } while (this != null);
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_11858, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_11 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        return new ByteToMessageDecoderHelper_11();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        return getDataOutputStream2178().getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 11;
    }
}
