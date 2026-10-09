package dev.myth.model.render;

import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.MotionEvent;
import dev.myth.event.impl.PacketEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.wrapper.Wrapper;
import java.util.Map;
import java.util.Optional;
import net.minecraft.core.Holder;
import net.minecraft.network.protocol.game.ClientboundSetTimePacket;
import net.minecraft.world.clock.ClockNetworkState;
import net.minecraft.world.clock.WorldClock;

/**
 * Starain TimeChanger: holds the world at a chosen time, client side only.
 *
 * <p>26.2 replaced the single day time with a registry of {@code WorldClock}s driven by
 * {@code ClockManager}, so {@code Level.setDayTime} is gone. The dimension's timelines sample
 * {@code clockManager.getTotalTicks(clock)}, and the sky colour, light level and sun position all
 * come off that, so pinning the clock's tick count is the whole of it.
 */
public final class TimeChanger extends Model implements Wrapper {
    public final Setting<Integer> time = addSetting(Setting.integer("time", "World Time", 8000, 0, 24000)
            .description("0 is sunrise, 6000 noon, 13000 nightfall, 18000 midnight."));

    public TimeChanger() {
        super("timechanger", "TimeChanger", Category.RENDER);
        setDescription("Changes the time of the world.");
    }

    @Override public String getInfo() { return Integer.toString(time.get()); }

    @EventTarget
    private void onMotion(MotionEvent event) {
        if (event.getType() != EventType.PRE || mc.level == null) return;
        Optional<Holder<WorldClock>> clock = mc.level.dimensionType().defaultClock();
        if (clock.isEmpty()) return;
        // handleUpdates is the only way in; it ticks first, but ClientLevel has already ticked the
        // manager with this same game time by now, so that leaves the delta at zero.
        // The rate stays 1 so the clock runs normally again once the module stops overwriting it.
        mc.level.clockManager().handleUpdates(mc.level.getGameTime(),
                Map.of(clock.get(), new ClockNetworkState(time.get(), 0, 1)));
    }

    /** Otherwise the server's periodic update fights the setting and the sky flickers. */
    @EventTarget
    private void onPacket(PacketEvent event) {
        if (event.getType() == EventType.INCOMING && event.getPacket() instanceof ClientboundSetTimePacket)
            event.setCancelled(true);
    }
}
