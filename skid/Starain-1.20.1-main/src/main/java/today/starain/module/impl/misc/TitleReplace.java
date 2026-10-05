package today.starain.module.impl.misc;

import net.minecraft.ChatFormatting;
import net.minecraft.network.chat.Component;
import net.minecraft.network.protocol.game.ClientboundSetActionBarTextPacket;
import net.minecraft.network.protocol.game.ClientboundSetSubtitleTextPacket;
import net.minecraft.network.protocol.game.ClientboundSetTitleTextPacket;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.PacketEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class TitleReplace extends ClientModule {

    private static final Pattern PATTERN = Pattern.compile("(祝你好运\\s*)[^!！\\n]*");
    private static final String TO = "Starain User";

    public TitleReplace() {
        setName("TitleReplace");
        setCategory(Category.MISC);
    }

    @EventTarget
    public void onPacket(PacketEvent event) {
        if (event.getType() != EventType.INCOMING) return;

        if (event.getPacket() instanceof ClientboundSetTitleTextPacket packet) {
            Component replaced = replace(packet.getText());
            if (replaced != null) event.setPacket(new ClientboundSetTitleTextPacket(replaced));
        } else if (event.getPacket() instanceof ClientboundSetSubtitleTextPacket packet) {
            Component replaced = replace(packet.getText());
            if (replaced != null) event.setPacket(new ClientboundSetSubtitleTextPacket(replaced));
        } else if (event.getPacket() instanceof ClientboundSetActionBarTextPacket packet) {
            Component replaced = replace(packet.getText());
            if (replaced != null) event.setPacket(new ClientboundSetActionBarTextPacket(replaced));
        }
    }

    private Component replace(Component component) {
        if (component == null) return null;
        String flat = component.getString();
        Matcher matcher = PATTERN.matcher(flat);
        if (!matcher.find()) return null;
        String prefix = flat.substring(0, matcher.start()) + matcher.group(1);
        String tail = flat.substring(matcher.end());
        return Component.literal(prefix)
                .append(Component.literal(TO).withStyle(ChatFormatting.AQUA))
                .append(Component.literal(tail));
    }
}
