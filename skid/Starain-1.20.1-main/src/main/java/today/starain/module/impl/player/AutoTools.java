package today.starain.module.impl.player;

import net.minecraft.core.BlockPos;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.SwordItem;
import net.minecraft.world.item.enchantment.EnchantmentHelper;
import net.minecraft.world.item.enchantment.Enchantments;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.DropExperienceBlock;
import net.minecraft.world.level.block.RedStoneOreBlock;
import net.minecraft.world.level.block.WebBlock;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.HitResult;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.player.InventoryUtils;
import today.starain.value.impl.BooleanValue;

public class AutoTools extends ClientModule {
    private final BooleanValue switchBack = new BooleanValue("Switch Back", true, this);

    private int originSlot = -1;

    public AutoTools() {
        setName("AutoTools");
        setCategory(Category.PLAYER);
    }

    @EventTarget
    public void onMotion(MotionEvent e) {
        if (mc == null || mc.player == null || mc.level == null || mc.gameMode == null) return;
        if (e.getType() == EventType.PRE) {
            if (mc.gameMode.isDestroying()) {
                if (mc.hitResult.getType() == HitResult.Type.BLOCK) {
                    BlockHitResult hitResult = (BlockHitResult) mc.hitResult;
                    int bestTool = this.getBestTool(hitResult.getBlockPos());
                    if (bestTool != -1 && bestTool != mc.player.getInventory().selected) {
                        this.originSlot = mc.player.getInventory().selected;
                        mc.player.getInventory().selected = bestTool;
                    }
                }
            }
        } else if (!mc.gameMode.isDestroying() && this.switchBack.get() && this.originSlot != -1) {
            mc.player.getInventory().selected = this.originSlot;
            this.originSlot = -1;
        }
    }

    private int getBestTool(BlockPos pos) {
        BlockState blockState = mc.level.getBlockState(pos);
        Block block = blockState.getBlock();
        int slot = 0;
        float dmg = 1.0F;

        for (int index = 0; index < 9; index++) {
            ItemStack itemStack = mc.player.getInventory().getItem(index);
            if (!InventoryUtils.isGodItem(itemStack)
                    && !itemStack.isEmpty()
                    && !blockState.isAir()
                    && (!(itemStack.getItem() instanceof SwordItem) || block instanceof WebBlock)) {
                float strVsBlock = itemStack.getItem().getDestroySpeed(itemStack, blockState);
                if (strVsBlock > 1.0F && !(block instanceof DropExperienceBlock) && !(block instanceof RedStoneOreBlock)) {
                    int i = EnchantmentHelper.getItemEnchantmentLevel(Enchantments.BLOCK_EFFICIENCY, itemStack);
                    if (i > 0) {
                        strVsBlock += (float) (i * i + 1);
                    }
                }

                if (strVsBlock > dmg) {
                    slot = index;
                    dmg = strVsBlock;
                }
            }
        }

        return dmg > 1.0F ? slot : -1;
    }
}
