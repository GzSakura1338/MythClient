package me.mioclient.module;

import java.awt.Color;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BooleanSetting;
import me.mioclient.EntityListObjectSetting;
import me.mioclient.EnumSetting;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.ScaffoldMode_2;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import me.mioclient.feature.Size;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.ItemEntity;
import net.minecraft.item.Item;
import net.minecraft.item.Items;
import net.minecraft.registry.Registries;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/EntityList.class */
public class EntityList extends me.mioclient.ModuleList {
    public Setting<Set<EntityType<?>>> setting;
    public Setting<Set<Item>> setting2;
    public Setting<ScaffoldMode_2> setting3;
    public Setting<ScaffoldMode_2> setting4;
    public Setting<EntityListMode> setting5;
    public Setting<Boolean> setting6;
    public Setting<Boolean> setting7;
    public final java.util.Map<String, Integer> map;
    public final List<Record> list;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/EntityList$EntityListMode.class */
    public enum EntityListMode implements EnumSettingHelper {
        ALPHABET("Alphabet"),
        COUNT("Count"),
        LENGTH("Length");

        public final String name;

        EntityListMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/EntityList$Record.class */
    public static final class Record {
        public final String string;
        public final int num;

        public Record(String str, int i) {
            this.string = str;
            this.num = i;
        }

        public static Record getRecord331(Map.Entry<String, Integer> entry) {
            return new Record(entry.getKey(), entry.getValue().intValue());
        }

        public String getString332(Formatting formatting) {
            return new ArgumentTypeHelper().getArgumentTypeHelper2906(this.num).getArgumentTypeHelper2919(this.string).getString2921("\u0001 x\u0001");
        }




        public String getString333() {
            return this.string;
        }

        public int get334() {
            return this.num;
        }
    }

    public EntityList() {
        super("EntityList", new String[0]);
        this.setting = add(new EntityListObjectSetting("WhiteList", Registries.ENTITY_TYPE, EntityType.PLAYER));
        this.setting2 = add(new EntityListObjectSetting("Items", Registries.ITEM, Items.OBSIDIAN));
        this.setting3 = add(new EnumSetting("Selection", ScaffoldMode_2.BLACKLIST));
        this.setting4 = add(new EnumSetting("ItemSelection", ScaffoldMode_2.ANY));
        this.setting5 = add(new EnumSetting("Sorting", EntityListMode.ALPHABET));
        this.setting6 = add(new BooleanSetting("CustomNames", true));
        this.setting7 = add(new BooleanSetting("ColoredCount", true));
        this.map = new HashMap();
        this.list = new ArrayList();
        this.setting4.do2343(scaffoldMode_2 -> {
            return this.setting3.getValue().is1391(EntityType.ITEM, this.setting.getValue());
        });
        do3019(new Size(this));
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        synchronized (this.list) {
            this.map.clear();
            for (Entity itemEntity : minecraftClient.world.getEntities()) {
                if (this.setting3.getValue().is1391(((Entity) itemEntity).getType(), this.setting.getValue())) {
                    if (itemEntity instanceof ItemEntity) {
                        ItemEntity itemEntity2 = (ItemEntity) itemEntity;
                        if (!itemEntity2.getStack().isEmpty()) {
                            if (!this.setting4.getValue().is1392(itemEntity2.getStack().getItem(), this.setting2)) {
                            }
                        }
                    }
                    String string = this.setting6.getValue().booleanValue() ? ((Entity) itemEntity).getName().getString() : ((Entity) itemEntity).getType().getName().getString();
                    if (itemEntity instanceof ItemEntity) {
                        string = ((ItemEntity) itemEntity).getStack().getName().getString();
                    }
                    int count = itemEntity instanceof ItemEntity ? ((ItemEntity) itemEntity).getStack().getCount() : 1;
                    this.map.compute(string, (str, num) -> {
                        return num == null ? Integer.valueOf(count) : Integer.valueOf(num.intValue() + count);
                    });
                }
            }
            this.list.clear();
            this.list.addAll(this.map.entrySet().stream().map(Record::getRecord331).toList());
            switch (this.setting5.getValue()) {
                case ALPHABET:
                    this.list.sort(Comparator.comparing(record -> {
                        return record.string;
                    }));
                    break;
                case COUNT:
                    this.list.sort(Comparator.comparing(record2 -> {
                        return Integer.valueOf(-record2.num);
                    }));
                    break;
                case LENGTH:
                    this.list.sort(Comparator.comparing(record3 -> {
                        return Float.valueOf(FontsSearchHelper4.fontsSearchHelper4.get1316(record3.getString332(Formatting.WHITE)));
                    }));
                    break;
            }
        }
    }

    @Override // me.mioclient.ModuleList
    public void do364(DrawContext drawContext) {
        float f = this.moduleListSearchHelper4.get2954(FontsSearchHelper4.fontsSearchHelper4.get93()) - this.moduleListSearchHelper4.get124();
        synchronized (this.list) {
            for (Record record : this.list) {
                Color color3018 = getColor3018(f);
                String string332 = record.getString332(this.setting7.getValue().booleanValue() ? Formatting.WHITE : Formatting.RESET);
                FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, string332, this.moduleListSearchHelper4.get2953(FontsSearchHelper4.fontsSearchHelper4.get1316(string332)) - this.moduleListSearchHelper4.get123(), f, color3018);
                f += (FontsSearchHelper4.fontsSearchHelper4.get93() + 1) * this.moduleListSearchHelper4.get2956();
            }
        }
    }

    @Override // me.mioclient.ModuleList
    public float[] getFloatArray365() {
        float f = 0.0f;
        float f2 = 0.0f;
        synchronized (this.list) {
            Iterator<Record> it = this.list.iterator();
            while (it.hasNext()) {
                float f3 = FontsSearchHelper4.fontsSearchHelper4.get1316(it.next().getString332(Formatting.WHITE));
                f += FontsSearchHelper4.fontsSearchHelper4.get93() + 1;
                if (f3 > f2) {
                    f2 = f3;
                }
            }
        }
        return new float[]{f2, f};
    }
}
