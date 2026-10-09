package dev.myth;

import dev.myth.model.ModelManager;
import dev.myth.model.combat.KillAura;
import dev.myth.model.combat.Velocity;
import dev.myth.model.misc.FakePlayer;
import dev.myth.model.player.Sprint;
import dev.myth.model.player.Scaffold;
import dev.myth.model.player.InvManager;
import dev.myth.model.player.ChestStealer;
import dev.myth.model.render.FullBright;
import dev.myth.model.render.Animations;
import dev.myth.model.render.ViewModel;
import dev.myth.model.render.NoRender;
import dev.myth.model.render.TimeChanger;
import dev.myth.model.render.Shader;
import dev.myth.utils.rotation.RotationManager;
import dev.myth.event.api.EventManager;
import dev.myth.event.EventHooks;
import dev.myth.command.CommandManager;
import dev.myth.command.Notifications;
import dev.myth.config.ConfigManager;
import dev.myth.UI.UiManager;
import dev.myth.UI.WorldEsp;
import dev.myth.UI.EntityShader;
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
    public static final WorldEsp WORLD_ESP = new WorldEsp();
    public static final EntityShader ENTITY_SHADER = new EntityShader();
    public static final UiManager UI_MANAGER = new UiManager();

    @Override
    public void onInitializeClient() {
        EventHooks.initialize();
        EVENT_MANAGER.register(RotationManager.newInstance());
        MODEL_MANAGER.register(new Sprint());
        MODEL_MANAGER.register(new Scaffold());
        MODEL_MANAGER.register(new InvManager());
        MODEL_MANAGER.register(new ChestStealer());
        MODEL_MANAGER.register(new KillAura());
        MODEL_MANAGER.register(new Velocity());
        MODEL_MANAGER.register(new FakePlayer());
        MODEL_MANAGER.register(new FullBright());
        MODEL_MANAGER.register(new Animations());
        MODEL_MANAGER.register(new ViewModel());
        MODEL_MANAGER.register(new Shader());
        MODEL_MANAGER.register(new TimeChanger());
        MODEL_MANAGER.register(new NoRender());
        MODEL_MANAGER.register(new Notifications()).setEnabled(true, false);
        MODEL_MANAGER.initialize();
        HUD_MANAGER.initialize();
        UI_MANAGER.initialize();
        // Vanilla Options loads registered key mappings after mod entrypoints.
        // Restore Myth last, so options.txt cannot overwrite saved bindings.
        ClientLifecycleEvents.CLIENT_STARTED.register(client -> CONFIG_MANAGER.initialize());
        COMMAND_MANAGER.initialize();
        ClientLifecycleEvents.CLIENT_STOPPING.register(client -> {
            CONFIG_MANAGER.stop();
            MODEL_MANAGER.disableAll();
            ENTITY_SHADER.close();
            UI_MANAGER.stop();
        });
        LOGGER.info("Myth initialized");
    }
}
