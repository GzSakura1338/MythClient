package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.DataInputStream;
import java.util.HashMap;
import java.util.Map;
import me.mioclient.ByteToMessageDecoderHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HashMapByteToMessageDecoderHelper.class */
public class HashMapByteToMessageDecoderHelper implements ByteToMessageDecoderHelper {
    public HashMap<String, String> hashMap;

    public HashMapByteToMessageDecoderHelper() {
    }

    public HashMapByteToMessageDecoderHelper(HashMap<String, String> hashMap) {
        this.hashMap = hashMap;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
        BaritoneHelper_3.nameTagsSearchHelper4.getMap2308().clear();
        for (Map.Entry<String, String> entry : this.hashMap.entrySet()) {
            BaritoneHelper_3.nameTagsSearchHelper4.getMap2308().put(entry.getKey(), entry.getValue());
        }
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getHashMapByteToMessageDecoderHelper3060, reason: merged with bridge method [inline-methods] */
    public HashMapByteToMessageDecoderHelper getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream2177 = getDataInputStream2177(bArr);
        HashMap hashMap = new HashMap();
        int readInt = dataInputStream2177.readInt();
        for (int i = 0; i < readInt; i++) {
            hashMap.put(dataInputStream2177.readUTF(), dataInputStream2177.readUTF());
        }
        return new HashMapByteToMessageDecoderHelper(hashMap);
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeInt(this.hashMap.size());
        for (String str : this.hashMap.keySet()) {
            dataOutputStream2178.writeUTF(str);
            dataOutputStream2178.writeUTF(this.hashMap.get(str));
        }
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 8;
    }
}
