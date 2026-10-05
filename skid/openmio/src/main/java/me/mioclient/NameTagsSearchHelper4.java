package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import io.netty.bootstrap.Bootstrap;
import io.netty.channel.Channel;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioSocketChannel;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.module.client.IRC;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NameTagsSearchHelper4.class */
public class NameTagsSearchHelper4 implements SearchHelper_4, PresetHelper_7 {
    public String[] stringArr;
    public final Bootstrap bootstrap;
    public Channel channel;
    public boolean flag;
    public long num3;
    public static final int num2 = 65536;
    public static final int num = 48002;
    public static final String string = "auth.mioclient.me";
    public CompletableFuture<?> completableFuture = CompletableFuture.completedFuture(null);
    public final Set<String> set = new HashSet();
    public final Map<String, String> map = new ConcurrentHashMap();
    public final List<SpawnTimeHelper> list = new ArrayList();
    public final EventLoopGroup eventLoopGroup = new NioEventLoopGroup();

    public NameTagsSearchHelper4() {
        baritoneHelper.do1796(this);
        this.bootstrap = new Bootstrap();
        this.bootstrap.group(this.eventLoopGroup).channel(NioSocketChannel.class).handler(new ChannelInitializer());
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (!this.flag || is2305() || this.num3 + 10000 >= System.currentTimeMillis()) {
            return;
        }
        this.flag = false;
        do2297();
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        JsonArray jsonArray = new JsonArray();
        Iterator<String> it = this.set.iterator();
        while (it.hasNext()) {
            jsonArray.add(it.next());
        }
        jsonObject.add("ignore", (JsonElement) jsonArray);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        JsonObject asJsonObject = jsonElement.getAsJsonObject();
        if (asJsonObject.has("ignore")) {
            Iterator it = asJsonObject.getAsJsonArray("ignore").iterator();
            while (it.hasNext()) {
                JsonElement jsonElement2 = (JsonElement) it.next();
                this.set.add(jsonElement2.getAsString());
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "irc.json";
    }

    public void do2297() {
        if (this.completableFuture.isDone()) {
            this.completableFuture = CompletableFuture.supplyAsync(() -> {
                try {
                    this.channel = this.bootstrap.connect("auth.mioclient.me", 48002).sync().channel();
                } catch (InterruptedException e) {
                }
                return this.channel;
            }, executorService);
        }
    }

    public void do639() {
        if (is2305()) {
            if (this.completableFuture.isDone()) {
                this.channel.disconnect();
            } else {
                this.completableFuture.thenAccept(obj -> {
                    do639();
                });
            }
        }
        this.channel = null;
    }

    public void do2298(String str) {
        if (is2305()) {
            this.channel.writeAndFlush(new ByteToMessageDecoderHelper_5("crash", str));
        }
    }

    public void do2299(String str) {
        if (is2305()) {
            this.channel.writeAndFlush(new ByteToMessageDecoderHelper_5("ban", str));
        }
    }

    public void do2300(String str) {
        if (is2305()) {
            this.channel.writeAndFlush(new ByteToMessageDecoderHelper_5("unban", str));
        }
    }

    public void do2301() {
        if (is2305()) {
            this.channel.writeAndFlush(new ByteToMessageDecoderHelper_5("players", "asd"));
        }
    }

    public void do1201(String str) {
        if (!is2305()) {
            do2306();
            return;
        }
        this.channel.writeAndFlush(new ByteToMessageDecoderHelper_2(str));
    }

    public void do2302() {
        if (!is2305()) {
            do2306();
            return;
        }
        this.channel.writeAndFlush(new ByteToMessageDecoderHelper_3());
    }

    public void do2303(String str, String str2, BlockPos blockPos) {
        if (!is2305()) {
            do2306();
            return;
        }
        this.channel.writeAndFlush(new ByteToMessageDecoderHelper_8(str, str2, blockPos.getX(), blockPos.getY(), blockPos.getZ()));
    }

    public void do2304(String str) {
        if (!is2305()) {
            do2306();
            return;
        }
        this.channel.writeAndFlush(new ByteToMessageDecoderHelper_4(str, minecraftClient.getSession().getUsername()));
    }

    public boolean is2305() {
        return this.channel != null && this.channel.isActive();
    }

    public void do2306() {
        try {
            MixinMessageIndicatorHelper.do345(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("\u0001You are not connected to the chat server")), MixinMessageIndicatorHelper.getMessageSignatureData337(12482345), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
        } catch (Exception e) {
        }
    }

    public List<SpawnTimeHelper> getList2307() {
        return this.list;
    }

    public Map<String, String> getMap2308() {
        return this.map;
    }

    public Identifier getIdentifier2309(String str) {
        for (IRC.Mode mode : IRC.Mode.values()) {
            if (mode.getName().equalsIgnoreCase(this.map.get(str))) {
                return mode.getIdentifier2642();
            }
        }
        return null;
    }

    public String[] getStringArray2310() {
        return this.stringArr;
    }

    public void do2311(String[] strArr) {
        this.stringArr = strArr;
    }

    public Set<String> getSet2312() {
        return this.set;
    }

    public void do2313(boolean z) {
        if (z && !this.flag) {
            String str = "Lost connection to the chat server. Retrying in 10 seconds";
            System.err.println("Lost connection to the chat server. Retrying in 10 seconds");
            BaritoneHelper_3.tooltipsSearchHelper4_2.do164(() -> {
                MixinMessageIndicatorHelper.do345(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("\u0001\u0001")), MixinMessageIndicatorHelper.getMessageSignatureData337(8345486), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
            }, 0);
        }
        this.flag = z;
        this.num3 = System.currentTimeMillis();
    }
}
