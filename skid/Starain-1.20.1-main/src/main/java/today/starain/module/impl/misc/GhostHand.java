package today.starain.module.impl.misc;

import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.ChestBlock;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.world.level.block.entity.ChestBlockEntity;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.ChestType;
import net.minecraft.world.level.chunk.LevelChunk;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.Vec3;
import net.minecraft.world.phys.shapes.Shapes;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.ClickEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;

import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public class GhostHand extends ClientModule {

    private static final Set<Block> BLOCKS = new HashSet<>();

    static {
        BLOCKS.add(Blocks.CHEST);
        BLOCKS.add(Blocks.ENDER_CHEST);
        BLOCKS.add(Blocks.TRAPPED_CHEST);
        BLOCKS.add(Blocks.SHULKER_BOX);
    }

    public GhostHand() {
        setName("GhostHand");
        setCategory(Category.MISC);
    }

    @EventTarget
    public void onClick(ClickEvent event) {
        if (mc.options.keyUse.isDown()) {
            if (ghostInteractWithChest()) {
                event.setCancelled(true);
            }
        }
    }

    public boolean ghostInteractWithChest() {
        if (mc.player == null || mc.level == null) {
            return false;
        }
        Vec3 eyePos = mc.player.getEyePosition(1.0f);
        Vec3 lookVec = mc.player.getViewVector(1.0f);
        Vec3 reachEnd = eyePos.add(lookVec.scale(4.5));
        ChestBlockEntity targetChest = null;
        BlockHitResult fakeHit = null;
        double closestDist = Double.MAX_VALUE;
        ArrayList<BlockEntity> blockEntities = getLoadedBlockEntities().collect(Collectors.toCollection(ArrayList::new));
        for (BlockEntity be : blockEntities) {
            double dist;
            Optional<Vec3> hit;
            ChestBlockEntity chest;
            AABB box;
            if (!(be instanceof ChestBlockEntity) || (box = this.getChestBox(chest = (ChestBlockEntity) be)) == null || !(hit = box.clip(eyePos, reachEnd)).isPresent() || !((dist = hit.get().distanceTo(eyePos)) < closestDist))
                continue;
            closestDist = dist;
            targetChest = chest;
            fakeHit = new BlockHitResult(hit.get(), Direction.UP, chest.getBlockPos(), false);
        }
        if (targetChest != null && fakeHit != null) {
            mc.gameMode.useItemOn(mc.player, InteractionHand.MAIN_HAND, fakeHit);
            mc.player.swing(InteractionHand.MAIN_HAND);
            return true;
        }
        return false;
    }

    private AABB getChestBox(ChestBlockEntity chestBE) {
        BlockPos pos2;
        BlockState state = chestBE.getBlockState();
        if (!state.hasProperty(ChestBlock.TYPE)) {
            return null;
        }
        ChestType chestType = state.getValue(ChestBlock.TYPE);
        if (chestType == ChestType.LEFT) {
            return null;
        }
        BlockPos pos = chestBE.getBlockPos();
        AABB box = getBoundingBox(pos);
        if (chestType != ChestType.SINGLE && canBeClicked(pos2 = pos.relative(ChestBlock.getConnectedDirection(state)))) {
            AABB box2 = getBoundingBox(pos2);
            box = box.minmax(box2);
        }
        return box;
    }

    // ---- 内联自 NaCl BlockUtils / ChunkUtils ----

    private AABB getBoundingBox(BlockPos pos) {
        return mc.level.getBlockState(pos).getShape(mc.level, pos).bounds().move(pos);
    }

    private boolean canBeClicked(BlockPos pos) {
        return mc.level.getBlockState(pos).getShape(mc.level, pos) != Shapes.empty();
    }

    private Stream<BlockEntity> getLoadedBlockEntities() {
        return getLoadedChunks().flatMap(chunk -> chunk.getBlockEntities().values().stream());
    }

    private Stream<LevelChunk> getLoadedChunks() {
        int radius = Math.max(2, mc.options.getEffectiveRenderDistance()) + 3;
        int diameter = radius * 2 + 1;
        ChunkPos center = mc.player.chunkPosition();
        ChunkPos min = new ChunkPos(center.x - radius, center.z - radius);
        ChunkPos max = new ChunkPos(center.x + radius, center.z + radius);
        return Stream.iterate(min, pos -> {
            int x = pos.x;
            int z = pos.z;
            if (++x > max.x) {
                x = min.x;
                z++;
            }
            if (z > max.z) {
                throw new IllegalStateException("Stream limit didn't work.");
            } else {
                return new ChunkPos(x, z);
            }
        }).limit((long) diameter * (long) diameter).filter(c -> mc.level.hasChunk(c.x, c.z)).map(c -> mc.level.getChunk(c.x, c.z)).filter(Objects::nonNull);
    }
}
