package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChestStealerEnumSettingHelper.class */
public final class ChestStealerEnumSettingHelper implements EnumSettingHelper {
    public final Map<Integer, Record> map = new HashMap();
    public final String name;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/ChestStealerEnumSettingHelper$Record.class */
    public static final class Record {
        public final int num;
        public final String string;

        public Record(int i, String str) {
            this.num = i;
            this.string = str;
        }

        public boolean is774(ItemStack itemStack) {
            return Item.getRawId(itemStack.getItem()) == this.num && Objects.equals(ChestStealerSearchHelper4_3.getString259(itemStack), this.string);
        }




        public int get2112() {
            return this.num;
        }

        public String getString2113() {
            return this.string;
        }
    }

    public ChestStealerEnumSettingHelper(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public Map<Integer, Record> getMap2747() {
        return this.map;
    }
}
