package me.mioclient.module.misc;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Pattern;
import me.mioclient.AntiSpamHelper;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.HoleSnapData_2;
import me.mioclient.KeyPearlMode;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.AddMessageEvent;
import me.mioclient.event.DisconnectEvent;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import net.minecraft.client.gui.hud.ChatHudLine;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/AntiSpam.class */
public class AntiSpam extends Module {
    public static final Pattern pattern = Pattern.compile("(https?:\\/\\/)?([\\w\\-])+\\.{1}([a-zA-Z]{2,63})([\\/\\w-]*)*\\/?\\??([^#\\n\\r]*)?#?([^\\n\\r]*)");
    public final Map<String, HoleSnapData_2<Long, ChatHudLine.Visible>> map;
    public Setting<Boolean> indicator;
    public Setting<Boolean> links;
    public String string;

    public AntiSpam() {
        super("AntiSpam", "Stacks similar chat messages with each other.", Category.MISC, new String[0]);
        this.map = new HashMap();
        PhaseESPHelper.do1351(this);
        setDrawn(false);
    }

    @Override // me.mioclient.module.Module
    public void onToggle() {
        this.map.clear();
        this.string = null;
    }

    @Listen
    public void onDisconnect(DisconnectEvent disconnectEvent) {
        this.map.clear();
        this.string = null;
    }

    @Listen
    public void onAddMessage(AddMessageEvent addMessageEvent) {
        if (addMessageEvent.getKeyPearlMode1472() != KeyPearlMode.Pre) {
            if (this.string != null) {
                this.map.computeIfPresent(this.string, (str, holeSnapData_2) -> {
                    return new HoleSnapData_2(Long.valueOf(((Long) holeSnapData_2.getObject3119()).longValue() + 1), addMessageEvent.getVisible2281());
                });
                return;
            }
            return;
        }
        if (addMessageEvent.getSignature() == null || addMessageEvent.getSignature().toByteBuffer().getInt() >= 0) {
            this.string = addMessageEvent.getText2279().getString().toLowerCase(Locale.ROOT);
            this.map.computeIfPresent(this.string, (str2, holeSnapData_22) -> {
                ((AntiSpamHelper) minecraftClient.inGameHud.getChatHud()).getVisible().remove(holeSnapData_22.getObject3120());
                return holeSnapData_22;
            });
            this.map.putIfAbsent(this.string, new HoleSnapData_2<>(1L, addMessageEvent.getVisible2281()));
            if (this.links.getValue().booleanValue()) {
                if (pattern.matcher(this.string).find()) {
                    addMessageEvent.do1162();
                }
            }
            long longValue = this.map.get(this.string).getObject3119().longValue();
            if (longValue <= 1 || !this.indicator.getValue().booleanValue()) {
                return;
            }
            MutableText append = Text.empty().append(addMessageEvent.getText2279());
            addMessageEvent.do2280(append.append(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2907(longValue).getString2921(" [x\u0001]"))));
        }
    }
}
