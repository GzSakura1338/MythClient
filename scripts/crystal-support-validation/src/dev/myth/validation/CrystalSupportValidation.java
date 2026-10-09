package dev.myth.validation;

import dev.myth.Myth;
import dev.myth.model.combat.AutoCrystal;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientLifecycleEvents;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.world.level.BlockGetter;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.material.FluidState;
import net.minecraft.world.level.material.Fluids;

/**
 * AutoCrystal's Support scan runs against a block snapshot, so the rule that decides whether a spot
 * can be built at all is testable without a world. The regression this guards: canBuild used to
 * accept any replaceable block with air above, which scored mid-air spots. The highest crystal
 * damage usually sits in the air beside the target, so that spot won every tick and the obsidian
 * followed the target around instead of staying where the damage was.
 */
public final class CrystalSupportValidation implements ClientModInitializer {
    private final List<String> results = new ArrayList<>();

    @Override
    public void onInitializeClient() {
        ClientLifecycleEvents.CLIENT_STARTED.register(client -> {
            try {
                run();
            } catch (Throwable error) {
                fail("validation crashed", error.toString());
                Myth.LOGGER.error("Crystal support validation crashed", error);
            }
            write();
            client.stop();
        });
    }

    private void run() {
        AutoCrystal module = Myth.MODEL_MANAGER.getModel(AutoCrystal.class).orElse(null);
        check(module != null, "AutoCrystal is registered", "not found in the model manager");
        if (module == null) return;
        BlockPos spot = new BlockPos(0, 64, 0);

        check(!module.airPlace.get(), "AirPlace defaults to off", "on by default");

        // Nothing but air anywhere: there is no face to click, so the spot is not buildable.
        Level floating = new Level();
        check(!module.canBuild(floating, spot), "a mid-air spot is rejected", "scored a floating spot");
        check(module.supportDirection(floating, spot) == null,
                "a mid-air spot has no support face", "found a face in thin air");

        // Obsidian underneath: buildable, and the click has to land on the block below.
        Level ground = new Level();
        ground.set(spot.below(), Blocks.OBSIDIAN.defaultBlockState());
        check(module.canBuild(ground, spot), "a spot standing on a block is accepted", "rejected a real spot");
        check(module.supportDirection(ground, spot) == Direction.UP,
                "the support face points up off the block below", "got " + module.supportDirection(ground, spot));

        // Mio's order prefers the block below; a side neighbour still works when it is the only one.
        Level wall = new Level();
        wall.set(spot.north(), Blocks.OBSIDIAN.defaultBlockState());
        check(module.canBuild(wall, spot), "a spot against a wall is accepted", "rejected a wall spot");
        check(module.supportDirection(wall, spot) == Direction.SOUTH,
                "the support face points away from the wall", "got " + module.supportDirection(wall, spot));

        // A crystal needs the space above, so a covered spot stays out of the scan either way.
        Level covered = new Level();
        covered.set(spot.below(), Blocks.OBSIDIAN.defaultBlockState());
        covered.set(spot.above(), Blocks.OBSIDIAN.defaultBlockState());
        check(!module.canBuild(covered, spot), "a covered spot is rejected", "scored a covered spot");

        // The spot itself has to be replaceable.
        Level occupied = new Level();
        occupied.set(spot, Blocks.OBSIDIAN.defaultBlockState());
        occupied.set(spot.below(), Blocks.OBSIDIAN.defaultBlockState());
        check(!module.canBuild(occupied, spot), "an occupied spot is rejected", "scored an occupied spot");

        // AirPlace is the explicit opt-in that brings the mid-air spots back.
        try {
            module.airPlace.set(true);
            check(module.canBuild(floating, spot), "AirPlace accepts a mid-air spot", "still rejected");
        } finally {
            module.airPlace.set(false);
        }
        check(!module.canBuild(floating, spot), "turning AirPlace back off rejects it again", "still accepted");
    }

    /** A flat air snapshot with individually placed blocks; enough for the Support rule. */
    private static final class Level implements BlockGetter {
        private final Map<BlockPos, BlockState> blocks = new HashMap<>();
        void set(BlockPos pos, BlockState state) { blocks.put(pos.immutable(), state); }
        @Override public BlockState getBlockState(BlockPos pos) {
            return blocks.getOrDefault(pos, Blocks.AIR.defaultBlockState());
        }
        @Override public FluidState getFluidState(BlockPos pos) { return Fluids.EMPTY.defaultFluidState(); }
        @Override public BlockEntity getBlockEntity(BlockPos pos) { return null; }
        @Override public int getHeight() { return 384; }
        @Override public int getMinY() { return -64; }
    }

    private void check(boolean condition, String label, String detail) {
        if (condition) results.add("PASS " + label); else results.add("FAIL " + label + " (" + detail + ")");
    }
    private void fail(String label, String detail) { results.add("FAIL " + label + " (" + detail + ")"); }

    private void write() {
        long failures = results.stream().filter(line -> line.startsWith("FAIL")).count();
        results.add("SUMMARY " + (results.size() - failures) + " pass, " + failures + " fail");
        String report = String.join(System.lineSeparator(), results) + System.lineSeparator();
        Path root = Path.of(System.getProperty("myth.validation.root", "."));
        try {
            Files.createDirectories(root);
            Files.writeString(root.resolve("result.txt"), report, StandardCharsets.UTF_8);
        } catch (IOException error) {
            Myth.LOGGER.error("Could not write the crystal support validation report", error);
        }
        results.forEach(line -> Myth.LOGGER.info("[crystal-support-validation] {}", line));
    }
}
