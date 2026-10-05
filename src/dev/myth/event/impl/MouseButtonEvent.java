package dev.myth.event.impl;

import dev.myth.event.api.Event;
import net.minecraft.client.gui.screens.Screen;

public record MouseButtonEvent(int button, int action, int modifiers, Screen screen) implements Event {
}
