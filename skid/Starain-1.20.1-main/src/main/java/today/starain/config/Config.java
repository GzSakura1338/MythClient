package today.starain.config;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import today.starain.Starain;
import today.starain.module.ClientModule;
import today.starain.utils.client.ChatUtils;
import today.starain.utils.wrapper.Wrapper;
import today.starain.value.Value;
import today.starain.value.impl.*;

import java.io.BufferedWriter;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.List;

public final class Config implements Wrapper {
    private final String name;
    private final File file;
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    public Config(String name) {
        this.name = name;
        this.file = new File(Wrapper.resolveConfigFolder(), name + ".json");
        File parent = file.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }
    }

    private static int parseColorArgb(JsonElement element, int fallback) {
        try {
            if (element.isJsonPrimitive() && element.getAsJsonPrimitive().isNumber()) {
                return element.getAsInt();
            }
            String text = element.getAsString();
            String normalized = text.startsWith("#") ? text.substring(1) : text;
            if (normalized.startsWith("0x") || normalized.startsWith("0X")) {
                normalized = normalized.substring(2);
            }
            if (normalized.length() == 6) {
                return 0xFF000000 | Integer.parseInt(normalized, 16);
            }
            if (normalized.length() == 8) {
                return (int) Long.parseLong(normalized, 16);
            }
        } catch (Exception ignored) {
        }
        return fallback;
    }

    public void save() {
        try {
            List<ClientModule> modules = Starain.getInstance().getModuleManager().getModules();
            JsonObject root = new JsonObject();
            root.addProperty("time", System.currentTimeMillis());

            JsonObject modulesJson = new JsonObject();
            for (ClientModule module : modules) {
                JsonObject moduleJson = new JsonObject();
                moduleJson.addProperty("enabled", module.isEnabled());
                moduleJson.addProperty("key", module.getKey());

                JsonObject valuesJson = new JsonObject();
                for (Value value : module.getValues()) {
                    if (value instanceof BooleanValue booleanValue) {
                        valuesJson.addProperty(value.getName(), booleanValue.get());
                    } else if (value instanceof ModeValue modeValue) {
                        valuesJson.addProperty(value.getName(), modeValue.get());
                    } else if (value instanceof NumberValue numberValue) {
                        if (numberValue.isRange()) {
                            JsonObject rangeJson = new JsonObject();
                            rangeJson.addProperty("min", numberValue.getMinValue());
                            rangeJson.addProperty("max", numberValue.getMaxValue());
                            valuesJson.add(value.getName(), rangeJson);
                        } else {
                            valuesJson.addProperty(value.getName(), numberValue.getValue());
                        }
                    } else if (value instanceof TextValue textValue) {
                        valuesJson.addProperty(value.getName(), textValue.get());
                    } else if (value instanceof ColorValue colorValue) {
                        valuesJson.addProperty(value.getName(), colorValue.getArgb());
                    } else if (value instanceof DragValue dragValue) {
                        JsonObject dragJson = new JsonObject();
                        dragJson.addProperty("x", dragValue.getX());
                        dragJson.addProperty("y", dragValue.getY());
                        dragJson.addProperty("width", dragValue.getWidth());
                        dragJson.addProperty("height", dragValue.getHeight());
                        valuesJson.add(value.getName(), dragJson);
                    }
                }

                moduleJson.add("values", valuesJson);
                modulesJson.add(module.getName(), moduleJson);
            }

            root.add("modules", modulesJson);

            if (!file.exists()) {
                file.createNewFile();
            }

            try (BufferedWriter writer = Files.newBufferedWriter(file.toPath(), StandardCharsets.UTF_8)) {
                writer.write(gson.toJson(root));
                writer.newLine();
            }
        } catch (Exception exception) {
            ChatUtils.addChatMessage("Failed to save config: ", name);
            ChatUtils.addChatMessage(exception.getMessage() == null ? exception.getClass().getSimpleName() : exception.getMessage());
            exception.printStackTrace();
        }
    }

    public void load() {
        if (!file.exists()) {
            save();
            return;
        }

        try {
            JsonObject root = gson.fromJson(Files.readString(file.toPath()).trim(), JsonObject.class);
            if (root == null || !root.has("modules")) {
                return;
            }

            JsonObject modulesJson = root.getAsJsonObject("modules");
            for (ClientModule module : Starain.getInstance().getModuleManager().getModules()) {
                JsonObject moduleJson = modulesJson.has(module.getName()) ? modulesJson.getAsJsonObject(module.getName()) : null;
                if (moduleJson == null) {
                    continue;
                }

                if (moduleJson.has("key")) {
                    module.setKey(moduleJson.get("key").getAsInt());
                }

                if (moduleJson.has("enabled")) {
                    module.setEnabled(moduleJson.get("enabled").getAsBoolean());
                }

                JsonObject valuesJson = moduleJson.has("values") ? moduleJson.getAsJsonObject("values") : null;
                if (valuesJson == null) {
                    continue;
                }

                for (Value value : module.getValues()) {
                    JsonElement element = valuesJson.get(value.getName());
                    if (element == null || element.isJsonNull()) {
                        continue;
                    }

                    if (value instanceof BooleanValue booleanValue) {
                        booleanValue.set(element.getAsBoolean());
                    } else if (value instanceof ModeValue modeValue) {
                        modeValue.set(element.getAsString());
                    } else if (value instanceof NumberValue numberValue) {
                        if (numberValue.isRange() && element.isJsonObject()) {
                            JsonObject rangeJson = element.getAsJsonObject();
                            if (rangeJson.has("min")) {
                                numberValue.setMinValue(rangeJson.get("min").getAsDouble());
                            }
                            if (rangeJson.has("max")) {
                                numberValue.setMaxValue(rangeJson.get("max").getAsDouble());
                            }
                        } else {
                            numberValue.setValue(element.getAsDouble());
                        }
                    } else if (value instanceof TextValue textValue) {
                        textValue.set(element.getAsString());
                    } else if (value instanceof ColorValue colorValue) {
                        colorValue.setArgb(parseColorArgb(element, colorValue.getArgb()));
                    } else if (value instanceof DragValue dragValue && element.isJsonObject()) {
                        JsonObject dragJson = element.getAsJsonObject();
                        if (dragJson.has("x")) {
                            dragValue.setX(dragJson.get("x").getAsFloat());
                        }
                        if (dragJson.has("y")) {
                            dragValue.setY(dragJson.get("y").getAsFloat());
                        }
                        if (dragJson.has("width")) {
                            dragValue.setWidth(dragJson.get("width").getAsFloat());
                        }
                        if (dragJson.has("height")) {
                            dragValue.setHeight(dragJson.get("height").getAsFloat());
                        }
                    }
                }
            }
        } catch (Exception exception) {
            ChatUtils.addChatMessage("Failed to load config: ", name);
            ChatUtils.addChatMessage(exception.getMessage() == null ? exception.getClass().getSimpleName() : exception.getMessage());
            exception.printStackTrace();
        }
    }
}
