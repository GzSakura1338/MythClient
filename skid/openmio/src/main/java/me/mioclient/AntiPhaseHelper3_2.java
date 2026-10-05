package me.mioclient;

import java.util.List;
import net.minecraft.block.Block;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AntiPhaseHelper3_2.class */
public class AntiPhaseHelper3_2 extends AntiPhaseHelper3 {
    public static final List<AntiPhaseHelperData> list = List.of(AntiPhaseHelperData.getAntiPhaseHelperData1239(Direction.EAST, Block.createCuboidShape(0.0d, Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4607182418800017408L), Double.longBitsToDouble(4624070917402656768L), Double.longBitsToDouble(4624070917402656768L))), AntiPhaseHelperData.getAntiPhaseHelperData1239(Direction.WEST, Block.createCuboidShape(Double.longBitsToDouble(4624633867356078080L), Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4625196817309499392L), Double.longBitsToDouble(4624070917402656768L), Double.longBitsToDouble(4624070917402656768L))), AntiPhaseHelperData.getAntiPhaseHelperData1239(Direction.NORTH, Block.createCuboidShape(Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4611686018427387904L), 0.0d, Double.longBitsToDouble(4624070917402656768L), Double.longBitsToDouble(4624070917402656768L), Double.longBitsToDouble(4607182418800017408L))), AntiPhaseHelperData.getAntiPhaseHelperData1239(Direction.SOUTH, Block.createCuboidShape(Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4624633867356078080L), Double.longBitsToDouble(4624070917402656768L), Double.longBitsToDouble(4624070917402656768L), Double.longBitsToDouble(4625196817309499392L))), AntiPhaseHelperData.getAntiPhaseHelperData1239(Direction.DOWN, Block.createCuboidShape(Double.longBitsToDouble(4611686018427387904L), 0.0d, Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4624070917402656768L), Double.longBitsToDouble(4607182418800017408L), Double.longBitsToDouble(4624070917402656768L))), AntiPhaseHelperData.getAntiPhaseHelperData1239(Direction.UP, Block.createCuboidShape(Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4624633867356078080L), Double.longBitsToDouble(4611686018427387904L), Double.longBitsToDouble(4624070917402656768L), Double.longBitsToDouble(4625196817309499392L), Double.longBitsToDouble(4624070917402656768L))));

    @Override // me.mioclient.AntiPhaseHelper3
    public Box getBox2417(BlockPos blockPos, Direction direction) {
        for (AntiPhaseHelperData antiPhaseHelperData : list) {
            if (antiPhaseHelperData.getDirection842() == direction.getOpposite()) {
                return antiPhaseHelperData.getBox799().offset(blockPos);
            }
        }
        return new Box(blockPos);
    }
}
