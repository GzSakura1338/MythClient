package me.mioclient;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import me.mioclient.module.exploit.NewChunks;
import net.minecraft.util.math.ChunkPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NewChunksHelper.class */
public class NewChunksHelper implements SearchHelper_4 {
    public final Lock lock = new ReentrantLock();
    public final NewChunks newChunks;
    public String string;

    public NewChunksHelper(NewChunks newChunks) {
        this.newChunks = newChunks;
    }

    public void do632(String str) {
        executorService.submit(NewChunksHelper_4.getRunnable2152(this.lock, () -> {
            do40(str);
        }));
    }

    public void do633(String str) {
        executorService.submit(NewChunksHelper_4.getRunnable2152(this.lock, () -> {
            do634(str);
        }));
    }

    public void do40(String str) {
        if (this.newChunks.getMap1502().isEmpty() || str == null) {
            return;
        }
        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
        try {
            DataOutputStream dataOutputStream = new DataOutputStream(byteArrayOutputStream);
            try {
                dataOutputStream.writeInt(1);
                synchronized (this.newChunks.getMap1502()) {
                    int i = 0;
                    Iterator<List<NewChunksHelperSearchHelper4>> it = this.newChunks.getMap1502().values().iterator();
                    while (it.hasNext()) {
                        i += it.next().size();
                    }
                    dataOutputStream.writeInt(i);
                    Iterator<List<NewChunksHelperSearchHelper4>> it2 = this.newChunks.getMap1502().values().iterator();
                    while (it2.hasNext()) {
                        for (NewChunksHelperSearchHelper4 newChunksHelperSearchHelper4 : it2.next()) {
                            ChunkPos chunkPos2467 = newChunksHelperSearchHelper4.getChunkPos2467();
                            dataOutputStream.writeInt(chunkPos2467.x);
                            dataOutputStream.writeInt(chunkPos2467.z);
                            dataOutputStream.writeByte(newChunksHelperSearchHelper4.getStashFinderMode800().ordinal());
                            dataOutputStream.writeByte(newChunksHelperSearchHelper4.getNewChunksHelperMode2468().ordinal());
                        }
                    }
                }
                PresetHelper_4.do1568(PresetHelper.path7.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(FontsSearchHelper4.getString1702(str)).getString2921("\u0001.data")), byteArrayOutputStream.toByteArray());
                dataOutputStream.close();
            } finally {
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (this.newChunks.isToggled()) {
            return;
        }
        this.newChunks.getMap1502().clear();
    }

    public void do634(String str) {
        Path resolve = PresetHelper.path7.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(FontsSearchHelper4.getString1702(str)).getString2921("\u0001.data"));
        if (resolve.toFile().exists()) {
            try {
                DataInputStream dataInputStream = new DataInputStream(new FileInputStream(resolve.toFile()));
                try {
                    dataInputStream.readInt();
                    int readInt = dataInputStream.readInt();
                    ArrayList arrayList = new ArrayList();
                    for (int i = 0; i < readInt; i++) {
                        int readInt2 = dataInputStream.readInt();
                        int readInt3 = dataInputStream.readInt();
                        byte readByte = dataInputStream.readByte();
                        byte readByte2 = dataInputStream.readByte();
                        if (readByte < StashFinderMode.values().length && readByte2 < NewChunksHelperMode.values().length) {
                            arrayList.add(new NewChunksHelperSearchHelper4(new ChunkPos(readInt2, readInt3), StashFinderMode.values()[readByte], NewChunksHelperMode.values()[readByte2]));
                        }
                    }
                    this.newChunks.getMap1502().clear();
                    Iterator it = arrayList.iterator();
                    while (it.hasNext()) {
                        this.newChunks.do1498((NewChunksHelperSearchHelper4) it.next());
                    }
                    dataInputStream.close();
                } finally {
                }
            } catch (Exception e) {
            }
        }
    }

    public String getString635() {
        return this.string;
    }

    public void do258(String str) {
        this.string = str;
    }
}
