package today.starain.event.impl;

import net.minecraft.client.gui.screens.Screen;
import today.starain.event.api.Event;

/**
 * @param screen 事件发出时打开着的界面，null 表示在游戏里。用来避免在聊天框/背包里打字时触发绑定。
 */
public record KeyboardEvent(int key, int scancode, int action, int modifiers, Screen screen) implements Event {
}
