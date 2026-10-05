package me.mioclient.module.misc;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.TimeUnit;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ChatFilterSearchHelper4_2;
import me.mioclient.FontsEvent;
import me.mioclient.KeyPearlMode;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.AddMessageEvent;
import me.mioclient.event.DisconnectEvent;
import me.mioclient.event.Listen;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.client.network.ServerInfo;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/DiscordNotifs.class */
public class DiscordNotifs extends Module {
    public Setting<Float> timeout;
    public Setting<Boolean> timestamp;
    public Setting<Boolean> queue;
    public Setting<Boolean> disconnect;
    public Setting<Boolean> dm;
    public Setting<Boolean> chatMessages;
    public Setting<Boolean> ignoreMio;
    public final DateTimeFormatter dateTimeFormatter;
    public final List<String> list;
    public final List<String> list2;
    public final Stopwatch stopwatch;
    public String string;
    public ServerInfo serverInfo;

    public DiscordNotifs() {
        super("DiscordNotifs", "Sends chat messages to your Discord webhook.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        this.dateTimeFormatter = DateTimeFormatter.ofPattern("HH:mm");
        this.list = Collections.synchronizedList(new ArrayList());
        this.list2 = new ArrayList();
        this.stopwatch = new Stopwatch();
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        this.stopwatch.reset();
        if (BaritoneHelper_3.discordNotifsHelper.getString1200() == null || BaritoneHelper_3.discordNotifsHelper.getString1200().isEmpty()) {
            MixinMessageIndicatorHelper.do345(Text.literal("You don't have a webhook URL set. Set one by typing \"").append(new ArgumentTypeHelper().getArgumentTypeHelper2919(ChatFilterSearchHelper4_2.getString2982()).getString2921("\u0001webhook set <url>")).append("\"."), MixinMessageIndicatorHelper.getMessageSignatureData337(-10395), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
        }
    }

    @Listen
    public void onEvent(FontsEvent fontsEvent) {
        if (minecraftClient.world != null) {
            this.serverInfo = BaritoneHelper_3.holeSnapSearchHelper4_4.getServerInfo2622();
        }
        if (BaritoneHelper_3.discordNotifsHelper.getString1200() == null || BaritoneHelper_3.discordNotifsHelper.getString1200().isEmpty() || !this.stopwatch.is418(this.timeout.getValue().floatValue(), TimeUnit.SECONDS) || this.list.isEmpty()) {
            return;
        }
        this.stopwatch.reset();
        synchronized (this.list) {
            StringBuilder sb = new StringBuilder();
            String formatted = "[%s] ".formatted(this.dateTimeFormatter.format(LocalDateTime.now()));
            for (String str : this.list) {
                if (sb.length() > 1900) {
                    break;
                }
                if (this.timestamp.getValue().booleanValue()) {
                    sb.append(formatted);
                }
                sb.append(str);
                sb.append("\n");
                this.list2.add(str);
            }
            String sb2 = sb.toString();
            BaritoneHelper_3.discordNotifsHelper.do1201("```%s```".formatted(sb2.substring(0, sb2.length() - 1)));
            Iterator<String> it = this.list2.iterator();
            while (it.hasNext()) {
                this.list.remove(it.next());
            }
            this.list2.clear();
        }
    }

    @Listen
    public void onAddMessage(AddMessageEvent addMessageEvent) {
        if (addMessageEvent.is2403() || BaritoneHelper_3.discordNotifsHelper.getString1200() == null || BaritoneHelper_3.discordNotifsHelper.getString1200().isEmpty() || addMessageEvent.getKeyPearlMode1472() != KeyPearlMode.Pre || addMessageEvent.getString2283() == null) {
            return;
        }
        if (this.ignoreMio.getValue().booleanValue() && addMessageEvent.getMessageIndicator2284() == MixinMessageIndicatorHelper.messageIndicator) {
            return;
        }
        String string2283 = addMessageEvent.getString2283();
        String[] split = string2283.split("\n");
        boolean is335 = MixinMessageIndicatorHelper.is335(string2283);
        boolean is336 = MixinMessageIndicatorHelper.is336(string2283);
        if (is335 || is336) {
            if (this.dm.getValue().booleanValue()) {
                do2825(split);
            }
        } else if (!string2283.contains("Position in queue:") || split.length <= 15) {
            if (this.chatMessages.getValue().booleanValue()) {
                do2825(split);
            }
        } else if (this.string == null) {
            this.string = string2283;
        } else if (!this.string.equals(string2283) && this.queue.getValue().booleanValue()) {
            do2825(split);
            this.string = string2283;
        }
    }

    @Listen
    public void onDisconnect(DisconnectEvent disconnectEvent) {
        if (!this.disconnect.getValue().booleanValue() || this.serverInfo == null) {
            return;
        }
        do2825("You have been disconnected from %s!".formatted(this.serverInfo.address).split("\n"));
        this.serverInfo = null;
    }

    public void do2825(String[] strArr) {
        StringBuilder sb = new StringBuilder();
        for (String str : strArr) {
            if (!str.isEmpty()) {
                sb.append(Formatting.strip(str));
                sb.append("\n");
            }
        }
        String sb2 = sb.toString();
        if (sb2.length() < 2) {
            return;
        }
        this.list.add(sb2.substring(0, sb2.length() - 1));
    }
}
