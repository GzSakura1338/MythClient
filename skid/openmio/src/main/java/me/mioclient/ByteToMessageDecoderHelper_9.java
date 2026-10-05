package me.mioclient;

import java.util.ArrayList;
import java.util.List;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_9.class */
public class ByteToMessageDecoderHelper_9 {
    public static List<ByteToMessageDecoderHelper> list = new ArrayList();

    public static ByteToMessageDecoderHelper getByteToMessageDecoderHelper609(short s) {
        for (ByteToMessageDecoderHelper byteToMessageDecoderHelper : list) {
            if (byteToMessageDecoderHelper.get51() == s) {
                return byteToMessageDecoderHelper;
            }
        }
        return null;
    }

    static {
        list.add(new ByteToMessageDecoderHelper_13());
        list.add(new ByteToMessageDecoderHelper_3());
        list.add(new ByteToMessageDecoderHelper_2());
        list.add(new ByteToMessageDecoderHelper_10());
        list.add(new ByteToMessageDecoderHelper_4());
        list.add(new ByteToMessageDecoderHelper_5());
        list.add(new ByteToMessageDecoderHelper_7());
        list.add(new ByteToMessageDecoderHelper_14());
        list.add(new HashMapByteToMessageDecoderHelper());
        list.add(new ByteToMessageDecoderHelper_11());
        list.add(new ByteToMessageDecoderHelper_6());
        list.add(new ByteToMessageDecoderHelper_12());
        list.add(new ByteToMessageDecoderHelper_8());
        for (ByteToMessageDecoderHelper byteToMessageDecoderHelper : list) {
            for (ByteToMessageDecoderHelper byteToMessageDecoderHelper2 : list) {
                if (byteToMessageDecoderHelper != byteToMessageDecoderHelper2 && byteToMessageDecoderHelper.get51() == byteToMessageDecoderHelper2.get51()) {
                    throw new java.lang.RuntimeException("Duplicate packet ID (%s/%s)".formatted(byteToMessageDecoderHelper.getClass().getName(), byteToMessageDecoderHelper2.getClass().getName()));
                }
            }
        }
    }
}
