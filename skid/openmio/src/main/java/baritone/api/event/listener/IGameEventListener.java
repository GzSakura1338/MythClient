package baritone.api.event.listener;
import baritone.api.event.events.TabCompleteEvent;
import baritone.api.event.events.ChatEvent;
public interface IGameEventListener {
    void onPreTabComplete(TabCompleteEvent event);
    void onSendChatMessage(ChatEvent event);
}
