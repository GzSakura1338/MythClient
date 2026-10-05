package today.starain.module.impl.movement;

import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.game.*;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.item.UseAnim;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.event.impl.MoveInputEvent;
import today.starain.event.impl.PacketEvent;
import today.starain.event.impl.SlowdownEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.client.NetworkUtils;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ModeValue;

import java.util.concurrent.LinkedBlockingQueue;

public class NoSlow extends ClientModule {
    private final ModeValue mode = new ModeValue("Mode", "Vanilla", new String[]{"Vanilla", "Jump", "GrimFull", "Grim1/2", "Grim1/3"}, this);
    private final BooleanValue food = new BooleanValue("Food", true, this);
    private final BooleanValue bow = new BooleanValue("Bow", true, this);
    private final BooleanValue crossbow = new BooleanValue("Crossbow", true, this);

    private int ticks;
    private int useDuration = 32;
    private int onGroundTick = 0;
    private boolean eating;
    private final LinkedBlockingQueue<Packet<ClientGamePacketListener>> packets = new LinkedBlockingQueue<>();
    private final NoSlowFullMode fullMode = new NoSlowFullMode();

    public NoSlow() {
        setName("NoSlow");
        setCategory(Category.MOVEMENT);
    }

    @Override
    protected void onDisable() {
        flush();
        eating = false;
        ticks = 0;
        useDuration = 32;
        fullMode.onDisable();
    }

    @EventTarget
    private void onMotion(MotionEvent event) {
        if (event.getType() != EventType.PRE || mc.player == null) return;

        if (mc.player.onGround()) {
            onGroundTick++;
        } else {
            onGroundTick = 0;
        }

        if (mode.isMode("GrimFull") && mc.player.tickCount >= 30) {
            fullMode.onPreTick();
        }
    }

    @EventTarget
    private void onPacket(PacketEvent event) {
        if (mc.player == null || mc.player.tickCount < 30 || !mode.isMode("GrimFull")) return;
        fullMode.onPacket(event);
    }

    @EventTarget
    private void onSlowdown(SlowdownEvent event) {
        if (mc.player.tickCount < 30) return;

        if (!food.get() && mc.player.getUseItem().getItem().isEdible()) return;
        if (!bow.get() && mc.player.getUseItem().is(Items.BOW)) return;
        if (!crossbow.get() && mc.player.getUseItem().is(Items.CROSSBOW)) return;

        switch (mode.get()) {
            case "Vanilla" -> event.setCancelled(true);
            case "GrimFull" -> fullMode.onSlow(event);
            case "Jump" -> jump(event);
            case "Grim1/2" -> grim50(event);
            case "Grim1/3" -> grim33(event);
        }
    }

    @EventTarget
    private void onMoveInput(MoveInputEvent event) {
        if (mc.player == null) return;
        if (mode.isMode("GrimFull")) {
            fullMode.onMoveInput(event);
            return;
        }
        if (mode.isMode("Jump") && mc.player.onGround() && mc.player.isUsingItem() && (event.getForward() != 0 || event.getStrafe() != 0)) {
            event.setJump(true);
        }
    }

    private void jump(SlowdownEvent event) {
        if (onGroundTick == 1 && mc.player.getUseItemRemainingTicks() <= 30) {
            event.setCancelled(true);
        }
    }

    private void grim50(SlowdownEvent event) {
        if (mc.player.getUseItemRemainingTicks() % 2 == 0 && mc.player.getUseItemRemainingTicks() <= 30) {
            event.setCancelled(true);
        }
    }

    private void grim33(SlowdownEvent event) {
        if (mc.player.getUseItemRemainingTicks() % 3 == 0 && mc.player.getUseItemRemainingTicks() <= 30) {
            event.setCancelled(true);
        }
    }

    private void flush() {
        Packet<ClientGamePacketListener> packet;
        while ((packet = packets.poll()) != null) {
            packet.handle(mc.getConnection());
        }
    }
}
