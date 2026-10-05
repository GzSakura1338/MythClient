package me.mioclient.module.misc;

import java.awt.Color;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Objects;
import java.util.function.Supplier;
import java.util.regex.Pattern;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingHelper;
import me.mioclient.HoleSnapMode;
import me.mioclient.KeyPearlMode;
import me.mioclient.MatrixStackEvent_2;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchIdentifier;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.AddMessageEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.feature.Event;
import me.mioclient.feature.Progress;
import me.mioclient.module.Module;
import net.minecraft.client.gui.hud.MessageIndicator;
import net.minecraft.network.packet.c2s.play.ChatMessageC2SPacket;
import net.minecraft.network.packet.c2s.play.CommandExecutionC2SPacket;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;
import net.minecraft.util.math.MathHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/BetterChat.class */
public class BetterChat extends Module {
    public static BetterChat betterChat;
    public Setting<Boolean> timestamps;
    public Setting<Boolean> onlyServer;
    public Setting<BetterChatMode> format;
    public Setting<Color> textColor;
    public Setting<Color> bracketsColor;
    public Setting<Boolean> rainbow;
    public Setting<Integer> rainbowAmount;
    public Setting<String> leftBracket;
    public Setting<String> rightBracket;
    public Setting<Boolean> mentionSound;
    public Setting<SearchIdentifier> soundMode;
    public Setting<Float> volume;
    public Setting<Boolean> animation;
    public Setting<BetterChatMode_2> type;
    public Setting<Float> speed;
    public Setting<Integer> alpha;
    public Setting<Boolean> always;
    public Setting<Boolean> customSuffix;
    public Setting<String> suffix;
    public Setting<Boolean> highlight;
    public Setting<Color> color;
    public Setting<Boolean> longChatHistory;
    public Setting<Boolean> noReset;
    public final Progress progress;
    public boolean flag3;
    public static final Pattern pattern = Pattern.compile("^\\d+$");
    public static boolean flag = false;
    public static boolean flag2 = false;
    public static PMSound pMSound = (PMSound) BaritoneHelper_3.baritoneHelper_4.getModule117(PMSound.class);

    /* JADX INFO: Access modifiers changed from: package-private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/BetterChat$BetterChatMode.class */
    public enum BetterChatMode implements EnumSettingHelper {
        HOURS(new SimpleDateFormat("HH"), "Hours"),
        MINUTES(new SimpleDateFormat("HH:mm"), "Minutes"),
        SECONDS(new SimpleDateFormat("HH:mm:ss"), "Seconds");

        public final DateFormat dateFormat;
        public final String name;

        BetterChatMode(DateFormat dateFormat, String str) {
            this.dateFormat = dateFormat;
            this.name = str;
        }

        public DateFormat getDateFormat628() {
            return this.dateFormat;
        }

        public String getString629(Date date) {
            return this.dateFormat.format(date);
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/BetterChat$BetterChatMode_2.class */
    public enum BetterChatMode_2 implements EnumSettingHelper {
        VERTICAL("Vertical"),
        HORIZONTAL("Horizontal"),
        BOTH("Both"),
        BOUNCE("Bounce"),
        NONE("None");

        public final String name;

        BetterChatMode_2(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public BetterChat() {
        super("BetterChat", "Allows you to customize your chat as you please.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        Setting<Float> setting = this.speed;
        Objects.requireNonNull(setting);
        this.progress = new Progress((Supplier<Float>) setting::getValue, true);
        this.alpha.getSetting2338("None", HoleSnapMode.MIN);
        betterChat = this;
        setDrawn(false);
        this.color.do2329("HighlightColor");
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        if (sendImmediatelyEvent.is2403()) {
            return;
        }
        if ((sendImmediatelyEvent.getPacket904() instanceof ChatMessageC2SPacket) || (sendImmediatelyEvent.getPacket904() instanceof CommandExecutionC2SPacket)) {
            this.flag3 = true;
        }
    }

    @Listen
    public void onAddMessage(AddMessageEvent addMessageEvent) {
        if (addMessageEvent.getKeyPearlMode1472() == KeyPearlMode.Pre) {
            do674(addMessageEvent);
            do673(addMessageEvent);
            do675();
            this.flag3 = false;
        }
    }

    @Listen
    public void onDabigbulletz(Event event) {
        String string2649 = event.getString2649();
        if (!pattern.matcher(string2649).matches() && this.customSuffix.getValue().booleanValue()) {
            String string2921 = new ArgumentTypeHelper().getArgumentTypeHelper2919(this.suffix.getValue()).getString2921(" \u0001");
            event.do2650(new ArgumentTypeHelper().getArgumentTypeHelper2919(string2921.substring(0, MathHelper.clamp(256 - string2649.length(), 0, string2921.length()))).getArgumentTypeHelper2919(string2649).getString2921("\u0001\u0001"));
        }
    }

    @Listen
    public void onEvent(MatrixStackEvent_2 matrixStackEvent_2) {
        this.progress.do2139(false);
        this.progress.get172();
    }

    public void do673(AddMessageEvent addMessageEvent) {
        if (this.timestamps.getValue().booleanValue()) {
            if (is676(addMessageEvent.getMessageIndicator2284()) || !this.onlyServer.getValue().booleanValue()) {
                MutableText append = Text.empty().append(Text.literal(this.leftBracket.getValue()).styled(style -> {
                    return style.withColor(this.bracketsColor.getValue().hashCode());
                })).append(Text.literal(this.format.getValue().getString629(new Date())).styled(style2 -> {
                    return style2.withColor(this.textColor.getValue().hashCode());
                })).append(Text.literal(this.rightBracket.getValue()).styled(style3 -> {
                    return style3.withColor(this.bracketsColor.getValue().hashCode());
                })).append(" ");
                if (this.rainbow.getValue().booleanValue()) {
                    append = Text.empty().append(MixinMessageIndicatorHelper.getText346(append.getString(), () -> {
                        return Integer.valueOf(this.rainbowAmount.getValue().intValue() * 10);
                    }, this.textColor.getValue()));
                }
                addMessageEvent.do2280(append.append(addMessageEvent.getText2279()));
            }
        }
    }

    public void do674(AddMessageEvent addMessageEvent) {
        if (this.mentionSound.getValue().booleanValue()) {
            String lowerCase = addMessageEvent.getText2279().getString().toLowerCase();
            if ((pMSound.isToggled() && MixinMessageIndicatorHelper.is335(lowerCase)) || this.flag3 || !lowerCase.contains(minecraftClient.getSession().getUsername().toLowerCase())) {
                return;
            }
            BaritoneHelper_3.searchHelper4_11.getSearchHelper4_42970(this.soundMode.getValue()).do1820(this.volume.getValue().floatValue());
        }
    }

    public void do675() {
        if (is677() && this.type.getValue() == BetterChatMode_2.BOUNCE) {
            this.progress.do2140(true);
        }
    }

    public boolean is676(MessageIndicator messageIndicator) {
        return messageIndicator == MessageIndicator.system() || messageIndicator == MessageIndicator.singlePlayer() || messageIndicator == MessageIndicator.notSecure();
    }

    public boolean is677() {
        return isToggled() && this.animation.getValue().booleanValue();
    }

    public static BetterChat getBetterChat678() {
        return betterChat;
    }
}
