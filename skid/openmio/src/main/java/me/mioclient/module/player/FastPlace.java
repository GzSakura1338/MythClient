package me.mioclient.module.player;

import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.event.TickPostEvent;
import me.mioclient.mixin.ducks.DuckMinecraftClient;
import me.mioclient.module.Module;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.item.BlockItem;
import net.minecraft.item.ExperienceBottleItem;
import net.minecraft.item.FireworkRocketItem;
import net.minecraft.item.Item;
import net.minecraft.item.Items;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/FastPlace.class */
public class FastPlace extends Module {
    public Setting<Integer> delay;
    public Setting<Boolean> exp;
    public Setting<Boolean> rockets;
    public Setting<Boolean> blocks;
    public Setting<Boolean> enderChests;
    public Setting<Boolean> all;
    public Setting<Boolean> autoPlace;

    public FastPlace() {
        super("FastPlace", "Removes the right-click delay for some items.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
        setDrawn(false);
    }

    @Listen
    public void do32(TickPostEvent tickPostEvent) {
        DuckMinecraftClient duckMinecraftClient = (DuckMinecraftClient)(minecraftClient);
        if (!minecraftClient.player.getMainHandStack().contains(DataComponentTypes.FOOD) && this.autoPlace.getValue().booleanValue() && duckMinecraftClient.getItemUseCooldown() == 0) {
            duckMinecraftClient.interact();
        }
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        if (is1469()) {
            return;
        }
        Item item = minecraftClient.player.getMainHandStack().getItem();
        if (((item instanceof ExperienceBottleItem) && this.exp.getValue().booleanValue()) || (((item instanceof FireworkRocketItem) && this.rockets.getValue().booleanValue()) || (((item instanceof BlockItem) && this.blocks.getValue().booleanValue()) || ((item == Items.ENDER_CHEST && this.enderChests.getValue().booleanValue()) || this.all.getValue().booleanValue())))) {
            ((DuckMinecraftClient) minecraftClient).setItemUseCooldown(Math.min(((DuckMinecraftClient) minecraftClient).getItemUseCooldown(), this.delay.getValue().intValue()));
        }
    }
}
