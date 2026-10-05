package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonPrimitive;
import java.lang.Number;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NumberSetting.class */
public final class NumberSetting<T extends Number> extends Setting<T> {
    public String string;

    public NumberSetting(String str, T t, T t2, T t3, java.util.function.Predicate<T> predicate) {
        super(str, t, t2, t3, predicate);
        this.string = "";
    }

    public NumberSetting(String str, T t, T t2, T t3) {
        super(str, t, t2, t3);
        this.string = "";
    }

    @Override // me.mioclient.api.Setting
    /* renamed from: do3022, reason: merged with bridge method [inline-methods] */
    public void do2333(T t) {
        super.do2333((T) getNumber3025(t));
    }

    public NumberSetting<T> getNumberSetting3023(String str) {
        this.string = str;
        return this;
    }

    public String getString3024() {
        return this.string;
    }

    @Override // me.mioclient.api.Setting
    public T getValue() {
        // 保底: 读取时也按声明类型(min)协调, 兜底任何残留的类型污染。
        T v = super.getValue();
        return (v instanceof Number && this.object3 instanceof Number && v.getClass() != this.object3.getClass())
                ? (T) getNumber3025(v) : v;
    }

    @Override // me.mioclient.api.Setting
    public void do2323(T t) {
        // 所有值赋值统一按声明类型(min)协调——包括 Setting.do2333 里直接 do2323(min/max) 的钳制路径，
        // 否则整型设置会被存成 Double，导致 getValue().intValue() 处 (Integer) 转型崩溃。
        super.do2323(t == null ? null : (T) getNumber3025(t));
    }

    public Number getNumber3025(T t) {
        // 按数值类型协调 t。以 min(object3, loader/构造器按声明类型设置、稳定) 为类型基准，
        // 而非易被损坏配置污染的 getValue()——否则整型设置被存成 Double 时会导致
        // getValue().intValue() 处 (Integer) 转型崩溃（打开 clickgui 崩）。min 为 null 时回退 getValue()。
        // ★ 必须用 if-else 而非三元! 三元操作数为 Double/Integer/Float/Long 混合包装类型时,
        //   Java 二元数值提升会把结果统一拆箱提升到 double 再装箱成 Double —— 无论走哪个分支都返回
        //   Double, 导致整型设置永远拿到 Double → getValue().intValue() 处 (Integer) 转型崩溃。
        Object ref = this.object3 != null ? this.object3 : getValue();
        if (ref instanceof Double) return Double.valueOf(t.doubleValue());
        if (ref instanceof Integer) return Integer.valueOf(t.intValue());
        if (ref instanceof Float) return Float.valueOf(t.floatValue());
        return Long.valueOf(t.longValue());
    }

    @Override // me.mioclient.api.Setting
    public void do134(String str) {
        if (getValue() instanceof Double) {
            do2333((T) Double.valueOf(Double.parseDouble(str)));
        } else if (getValue() instanceof Integer) {
            do2333((T) Integer.valueOf(Integer.parseInt(str)));
        } else if (getValue() instanceof Float) {
            do2333((T) Float.valueOf(Float.parseFloat(str)));
        }
    }

    /* JADX WARN: Multi-variable type inference failed */
    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        return new JsonPrimitive((Number) getValue());
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (getValue() instanceof Double) {
            do2333((T) Double.valueOf(jsonElement.getAsDouble()));
        } else if (getValue() instanceof Integer) {
            do2333((T) Integer.valueOf(jsonElement.getAsInt()));
        } else if (getValue() instanceof Float) {
            do2333((T) Float.valueOf(jsonElement.getAsFloat()));
        }
    }
}
