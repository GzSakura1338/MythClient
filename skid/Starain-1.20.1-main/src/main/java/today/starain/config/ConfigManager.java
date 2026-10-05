package today.starain.config;

import lombok.Getter;
import today.starain.utils.wrapper.Wrapper;

@Getter
public final class ConfigManager implements Wrapper {
    private String currentConfig = "latest";

    private ConfigManager() {
        new Config(currentConfig).load();
        // 正常退出、Alt+F4、IDE 的 Stop 都会走 JVM 关闭钩子；挂 Minecraft.close() 覆盖不到最后一种。
        Runtime.getRuntime().addShutdownHook(new Thread(this::saveConfig, "Starain-Config-Save"));
    }

    // 如果直接去new一个对象这个zkm的indy没法混 直接是明文调用 所以要搞一个newInstance方法
    public static ConfigManager newInstance() {
        return new ConfigManager();
    }

    public void saveConfig() {
        new Config(currentConfig).save();
        if (!"latest".equals(currentConfig)) {
            new Config("latest").save();
        }
    }

    public void loadConfig() {
        new Config(currentConfig).load();
        if (!"latest".equals(currentConfig)) {
            new Config("latest").save();
        }
    }

    public void saveConfig(String name) {
        currentConfig = name;
        saveConfig();
    }

    public void loadConfig(String name) {
        currentConfig = name;
        loadConfig();
    }
}
