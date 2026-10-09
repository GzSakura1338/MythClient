package dev.myth.UI.setting;

import com.google.gson.JsonElement;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import dev.myth.Myth;
import java.awt.Color;
import java.util.List;
import java.util.Objects;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;

/** Values consumed by the Mio-style controls. This does not register a feature. */
public final class Setting<T> {
    public enum Kind { BOOLEAN, NUMBER, STRING, ENUM, COLOR, SOUND, SET }

    public record State(Object value, boolean expanded, boolean rainbow, boolean sync) {}

    private final String id;
    private final String name;
    private final Kind kind;
    private final T defaultValue;
    private T value;
    private double min;
    private double max;
    private double step = .1;
    private List<T> choices = List.of();
    private List<String> entries = List.of();
    private BooleanSupplier visible = () -> true;
    private Consumer<T> changed = ignored -> {};
    private String description = "";
    private String unit = "";
    private boolean group;
    private boolean action;
    private boolean expanded;
    private boolean rainbow;
    private boolean sync;
    private boolean alpha = true;
    private boolean allowSync;

    private Setting(String id, String name, Kind kind, T value) {
        if (!id.matches("[a-zA-Z0-9_]+")) throw new IllegalArgumentException("Invalid setting id");
        this.id = id;
        this.name = Objects.requireNonNull(name);
        this.kind = kind;
        this.defaultValue = this.value = Objects.requireNonNull(value);
    }

    public static Setting<Boolean> bool(String id, String name, boolean value) {
        return new Setting<>(id, name, Kind.BOOLEAN, value);
    }

    public static Setting<Integer> integer(String id, String name, int value, int min, int max) {
        var setting = new Setting<>(id, name, Kind.NUMBER, value);
        setting.min = min; setting.max = max;
        setting.step = 1;
        return setting;
    }

    public static Setting<Float> decimal(String id, String name, float value, float min, float max) {
        var setting = new Setting<>(id, name, Kind.NUMBER, value);
        setting.min = min; setting.max = max;
        return setting;
    }

    public static Setting<String> text(String id, String name, String value) {
        return new Setting<>(id, name, Kind.STRING, value);
    }

    public static <T> Setting<T> choice(String id, String name, T value, List<T> choices) {
        var setting = new Setting<>(id, name, Kind.ENUM, value);
        setting.choices = List.copyOf(choices);
        if (!setting.choices.contains(value)) throw new IllegalArgumentException("Invalid default choice");
        return setting;
    }

    public static Setting<Integer> color(String id, String name, int argb, boolean alpha, boolean defaultSync) {
        var setting = new Setting<>(id, name, Kind.COLOR, argb);
        setting.alpha = alpha; setting.allowSync = true; setting.sync = defaultSync;
        return setting;
    }

    public static Setting<String> sound(String id, String name, String value) {
        return new Setting<>(id, name, Kind.SOUND, value);
    }

    public static Setting<List<String>> selection(String id, String name, List<String> selected, List<String> entries) {
        var setting = new Setting<List<String>>(id, name, Kind.SET, List.copyOf(selected));
        setting.entries = List.copyOf(entries);
        if (!setting.entries.containsAll(selected)) throw new IllegalArgumentException("Unknown selection");
        return setting;
    }
    public List<String> getEntries() { return entries; }

    public Setting<T> visibleWhen(BooleanSupplier visible) { this.visible = visible; return this; }
    public Setting<T> onChange(Consumer<T> changed) { this.changed = changed; return this; }
    public Setting<T> description(String description) { this.description = description; return this; }
    public Setting<T> unit(String unit) { this.unit = unit; return this; }
    public Setting<T> step(double step) {
        if (!Double.isFinite(step) || step <= 0) throw new IllegalArgumentException("Invalid number step");
        this.step = step; return this;
    }
    /** Only the boolean control reacts to a right click by expanding, so a group has to be one. */
    public Setting<T> group() {
        if (kind != Kind.BOOLEAN) throw new IllegalArgumentException("Only a boolean setting can be a group: " + id);
        group = true; return this;
    }
    public Setting<T> action() { action = true; return this; }
    public String getId() { return id; }
    public String getName() { return name; }
    public String getDescription() { return description; }
    public String getUnit() { return unit; }
    public Kind getKind() { return kind; }
    public T get() { return value; }
    public double getMin() { return min; }
    public double getMax() { return max; }
    public double getStep() { return step; }
    public List<T> getChoices() { return choices; }
    public boolean isVisible() { return visible.getAsBoolean(); }
    public boolean isGroup() { return group; }
    public boolean isAction() { return action; }
    public boolean isExpanded() { return expanded; }
    public void setExpanded(boolean expanded) { this.expanded = expanded; }
    public boolean hasAlpha() { return alpha; }
    public boolean canSync() { return allowSync; }
    public boolean isRainbow() { return rainbow; }
    public boolean isSync() { return sync; }
    public void setRainbow(boolean rainbow) { this.rainbow = rainbow; }
    public void setSync(boolean sync) { this.sync = allowSync && sync; }

    public void set(T value) {
        T normalized = normalize(Objects.requireNonNull(value));
        if (this.value.equals(normalized) && !action) return;
        T previous = this.value;
        this.value = normalized;
        try { changed.accept(normalized); } catch (RuntimeException exception) { this.value = previous; throw exception; }
        if (action) this.value = defaultValue;
    }

    @SuppressWarnings("unchecked")
    private T normalize(Object input) {
        if (kind == Kind.SET) {
            if (!(input instanceof List<?> list) || list.stream().anyMatch(value -> !(value instanceof String) || !entries.contains(value)))
                throw new IllegalArgumentException("Unknown selection");
            return (T) List.copyOf(list.stream().distinct().toList());
        }
        if (kind == Kind.NUMBER) {
            double number = ((Number) input).doubleValue();
            if (!Double.isFinite(number)) throw new IllegalArgumentException("Non-finite number");
            number = Math.clamp(number, min, max);
            if (defaultValue instanceof Integer) return (T) Integer.valueOf((int) Math.round(number));
            return (T) Float.valueOf((float) number);
        }
        if (!defaultValue.getClass().isInstance(input)) throw new IllegalArgumentException("Invalid value type");
        if (kind == Kind.ENUM && !choices.contains(input)) throw new IllegalArgumentException("Unknown choice");
        if (kind == Kind.COLOR && !alpha) input = ((Integer) input) | 0xff000000;
        if ((kind == Kind.STRING || kind == Kind.SOUND) && ((String) input).length() > 4096)
            throw new IllegalArgumentException("Value is too long");
        return (T) input;
    }

    @SuppressWarnings("unchecked")
    public void setNumber(double number) { set((T) normalize(number)); }
    public State snapshot() { return new State(value, expanded, rainbow, sync); }

    @SuppressWarnings("unchecked")
    public void apply(State state) {
        set((T) state.value());
        expanded = state.expanded(); rainbow = state.rainbow(); sync = allowSync && state.sync();
    }

    public JsonElement toJson() {
        JsonObject data = new JsonObject();
        Object stored = action ? defaultValue : value;
        if (kind == Kind.SET) {
            JsonArray selected = new JsonArray();
            for (Object item : (List<?>) stored) selected.add(item.toString());
            data.add("value", selected);
        } else data.add("value", stored instanceof Boolean b ? new JsonPrimitive(b)
                : stored instanceof Number n ? new JsonPrimitive(n) : new JsonPrimitive(stored.toString()));
        data.addProperty("expanded", expanded);
        if (kind == Kind.COLOR) {
            data.addProperty("rainbow", rainbow); data.addProperty("sync", sync);
        }
        return data;
    }

    /**
     * One unreadable value must not cost the whole profile. A setting's value is isolated state,
     * so the caller skips it and keeps the live one; returning null says exactly that. Schema
     * drift lands here: a saved enum choice that no longer exists is not a corrupt config.
     */
    public State decodeOrKeep(JsonElement element, String owner) {
        try {
            return decode(element);
        } catch (RuntimeException exception) {
            Myth.LOGGER.warn("Myth config kept {}.{} = {} ({})", owner, id, value, exception.getMessage());
            return null;
        }
    }

    public State decode(JsonElement element) {
        JsonObject data = element.getAsJsonObject();
        if (kind == Kind.SET) {
            var selected = new java.util.ArrayList<String>();
            for (JsonElement item : data.getAsJsonArray("value")) {
                if (!item.isJsonPrimitive() || !item.getAsJsonPrimitive().isString()) throw new IllegalArgumentException("Expected selection name");
                selected.add(item.getAsString());
            }
            return new State(normalize(selected), flag(data, "expanded", expanded), false, false);
        }
        JsonPrimitive raw = data.getAsJsonPrimitive("value");
        Object next = switch (kind) {
            case BOOLEAN -> {
                if (!raw.isBoolean()) throw new IllegalArgumentException("Expected boolean");
                yield raw.getAsBoolean();
            }
            case NUMBER, COLOR -> {
                if (!raw.isNumber()) throw new IllegalArgumentException("Expected number");
                yield kind == Kind.COLOR ? Integer.valueOf(raw.getAsInt()) : normalize(raw.getAsDouble());
            }
            case ENUM -> choices.stream().filter(choice -> choice.toString().equals(raw.getAsString()))
                    .findFirst().orElseThrow(() -> new IllegalArgumentException("Unknown choice"));
            case STRING, SOUND -> {
                if (!raw.isString()) throw new IllegalArgumentException("Expected string");
                yield raw.getAsString();
            }
            case SET -> throw new IllegalStateException("Selection handled above");
        };
        return new State(action ? defaultValue : normalize(next), flag(data, "expanded", expanded),
                flag(data, "rainbow", rainbow), flag(data, "sync", sync));
    }

    private static boolean flag(JsonObject data, String key, boolean fallback) {
        if (!data.has(key)) return fallback;
        if (!data.get(key).isJsonPrimitive() || !data.getAsJsonPrimitive(key).isBoolean())
            throw new IllegalArgumentException("Expected boolean: " + key);
        return data.get(key).getAsBoolean();
    }

    public int color(int themeColor) {
        int argb = (Integer) value;
        if (sync) return (argb & 0xff000000) | (themeColor & 0xffffff);
        if (rainbow) {
            float[] hsv = Color.RGBtoHSB(argb >> 16 & 255, argb >> 8 & 255, argb & 255, null);
            return (argb & 0xff000000) | (Color.HSBtoRGB((System.currentTimeMillis() % 6000) / 6000f,
                    hsv[1], hsv[2]) & 0xffffff);
        }
        return argb;
    }
}
