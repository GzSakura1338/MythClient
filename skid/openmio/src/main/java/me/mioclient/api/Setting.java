package me.mioclient.api;

import java.util.Objects;
import java.util.function.Predicate;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingConverter;
import me.mioclient.Helper_10;
import me.mioclient.HoleSnapMode;
import me.mioclient.PresetHelper_7;
import me.mioclient.module.Feature;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/api/Setting.class */
public abstract class Setting<T> extends Feature implements Helper_10<T>, PresetHelper_7 {
    public T object;
    public boolean flag;
    public boolean flag2;
    public boolean flag3;
    public boolean flag4;
    public Predicate<T> predicate;
    public String configName;
    public String string;
    public Runnable runnable;
    public T value;
    public T object2;
    public T object3;
    public T object4;
    public T object5;
    public boolean flag5;
    public boolean flag6;
    public HoleSnapMode holeSnapMode;

    public Setting(String str, T t, T t2, T t3, Predicate<T> predicate) {
        super(str);
        this.configName = getName();
        this.string = "";
        this.object = t;
        this.object3 = t2;
        this.object4 = t3;
        this.predicate = predicate;
        this.value = t;
        this.object2 = t;
        this.object5 = t;
        this.flag5 = true;
    }

    public Setting(String str, T t, Predicate<T> predicate) {
        super(str);
        this.configName = getName();
        this.string = "";
        this.object = t;
        this.predicate = predicate;
        this.value = t;
        this.object2 = t;
        this.object5 = t;
    }

    public Setting(String str, T t, T t2, T t3) {
        super(str);
        this.configName = getName();
        this.string = "";
        this.object = t;
        this.object3 = t2;
        this.object4 = t3;
        this.value = t;
        this.object2 = t;
        this.object5 = t;
        this.flag5 = true;
    }

    public Setting(String str, T t) {
        super(str);
        this.configName = getName();
        this.string = "";
        this.object = t;
        this.value = t;
        this.object2 = t;
        this.object5 = t;
    }

    public T getValue() {
        return this.value;
    }

    public T getObject2322() {
        return this.object2;
    }

    public void do2323(T t) {
        this.object2 = t;
    }

    public T getObject2324() {
        return this.object;
    }

    public T getObject2325() {
        return this.object3;
    }

    public T getObject2326() {
        return this.object4;
    }

    public boolean is2327() {
        return getObject2325().equals(getValue());
    }

    public boolean is2328() {
        return getObject2326().equals(getValue());
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return this.configName;
    }

    public void do2329(String str) {
        this.configName = str;
    }

    public String getString2330() {
        return EnumSettingConverter.getString913((Enum) this.object2);
    }

    public String getString2331() {
        return this.string;
    }

    public <T> String getString2332(T t) {
        return t.getClass().getSimpleName();
    }

    public void reset() {
        do2333(this.object);
    }

    public abstract void do134(String str);

    /* JADX WARN: Multi-variable type inference failed */
    public void do2333(T t) {
        do2323(t);
        if (this.flag5) {
            if (((Number) this.object3).floatValue() > ((Number) t).floatValue()) {
                do2323(this.object3);
            }
            if (((Number) this.object4).floatValue() < ((Number) t).floatValue()) {
                do2323(this.object4);
            }
        }
        this.value = this.object2;
        if (this.value != this.object5) {
            do2340();
        }
        this.object5 = this.value;
    }

    public void do2334(T t) {
        this.object = t;
    }

    public T getObject2335() {
        return this.object5;
    }

    public Setting<T> getSetting2336() {
        this.flag2 = true;
        return this;
    }

    public Setting<T> getSetting2337() {
        this.flag4 = true;
        getSetting2336();
        return this;
    }

    public Setting<T> getSetting2338(String str, HoleSnapMode holeSnapMode) {
        this.string = str;
        this.holeSnapMode = holeSnapMode;
        return this;
    }

    public void do2339(Runnable runnable) {
        this.runnable = runnable;
    }

    public void do2340() {
        if (this.runnable == null) {
            return;
        }
        try {
            this.runnable.run();
        } catch (Throwable th) {
        }
    }

    public Setting<T> getSetting2341() {
        this.flag3 = true;
        return this;
    }

    @SafeVarargs
    public final Setting<T> getSetting2342(Setting<Boolean>... settingArr) {
        do2343(obj -> {
            for (Setting setting : settingArr) {
                if (setting.flag4 || setting.flag2) {
                    if (!setting.is623()) {
                        return false;
                    }
                } else if (!((Boolean) setting.getValue()).booleanValue()) {
                    return false;
                }
            }
            return true;
        });
        return this;
    }

    public void do2343(Predicate<T> predicate) {
        if (this.predicate == null) {
            this.predicate = predicate;
        } else {
            this.predicate = this.predicate.and(predicate);
        }
    }

    public void do2344() {
        do2343(obj -> {
            return BaritoneHelper_3.obstaclePasserHelper.is709();
        });
    }

    public boolean is2345() {
        return this.value instanceof Number;
    }

    public boolean is2346() {
        return this.value instanceof Enum;
    }

    public boolean is2347() {
        return this.value instanceof String;
    }

    public boolean is2348() {
        return is2345() && !this.string.isEmpty() && this.holeSnapMode.is220(this);
    }

    public boolean is2349() {
        if (this.predicate == null) {
            return true;
        }
        return this.predicate.test(getValue());
    }

    public boolean is623() {
        return this.flag && this.flag2;
    }

    public boolean is2350() {
        return this.flag5;
    }

    public void do2351() {
        this.flag5 = false;
    }

    public boolean is2352() {
        return this.flag4;
    }

    public void do2353(boolean z) {
        this.flag6 = z;
    }

    public boolean is2354() {
        return this.flag6;
    }

    @Override // me.mioclient.module.Feature
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj != null && getClass() == obj.getClass() && super.equals(obj)) {
            return Objects.equals(this.configName, ((Setting) obj).configName);
        }
        return false;
    }

    @Override // me.mioclient.module.Feature
    public int hashCode() {
        return Objects.hash(Integer.valueOf(super.hashCode()), this.configName);
    }
}
