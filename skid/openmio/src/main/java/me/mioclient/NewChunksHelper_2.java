package me.mioclient;

import it.unimi.dsi.fastutil.ints.IntOpenHashSet;
import it.unimi.dsi.fastutil.ints.IntSet;
import java.util.Iterator;
import java.util.Objects;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.registry.entry.RegistryEntry;
import net.minecraft.util.collection.PaletteStorage;
import net.minecraft.world.biome.BiomeKeys;
import net.minecraft.world.chunk.ArrayPalette;
import net.minecraft.world.chunk.BiMapPalette;
import net.minecraft.world.chunk.ChunkSection;
import net.minecraft.world.chunk.Palette;
import net.minecraft.world.chunk.PalettedContainer;
import net.minecraft.world.chunk.WorldChunk;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NewChunksHelper_2.class */
public final class NewChunksHelper_2 {
    public static final IntSet intSet = new IntOpenHashSet();

    /* JADX INFO: Access modifiers changed from: package-private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/NewChunksHelper_2$Mode.class */
    public enum Mode {
        NO_PLAINS,
        PLAINS_IN_PALETTE,
        PLAINS_PRESENT
    }

    public boolean is1358(WorldChunk worldChunk) {
        switch (SearchHelper4_7.getStashFinderMode2438()) {
            case OVERWORLD:
                switch (getMode1362(worldChunk, true)) {
                    case NO_PLAINS:
                        return false;
                    case PLAINS_IN_PALETTE:
                        return true;
                    case PLAINS_PRESENT:
                        return is1361(worldChunk);
                    default:
                        throw new MatchException((String) null, (Throwable) null);
                }
            case THE_NETHER:
            case THE_END:
                return getMode1362(worldChunk, false) == Mode.PLAINS_IN_PALETTE;
            default:
                throw new MatchException((String) null, (Throwable) null);
        }
    }

    public boolean is1359(Palette<?> palette) {
        return palette.getSize() <= 0 || !((palette instanceof ArrayPalette) || (palette instanceof BiMapPalette));
    }

    public synchronized boolean is1360(PalettedContainer<BlockState> palettedContainer) {
        intSet.clear();
        Palette palette2722 = MixinPalettedContainerHelper.getPalette2722(palettedContainer);
        PaletteStorage paletteStorage2721 = MixinPalettedContainerHelper.getPaletteStorage2721(palettedContainer);
        IntSet intSet2 = intSet;
        Objects.requireNonNull(intSet2);
        paletteStorage2721.forEach(intSet2::add);
        return palette2722.getSize() > intSet.size();
    }

    public boolean is1361(WorldChunk worldChunk) {
        ChunkSection[] sectionArray = worldChunk.getSectionArray();
        if (sectionArray.length == 0) {
            return false;
        }
        Palette<?> palette2722 = MixinPalettedContainerHelper.getPalette2722(sectionArray[0].getBlockStateContainer());
        if (is1359(palette2722)) {
            return false;
        }
        if (palette2722 instanceof ArrayPalette) {
            return ((BlockState) palette2722.get(0)).getBlock() == Blocks.AIR;
        }
        for (int i = 0; i < Math.min(sectionArray.length, 3); i++) {
            PalettedContainer<BlockState> blockStateContainer = sectionArray[i].getBlockStateContainer();
            if (!is1359(MixinPalettedContainerHelper.getPalette2722(blockStateContainer)) && is1360(blockStateContainer)) {
                return true;
            }
        }
        return false;
    }

    public synchronized Mode getMode1362(WorldChunk worldChunk, boolean z) {
        ChunkSection[] sectionArray = worldChunk.getSectionArray();
        if (sectionArray.length == 0) {
            return Mode.NO_PLAINS;
        }
        PalettedContainer biomeContainer = (sectionArray[0].getBiomeContainer()) instanceof PalettedContainer ? (PalettedContainer) (sectionArray[0].getBiomeContainer()) : null;
        if (biomeContainer instanceof PalettedContainer) {
            PalettedContainer palettedContainer = biomeContainer;
            Palette palette2722 = MixinPalettedContainerHelper.getPalette2722(palettedContainer);
            boolean hasAny = palette2722.hasAny(registryEntry -> {
                return ((RegistryEntry) registryEntry).matchesKey(BiomeKeys.PLAINS);
            });
            if (hasAny && z) {
                if (palette2722.getSize() == 1) {
                    return Mode.PLAINS_PRESENT;
                }
                PaletteStorage paletteStorage2721 = MixinPalettedContainerHelper.getPaletteStorage2721(palettedContainer);
                intSet.clear();
                IntSet intSet2 = intSet;
                Objects.requireNonNull(intSet2);
                paletteStorage2721.forEach(intSet2::add);
                Iterator it = intSet.iterator();
                while (it.hasNext()) {
                    if (((RegistryEntry) palette2722.get(((Integer) it.next()).intValue())).matchesKey(BiomeKeys.PLAINS)) {
                        return Mode.PLAINS_PRESENT;
                    }
                }
            }
            if (hasAny) {
                return Mode.PLAINS_IN_PALETTE;
            }
        }
        return Mode.NO_PLAINS;
    }
}
