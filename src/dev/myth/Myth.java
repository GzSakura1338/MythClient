package dev.myth;

import dev.myth.model.ModelManager;
import dev.myth.model.player.Sprint;
import dev.myth.model.render.FullBright;
import dev.myth.utils.rotation.RotationManager;
import dev.myth.event.api.EventManager;
import dev.myth.event.EventHooks;
import dev.myth.command.CommandManager;
import dev.myth.config.ConfigManager;
import dev.myth.UI.UiManager;
import dev.myth.UI.hud.HudManager;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientLifecycleEvents;
import net.fabricmc.api.ClientModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public final class Myth implements ClientModInitializer {
    public static final String MOD_ID = "myth";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
    public static final EventManager EVENT_MANAGER = EventManager.netInstance();
    public static final ModelManager MODEL_MANAGER = new ModelManager();
    public static final CommandManager COMMAND_MANAGER = new CommandManager();
    public static final ConfigManager CONFIG_MANAGER = new ConfigManager();
    public static final HudManager HUD_MANAGER = new HudManager();
    public static final UiManager UI_MANAGER = new UiManager();

    @Override
    public void onInitializeClient() {
        EventHooks.initialize();
        EVENT_MANAGER.register(RotationManager.newInstance());
        MODEL_MANAGER.register(new Sprint());
        MODEL_MANAGER.register(new FullBright());
        MODEL_MANAGER.initialize();
        HUD_MANAGER.initialize();
        UI_MANAGER.initialize();
        CONFIG_MANAGER.initialize();
        COMMAND_MANAGER.initialize();
        ClientLifecycleEvents.CLIENT_STOPPING.register(client -> {
            CONFIG_MANAGER.stop();
            MODEL_MANAGER.disableAll();
            UI_MANAGER.stop();
        });
        LOGGER.info("Myth initialized");
    }
}
