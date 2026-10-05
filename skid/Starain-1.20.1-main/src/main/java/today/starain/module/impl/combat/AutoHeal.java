package today.starain.module.impl.combat;

import net.minecraft.client.multiplayer.prediction.BlockStatePredictionHandler;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.network.protocol.game.ServerboundPlayerActionPacket;
import net.minecraft.network.protocol.game.ServerboundPlayerActionPacket.Action;
import net.minecraft.network.protocol.game.ServerboundSetCarriedItemPacket;
import net.minecraft.network.protocol.game.ServerboundUseItemPacket;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.effect.MobEffects;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.player.InventoryUtils;
import today.starain.utils.timer.TimerUtils;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;


public class AutoHeal extends ClientModule {
    private final BooleanValue speedCheck = new BooleanValue("Speed Check", true, this);
    private final BooleanValue regenCheck = new BooleanValue("Regen Check", true, this);
    private final NumberValue delay = new NumberValue("Delay", 300, 1000, 500, 1, this);
    private final NumberValue health = new NumberValue("Health Percent", 0.0, 1.0, 0.5, 0.05, this);
    private final ModeValue mode = new ModeValue("Mode", "Soup", new String[]{"Soup", "Head"}, this);

    private final TimerUtils timer = new TimerUtils();
    private boolean switchBack = false;
    private boolean useItem = false;
    private boolean throwItem = false;

    public AutoHeal() {
        setName("AutoHeal");
        setCategory(Category.COMBAT);
    }

    @EventTarget
    public void onMotion(MotionEvent e) {
        if (e.getType() != EventType.PRE) {
            return;
        }
        if (mc.player == null || mc.level == null || mc.getConnection() == null) {
            return;
        }

        // 上一 tick 排队的动作按序补发：用物品 -> 丢空碗 -> 切回原槽位。
        if (this.useItem) {
            try (BlockStatePredictionHandler handler = mc.level.getBlockStatePredictionHandler().startPredicting()) {
                mc.getConnection().send(new ServerboundUseItemPacket(InteractionHand.MAIN_HAND, handler.currentSequence()));
            }
            this.useItem = false;
            return;
        }

        if (this.throwItem) {
            mc.getConnection().send(new ServerboundPlayerActionPacket(Action.DROP_ITEM, BlockPos.ZERO, Direction.DOWN));
            this.throwItem = false;
            return;
        }

        if (this.switchBack) {
            mc.getConnection().send(new ServerboundSetCarriedItemPacket(mc.player.getInventory().selected));
            this.switchBack = false;
            return;
        }

        if (!this.timer.delay(this.delay.get().longValue())) {
            return;
        }

        if (this.speedCheck.get() && mc.player.hasEffect(MobEffects.MOVEMENT_SPEED)) {
            return;
        }

        if (this.regenCheck.get() && mc.player.hasEffect(MobEffects.REGENERATION)) {
            return;
        }

        if (mc.player.getHealth() / mc.player.getMaxHealth() >= this.health.get().floatValue()) {
            return;
        }

        if (this.mode.isMode("Soup")) {
            for (int i = 0; i < 9; i++) {
                ItemStack stack = mc.player.getInventory().items.get(i);
                if (stack.getItem() == Items.MUSHROOM_STEW) {
                    this.switchUseItem(i, true);
                    this.switchBack = true;
                    this.timer.reset();
                    break;
                }
            }
        } else if (this.mode.isMode("Head")) {
            for (int i = 0; i < 9; i++) {
                ItemStack stack = mc.player.getInventory().items.get(i);
                if (InventoryUtils.isGoldenHead(stack)) {
                    this.switchUseItem(i, false);
                    this.switchBack = true;
                    this.timer.reset();
                    break;
                }
            }
        }
    }

    private void switchUseItem(int slot, boolean throwItem) {
        mc.getConnection().send(new ServerboundSetCarriedItemPacket(slot));
        this.throwItem = throwItem;
        this.useItem = true;
    }
}
