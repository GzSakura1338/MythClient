package today.starain.module.impl.misc;

import net.minecraft.world.entity.Entity;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.ClientModule;

/**
 * @Author：jiuxian_baka
 * @Date：2026/3/1 15:08
 */
public class AntiBot extends ClientModule {


    public AntiBot() {
        setName("AntiBot");
        setCategory(Category.MISC);
    }

    public static boolean isBot(Entity entity) {
        return !mc.getConnection().getOnlinePlayerIds().contains(entity.getUUID()) && Starain.getInstance().getModuleManager().getModule(AntiBot.class).isEnabled();
    }

}
