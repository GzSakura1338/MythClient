package me.mioclient.event;

import net.minecraft.util.Hand;
import net.minecraft.util.hit.BlockHitResult;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/InteractBlockEvent.class */
public class InteractBlockEvent extends Event {
    public final BlockHitResult blockHitResult;
    public final Hand hand;

    public InteractBlockEvent(BlockHitResult blockHitResult, Hand hand) {
        this.blockHitResult = blockHitResult;
        this.hand = hand;
    }

    public BlockHitResult getBlockHitResult2585() {
        return this.blockHitResult;
    }

    public Hand getHand2084() {
        return this.hand;
    }
}
