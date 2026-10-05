package today.starain.module.impl.client;

import net.minecraft.sounds.SoundEvents;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.client.SoundUtils;
import today.starain.value.impl.ModeValue;

public class Notification extends ClientModule {

    private final ModeValue sound = new ModeValue("Toggle Sound", "Vanilla", new String[]{"None", "Vanilla", "JelloBounce"}, this);

    public Notification() {
        setName("Notification");
        setCategory(Category.CLIENT);
    }

    public static void playToggleSound(boolean enabled) {
        if (mc.player == null) return;
        Notification notification = Starain.getInstance().getModuleManager().getModule(Notification.class);
        if (!notification.isEnabled()) return;
        switch (notification.sound.get()) {
            case "Vanilla" -> {
                if (enabled) {
                    mc.player.playSound(SoundEvents.WOODEN_BUTTON_CLICK_ON, 1F, 1.3F);
                } else {
                    mc.player.playSound(SoundEvents.WOODEN_BUTTON_CLICK_OFF, 1F, 0.8F);
                }
            }
            case "JelloBounce" -> {
                if (enabled) {
                    SoundUtils.playSound("enable.wav", 1f);
                } else {
                    SoundUtils.playSound("disable.wav", 1f);
                }
            }
        }
    }


}
