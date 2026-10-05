package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.List;
import me.mioclient.feature.IllegalConstructorCall;
import net.minecraft.enchantment.Enchantment;
import net.minecraft.enchantment.Enchantments;
import net.minecraft.entity.EquipmentSlot;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.item.SwordItem;
import net.minecraft.registry.RegistryKey;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NameTagsHelper_2.class */
public final class NameTagsHelper_2 {
    public static final List<Record> list = List.of(new Record(Enchantments.PROTECTION, 4), new Record(Enchantments.UNBREAKING, 3), new Record(Enchantments.MENDING, 1));
    public static final List<Record> list2 = List.of(new Record(Enchantments.PROTECTION, 4), new Record(Enchantments.UNBREAKING, 3), new Record(Enchantments.MENDING, 1));
    public static final List<Record> list3 = List.of(new Record(Enchantments.BLAST_PROTECTION, 4), new Record(Enchantments.UNBREAKING, 3), new Record(Enchantments.MENDING, 1));
    public static final List<Record> list4 = List.of(new Record(Enchantments.PROTECTION, 4), new Record(Enchantments.UNBREAKING, 3), new Record(Enchantments.MENDING, 1), new Record(Enchantments.FEATHER_FALLING, 4));
    public static final List<Record> list5 = List.of(new Record(Enchantments.UNBREAKING, 3), new Record(Enchantments.MENDING, 1));
    public static final List<Record> list6 = List.of(new Record(Enchantments.SHARPNESS, 5), new Record(Enchantments.FIRE_ASPECT, 2), new Record(Enchantments.KNOCKBACK, 2), new Record(Enchantments.SWEEPING_EDGE, 3), new Record(Enchantments.UNBREAKING, 3), new Record(Enchantments.MENDING, 1));

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/NameTagsHelper_2$Inner.class */
    static /* synthetic */ class Inner {
        public static final /* synthetic */ int[] intArr = new int[EquipmentSlot.values().length];

        static {
            try {
                intArr[EquipmentSlot.MAINHAND.ordinal()] = 1;
            } catch (NoSuchFieldError e) {
            }
            try {
                intArr[EquipmentSlot.OFFHAND.ordinal()] = 2;
            } catch (NoSuchFieldError e2) {
            }
            try {
                intArr[EquipmentSlot.FEET.ordinal()] = 3;
            } catch (NoSuchFieldError e3) {
            }
            try {
                intArr[EquipmentSlot.LEGS.ordinal()] = 4;
            } catch (NoSuchFieldError e4) {
            }
            try {
                intArr[EquipmentSlot.CHEST.ordinal()] = 5;
            } catch (NoSuchFieldError e5) {
            }
            try {
                intArr[EquipmentSlot.HEAD.ordinal()] = 6;
            } catch (NoSuchFieldError e6) {
            }
            try {
                intArr[EquipmentSlot.BODY.ordinal()] = 7;
            } catch (NoSuchFieldError e7) {
            }
        }
    }

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/NameTagsHelper_2$Record.class */
    public static final class Record {
        public final RegistryKey<Enchantment> registryKey;
        public final int num;

        public Record(RegistryKey<Enchantment> registryKey, int i) {
            this.registryKey = registryKey;
            this.num = i;
        }




        public RegistryKey<Enchantment> getRegistryKey3086() {
            return this.registryKey;
        }

        public int get3087() {
            return this.num;
        }
    }

    public static boolean is2028(ItemStack itemStack) {
        ArmorItem item = (itemStack.getItem()) instanceof ArmorItem ? (ArmorItem) (itemStack.getItem()) : null;
        if (!(item instanceof ArmorItem)) {
            if (itemStack.getItem() instanceof SwordItem) {
                return is2029(itemStack, list6);
            }
            if (itemStack.isOf(Items.ELYTRA)) {
                return is2029(itemStack, list5);
            }
            return false;
        }
        switch (Inner.intArr[item.getSlotType().ordinal()]) {
            case 1:
            case 2:
                return false;
            case 3:
                return is2029(itemStack, list4);
            case 4:
                return is2029(itemStack, list3);
            case 5:
                return is2029(itemStack, list2);
            case 6:
                return is2029(itemStack, list);
            case 7:
                return false;
            default:
                throw new MatchException((String) null, (Throwable) null);
        }
    }

    public static boolean is2029(ItemStack itemStack, Iterable<Record> iterable) {
        for (Record record : iterable) {
            if (IllegalConstructorCall.get1413(record.getRegistryKey3086(), itemStack) != record.get3087()) {
                return false;
            }
        }
        return true;
    }
}
