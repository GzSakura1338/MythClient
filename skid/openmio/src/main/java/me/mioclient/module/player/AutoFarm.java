package me.mioclient.module.player;

import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.function.Predicate;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingHelper;
import me.mioclient.HoleSnapEvent;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper4_7;
import me.mioclient.SearchHelper4_8;
import me.mioclient.SearchHelper_4;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.block.CropBlock;
import net.minecraft.block.SaplingBlock;
import net.minecraft.block.SugarCaneBlock;
import net.minecraft.fluid.FluidState;
import net.minecraft.item.BlockItem;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.registry.tag.BlockTags;
import net.minecraft.registry.tag.FluidTags;
import net.minecraft.util.Hand;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.BlockStateRaycastContext;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoFarm.class */
public class AutoFarm extends Module {
    public Setting<AutoFarmMode> mode;
    public Setting<Float> range;
    public Setting<Integer> bpt;
    public Setting<Boolean> place;
    public Setting<Boolean> rotate;
    public Setting<Boolean> fertilize;
    public Setting<Boolean> remove;
    public Setting<Float> removeDelay;
    public Setting<Boolean> onlyGrown;
    public Setting<Boolean> crops;
    public Setting<Boolean> wheat;
    public Setting<Boolean> carrots;
    public Setting<Boolean> potatoes;
    public Setting<Boolean> beetRoots;
    public Setting<Boolean> sugarCane;
    public Setting<Boolean> trees;
    public final Stopwatch stopwatch;
    public final Stopwatch stopwatch2;
    public int num;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoFarm$AutoFarmMode.class */
    public static enum AutoFarmMode implements EnumSettingHelper {
        autoFarmMode("Crosshair") {
            @Override
            public List<BlockPos> getList610(AutoFarm autoFarm) {
                return Collections.singletonList(BlockPos.ofFloored(SearchHelper_4.minecraftClient.crosshairTarget.getPos()));
            }
        },
        autoFarmMode2("Sphere") {
            @Override
            public List<BlockPos> getList610(AutoFarm autoFarm) {
                return SearchHelper4_7.getList2429(SearchHelper_4.minecraftClient.player.getPos(), autoFarm.range.getValue().floatValue(), true);
            }
        };

        public final String name;

        AutoFarmMode(String str2) {
            this.name = str2;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public List<BlockPos> getList610(AutoFarm autoFarm) {
            return null;
        }
    }

    public AutoFarm() {
        super("AutoFarm", "Farms your crops for you.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
        this.stopwatch2 = new Stopwatch();
    }

    @Listen
    public void onEvent(HoleSnapEvent holeSnapEvent) {
        for (BlockPos blockPos : this.mode.getValue().getList610(this)) {
            if (this.num >= this.bpt.getValue().intValue()) {
                break;
            }
            do1849(blockPos, minecraftClient.world.raycast(new BlockStateRaycastContext(minecraftClient.player.getEyePos(), blockPos.toCenterPos(), blockState -> {
                return true;
            })).getSide());
        }
        this.num = 0;
    }

    public void do1849(BlockPos blockPos, Direction direction) {
        BlockState blockState = minecraftClient.world.getBlockState(blockPos);
        boolean isOf = blockState.isOf(Blocks.SUGAR_CANE);
        Hand hand1855 = getHand1855(itemStack -> {
            return is1856(itemStack.getItem());
        });
        if (this.place.getValue().booleanValue() && hand1855 != null) {
            if (is1853(blockPos, blockState, minecraftClient.player.getStackInHand(hand1855).getItem())) {
                do1851(blockPos, hand1855);
                return;
            }
        }
        if (is1856(blockState.getBlock().asItem())) {
            boolean is1854 = is1854(blockState, blockPos);
            Hand hand450 = getHand450(Items.BONE_MEAL);
            if (!this.fertilize.getValue().booleanValue() || is1854 || hand450 == null || isOf) {
                do1850(blockPos, direction, is1854);
            } else {
                do1852(blockPos, hand450);
            }
        }
    }

    public void do1850(BlockPos blockPos, Direction direction, boolean z) {
        if (!(minecraftClient.world.getBlockState(blockPos).getBlock() instanceof SaplingBlock) && this.remove.getValue().booleanValue()) {
            if (this.stopwatch.is418(this.removeDelay.getValue().floatValue(), TimeUnit.SECONDS)) {
                if (z || !this.onlyGrown.getValue().booleanValue()) {
                    minecraftClient.interactionManager.updateBlockBreakingProgress(blockPos, direction);
                    minecraftClient.player.swingHand(Hand.MAIN_HAND);
                    this.stopwatch2.reset();
                    this.num++;
                    this.stopwatch.reset();
                    if (this.rotate.getValue().booleanValue()) {
                        BaritoneHelper_3.searchHelper4_8.do2477(SearchHelper4_8.getFloatArray2484(blockPos.toCenterPos()), 0);
                    }
                }
            }
        }
    }

    public void do1851(BlockPos blockPos, Hand hand) {
        minecraftClient.interactionManager.interactBlock(minecraftClient.player, hand, new BlockHitResult(blockPos.toCenterPos(), Direction.UP, blockPos, false));
        minecraftClient.player.swingHand(hand);
        this.stopwatch2.reset();
        this.num++;
        if (this.rotate.getValue().booleanValue()) {
            BaritoneHelper_3.searchHelper4_8.do2477(SearchHelper4_8.getFloatArray2484(blockPos.toCenterPos()), 0);
        }
    }

    public void do1852(BlockPos blockPos, Hand hand) {
        if (this.rotate.getValue().booleanValue()) {
            BaritoneHelper_3.searchHelper4_8.do2477(SearchHelper4_8.getFloatArray2484(Vec3d.ofBottomCenter((Vec3i) blockPos)), 0);
        }
        minecraftClient.interactionManager.interactBlock(minecraftClient.player, hand, new BlockHitResult(Vec3d.ofBottomCenter((Vec3i) blockPos), Direction.UP, blockPos, false));
        minecraftClient.player.swingHand(hand);
        this.stopwatch2.reset();
        this.num++;
    }

    public boolean is1853(BlockPos blockPos, BlockState blockState, Item item) {
        if (!(item instanceof BlockItem)) {
            return false;
        }
        BlockItem blockItem = (BlockItem) item;
        if (blockItem.getBlock() instanceof CropBlock) {
            if (blockState.isOf(Blocks.FARMLAND)) {
                if (minecraftClient.world.getBlockState(blockPos.up()).isOf(Blocks.AIR)) {
                    return true;
                }
            }
            return false;
        }
        if (!(blockItem.getBlock() instanceof SugarCaneBlock)) {
            return false;
        }
        if (!minecraftClient.world.getBlockState(blockPos.up()).isOf(Blocks.AIR)) {
            return false;
        }
        if (!blockState.isIn(BlockTags.DIRT) && !blockState.isIn(BlockTags.SAND)) {
            return false;
        }
        Iterator it = Direction.Type.HORIZONTAL.iterator();
        while (it.hasNext()) {
            Direction direction = (Direction) it.next();
            FluidState fluidState = minecraftClient.world.getFluidState(blockPos.offset(direction));
            BlockState blockState2 = minecraftClient.world.getBlockState(blockPos.offset(direction));
            if (fluidState.isIn(FluidTags.WATER) || blockState2.isOf(Blocks.FROSTED_ICE)) {
                return true;
            }
        }
        return false;
    }

    public boolean is1854(BlockState blockState, BlockPos blockPos) {
        if (blockState.getBlock() instanceof SaplingBlock) {
            return false;
        }
        CropBlock block = (blockState.getBlock()) instanceof CropBlock ? (CropBlock) (blockState.getBlock()) : null;
        if (block instanceof CropBlock) {
            CropBlock cropBlock = block;
            return cropBlock.getAge(blockState) == cropBlock.getMaxAge();
        }
        if (blockState.getBlock() instanceof SugarCaneBlock) {
            return minecraftClient.world.getBlockState(blockPos.down()).isOf(Blocks.SUGAR_CANE);
        }
        return false;
    }

    public Hand getHand450(Item item) {
        return getHand1855(itemStack -> {
            return itemStack.isOf(item);
        });
    }

    public Hand getHand1855(Predicate<ItemStack> predicate) {
        if (predicate.test(minecraftClient.player.getMainHandStack())) {
            return Hand.MAIN_HAND;
        }
        if (predicate.test(minecraftClient.player.getOffHandStack())) {
            return Hand.OFF_HAND;
        }
        return null;
    }

    public boolean is1856(Item item) {
        return ((item instanceof BlockItem) && (((BlockItem) item).getBlock() instanceof SaplingBlock)) ? this.trees.getValue().booleanValue() : (item == Items.WHEAT_SEEDS && this.wheat.getValue().booleanValue()) || (item == Items.CARROT && this.carrots.getValue().booleanValue()) || ((item == Items.POTATO && this.potatoes.getValue().booleanValue()) || ((item == Items.BEETROOT_SEEDS && this.beetRoots.getValue().booleanValue()) || (item == Items.SUGAR_CANE && this.sugarCane.getValue().booleanValue())));
    }
}
