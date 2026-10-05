package dev.myth.model;

import dev.myth.utils.Keybind;
import dev.myth.command.Notifications;
import dev.myth.Myth;
import java.util.Objects;
import java.util.ArrayList;
import java.util.List;
import dev.myth.UI.setting.Setting;
import net.minecraft.client.Minecraft;

/** Base class only; concrete features are added explicitly when needed. */
public abstract class Model {
    private final String id;
    private final String name;
    private final Category category;
    private final int defaultKey;
    private boolean enabled;
    private String description = "";
    private final List<Setting<?>> settings = new ArrayList<>();

    protected Model(String id, String name, Category category) {
        this(id, name, category, Keybind.UNBOUND);
    }

    protected Model(String id, String name, Category category, int defaultKey) {
        this.id = Objects.requireNonNull(id, "id");
        this.name = Objects.requireNonNull(name, "name");
        this.category = Objects.requireNonNull(category, "category");
        if (!id.matches("[a-z0-9_]+") || name.isBlank()) {
            throw new IllegalArgumentException("Invalid model id or name");
        }
        this.defaultKey = defaultKey;
    }

    public final String getId() {
        return id;
    }

    public final String getName() {
        return name;
    }

    public final Category getCategory() {
        return category;
    }

    public final int getDefaultKey() {
        return defaultKey;
    }

    public final String getDescription() { return description; }
    /** Mio excludes utility modules from its HUD list and allows an info suffix. */
    public boolean isDrawn() { return true; }
    public String getInfo() { return null; }
    public final String getInfoString() {
        String info = getInfo();
        return getName() + (info == null ? "" : " §7[§f" + info + "§7]");
    }
    protected final void setDescription(String description) { this.description = Objects.requireNonNull(description); }
    public final List<Setting<?>> getSettings() { return List.copyOf(settings); }
    protected final <T> Setting<T> addSetting(Setting<T> setting) {
        if (settings.stream().anyMatch(existing -> existing.getId().equals(setting.getId())))
            throw new IllegalArgumentException("Duplicate setting id: " + setting.getId());
        settings.add(Objects.requireNonNull(setting)); return setting;
    }

    public final boolean isEnabled() {
        return enabled;
    }

    public final void toggle() {
        setEnabled(!enabled);
    }

    public final void setEnabled(boolean enabled) {
        setEnabled(enabled, true);
    }

    /** Config restoration and shutdown can change state without chat notifications. */
    public final void setEnabled(boolean enabled, boolean notify) {
        if (this.enabled == enabled) {
            return;
        }
        this.enabled = enabled;
        try {
            if (enabled) {
                Myth.EVENT_MANAGER.register(this);
                onEnable();
            } else {
                Myth.EVENT_MANAGER.unregister(this);
                onDisable();
            }
        } catch (RuntimeException exception) {
            // A failed callback must not leave the model enabled.
            this.enabled = false;
            Myth.EVENT_MANAGER.unregister(this);
            throw exception;
        }
        if (notify && this.enabled == enabled) Notifications.onToggle(this);
    }

    protected boolean requiresWorld() {
        return category != Category.CLIENT;
    }

    protected void onEnable() {
    }

    protected void onDisable() {
    }

    protected void onTick(Minecraft client) {
    }
}
