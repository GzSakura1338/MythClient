package dev.myth.event.impl;

import dev.myth.event.api.Event;
import net.minecraft.client.gui.screens.Screen;

public record KeyboardEvent(int key, int scancode, int action, int modifiers, Screen screen) implements Event {
}
