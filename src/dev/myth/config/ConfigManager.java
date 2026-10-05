package dev.myth.config;

import com.google.gson.JsonObject;
import dev.myth.Myth;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.GameTickEvent;
import dev.myth.model.Model;
import dev.myth.utils.ClientUtils;
import dev.myth.utils.Keybind;
import dev.myth.UI.UiManager;
import dev.myth.UI.hud.HudManager;
import dev.myth.UI.setting.Setting;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public final class ConfigManager {
    private String currentConfig = "latest";
    private JsonObject snapshot;
    private Path directory;
    private boolean initialized;
    private boolean ready;
    private boolean stopping;
    private int ticks;

    public void initialize() {
        if (initialized) {
            return;
        }
        directory = ClientUtils.getClient().gameDirectory.toPath().resolve("config/myth");
        initialized = true;
        try {
            if (new Config(directory, currentConfig).exists()) {
                loadConfig();
            } else {
                saveConfig();
            }
        } catch (IOException | RuntimeException exception) {
            // Preserve a broken profile until an explicit load or save succeeds.
            Myth.LOGGER.error("Could not initialize Myth config", exception);
        }
        Myth.EVENT_MANAGER.register(this);
        Runtime.getRuntime().addShutdownHook(new Thread(this::saveSnapshot, "Myth-Config-Save"));
    }

    public String getCurrentConfig() {
        return currentConfig;
    }

    public Path getDirectory() {
        return directory;
    }

    public List<String> getConfigs() throws IOException {
        if (!Files.isDirectory(directory)) {
            return List.of();
        }
        try (var files = Files.list(directory)) {
            return files.filter(Files::isRegularFile).map(path -> path.getFileName().toString())
                    .filter(name -> name.endsWith(".json"))
                    .map(name -> name.substring(0, name.length() - 5)).sorted().toList();
        }
    }

    public synchronized void saveConfig() throws IOException {
        saveConfig(currentConfig);
    }

    public synchronized void saveConfig(String name) throws IOException {
        Config target = new Config(directory, name);
        JsonObject next = capture();
        target.save(next);
        if (!target.getName().equals("latest")) {
            new Config(directory, "latest").save(next);
        }
        currentConfig = target.getName();
        snapshot = next;
        ready = true;
    }

    public synchronized void loadConfig() throws IOException {
        loadConfig(currentConfig);
    }

    public synchronized void loadConfig(String name) throws IOException {
        Config target = new Config(directory, name);
        JsonObject root = target.load();
        JsonObject modules = root.getAsJsonObject("modules");
        List<PendingState> states = new ArrayList<>();
        List<PendingSetting> settings = new ArrayList<>();
        JsonObject previousUi = Myth.UI_MANAGER.capture();
        UiManager.State previousUiState = Myth.UI_MANAGER.decode(previousUi);
        UiManager.State nextUiState;
        JsonObject previousHud = Myth.HUD_MANAGER.capture();
        HudManager.State previousHudState = Myth.HUD_MANAGER.decode(previousHud);
        HudManager.State nextHudState;
        JsonObject nextHud = root.has("hud") ? root.getAsJsonObject("hud") : previousHud;
        JsonObject nextUi = root.has("ui") ? root.getAsJsonObject("ui") : previousUi;
        try {
            nextUiState = Myth.UI_MANAGER.decode(nextUi);
            nextHudState = Myth.HUD_MANAGER.decode(nextHud);
            // Validate every known model before mutating any live state.
            for (Model model : Myth.MODEL_MANAGER.getModels()) {
                var element = modules.get(model.getId());
                if (element == null) {
                    continue;
                }
                JsonObject data = element.getAsJsonObject();
                var binding = Myth.MODEL_MANAGER.getKeybind(model.getId()).orElseThrow();
                String key = data.has("key") ? data.get("key").getAsString() : binding.serialize();
                var parsedKey = Keybind.parseSavedKey(key);
                if (data.has("enabled") && (!data.get("enabled").isJsonPrimitive()
                        || !data.getAsJsonPrimitive("enabled").isBoolean())) {
                    throw new IllegalArgumentException("enabled must be a boolean");
                }
                boolean enabled = data.has("enabled") ? data.get("enabled").getAsBoolean() : model.isEnabled();
                Keybind.Mode mode = data.has("mode") ? Keybind.Mode.parse(data.get("mode").getAsString()) : binding.getMode();
                states.add(new PendingState(model, binding, parsedKey, enabled, mode, binding.serialize(), model.isEnabled(), binding.getMode()));
                if (data.has("settings")) {
                    JsonObject values = data.getAsJsonObject("settings");
                    for (Setting<?> setting : model.getSettings()) if (values.has(setting.getId()))
                        settings.add(new PendingSetting(setting, setting.decode(values.get(setting.getId())), setting.snapshot()));
                }
            }
        } catch (RuntimeException exception) {
            throw new IOException("Invalid module data in config: " + name, exception);
        }
        try {
            Myth.UI_MANAGER.apply(nextUiState, nextUi);
            Myth.HUD_MANAGER.apply(nextHudState, nextHud);
            for (PendingSetting state : settings) state.setting().apply(state.next());
            for (PendingState state : states) {
                state.binding().setKey(state.key());
                state.binding().setMode(state.mode());
                state.model().setEnabled(state.enabled());
            }
        } catch (RuntimeException exception) {
            try { Myth.UI_MANAGER.apply(previousUiState, previousUi); }
            catch (RuntimeException rollbackFailure) { exception.addSuppressed(rollbackFailure); }
            try { Myth.HUD_MANAGER.apply(previousHudState, previousHud); }
            catch (RuntimeException rollbackFailure) { exception.addSuppressed(rollbackFailure); }
            for (PendingSetting state : settings.reversed()) {
                try { state.setting().apply(state.previous()); }
                catch (RuntimeException rollbackFailure) { exception.addSuppressed(rollbackFailure); }
            }
            for (PendingState state : states.reversed()) {
                try {
                    state.binding().setKey(Keybind.parseSavedKey(state.previousKey()));
                    state.binding().setMode(state.previousMode());
                    state.model().setEnabled(state.previousEnabled());
                } catch (RuntimeException rollbackFailure) {
                    exception.addSuppressed(rollbackFailure);
                }
            }
            throw new IOException("Failed to load config; attempted to restore previous module states", exception);
        }
        snapshot = root.deepCopy();
        currentConfig = target.getName();
        ready = true;
        // Keep the startup profile in sync without overwriting the selected profile.
        if (!currentConfig.equals("latest")) {
            new Config(directory, "latest").save(capture());
        }
    }

    private JsonObject capture() {
        JsonObject root = snapshot == null ? new JsonObject() : snapshot.deepCopy();
        root.addProperty("version", 1);
        JsonObject modules = root.has("modules") ? root.getAsJsonObject("modules") : new JsonObject();
        for (Model model : Myth.MODEL_MANAGER.getModels()) {
            JsonObject state = modules.has(model.getId()) && modules.get(model.getId()).isJsonObject()
                    ? modules.getAsJsonObject(model.getId()) : new JsonObject();
            state.addProperty("enabled", model.isEnabled());
            Myth.MODEL_MANAGER.getKeybind(model.getId()).ifPresent(binding -> {
                state.addProperty("key", binding.serialize()); state.addProperty("mode", binding.getMode().getId());
            });
            JsonObject settings = state.has("settings") ? state.getAsJsonObject("settings") : new JsonObject();
            for (Setting<?> setting : model.getSettings()) settings.add(setting.getId(), setting.toJson());
            state.add("settings", settings);
            modules.add(model.getId(), state);
        }
        root.add("modules", modules);
        root.add("ui", Myth.UI_MANAGER.capture());
        root.add("hud", Myth.HUD_MANAGER.capture());
        return root;
    }

    @EventTarget
    private void onGameTick(GameTickEvent event) {
        if (event.getType() != EventType.POST || !ready || stopping || ++ticks < 100) {
            return;
        }
        ticks = 0;
        if (!capture().equals(snapshot)) {
            try {
                saveConfig();
            } catch (IOException exception) {
                Myth.LOGGER.error("Could not autosave Myth config", exception);
            }
        }
    }

    public synchronized void stop() {
        if (ready) {
            try {
                saveConfig();
            } catch (IOException exception) {
                Myth.LOGGER.error("Could not save Myth config on stop", exception);
            }
        }
        stopping = true;
    }

    private synchronized void saveSnapshot() {
        // Never read game objects on the JVM shutdown thread or save post-disable state.
        if (!ready || stopping || snapshot == null) {
            return;
        }
        try {
            new Config(directory, currentConfig).save(snapshot);
            if (!currentConfig.equals("latest")) {
                new Config(directory, "latest").save(snapshot);
            }
        } catch (IOException exception) {
            Myth.LOGGER.error("Could not save cached Myth config", exception);
        }
    }

    private record PendingState(Model model, Keybind binding,
                                com.mojang.blaze3d.platform.InputConstants.Key key, boolean enabled, Keybind.Mode mode,
                                String previousKey, boolean previousEnabled, Keybind.Mode previousMode) {
    }
    private record PendingSetting(Setting<?> setting, Setting.State next, Setting.State previous) {}
}
