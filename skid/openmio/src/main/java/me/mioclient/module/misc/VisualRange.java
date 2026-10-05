package me.mioclient.module.misc;

import java.awt.Color;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.concurrent.atomic.AtomicBoolean;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.HoleSnapSearchHelper4;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchIdentifier;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.DisconnectEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.LoadChunkFromPacketEvent;
import me.mioclient.event.TickEvent;
import me.mioclient.module.Module;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.network.packet.s2c.play.EntitySpawnS2CPacket;
import net.minecraft.text.HoverEvent;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/VisualRange.class */
public class VisualRange extends Module {
    public static final int num = -Math.abs("EnderPearl".hashCode());
    public Setting<Boolean> join;
    public Setting<Boolean> leave;
    public Setting<Boolean> targets;
    public Setting<Boolean> players;
    public Setting<Boolean> friends;
    public Setting<Boolean> enemies;
    public Setting<Boolean> nakeds;
    public Setting<Boolean> signs;
    public Setting<Boolean> enderPearls;
    public Setting<Boolean> playSound;
    public Setting<Boolean> onlyEnemies;
    public Setting<SearchIdentifier> sound;
    public Setting<Float> volume;
    public final ArrayList<String> arrayList;
    public final ArrayList<String> arrayList2;

    public VisualRange() {
        super("VisualRange", "Informs you of players who enter/leave your visual range.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        this.arrayList = new ArrayList<>();
        this.arrayList2 = new ArrayList<>();
        setDrawn(false);
    }

    @Override // me.mioclient.module.Module
    public void onDisable() {
        this.arrayList.clear();
        this.arrayList2.clear();
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (is1469()) {
            return;
        }
        this.arrayList2.clear();
        try {
            for (AbstractClientPlayerEntity abstractClientPlayerEntity : minecraftClient.world.getPlayers()) {
                if (!abstractClientPlayerEntity.getName().equals(minecraftClient.player.getName()) && is1015((PlayerEntity) abstractClientPlayerEntity)) {
                    this.arrayList2.add(abstractClientPlayerEntity.getName().getString());
                }
            }
            if (!this.arrayList.equals(this.arrayList2)) {
                Iterator<String> it = this.arrayList2.iterator();
                while (it.hasNext()) {
                    String next = it.next();
                    if (!this.arrayList.contains(next) && this.join.getValue().booleanValue()) {
                        Color color530 = BaritoneHelper_3.searchHelper4_14.getColor530(next, Color.WHITE);
                        MixinMessageIndicatorHelper.do345(Text.literal("%s%s entered visual range.".formatted(next, Formatting.WHITE)).styled(style -> {
                            return style.withColor(color530.hashCode());
                        }), MixinMessageIndicatorHelper.getMessageSignatureData337((Math.abs(next.hashCode()) * (-1)) / 2), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
                        if ((!BaritoneHelper_3.searchHelper4_14.is519(next) && BaritoneHelper_3.searchHelper4_14.is521(next)) || !this.onlyEnemies.getValue().booleanValue()) {
                            do1016();
                        }
                    }
                }
                Iterator<String> it2 = this.arrayList.iterator();
                while (it2.hasNext()) {
                    String next2 = it2.next();
                    if (!this.arrayList2.contains(next2) && this.leave.getValue().booleanValue()) {
                        Color color5302 = BaritoneHelper_3.searchHelper4_14.getColor530(next2, Color.WHITE);
                        MixinMessageIndicatorHelper.do345(Text.literal("%s%s left visual range.".formatted(next2, Formatting.WHITE)).styled(style2 -> {
                            return style2.withColor(color5302.hashCode());
                        }), MixinMessageIndicatorHelper.getMessageSignatureData337((Math.abs(next2.hashCode()) * (-1)) / 2), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode3);
                    }
                }
                this.arrayList.clear();
                this.arrayList.addAll(this.arrayList2);
            }
        } catch (Exception e) {
        }
    }

    @Listen
    public void onLoadChunkFromPacket(LoadChunkFromPacketEvent loadChunkFromPacketEvent) {
        if (this.signs.getValue().booleanValue()) {
            AtomicBoolean atomicBoolean = new AtomicBoolean(false);
            loadChunkFromPacketEvent.getWorldChunk2555().getBlockEntities().forEach((blockPos, blockEntity) -> {
                if (blockEntity instanceof SignBlockEntity) {
                    int i = -Math.abs(blockPos.hashCode());
                    MutableText literal = Text.literal("Found a sign: ");
                    boolean z = true;
                    for (Text text : ((SignBlockEntity) blockEntity).getFrontText().getMessages(false)) {
                        if (!Formatting.strip(text.getString()).isBlank()) {
                            literal.append(text);
                            literal.append(" ");
                            z = false;
                        }
                    }
                    if (z) {
                        literal.append("<empty>");
                    }
                    literal.styled(style -> {
                        return style.withFormatting(Formatting.WHITE);
                    });
                    String formatted = "%d, %d, %d".formatted(Integer.valueOf(blockPos.getX()), Integer.valueOf(blockPos.getY()), Integer.valueOf(blockPos.getZ()));
                    HoverEvent hoverEvent = new HoverEvent(HoverEvent.Action.SHOW_TEXT, Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(formatted).getString2921("at \u0001")));
                    literal.styled(style2 -> {
                        return style2.withHoverEvent(hoverEvent);
                    });
                    literal.styled(style3 -> {
                        return style3.withClickEvent(MixinMessageIndicatorHelper.getClickEvent348(new ArgumentTypeHelper().getArgumentTypeHelper2919(formatted.replace(",", "")).getString2921("highlight \u0001")));
                    });
                    MixinMessageIndicatorHelper.do344((Text) literal, MixinMessageIndicatorHelper.getMessageSignatureData337(i));
                    atomicBoolean.set(true);
                }
            });
            if (atomicBoolean.get()) {
                do1016();
            }
        }
    }

    @Listen
    public void do29(ChannelRead0Event channelRead0Event) {
        EntitySpawnS2CPacket packet904 = (channelRead0Event.getPacket904()) instanceof EntitySpawnS2CPacket ? (EntitySpawnS2CPacket) (channelRead0Event.getPacket904()) : null;
        if (packet904 instanceof EntitySpawnS2CPacket) {
            EntitySpawnS2CPacket entitySpawnS2CPacket = packet904;
            if (entitySpawnS2CPacket.getEntityType() == EntityType.ENDER_PEARL && this.enderPearls.getValue().booleanValue()) {
                Text literal = Text.literal("Ender Pearl entered visual range.");
                String formatted = "%.0f, %.0f, %.0f".formatted(Double.valueOf(entitySpawnS2CPacket.getX()), Double.valueOf(entitySpawnS2CPacket.getY()), Double.valueOf(entitySpawnS2CPacket.getZ()));
                MutableText literal2 = Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(formatted).getString2921("at \u0001"));
                ((MutableText) literal).styled(style -> {
                    return style.withHoverEvent(new HoverEvent(HoverEvent.Action.SHOW_TEXT, literal2));
                });
                ((MutableText) literal).styled(style2 -> {
                    return style2.withClickEvent(MixinMessageIndicatorHelper.getClickEvent348(new ArgumentTypeHelper().getArgumentTypeHelper2919(formatted.replace(",", "")).getString2921("highlight \u0001")));
                });
                MixinMessageIndicatorHelper.do345(literal, MixinMessageIndicatorHelper.getMessageSignatureData337(num), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
                do1016();
            }
        }
    }

    public boolean is1015(PlayerEntity playerEntity) {
        return !HoleSnapSearchHelper4.is2013((LivingEntity) playerEntity) ? this.nakeds.getValue().booleanValue() : BaritoneHelper_3.searchHelper4_14.is520(playerEntity) ? this.friends.getValue().booleanValue() : BaritoneHelper_3.searchHelper4_14.is522(playerEntity) ? this.enemies.getValue().booleanValue() : this.players.getValue().booleanValue();
    }

    @Listen
    public void onDisconnect(DisconnectEvent disconnectEvent) {
        this.arrayList.clear();
        this.arrayList2.clear();
    }

    public void do1016() {
        if (this.playSound.getValue().booleanValue()) {
            BaritoneHelper_3.searchHelper4_11.getSearchHelper4_42970(this.sound.getValue()).do1820(this.volume.getValue().floatValue());
        }
    }
}
