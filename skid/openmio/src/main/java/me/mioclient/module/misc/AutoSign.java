package me.mioclient.module.misc;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import net.minecraft.network.packet.c2s.play.UpdateSignC2SPacket;
import net.minecraft.network.packet.s2c.play.SignEditorOpenS2CPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/AutoSign.class */
public class AutoSign extends Module {
    public static final DateFormat dateFormat = new SimpleDateFormat("dd/MM/yyyy");
    public Setting<String> line1;
    public Setting<String> line2;
    public Setting<String> line3;
    public Setting<String> line4;

    public AutoSign() {
        super("AutoSign", "Puts stuff on your signs automatically.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void onChannelRead0(ChannelRead0Event channelRead0Event) {
        SignEditorOpenS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof SignEditorOpenS2CPacket ? (SignEditorOpenS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet904 instanceof SignEditorOpenS2CPacket) {
            SignEditorOpenS2CPacket signEditorOpenS2CPacket = packet904;
            AutoSignSearchHelper4.do2571(new UpdateSignC2SPacket(signEditorOpenS2CPacket.getPos(), signEditorOpenS2CPacket.isFront(), getString126(this.line1), getString126(this.line2), getString126(this.line3), getString126(this.line4)));
            channelRead0Event.do1162();
        }
    }

    public String getString126(Setting<String> setting) {
        String replace = setting.getValue().replace("<date>", dateFormat.format(Calendar.getInstance().getTime())).replace("<name>", minecraftClient.player.getName().getString());
        return replace.substring(0, Math.min(16, replace.length()));
    }
}
