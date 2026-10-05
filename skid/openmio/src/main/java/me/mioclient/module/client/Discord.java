package me.mioclient.module.client;

import com.jagrosh.discordipc.IPCClient;
import com.jagrosh.discordipc.entities.DiscordBuild;
import com.jagrosh.discordipc.entities.RichPresence;
import java.util.concurrent.CompletableFuture;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FontsEvent;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.client.network.ServerInfo;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/client/Discord.class */
public class Discord extends Module {
    public Setting<Boolean> uid;
    public Setting<Boolean> server;
    public CompletableFuture<IPCClient> completableFuture;
    public long num;
    public final Stopwatch stopwatch;

    public Discord() {
        super("Discord", "Displays mioclient.me as your discord activity.", Category.CLIENT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.num = System.currentTimeMillis();
        this.stopwatch = new Stopwatch();
        setDrawn(false);
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        IPCClient iPCClient = new IPCClient(1162833254580228176L);
        this.completableFuture = CompletableFuture.supplyAsync(() -> {
            try {
                iPCClient.connect(new DiscordBuild[0]);
                return iPCClient;
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }).exceptionally(th -> {
            if (!isToggled()) {
                return null;
            }
            do496();
            return null;
        });
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        if (!this.completableFuture.isDone() || this.completableFuture.isCancelled()) {
            return;
        }
        try {
            this.completableFuture.get().close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Listen
    public void onEvent(FontsEvent fontsEvent) {
        try {
            if (this.completableFuture.isDone() && this.stopwatch.is419(2500L)) {
                this.stopwatch.reset();
                IPCClient now = this.completableFuture.getNow(null);
                if (now == null) {
                    return;
                }
                RichPresence.Builder builder = new RichPresence.Builder();
                builder.setLargeImage("https://mioclient.me/assets/dcrpc3.png", getString2798());
                builder.setStartTimestamp(this.num);
                if (this.uid.getValue().booleanValue()) {
                    builder.setDetails(new ArgumentTypeHelper().getArgumentTypeHelper2906(BaritoneHelper_3.welcomerHelper.get2811()).getString2921("uid \u0001"));
                }
                if (this.server.getValue().booleanValue()) {
                    ServerInfo serverInfo = null;
                    String str = "Not playing";
                    if (!is1469()) {
                        serverInfo = minecraftClient.player.networkHandler.getServerInfo();
                        str = new ArgumentTypeHelper().getArgumentTypeHelper2919((minecraftClient.isInSingleplayer() || serverInfo == null) ? "singleplayer" : serverInfo.address).getString2921("Playing \u0001");
                    }
                    builder.setState(str);
                    if (serverInfo != null && serverInfo.players != null) {
                        builder.setParty(Integer.toString(str.hashCode()), serverInfo.players.online(), serverInfo.players.max(), 0);
                    }
                }
                now.sendRichPresence(builder.build());
            }
        } catch (Exception e) {
        }
    }

    public String getString2798() {
        return "Mio v2.1.7";
    }
}
