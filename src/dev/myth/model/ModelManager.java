package dev.myth.model;

import dev.myth.Myth;
import dev.myth.utils.ClientUtils;
import dev.myth.utils.Keybind;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.GameTickEvent;
import net.minecraft.client.Minecraft;

/** Register and access models on the client thread. */
public final class ModelManager {
    private final Map<String, Model> models = new LinkedHashMap<>();
    private final Map<String, Keybind> keybinds = new LinkedHashMap<>();
    private boolean initialized;

    public void initialize() {
        if (initialized) {
            return;
        }
        for (Model model : models.values()) {
            registerKeybind(model);
        }
        Myth.EVENT_MANAGER.register(this);
        initialized = true;
    }

    public <T extends Model> T register(T model) {
        Objects.requireNonNull(model, "model");
        if (models.containsKey(model.getId())) {
            throw new IllegalArgumentException("Duplicate model id: " + model.getId());
        }
        if (initialized) {
            registerKeybind(model);
        }
        models.put(model.getId(), model);
        return model;
    }

    public List<Model> getModels() {
        return List.copyOf(models.values());
    }

    public List<Model> getModels(Category category) {
        Objects.requireNonNull(category, "category");
        return models.values().stream().filter(model -> model.getCategory() == category).toList();
    }

    public Optional<Model> getModel(String id) {
        return Optional.ofNullable(models.get(id));
    }

    public <T extends Model> Optional<T> getModel(Class<T> type) {
        Objects.requireNonNull(type, "type");
        return models.values().stream().filter(type::isInstance).map(type::cast).findFirst();
    }

    public Optional<Keybind> getKeybind(String id) {
        return Optional.ofNullable(keybinds.get(id));
    }

    public void disableAll() {
        for (Model model : getModels()) {
            try {
                model.setEnabled(false, false);
            } catch (RuntimeException exception) {
                Myth.LOGGER.error("Could not disable model {}", model.getId(), exception);
            }
        }
    }

    private void registerKeybind(Model model) {
        keybinds.computeIfAbsent(model.getId(), id -> new Keybind(id, model.getDefaultKey()));
    }

    @EventTarget
    private void onGameTick(GameTickEvent event) {
        if (event.getType() == EventType.POST) {
            tick(ClientUtils.getClient());
        }
    }

    private void tick(Minecraft client) {
        boolean handleKeys = ClientUtils.canHandleKeybinds(client);
        boolean inGame = ClientUtils.isInGame(client);
        for (Model model : getModels()) {
            Keybind keybind = keybinds.get(model.getId());
            try {
                if (handleKeys) {
                    if (keybind.getMode() == Keybind.Mode.TOGGLE) {
                        while (keybind.consumePress()) model.toggle();
                    } else keybind.discardPresses();
                } else {
                    keybind.discardPresses();
                }
                if (model.isEnabled() && (!model.requiresWorld() || inGame)) {
                    model.onTick(client);
                }
            } catch (RuntimeException exception) {
                Myth.LOGGER.error("Model {} failed", model.getId(), exception);
                keybind.discardPresses();
                disable(model);
            }
        }
    }

    /** Mio polls held bindings on every rendered frame, including releases in menus. */
    public void updateHeldKeybinds(Minecraft client) {
        if (!initialized) return;
        for (Model model : getModels()) {
            Keybind keybind = keybinds.get(model.getId());
            if (!keybind.canUpdateHeldState(model.isEnabled(), client.gui.screen() != null)) continue;
            try { model.setEnabled(keybind.getMode().heldState(keybind.isPressed())); }
            catch (RuntimeException exception) {
                Myth.LOGGER.error("Model {} failed while updating its held binding", model.getId(), exception);
                disable(model);
            }
        }
    }

    private void disable(Model model) {
        try {
            model.setEnabled(false);
        } catch (RuntimeException exception) {
            Myth.LOGGER.error("Could not disable model {}", model.getId(), exception);
        }
    }
}
