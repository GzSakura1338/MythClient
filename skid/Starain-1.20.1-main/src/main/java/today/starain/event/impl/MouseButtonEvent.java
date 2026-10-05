package today.starain.event.impl;

import net.minecraft.client.gui.screens.Screen;
import today.starain.event.api.Event;

public record MouseButtonEvent(int button, int action, int modifiers, Screen screen) implements Event {
}
