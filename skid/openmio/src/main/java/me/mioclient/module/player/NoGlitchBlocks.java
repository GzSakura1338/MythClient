package me.mioclient.module.player;

import me.mioclient.ArgumentTypeHelper;
import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.BreakBlockEvent;
import me.mioclient.event.InteractBlockEvent;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import net.minecraft.item.BlockItem;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/NoGlitchBlocks.class */
public class NoGlitchBlocks extends Module {
    public Setting<Boolean> place;
    public Setting<Boolean> break_;

    public NoGlitchBlocks() {
        super("NoGlitchBlocks", new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("Makes sure there are no ghost blocks as you interact with them. \n\u0001Not recommended on 2b2t/Grim servers."), Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void onInteractBlock(InteractBlockEvent interactBlockEvent) {
        BlockItem item = (minecraftClient.player.getStackInHand(interactBlockEvent.getHand2084()).getItem()) instanceof BlockItem ? (BlockItem) (minecraftClient.player.getStackInHand(interactBlockEvent.getHand2084()).getItem()) : null;
        if (this.place.getValue().booleanValue() && (item instanceof BlockItem)) {
            BlockItem blockItem = item;
            if (PhaseESPSearchHelper4_2.is3039(interactBlockEvent.getBlockHitResult2585().getBlockPos().offset(interactBlockEvent.getBlockHitResult2585().getSide()), blockItem.getBlock(), false)) {
                AutoSignSearchHelper4.do2556(interactBlockEvent.getHand2084(), interactBlockEvent.getBlockHitResult2585());
                minecraftClient.player.swingHand(interactBlockEvent.getHand2084());
                interactBlockEvent.do1162();
            }
        }
    }

    @Listen
    public void onBreakBlock(BreakBlockEvent breakBlockEvent) {
        if (this.break_.getValue().booleanValue()) {
            breakBlockEvent.do1162();
        }
    }
}
