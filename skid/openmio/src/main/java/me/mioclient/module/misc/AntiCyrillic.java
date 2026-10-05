package me.mioclient.module.misc;

import java.util.Map;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.feature.Event;
import me.mioclient.mixin.ducks.DuckCommandExecutionC2SPacket;
import me.mioclient.module.Module;
import net.minecraft.network.packet.c2s.play.CommandExecutionC2SPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/AntiCyrillic.class */
public class AntiCyrillic extends Module {
    public Setting<AntiCyrillicMode> mode;
    public static final Map<Character, String> map = new HashMap();

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/AntiCyrillic$AntiCyrillicMode.class */
    public enum AntiCyrillicMode implements EnumSettingHelper {
        CHAT("Chat"),
        COMMANDS("Commands"),
        BOTH("Both");

        public final String name;

        AntiCyrillicMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/AntiCyrillic$HashMap.class */
    static class HashMap extends java.util.HashMap<Character, String> {
        public HashMap() {
            put((char) 1049, "u'");
            put((char) 1081, "u'");
            put((char) 1062, "U,");
            put((char) 1094, "u,");
            put((char) 1059, "y");
            put((char) 1091, "y");
            put((char) 1050, "K");
            put((char) 1082, "K");
            put((char) 1045, "E");
            put((char) 1077, "e");
            put((char) 1053, "H");
            put((char) 1085, "H");
            put((char) 1043, "r");
            put((char) 1075, "r");
            put((char) 1064, "LLI");
            put((char) 1096, "LLI");
            put((char) 1065, "LLI,");
            put((char) 1097, "LLI,");
            put((char) 1047, "3");
            put((char) 1079, "3");
            put((char) 1061, "X");
            put((char) 1093, "X");
            put((char) 1066, "'b");
            put((char) 1098, "'b");
            put((char) 1060, "qp");
            put((char) 1092, "qp");
            put((char) 1067, "bI");
            put((char) 1099, "bI");
            put((char) 1042, "B");
            put((char) 1074, "B");
            put((char) 1040, "A");
            put((char) 1072, "a");
            put((char) 1055, "II");
            put((char) 1087, "n");
            put((char) 1056, "P");
            put((char) 1088, "p");
            put((char) 1054, "O");
            put((char) 1086, "o");
            put((char) 1051, "JI");
            put((char) 1083, "JI");
            put((char) 1044, "D");
            put((char) 1076, "D");
            put((char) 1046, ")I(");
            put((char) 1078, ")I(");
            put((char) 1069, "3");
            put((char) 1101, "3");
            put((char) 1071, "9I");
            put((char) 1103, "9I");
            put((char) 1063, "4");
            put((char) 1095, "4");
            put((char) 1057, "C");
            put((char) 1089, "c");
            put((char) 1052, "M");
            put((char) 1084, "M");
            put((char) 1048, "U");
            put((char) 1080, "u");
            put((char) 1058, "T");
            put((char) 1090, "T");
            put((char) 1068, "b");
            put((char) 1100, "b");
            put((char) 1041, "6");
            put((char) 1073, "6");
            put((char) 1070, "IO");
            put((char) 1102, "IO");
            put((char) 1025, "E");
            put((char) 1105, "E");
        }
    }

    public AntiCyrillic() {
        super("AntiCyrillic", "Replaces cyrillic letters in your messages with latin equivalents.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        try {
            return FontsSearchHelper4.getString1684(this.mode.getValue());
        } catch (Exception e) {
            return null;
        }
    }

    @Listen
    public void onDabigbulletz(Event event) {
        if (this.mode.getValue() == AntiCyrillicMode.COMMANDS) {
            return;
        }
        event.do2650(getString127(event.getString2649()));
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        CommandExecutionC2SPacket packet904 = (sendImmediatelyEvent.getPacket904()) instanceof CommandExecutionC2SPacket ? (CommandExecutionC2SPacket) (sendImmediatelyEvent.getPacket904()) : null;
        if (packet904 instanceof CommandExecutionC2SPacket) {
            CommandExecutionC2SPacket commandExecutionC2SPacket = packet904;
            if (this.mode.getValue() != AntiCyrillicMode.CHAT) {
                ((DuckCommandExecutionC2SPacket)(Object) commandExecutionC2SPacket).setCommand(getString127(commandExecutionC2SPacket.command()));
            }
        }
    }

    public String getString127(String str) {
        String str2 = str;
        if (str2.startsWith("/")) {
            String[] split = str2.split(" ");
            if (split.length > 1) {
                StringBuilder sb = new StringBuilder();
                for (int length = split.length - 1; length >= 0; length--) {
                    if (length == 0) {
                        sb.insert(0, new ArgumentTypeHelper().getArgumentTypeHelper2919(split[0]).getString2921("\u0001 "));
                    } else {
                        sb.insert(0, new ArgumentTypeHelper().getArgumentTypeHelper2919(getString128(split[length])).getString2921("\u0001 "));
                    }
                }
                str2 = sb.toString();
            }
        } else {
            str2 = getString128(str2);
        }
        if (str2.length() > 256) {
            str2 = str2.substring(0, 256);
        }
        return str2;
    }

    public String getString128(String str) {
        String str2 = str;
        for (int length = str2.length() - 1; length >= 0; length--) {
            char charAt = str2.charAt(length);
            if (map.containsKey(Character.valueOf(charAt))) {
                str2 = new ArgumentTypeHelper().getArgumentTypeHelper2919(str2.substring(length + 1)).getArgumentTypeHelper2919(map.get(Character.valueOf(charAt))).getArgumentTypeHelper2919(str2.substring(0, length)).getString2921("\u0001\u0001\u0001");
            }
        }
        return str2;
    }
}
