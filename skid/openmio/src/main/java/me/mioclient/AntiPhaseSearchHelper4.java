package me.mioclient;

import it.unimi.dsi.fastutil.objects.ObjectLinkedOpenHashSet;
import java.awt.Color;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.Set;
import me.mioclient.MatrixStackEvent;
import me.mioclient.event.Listen;
import me.mioclient.event.TickEvent;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AntiPhaseSearchHelper4.class */
public final class AntiPhaseSearchHelper4 extends ModuleListHelper<AntiPhaseSearchHelper4.Record, List<AntiPhaseSearchHelper4.Record>> implements SearchHelper_4 {
    public final Map<Record, Set<SpeedMineHelper_3>> map;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/AntiPhaseSearchHelper4$Record.class */
    public static final class Record {
        public final me.mioclient.module.Feature feature;
        public final Helper_10<Color> helper_10;
        public final Helper_10<Color> helper_102;
        public final Helper_10<Float> helper_103;
        public final Helper_10<Float> helper_104;
        public final Helper_10<Boolean> helper_105;
        public final Helper_10<Boolean> helper_106;
        public final int num;

        public Record(me.mioclient.module.Feature feature, Helper_10<Color> helper_10, Helper_10<Color> helper_102, Helper_10<Float> helper_103, Helper_10<Float> helper_104, Helper_10<Boolean> helper_105, Helper_10<Boolean> helper_106, int i) {
            this.feature = feature;
            this.helper_10 = helper_10;
            this.helper_102 = helper_102;
            this.helper_103 = helper_103;
            this.helper_104 = helper_104;
            this.helper_105 = helper_105;
            this.helper_106 = helper_106;
            this.num = i;
        }

        public Color getColor97() {
            return this.helper_10.getValue();
        }

        public Color getColor98() {
            return this.helper_102.getValue();
        }

        public float get99() {
            return this.helper_103.getValue().floatValue();
        }

        public float get100() {
            if (this.helper_106.getValue().booleanValue()) {
                return this.helper_104.getValue().floatValue() * this.num;
            }
            return 0.0f;
        }

        public boolean is101() {
            return this.helper_105.getValue().booleanValue();
        }

        public boolean is102() {
            return this.helper_106.getValue().booleanValue();
        }

        @Override // java.lang.Record
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj == null || getClass() != obj.getClass()) {
                return false;
            }
            return Objects.equals(this.feature, ((Record) obj).feature);
        }

        @Override // java.lang.Record
        public int hashCode() {
            return Objects.hash(this.feature);
        }


        public me.mioclient.module.Feature getFeature103() {
            return this.feature;
        }

        public Helper_10<Color> getHelper_10104() {
            return this.helper_10;
        }

        public Helper_10<Color> getHelper_10105() {
            return this.helper_102;
        }

        public Helper_10<Float> getHelper_10106() {
            return this.helper_103;
        }

        public Helper_10<Float> getHelper_10107() {
            return this.helper_104;
        }

        public Helper_10<Boolean> getHelper_10108() {
            return this.helper_105;
        }

        public Helper_10<Boolean> getHelper_10109() {
            return this.helper_106;
        }

        public int get110() {
            return this.num;
        }
    }

    public AntiPhaseSearchHelper4() {
        super(new ArrayList());
        this.map = new HashMap();
        SearchHelper_4.baritoneHelper.do1796(this);
    }

    public void do2132(me.mioclient.module.Feature feature, BlockPos blockPos) {
        if (blockPos == null) {
            return;
        }
        do2133(feature, new Box(blockPos));
    }

    public void do2133(me.mioclient.module.Feature feature, Box box) {
        if (box == null) {
            return;
        }
        Record object2405 = getObject2405(record -> {
            return record.getFeature103().equals(feature);
        });
        if (object2405 == null) {
            throw new NoSuchElementException();
        }
        synchronized (this.map) {
            Set<SpeedMineHelper_3> set = this.map.get(object2405);
            if (object2405.is101()) {
                for (SpeedMineHelper_3 speedMineHelper_3 : set) {
                    if (box.equals(speedMineHelper_3.getBox2263())) {
                        speedMineHelper_3.do2258();
                        return;
                    }
                }
            } else {
                set.clear();
            }
            SpeedMineHelper_3 speedMineHelper_32 = new SpeedMineHelper_3();
            speedMineHelper_32.do2259(box);
            speedMineHelper_32.do2258();
            set.add(speedMineHelper_32);
        }
    }

    @Listen
    public void do27(TickEvent tickEvent) {
        synchronized (this.map) {
            for (Map.Entry<Record, Set<SpeedMineHelper_3>> entry : this.map.entrySet()) {
                entry.getValue().removeIf(speedMineHelper_3 -> {
                    return speedMineHelper_3.get2262(((Record) entry.getKey()).get100()) == 0.0f;
                });
            }
        }
    }

    @Listen
    public void onEvent(MatrixStackEvent.Inner_3 inner_3) {
        synchronized (this.map) {
            for (Map.Entry<Record, Set<SpeedMineHelper_3>> entry : this.map.entrySet()) {
                Record key = entry.getKey();
                for (SpeedMineHelper_3 speedMineHelper_3 : entry.getValue()) {
                    speedMineHelper_3.do2260(key.get99());
                    speedMineHelper_3.do2261(inner_3.getMatrixStack472(), key.getColor97(), key.getColor98(), key.get100(), key.is102());
                }
            }
        }
    }

    @Override // me.mioclient.Helper_9
    /* renamed from: is2134, reason: merged with bridge method [inline-methods] */
    public boolean register(Record record) {
        this.map.put(record, new ObjectLinkedOpenHashSet());
        return getRegistry().add(record);
    }

    @Override // me.mioclient.Helper_9
    /* renamed from: is2135, reason: merged with bridge method [inline-methods] */
    public boolean unregister(Record record) {
        throw new UnsupportedOperationException();
    }
}
