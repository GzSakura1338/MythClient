package me.mioclient;

import com.mojang.brigadier.context.CommandContext;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Enum;
import net.minecraft.command.CommandSource;
import net.minecraft.item.AnimalArmorItem;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.Item;
import net.minecraft.item.Items;
import net.minecraft.registry.Registries;
import me.mioclient.mixin.ducks.DuckMinecraftClient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_9.class */
public class Feature_9 extends Feature {
    public final Map<Mode, List<Record>> map;
    public boolean flag;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/Feature_9$Mode.class */
    private enum Mode implements EnumSettingHelper {
        SWAP("swap", item -> {
            return true;
        }, new SearchHelper418_2()),
        ALTERNATIVE("alternative", item2 -> {
            return true;
        }, new SearchHelper418()),
        ARMOR("armor", Feature_9::is1295, new SearchHelper418_3()),
        OFFHAND("offhand", item3 -> {
            return true;
        }, new SearchHelper418_4());

        public final String name;
        public final SearchHelper4_18 searchHelper4_18;
        public final java.util.function.Predicate<Item> predicate;

        Mode(String str, java.util.function.Predicate<Item> predicate, SearchHelper4_18 searchHelper4_18) {
            this.name = str;
            this.predicate = predicate;
            this.searchHelper4_18 = searchHelper4_18;
        }

        public boolean is425(Item item) {
            return this.predicate.test(item);
        }

        public SearchHelper4_18 getSearchHelper4_18426() {
            return this.searchHelper4_18;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/Feature_9$Record.class */
    private static final class Record {
        public final SearchHelper4_18 searchHelper4_18;
        public final Data_4 data_4;

        public Record(SearchHelper4_18 searchHelper4_18, Data_4 data_4) {
            this.searchHelper4_18 = searchHelper4_18;
            this.data_4 = data_4;
        }




        public SearchHelper4_18 getSearchHelper4_18764() {
            return this.searchHelper4_18;
        }

        public Data_4 getData_4765() {
            return this.data_4;
        }
    }

    public Feature_9() {
        super("action");
        this.map = new HashMap();
        baritoneHelper.do1796(this);
        for (Mode mode : Mode.values()) {
            this.map.put(mode, new ArrayList());
        }
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("swap", new Enum(Mode.class)).then(Feature.argument("item", BiPredicateArgumentType.registry(Registries.ITEM, this::is1294)).executes(commandContext -> {
            Item item = (Item) commandContext.getArgument("item", Item.class);
            Mode mode = (Mode) commandContext.getArgument("swap", Mode.class);
            Data_4 data_4253 = mode.getSearchHelper4_18426().getData_4253(item);
            if (data_4253.is1775()) {
                this.flag = true;
            }
            this.map.get(mode).add(new Record(mode.getSearchHelper4_18426(), data_4253));
            if (data_4253.is1775()) {
                return 1;
            }
            mode.getSearchHelper4_18426().do251(data_4253);
            return 1;
        })).then(Feature.literal("back").executes(commandContext2 -> {
            List<Record> list = this.map.get((Mode) commandContext2.getArgument("swap", Mode.class));
            if (list.isEmpty()) {
                return 1;
            }
            Record record = (Record) list.removeLast();
            if (record.getData_4765().is1775()) {
                return 1;
            }
            record.getSearchHelper4_18764().do252(record.data_4);
            return 1;
        })));
        literalArgumentBuilder.then(Feature.literal("attack").executes(commandContext3 -> {
            if (this.flag) {
                return 1;
            }
            ((DuckMinecraftClient) minecraftClient).attack();
            return 1;
        }));
        literalArgumentBuilder.then(Feature.literal("interact").executes(commandContext4 -> {
            if (this.flag) {
                return 1;
            }
            ((DuckMinecraftClient) minecraftClient).interact();
            return 1;
        }));
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        this.flag = false;
    }

    public boolean is1294(CommandContext<?> commandContext, Item item) {
        return ((Mode) commandContext.getArgument("swap", Mode.class)).is425(item);
    }

    public static boolean is1295(Item item) {
        if (item instanceof AnimalArmorItem) {
            return false;
        }
        return (item instanceof ArmorItem) || item == Items.ELYTRA;
    }
}
