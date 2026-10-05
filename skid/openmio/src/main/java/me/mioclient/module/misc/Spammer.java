package me.mioclient.module.misc;

import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.TimeUnit;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PresetHelper;
import me.mioclient.PresetHelper_4;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import me.mioclient.module.client.IRC;
import net.minecraft.network.packet.c2s.play.ChatMessageC2SPacket;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;
import org.apache.commons.io.FileUtils;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/Spammer.class */
public class Spammer extends Module {
    public Setting<String> path;
    public Setting<Float> delay;
    public Setting<Boolean> refresh;
    public Setting<Boolean> smartDelay;
    public Setting<Boolean> random;
    public Setting<Boolean> autoOff;
    public static IRC iRC = (IRC) BaritoneHelper_3.baritoneHelper_4.getModule117(IRC.class);
    public final Random random2;
    public final List<String> list;
    public final Stopwatch stopwatch;
    public int current;

    public Spammer() {
        super("Spammer", "Spams messages from a selected text file.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        this.random2 = new Random();
        this.list = new ArrayList();
        this.stopwatch = new Stopwatch();
        this.current = 0;
        this.path.do2339(this::do466);
        this.refresh.do2339(() -> {
            if (this.refresh.getValue().booleanValue()) {
                this.refresh.do2333(false);
                do466();
            }
        });
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        do466();
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        if ((sendImmediatelyEvent.getPacket904() instanceof ChatMessageC2SPacket) && this.smartDelay.getValue().booleanValue()) {
            this.stopwatch.reset();
        }
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        int i;
        if (!this.stopwatch.is418(this.delay.getValue().floatValue(), TimeUnit.SECONDS) || this.list.isEmpty()) {
            return;
        }
        List<String> list = this.list;
        if (!this.random.getValue().booleanValue() || this.list.size() <= 1) {
            i = this.current;
        } else {
            i = this.random2.nextInt(this.list.size() - 1);
        }
        String str = list.get(i);
        if (!iRC.isToggled() || !str.startsWith(iRC.prefix.getValue())) {
            MixinMessageIndicatorHelper.do347(str);
        }
        this.current = (this.current + 1) % this.list.size();
        this.stopwatch.reset();
        if (this.autoOff.getValue().booleanValue()) {
            do496();
        }
    }

    public void do466() {
        try {
            Path path1566 = PresetHelper_4.getPath1566(PresetHelper.path3.resolve(this.path.getValue()), ".txt");
            if (!path1566.toFile().exists()) {
                throw new RuntimeException("Invalid spammer file path");
            }
            this.current = 0;
            this.list.clear();
            this.list.addAll(FileUtils.readLines(path1566.toFile(), StandardCharsets.UTF_8));
        } catch (Exception e) {
            if (is1469()) {
                return;
            }
            MixinMessageIndicatorHelper.do345(Text.literal("Failed to open spammer file").styled(style -> {
                return style.withColor(Formatting.RED);
            }), MixinMessageIndicatorHelper.getMessageSignatureData337(e.toString().hashCode()), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode3);
        }
    }
}
