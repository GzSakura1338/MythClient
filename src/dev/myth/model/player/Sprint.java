package dev.myth.model.player;

import dev.myth.event.api.EventType;
import dev.myth.event.api.Priority;
import dev.myth.event.api.annotations.EventPriority;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.MotionEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import net.minecraft.client.Minecraft;

/** Starain Sprint, using the vanilla sprint key and movement restrictions. */
public final class Sprint extends Model {
    public Sprint() {
        super("sprint", "Sprint", Category.PLAYER);
        setDescription("Automatically holds the sprint key.");
    }

    @EventTarget
    @EventPriority(Priority.HIGHEST)
    private void onMotion(MotionEvent event) {
        if (event.getType() != EventType.PRE) return;
        var options = Minecraft.getInstance().options;
        options.keySprint.setDown(true);
        options.toggleSprint().set(false);
    }

    @Override protected void onDisable() {
        Minecraft.getInstance().options.keySprint.setDown(false);
    }
}
