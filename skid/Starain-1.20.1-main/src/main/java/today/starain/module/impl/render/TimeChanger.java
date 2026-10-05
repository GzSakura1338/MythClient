package today.starain.module.impl.render;

import net.minecraft.network.protocol.game.ClientboundSetTimePacket;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.event.impl.PacketEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.value.impl.NumberValue;

public final class TimeChanger extends ClientModule {
    private final NumberValue time = new NumberValue("World Time", 0.0, 24000.0, 8000.0, 1.0, this);

    public TimeChanger() {
        setName("TimeChanger");
        setCategory(Category.RENDER);
    }

    @EventTarget
    public void onMotion(MotionEvent e) {
        if (e.getType() == EventType.PRE) {
            if (mc.level != null) {
                mc.level.setDayTime(time.get().longValue());
            }
        }
    }

    @EventTarget
    public void onPacket(PacketEvent event) {
        if (event.getPacket() instanceof ClientboundSetTimePacket) {
            event.setCancelled(true);
        }
    }
}
