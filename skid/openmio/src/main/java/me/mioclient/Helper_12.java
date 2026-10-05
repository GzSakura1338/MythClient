package me.mioclient;

import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;
import net.minecraft.block.AbstractSignBlock;
import net.minecraft.block.BannerBlock;
import net.minecraft.block.BedBlock;
import net.minecraft.block.Block;
import net.minecraft.block.Blocks;
import net.minecraft.block.CarpetBlock;
import net.minecraft.block.DoorBlock;
import net.minecraft.block.ShulkerBoxBlock;
import net.minecraft.block.SlabBlock;
import net.minecraft.block.StainedGlassBlock;
import net.minecraft.block.StainedGlassPaneBlock;
import net.minecraft.registry.Registries;
import net.minecraft.registry.RegistryKeys;
import net.minecraft.util.Identifier;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Helper_12.class */
public class Helper_12 implements Helper {
    public static final Map<String, List<String>> map = new HashMap();

    @Override // me.mioclient.Helper
    public Collection<String> getCollection53(String str) {
        return map.getOrDefault(str.toLowerCase(), Collections.emptyList());
    }

    @Override // me.mioclient.Helper
    public Collection<String> getCollection54() {
        return map.keySet();
    }

    @Override // me.mioclient.Helper
    public Identifier getIdentifier55() {
        return RegistryKeys.BLOCK.getValue();
    }

    public static void do1276(String str, Stream<Block> stream) {
        map.put(str, stream.map(Helper_12::getString1277).toList());
    }

    public static String getString1277(Block block) {
        return Registries.BLOCK.getId(block).toShortTranslationKey();
    }

    static {
        do1276("beds", Registries.BLOCK.stream().filter(block -> {
            return block instanceof BedBlock;
        }));
        do1276("banners", Registries.BLOCK.stream().filter(block2 -> {
            return block2 instanceof BannerBlock;
        }));
        do1276("doors", Registries.BLOCK.stream().filter(block3 -> {
            return block3 instanceof DoorBlock;
        }));
        do1276("slabs", Registries.BLOCK.stream().filter(block4 -> {
            return block4 instanceof SlabBlock;
        }));
        do1276("shulkers", Registries.BLOCK.stream().filter(block5 -> {
            return block5 instanceof ShulkerBoxBlock;
        }));
        do1276("carpets", Registries.BLOCK.stream().filter(block6 -> {
            return block6 instanceof CarpetBlock;
        }));
        do1276("glasses", Registries.BLOCK.stream().filter(block7 -> {
            return (block7 instanceof StainedGlassBlock) || block7 == Blocks.GLASS;
        }));
        do1276("glass_panes", Registries.BLOCK.stream().filter(block8 -> {
            return (block8 instanceof StainedGlassPaneBlock) || block8 == Blocks.GLASS_PANE;
        }));
        do1276("signs", Registries.BLOCK.stream().filter(block9 -> {
            return block9 instanceof AbstractSignBlock;
        }));
        do1276("ores", Registries.BLOCK.streamEntries().filter(reference -> {
            return reference.matches(registryKey -> {
                return registryKey.getValue().toTranslationKey().contains("ore");
            });
        }).map((v0) -> {
            return v0.value();
        }));
    }
}
