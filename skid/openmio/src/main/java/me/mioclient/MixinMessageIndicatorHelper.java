package me.mioclient;

import java.awt.Color;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.function.Supplier;
import java.util.regex.Pattern;
import me.mioclient.module.Module;
import me.mioclient.module.client.Notifications;
import net.minecraft.client.gui.hud.MessageIndicator;
import net.minecraft.network.message.MessageSignatureData;
import net.minecraft.text.ClickEvent;
import net.minecraft.text.MutableText;
import net.minecraft.text.Style;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MixinMessageIndicatorHelper.class */
public class MixinMessageIndicatorHelper implements SearchHelper_4 {
    public static final Notifications notifications = (Notifications) BaritoneHelper_3.baritoneHelper_4.getModule117(Notifications.class);
    public static final MessageIndicator messageIndicator = new MessageIndicator(-1, (MessageIndicator.Icon) null, Text.empty(), "Mio");
    public static final Pattern pattern = Pattern.compile("^(\\w{3,16}) -> (\\w{3,16}): (.*)");
    public static final List<Pattern> list = List.of(Pattern.compile("^From (\\w{3,16}): (.*)"), Pattern.compile("^from (\\w{3,16}): (.*)"), Pattern.compile("^(\\w{3,16}) whispers: (.*)"), Pattern.compile("^(\\w{3,16}) -> me (.*)"), Pattern.compile("^(\\w{3,16}) whispers to you: (.*)"), Pattern.compile("^(\\w{3,16}) says: (.*)"), Pattern.compile("^(\\w{3,16}) пишет: (.*)"), pattern);
    public static final List<Pattern> list2 = List.of(Pattern.compile("^To (\\w{3,16}): (.*)"), Pattern.compile("^to (\\w{3,16}): (.*)"), Pattern.compile("^\\[me -> (\\w{3,16})\\] (.*)"), Pattern.compile("^You whisper to (\\w{3,16}): (.*)"), Pattern.compile("^К (\\w{3,16}): (.*)"), pattern);

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/MixinMessageIndicatorHelper$MixinClientConnectionMode.class */
    public static enum MixinClientConnectionMode {
        mixinClientConnectionMode {
            @Override // me.mioclient.MixinMessageIndicatorHelper.MixinClientConnectionMode
            public Color getColor1125() {
                return new Color(225, 10, 10);
            }
        },
        mixinClientConnectionMode2 {
            @Override // me.mioclient.MixinMessageIndicatorHelper.MixinClientConnectionMode
            public Color getColor1125() {
                return new Color(225, 170, 10);
            }
        },
        mixinClientConnectionMode3 {
            @Override // me.mioclient.MixinMessageIndicatorHelper.MixinClientConnectionMode
            public Color getColor1125() {
                return new Color(225, 225, 225);
            }
        };

        public Color getColor1125() {
            return null;
        }
    }

    public static boolean is335(String str) {
        if (str.toLowerCase(Locale.ROOT).startsWith(minecraftClient.getSession().getUsername().toLowerCase(Locale.ROOT))) {
            return false;
        }
        if (str.contains(" шепчет: ") || str.contains(" шепчет тебе: ")) {
            return true;
        }
        Iterator<Pattern> it = list.iterator();
        while (it.hasNext()) {
            if (it.next().matcher(str).find()) {
                return true;
            }
        }
        return false;
    }

    public static boolean is336(String str) {
        boolean startsWith = str.toLowerCase(Locale.ROOT).startsWith(minecraftClient.getSession().getUsername().toLowerCase(Locale.ROOT));
        for (Pattern pattern2 : list2) {
            if (pattern2 != pattern || startsWith) {
                if (pattern2.matcher(str).find()) {
                    return true;
                }
            }
        }
        return false;
    }

    public static MessageSignatureData getMessageSignatureData337(int i) {
        return new MessageSignatureData(ByteBuffer.allocate(256).putInt(i).array());
    }

    public static MessageSignatureData getMessageSignatureData338(Object obj) {
        return getMessageSignatureData337(Math.abs(obj.hashCode()) * (-1));
    }

    public static MessageSignatureData getMessageSignatureData339(Module module) {
        return getMessageSignatureData337(Math.abs(module.getName().hashCode()) * (-1));
    }

    public static Style getStyle340(Style style, Supplier<Integer> supplier) {
        return ((Helper_15) (Object) style.withColor(supplier.get().intValue())).mio$withColor(supplier);
    }

    public static MutableText getMutableText341(String str) {
        if (str == null) {
            return Text.empty();
        }
        return Text.literal(str).styled(style -> {
            return getStyle340(style, () -> {
                return Integer.valueOf(notifications.watermark.getValue().hashCode());
            });
        });
    }

    public static Text getText342() {
        MutableText empty = Text.empty();
        empty.append(Text.literal("[").styled(style -> {
            return getStyle340(style, () -> {
                return Integer.valueOf(notifications.getColor1247().hashCode());
            });
        }));
        empty.append(Text.literal("Mio").styled(style2 -> {
            return getStyle340(style2, () -> {
                return Integer.valueOf(notifications.getColor1246().hashCode());
            });
        }));
        empty.append(Text.literal("]").styled(style3 -> {
            return getStyle340(style3, () -> {
                return Integer.valueOf(notifications.getColor1247().hashCode());
            });
        }));
        return empty;
    }

    public static void do343(Text text, int i) {
        do344(text, getMessageSignatureData337(i));
    }

    public static void do344(Text text, MessageSignatureData messageSignatureData) {
        minecraftClient.execute(() -> {
            try {
                minecraftClient.inGameHud.getChatHud().addMessage(Text.empty().append(getText342()).append(" ").append(text), messageSignatureData, messageIndicator);
            } catch (Throwable th) {
            }
        });
    }

    public static void do345(Text text, MessageSignatureData messageSignatureData, MixinClientConnectionMode mixinClientConnectionMode) {
        MutableText empty = Text.empty();
        empty.append(Text.literal("[").styled(style -> {
            return style.withColor(mixinClientConnectionMode.getColor1125().darker().hashCode());
        }));
        empty.append(Text.literal("!").styled(style2 -> {
            return style2.withColor(mixinClientConnectionMode.getColor1125().hashCode());
        }));
        empty.append(Text.literal("] ").styled(style3 -> {
            return style3.withColor(mixinClientConnectionMode.getColor1125().darker().hashCode());
        }));
        do344(empty.append(text), messageSignatureData);
    }

    public static Text getText346(String str, Supplier<Integer> supplier, Color color) {
        MutableText empty = Text.empty();
        float[] RGBtoHSB = Color.RGBtoHSB(color.getRed(), color.getGreen(), color.getBlue(), (float[]) null);
        for (int i = 0; i < str.length(); i++) {
            int i2 = i;
            empty.append(Text.literal(String.valueOf(str.charAt(i))).styled(style -> {
                return getStyle340(style, () -> {
                    return Integer.valueOf(MixinMessageIndicatorHelper_2.getColor812(i2 * ((Integer) supplier.get()).intValue(), RGBtoHSB[1], RGBtoHSB[2], 255).hashCode());
                });
            }));
        }
        return empty;
    }

    public static void do347(String str) {
        if (!str.startsWith("/")) {
            minecraftClient.player.networkHandler.sendChatMessage(str);
        } else {
            minecraftClient.player.networkHandler.sendChatCommand(str.substring(1));
        }
    }

    public static ClickEvent getClickEvent348(String str) {
        return new ClickEvent(ClickEvent.Action.SUGGEST_COMMAND, new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getArgumentTypeHelper2919(ChatFilterSearchHelper4_2.getString2982()).getString2921("\u0001\u0001"));
    }

    public static Text getText349(String str) {
        return Text.empty().append(Text.literal("[").formatted(Formatting.GRAY)).append(str).append(Text.literal("]").formatted(Formatting.GRAY));
    }
}
