package today.starain;

import lombok.Getter;
import today.starain.command.CommandManager;
import today.starain.config.ConfigManager;
import today.starain.event.api.EventManager;
import today.starain.module.ModuleManager;
import today.starain.utils.rotation.RotationManager;
import today.starain.utils.wrapper.Wrapper;

@Getter
public class Starain implements Wrapper {
    public static final String NAME = "Starain";
    public static final String VERSION = "260717";
    public static int skipTicks = 0;
    @Getter
    private static Starain instance;
    private EventManager eventManager;
    private ModuleManager moduleManager;
    private CommandManager commandManager;
    private ConfigManager configManager;

    private Starain() {
        instance = this;
        init();
    }

    // 如果直接去new一个对象这个zkm的indy没法混 直接是明文调用 所以要搞一个newInstance方法
    public static Starain newInstance() {
        return new Starain();
    }

    private void init() {
        eventManager = EventManager.netInstance();
        eventManager.register(this);
        eventManager.register(RotationManager.newInstance());
        moduleManager = ModuleManager.newInstance();
        commandManager = CommandManager.newInstance();
        configManager = ConfigManager.newInstance();
    }

}
