package me.mioclient.module.misc;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.FireworksHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickPostEvent;
import me.mioclient.module.Module;
import me.mioclient.module.exploit.RocketExtender;
import me.mioclient.module.movement.ElytraFly;
import me.mioclient.module.movement.Fireworks;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Items;
import net.minecraft.util.Hand;
import net.minecraft.util.hit.EntityHitResult;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/MiddleClick.class */
public class MiddleClick extends Module {
    public static final Fireworks fireworks2 = (Fireworks) BaritoneHelper_3.baritoneHelper_4.getModule117(Fireworks.class);
    public static final RocketExtender rocketExtender = (RocketExtender) BaritoneHelper_3.baritoneHelper_4.getModule117(RocketExtender.class);
    public static ElytraFly elytrafly = (ElytraFly) BaritoneHelper_3.baritoneHelper_4.getModule117(ElytraFly.class);
    public Setting<Boolean> friends;
    public Setting<Boolean> fireworks;
    public Setting<Boolean> alternative;
    public boolean flag;

    public MiddleClick() {
        super("MiddleClick", "Middle click actions.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        setDrawn(false);
    }

    @Listen
    public void do32(TickPostEvent tickPostEvent) {
        if (minecraftClient.currentScreen != null) {
            return;
        }
        if (GLFW.glfwGetMouseButton(minecraftClient.getWindow().getHandle(), 2) != 1 || this.flag) {
            if (GLFW.glfwGetMouseButton(minecraftClient.getWindow().getHandle(), 2) == 0) {
                this.flag = false;
                return;
            }
            return;
        }
        this.flag = true;
        EntityHitResult entityHitResult = (EntityHitResult)(minecraftClient.crosshairTarget);
        if ((minecraftClient.player.isFallFlying() || fireworks2.is144() || elytrafly.is956()) && this.fireworks.getValue().booleanValue()) {
            do1492();
            return;
        }
        if (entityHitResult instanceof EntityHitResult) {
            PlayerEntity entity = (entityHitResult.getEntity()) instanceof PlayerEntity ? (PlayerEntity) (entityHitResult.getEntity()) : null;
            if (entity instanceof PlayerEntity) {
                PlayerEntity playerEntity = entity;
                if (this.friends.getValue().booleanValue()) {
                    if (BaritoneHelper_3.searchHelper4_14.is519(playerEntity.getGameProfile().getName())) {
                        BaritoneHelper_3.searchHelper4_14.is527(playerEntity.getGameProfile().getName());
                    } else {
                        BaritoneHelper_3.searchHelper4_14.do523(playerEntity.getGameProfile().getName());
                    }
                }
            }
        }
    }

    public void do1492() {
        Hand hand450 = FireworksHelper.getHand450(Items.FIREWORK_ROCKET);
        int i = minecraftClient.player.getInventory().selectedSlot;
        int i2 = FireworksHelper.get443(Items.FIREWORK_ROCKET);
        int i3 = FireworksHelper.get447(Items.FIREWORK_ROCKET);
        if (hand450 != null) {
            minecraftClient.interactionManager.interactItem(minecraftClient.player, hand450);
            return;
        }
        if (i2 != -1) {
            boolean z = i3 == -1 || this.alternative.getValue().booleanValue();
            do2707(i3, i2, z);
            minecraftClient.interactionManager.interactItem(minecraftClient.player, Hand.MAIN_HAND);
            rocketExtender.vec3d = minecraftClient.player.getPos();
            do2707(i, i2, z);
        }
    }

    public void do2707(int i, int i2, boolean z) {
        if (z) {
            FireworksHelper.do439(i2);
        } else {
            FireworksHelper.do456(i);
        }
    }
}
