package me.mioclient;

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.feature.Stopwatch;
import net.minecraft.command.CommandSource;
import net.minecraft.network.packet.c2s.play.RequestCommandCompletionsC2SPacket;
import net.minecraft.network.packet.s2c.play.CommandSuggestionsS2CPacket;
import net.minecraft.text.Text;
import net.minecraft.text.Texts;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/StopwatchFeature.class */
public final class StopwatchFeature extends Feature {
    public final Stopwatch stopwatch;
    public boolean flag;

    public StopwatchFeature() {
        super("plugins");
        this.stopwatch = new Stopwatch();
        baritoneHelper.do1796(this);
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.executes(commandContext -> {
            minecraftClient.player.networkHandler.sendPacket(new RequestCommandCompletionsC2SPacket(0, "/"));
            MixinMessageIndicatorHelper.do344(Text.literal("Fetching plugins..."), MixinMessageIndicatorHelper.getMessageSignatureData337(-3));
            this.stopwatch.reset();
            this.flag = true;
            return 1;
        });
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        if (this.flag) {
            if (this.stopwatch.is419(3000L)) {
                this.flag = false;
                List list = minecraftClient.player.networkHandler.getCommandDispatcher().getRoot().getChildren().stream().map((v0) -> {
                    return v0.getName();
                }).toList();
                if (list.isEmpty()) {
                    MixinMessageIndicatorHelper.do344(Text.literal("Couldn't get plugins"), MixinMessageIndicatorHelper.getMessageSignatureData337(-3));
                } else {
                    MixinMessageIndicatorHelper.do344(getText600(list), MixinMessageIndicatorHelper.getMessageSignatureData337(-3));
                }
            }
            CommandSuggestionsS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof CommandSuggestionsS2CPacket ? (CommandSuggestionsS2CPacket) (channelRead0Event.getPacket904()) : null;
            if (packet904 instanceof CommandSuggestionsS2CPacket) {
                this.flag = false;
                MixinMessageIndicatorHelper.do344(getText600(packet904.getSuggestions().getList().stream().map((v0) -> {
                    return v0.getText();
                }).toList()), MixinMessageIndicatorHelper.getMessageSignatureData337(-3));
            }
        }
    }

    public Text getText600(Iterable<String> iterable) {
        HashSet<String> hashSet = new HashSet<>();
        Iterator<String> it = iterable.iterator();
        while (it.hasNext()) {
            String[] split = it.next().split(":");
            if (split.length > 1 && !split[0].isEmpty()) {
                hashSet.add(split[0]);
            }
        }
        return Text.empty().append("Found plugins [").append(Text.literal(String.valueOf(hashSet.size())).formatted(Formatting.GRAY)).append("]: ").append(Texts.join(hashSet, str -> {
            return Text.literal(str).styled(style -> {
                return MixinMessageIndicatorHelper.getStyle340(style, () -> {
                    return Integer.valueOf(MixinMessageIndicatorHelper_2.getColor811().hashCode());
                });
            });
        }));
    }
}
