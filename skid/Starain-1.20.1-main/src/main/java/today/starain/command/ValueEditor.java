package today.starain.command;

import today.starain.module.ClientModule;
import today.starain.value.Value;
import today.starain.value.impl.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

/**
 * 聊天里读写模块参数。参数名带空格（"Glow Radius"）没法直接当一个词打，
 * 所以匹配一律走 {@link #normalize} 去掉非字母数字再比。
 */
public final class ValueEditor {
    private ValueEditor() {
    }

    public static String normalize(String text) {
        StringBuilder builder = new StringBuilder();
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            if (Character.isLetterOrDigit(c)) {
                builder.append(Character.toLowerCase(c));
            }
        }
        return builder.toString();
    }

    /**
     * 补全用的紧凑名："Glow Radius" -> "GlowRadius"
     */
    public static String compactName(Value value) {
        return value.getName().replace(" ", "");
    }

    public static Value find(ClientModule module, String name) {
        String wanted = normalize(name);
        for (Value value : module.getValues()) {
            if (normalize(value.getName()).equals(wanted)) {
                return value;
            }
        }
        return null;
    }

    public static List<String> valueNames(ClientModule module) {
        List<String> names = new ArrayList<>();
        for (Value value : module.getValues()) {
            names.add(compactName(value));
        }
        return names;
    }

    public static List<String> suggestionsFor(Value value) {
        if (value instanceof BooleanValue) {
            return List.of("true", "false", "toggle");
        }
        if (value instanceof ModeValue modeValue) {
            List<String> modes = new ArrayList<>(Arrays.asList(modeValue.getModes()));
            modes.add("next");
            return modes;
        }
        if (value instanceof NumberValue numberValue) {
            return List.of(trim(numberValue.getMin()), trim(numberValue.getMax()));
        }
        if (value instanceof ColorValue) {
            return List.of("#FFFFFFFF", "255 255 255 255");
        }
        return List.of();
    }

    public static String describe(Value value) {
        if (value instanceof BooleanValue booleanValue) {
            return String.valueOf(booleanValue.get());
        }
        if (value instanceof ModeValue modeValue) {
            return modeValue.get();
        }
        if (value instanceof NumberValue numberValue) {
            return numberValue.isRange()
                    ? trim(numberValue.getMinValue()) + " - " + trim(numberValue.getMaxValue())
                    : trim(numberValue.getValue());
        }
        if (value instanceof ColorValue colorValue) {
            return String.format("#%08X", colorValue.getArgb());
        }
        if (value instanceof TextValue textValue) {
            return textValue.get();
        }
        return "?";
    }

    /**
     * @return 成功时返回改完之后的值（给玩家回显），输入非法时返回 null
     */
    public static String apply(Value value, String[] args, int from) {
        if (value instanceof BooleanValue booleanValue) {
            return applyBoolean(booleanValue, args[from]);
        }
        if (value instanceof ModeValue modeValue) {
            return applyMode(modeValue, args[from]);
        }
        if (value instanceof NumberValue numberValue) {
            return applyNumber(numberValue, args, from);
        }
        if (value instanceof ColorValue colorValue) {
            Integer argb = parseColor(args, from);
            if (argb == null) {
                return null;
            }
            colorValue.setArgb(argb);
            return describe(colorValue);
        }
        if (value instanceof TextValue textValue) {
            textValue.set(String.join(" ", Arrays.copyOfRange(args, from, args.length)));
            return textValue.get();
        }
        return null;
    }

    private static String applyBoolean(BooleanValue value, String input) {
        switch (input.toLowerCase(Locale.ROOT)) {
            case "true", "on", "yes", "1" -> value.set(true);
            case "false", "off", "no", "0" -> value.set(false);
            case "toggle" -> value.toggle();
            default -> {
                return null;
            }
        }
        return String.valueOf(value.get());
    }

    private static String applyMode(ModeValue value, String input) {
        if (input.equalsIgnoreCase("next")) {
            value.next();
            return value.get();
        }
        for (String mode : value.getModes()) {
            if (normalize(mode).equals(normalize(input))) {
                value.set(mode);
                return value.get();
            }
        }
        return null;
    }

    private static String applyNumber(NumberValue value, String[] args, int from) {
        try {
            if (value.isRange()) {
                // range 型有上下界两个值，只给一个数没法确定改哪个。
                if (args.length - from < 2) {
                    return null;
                }
                value.setMinValue(Double.parseDouble(args[from]));
                value.setMaxValue(Double.parseDouble(args[from + 1]));
            } else {
                value.setValue(Double.parseDouble(args[from]));
            }
        } catch (NumberFormatException exception) {
            return null;
        }
        return describe(value);
    }

    private static Integer parseColor(String[] args, int from) {
        if (args.length - from >= 3) {
            try {
                int r = Integer.parseInt(args[from]);
                int g = Integer.parseInt(args[from + 1]);
                int b = Integer.parseInt(args[from + 2]);
                int a = args.length - from >= 4 ? Integer.parseInt(args[from + 3]) : 255;
                return ((a & 255) << 24) | ((r & 255) << 16) | ((g & 255) << 8) | (b & 255);
            } catch (NumberFormatException ignored) {
                return null;
            }
        }

        String hex = args[from];
        if (hex.startsWith("#")) {
            hex = hex.substring(1);
        } else if (hex.startsWith("0x") || hex.startsWith("0X")) {
            hex = hex.substring(2);
        }
        try {
            if (hex.length() == 6) {
                return 0xFF000000 | Integer.parseInt(hex, 16);
            }
            if (hex.length() == 8) {
                return (int) Long.parseLong(hex, 16);
            }
        } catch (NumberFormatException ignored) {
            return null;
        }
        return null;
    }

    // 15.0 -> "15"，3.5 还是 "3.5"，免得聊天里全是没意义的小数点
    private static String trim(Number number) {
        double d = number.doubleValue();
        return d == Math.floor(d) && !Double.isInfinite(d)
                ? String.valueOf((long) d)
                : String.valueOf(d);
    }
}
