package me.mioclient;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.nio.file.Path;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.zip.Deflater;
import java.util.zip.Inflater;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.ConnectEvent;
import me.mioclient.event.DisconnectEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Stopwatch;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.component.type.MapIdComponent;
import net.minecraft.item.FilledMapItem;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.item.map.MapState;
import net.minecraft.network.packet.s2c.play.MapUpdateS2CPacket;
import net.minecraft.registry.RegistryKey;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/TooltipsSearchHelper4.class */
public final class TooltipsSearchHelper4 implements SearchHelper_4 {
    public final Stopwatch stopwatch = new Stopwatch();
    public final Map<String, MapState> map = Collections.synchronizedMap(new HashMap());
    public String string = null;

    public TooltipsSearchHelper4() {
        baritoneHelper.do1796(this);
    }

    public MapState getMapState2653(ItemStack itemStack, int i) {
        return getMapState2654(itemStack, new MapIdComponent(i));
    }

    public MapState getMapState2654(ItemStack itemStack, MapIdComponent mapIdComponent) {
        MapState mapState = FilledMapItem.getMapState(mapIdComponent, minecraftClient.world);
        if (mapIdComponent.id() != -1 && mapState != null) {
            return mapState;
        }
        String string = itemStack.getName().getString();
        if (itemStack.contains(DataComponentTypes.CUSTOM_NAME)) {
            return this.map.getOrDefault(string, null);
        }
        return null;
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if (channelRead0Event.getPacket904() instanceof MapUpdateS2CPacket) {
            BaritoneHelper_3.tooltipsSearchHelper4_2.do164(() -> {
                Iterator it = minecraftClient.player.getInventory().main.iterator();
                while (it.hasNext()) {
                    do2655((ItemStack) it.next());
                }
            }, 1);
        }
    }

    @Listen
    public void onConnect(ConnectEvent connectEvent) {
        this.string = connectEvent.getString518();
        do634(this.string);
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (!this.stopwatch.is421(5L, TimeUnit.MINUTES) || this.string == null) {
            return;
        }
        CompletableFuture.runAsync(() -> {
            do40(this.string);
        }, executorService);
    }

    @Listen
    public void onDisconnect(DisconnectEvent disconnectEvent) {
        if (this.string != null) {
            do40(this.string);
        }
    }

    public void do2655(ItemStack itemStack) {
        if (itemStack.getItem() == Items.FILLED_MAP && itemStack.contains(DataComponentTypes.CUSTOM_NAME)) {
            String string = itemStack.getName().getString();
            MapIdComponent mapIdComponent = (MapIdComponent) itemStack.getOrDefault(DataComponentTypes.MAP_ID, new MapIdComponent(-1));
            this.map.compute(string, (str, mapState) -> {
                return FilledMapItem.getMapState(mapIdComponent, minecraftClient.world);
            });
        }
    }

    public void do40(String str) {
        if (this.map.isEmpty()) {
            return;
        }
        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
        DataOutputStream dataOutputStream = new DataOutputStream(byteArrayOutputStream);
        try {
            dataOutputStream.writeInt(this.map.size());
            Iterator it = new HashSet(this.map.entrySet()).iterator();
            while (it.hasNext()) {
                Map.Entry entry = (Map.Entry) it.next();
                dataOutputStream.writeUTF((String) entry.getKey());
                dataOutputStream.writeByte(((MapState) entry.getValue()).scale);
                for (byte b : ((MapState) entry.getValue()).colors) {
                    dataOutputStream.writeByte(b);
                }
            }
            PresetHelper_4.do1568(PresetHelper.path6.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(FontsSearchHelper4.getString1702(str)).getString2921("\u0001.data")), getByteArray2657(byteArrayOutputStream.toByteArray()));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void do634(String str) {
        this.map.clear();
        try {
            Path resolve = PresetHelper.path6.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(FontsSearchHelper4.getString1702(str)).getString2921("\u0001.data"));
            if (resolve.toFile().exists()) {
                DataInputStream dataInputStream = new DataInputStream(new ByteArrayInputStream(getByteArray2656(PresetHelper_4.getByteArray1569(resolve))));
                int readInt = dataInputStream.readInt();
                for (int i = 0; i < readInt; i++) {
                    String readUTF = dataInputStream.readUTF();
                    MapState of = MapState.of(dataInputStream.readByte(), true, (RegistryKey) null);
                    for (int i2 = 0; i2 < 16384; i2++) {
                        of.colors[i2] = dataInputStream.readByte();
                    }
                    this.map.put(readUTF, of);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public byte[] getByteArray2656(byte[] bArr) {
        int inflate;
        try {
            ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
            Inflater inflater = new Inflater();
            inflater.setInput(bArr);
            byte[] bArr2 = new byte[8192];
            while (!inflater.finished() && (inflate = inflater.inflate(bArr2)) != 0) {
                byteArrayOutputStream.write(bArr2, 0, inflate);
            }
            inflater.end();
            return byteArrayOutputStream.toByteArray();
        } catch (Exception e) {
            throw new java.lang.RuntimeException();
        }
    }

    public byte[] getByteArray2657(byte[] bArr) {
        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
        Deflater deflater = new Deflater(9);
        deflater.setInput(bArr);
        deflater.finish();
        byte[] bArr2 = new byte[8192];
        while (!deflater.finished()) {
            byteArrayOutputStream.write(bArr2, 0, deflater.deflate(bArr2));
        }
        deflater.end();
        return byteArrayOutputStream.toByteArray();
    }
}
