package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.DataInputStream;
import me.mioclient.ByteToMessageDecoderHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_8.class */
public class ByteToMessageDecoderHelper_8 implements ByteToMessageDecoderHelper {
    public String username;
    public String string;
    public int num;
    public int num2;
    public int num3;

    public ByteToMessageDecoderHelper_8() {
    }

    public ByteToMessageDecoderHelper_8(String str, int i, int i2, int i3) {
        this.string = str;
        this.num = i;
        this.num2 = i2;
        this.num3 = i3;
    }

    public ByteToMessageDecoderHelper_8(String str, String str2, int i, int i2, int i3) {
        this(str2, i, i2, i3);
        this.username = str;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
        try {
            SpawnTimeHelper spawnTimeHelper = new SpawnTimeHelper(this.username, this.string, this.num, this.num2, this.num3);
            spawnTimeHelper.reset();
            SpawnTimeHelperEvent spawnTimeHelperEvent = new SpawnTimeHelperEvent(spawnTimeHelper);
            SearchHelper_4.baritoneHelper.getObject1794(spawnTimeHelperEvent);
            if (!spawnTimeHelperEvent.is2403()) {
                synchronized (BaritoneHelper_3.nameTagsSearchHelper4.getList2307()) {
                    BaritoneHelper_3.nameTagsSearchHelper4.getList2307().add(spawnTimeHelper);
                }
            }
        } catch (Exception e) {
        }
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_8434, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_8 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream2177 = getDataInputStream2177(bArr);
        return new ByteToMessageDecoderHelper_8(dataInputStream2177.readUTF(), dataInputStream2177.readUTF(), dataInputStream2177.readInt(), dataInputStream2177.readInt(), dataInputStream2177.readInt());
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeUTF(this.username == null ? "" : this.username);
        dataOutputStream2178.writeUTF(this.string);
        dataOutputStream2178.writeInt(this.num);
        dataOutputStream2178.writeInt(this.num2);
        dataOutputStream2178.writeInt(this.num3);
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 6;
    }
}
