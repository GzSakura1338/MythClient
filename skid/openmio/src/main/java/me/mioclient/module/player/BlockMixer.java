package me.mioclient.module.player;

import java.util.ArrayList;
import java.util.Random;
import me.mioclient.api.Category;
import me.mioclient.event.InteractBlockEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.module.Module;
import net.minecraft.item.BlockItem;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.c2s.play.PlayerInteractBlockC2SPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/BlockMixer.class */
public class BlockMixer extends Module {
    public final Random random;

    public BlockMixer() {
        super("BlockMixer", "Picks a random block from your hotbar as you build.", Category.PLAYER, new String[0]);
        this.random = new Random();
    }

    @Listen
    public void onInteractBlock(InteractBlockEvent interactBlockEvent) {
        if (minecraftClient.player.getMainHandStack().getItem() instanceof BlockItem) {
            do3061();
        }
    }

    @Listen
    public void do30(SendImmediatelyEvent sendImmediatelyEvent) {
        if ((sendImmediatelyEvent.getPacket904() instanceof PlayerInteractBlockC2SPacket) && (minecraftClient.player.getMainHandStack().getItem() instanceof BlockItem)) {
            do3061();
        }
    }

    public void do3061() {
        ArrayList arrayList = new ArrayList();
        for (int i = 0; i < 9; i++) {
            ItemStack stack = minecraftClient.player.getInventory().getStack(i);
            if (!stack.isEmpty() && (stack.getItem() instanceof BlockItem)) {
                arrayList.add(Integer.valueOf(i));
            }
        }
        if (arrayList.size() < 2) {
            return;
        }
        minecraftClient.player.getInventory().selectedSlot = ((Integer) arrayList.get(this.random.nextInt(arrayList.size()))).intValue();
    }
}
