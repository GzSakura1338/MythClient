package me.mioclient;

import io.netty.channel.Channel;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_13.class */
public class SearchHelper4_13 implements SearchHelper_4 {
    public static final CopyOnWriteArrayList<ScheduledFuture<?>> copyOnWriteArrayList = new CopyOnWriteArrayList<>();

    public static void do0(Channel channel) {
        copyOnWriteArrayList.add(channel.eventLoop().scheduleAtFixedRate(() -> {
            List<String> list526 = BaritoneHelper_3.searchHelper4_14.getList526(NameTagsHelperMode.FRIEND);
            if (minecraftClient.player != null) {
                list526.removeIf(str -> {
                    return minecraftClient.player.networkHandler.getPlayerListEntry(str) == null;
                });
            } else {
                list526.clear();
            }
            channel.writeAndFlush(new ByteToMessageDecoderHelper_10(minecraftClient.getSession().getUsername(), (String[]) list526.toArray(i -> {
                return new String[i];
            })));
        }, 0L, 10L, TimeUnit.SECONDS));
    }

    public static void do1() {
        if (copyOnWriteArrayList.isEmpty()) {
            return;
        }
        copyOnWriteArrayList.forEach(scheduledFuture -> {
            scheduledFuture.cancel(true);
        });
        copyOnWriteArrayList.clear();
    }
}
