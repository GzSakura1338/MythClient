package today.starain.module.impl.movement;

import net.minecraft.client.Minecraft;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.game.ClientboundContainerSetContentPacket;
import net.minecraft.network.protocol.game.ClientboundContainerSetSlotPacket;
import net.minecraft.network.protocol.game.ServerboundContainerClosePacket;
import net.minecraft.network.protocol.game.ServerboundPlayerActionPacket;
import net.minecraft.network.protocol.game.ServerboundPongPacket;
import net.minecraft.network.protocol.game.ServerboundUseItemPacket;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.PotionItem;
import today.starain.event.api.EventType;
import today.starain.event.impl.MoveInputEvent;
import today.starain.event.impl.PacketEvent;
import today.starain.event.impl.SlowdownEvent;
import today.starain.utils.client.NetworkUtils;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

final class NoSlowFullMode {

    private enum Step {
        NONE,
        CANCEL_PONG,
        SWAP_HANDS,
        USING
    }

    private final Minecraft mc = Minecraft.getInstance();
    private final Queue<ServerboundPongPacket> queuedPongs = new ConcurrentLinkedQueue<>();
    private Step step = Step.NONE;
    private int noUsingItemTicks = 0;
    private InteractionHand targetHand = InteractionHand.MAIN_HAND;
    private boolean swapSent = false;
    private boolean forcedUseKeyDown = false;

    void reset() {
        this.step = Step.NONE;
        this.noUsingItemTicks = 0;
        this.queuedPongs.clear();
        this.targetHand = InteractionHand.MAIN_HAND;
        this.swapSent = false;
        this.forcedUseKeyDown = false;
    }

    void onDisable() {
        abort(true);
    }

    void onPreTick() {
        if (mc.player == null) {
            return;
        }

        boolean using = isUsingConsumable();
        if (using) {
            if (this.step == Step.NONE) {
                InteractionHand usedHand = mc.player.getUsedItemHand();
                this.targetHand = usedHand == InteractionHand.MAIN_HAND ? InteractionHand.OFF_HAND : InteractionHand.MAIN_HAND;
                ItemStack opposite = mc.player.getItemInHand(this.targetHand);
                if (isConsumable(opposite)) {
                    return;
                }
                this.step = Step.CANCEL_PONG;
                this.swapSent = false;
                if (mc.player.containerMenu != mc.player.inventoryMenu && mc.getConnection() != null) {
                    NetworkUtils.sendPacketNoEvent(new ServerboundContainerClosePacket(mc.player.containerMenu.containerId));
                }
            }
            if (mc.options != null && this.step != Step.USING) {
                mc.options.keyUse.setDown(false);
                this.forcedUseKeyDown = true;
            }
        }

        if (this.step == Step.USING) {
            if (using) {
                this.noUsingItemTicks = 0;
            } else {
                this.noUsingItemTicks++;
                if (this.noUsingItemTicks >= 5) {
                    releaseQueuedPongs();
                    NetworkUtils.sendPacketNoEvent(new ServerboundPlayerActionPacket(
                            ServerboundPlayerActionPacket.Action.SWAP_ITEM_WITH_OFFHAND,
                            BlockPos.ZERO,
                            Direction.DOWN
                    ));
                    abort(false);
                }
            }
        } else {
            this.noUsingItemTicks = 0;
        }
    }

    void onSlow(SlowdownEvent event) {
        if (mc.player == null || this.step != Step.USING || !isUsingConsumable()) {
            return;
        }
        event.setCancelled(true);
    }

    void onMoveInput(MoveInputEvent event) {
        if (mc.player == null) {
            return;
        }
        if (this.step == Step.USING && !mc.player.isShiftKeyDown()) {
            mc.player.setSprinting(true);
        }
    }

    @SuppressWarnings("unchecked")
    void onPacket(PacketEvent event) {
        if (mc.player == null) {
            return;
        }

        Packet<?> packet = event.getPacket();
        if (event.getType() == EventType.OUTGOING) {
            if (packet instanceof ServerboundPongPacket pong && this.step != Step.NONE) {
                event.setCancelled(true);
                this.queuedPongs.add(pong);
                if (this.step == Step.CANCEL_PONG) {
                    this.step = Step.SWAP_HANDS;
                    this.swapSent = true;
                    swapSilent();
                }
                return;
            }

            if (packet instanceof ServerboundPlayerActionPacket actionPacket
                    && actionPacket.getAction() == ServerboundPlayerActionPacket.Action.RELEASE_USE_ITEM
                    && this.step == Step.USING) {
                abort(true);
                return;
            }

            if (packet instanceof ServerboundUseItemPacket usePacket
                    && this.step == Step.USING
                    && this.swapSent
                    && usePacket.getHand() != this.targetHand) {
                event.setCancelled(true);
                NetworkUtils.sendPacketNoEvent(new ServerboundUseItemPacket(this.targetHand, usePacket.getSequence()));
            }
            return;
        }

        if (event.getType() == EventType.INCOMING && this.step == Step.SWAP_HANDS) {
            if (packet instanceof ClientboundContainerSetSlotPacket || packet instanceof ClientboundContainerSetContentPacket) {
                if (mc.options != null) {
                    mc.options.keyUse.setDown(true);
                    this.forcedUseKeyDown = true;
                }
                this.step = Step.USING;
                this.noUsingItemTicks = 0;
            }
        }
    }

    private boolean isConsumable(ItemStack stack) {
        if (stack == null || stack.isEmpty()) {
            return false;
        }
        return stack.isEdible() || stack.getItem() instanceof PotionItem;
    }

    private boolean isUsingConsumable() {
        return mc.player != null
                && mc.player.isUsingItem()
                && mc.player.getUseItemRemainingTicks() > 0
                && isConsumable(mc.player.getUseItem());
    }

    private void releaseQueuedPongs() {
        ServerboundPongPacket pong;
        while ((pong = queuedPongs.poll()) != null) {
            NetworkUtils.sendPacketNoEvent(pong);
        }
    }

    private void swapSilent() {
        NetworkUtils.sendPacketNoEvent(new ServerboundPlayerActionPacket(
                ServerboundPlayerActionPacket.Action.SWAP_ITEM_WITH_OFFHAND,
                BlockPos.ZERO,
                Direction.DOWN
        ));
    }

    private void abort(boolean revertSwap) {
        releaseQueuedPongs();
        if (revertSwap && this.swapSent) {
            swapSilent();
        }
        this.step = Step.NONE;
        this.noUsingItemTicks = 0;
        this.swapSent = false;
        this.targetHand = InteractionHand.MAIN_HAND;
        if (this.forcedUseKeyDown && mc.options != null) {
            mc.options.keyUse.setDown(false);
        }
        this.forcedUseKeyDown = false;
    }
}
