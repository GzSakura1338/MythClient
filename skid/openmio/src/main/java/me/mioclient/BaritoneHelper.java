package me.mioclient;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.Function;
import me.mioclient.BaritoneHelper_2;
import me.mioclient.event.Listen;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BaritoneHelper.class */
public class BaritoneHelper implements Helper_19 {
    public final Map<Object, List<BaritoneHelper_5>> map = new ConcurrentHashMap();
    public final Map<Class<?>, List<BaritoneHelper_5>> map2 = new ConcurrentHashMap();
    public final Map<Class<?>, List<BaritoneHelper_5>> map3 = new ConcurrentHashMap();
    public final List<Inner> list = new ArrayList();

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/BaritoneHelper$Inner.class */
    public static class Inner {
        public final String string;
        public final BaritoneHelper_2.Inner inner;

        public Inner(String str, BaritoneHelper_2.Inner inner) {
            this.string = str;
            this.inner = inner;
        }
    }

    @Override // me.mioclient.Helper_19
    public void do1793(String str, BaritoneHelper_2.Inner inner) {
        synchronized (this.list) {
            this.list.add(new Inner(str, inner));
        }
    }

    @Override // me.mioclient.Helper_19
    public <T> T getObject1794(T t) {
        List<BaritoneHelper_5> list = this.map3.get(t.getClass());
        if (list != null) {
            Iterator<BaritoneHelper_5> it = list.iterator();
            while (it.hasNext()) {
                it.next().do2109(t);
            }
        }
        return t;
    }

    @Override // me.mioclient.Helper_19
    public <T extends BaritoneHelper_6> T getBaritoneHelper_61795(T t) {
        List<BaritoneHelper_5> list = this.map3.get(t.getClass());
        if (list != null) {
            t.do3062(false);
            Iterator<BaritoneHelper_5> it = list.iterator();
            while (it.hasNext()) {
                it.next().do2109(t);
                if (t.is2403()) {
                    break;
                }
            }
        }
        return t;
    }

    @Override // me.mioclient.Helper_19
    public void do1796(Object obj) {
        do1799(getList1807(obj.getClass(), obj), false);
    }

    @Override // me.mioclient.Helper_19
    public void do1797(Class<?> cls) {
        do1799(getList1807(cls, null), true);
    }

    @Override // me.mioclient.Helper_19
    public void do1798(BaritoneHelper_5 baritoneHelper_5) {
        do1800(baritoneHelper_5, false);
    }

    public void do1799(List<BaritoneHelper_5> list, boolean z) {
        Iterator<BaritoneHelper_5> it = list.iterator();
        while (it.hasNext()) {
            do1800(it.next(), z);
        }
    }

    public void do1800(BaritoneHelper_5 baritoneHelper_5, boolean z) {
        if (!z) {
            do1801(this.map3.computeIfAbsent(baritoneHelper_5.getClass2110(), cls -> {
                return new CopyOnWriteArrayList();
            }), baritoneHelper_5);
        } else if (baritoneHelper_5.is2111()) {
            do1801(this.map3.computeIfAbsent(baritoneHelper_5.getClass2110(), cls2 -> {
                return new CopyOnWriteArrayList();
            }), baritoneHelper_5);
        }
    }

    public void do1801(List<BaritoneHelper_5> list, BaritoneHelper_5 baritoneHelper_5) {
        int i = 0;
        while (i < list.size() && baritoneHelper_5.get888() <= list.get(i).get888()) {
            i++;
        }
        list.add(i, baritoneHelper_5);
    }

    @Override // me.mioclient.Helper_19
    public void do1802(Object obj) {
        do1805(getList1807(obj.getClass(), obj), false);
    }

    @Override // me.mioclient.Helper_19
    public void do1803(Class<?> cls) {
        do1805(getList1807(cls, null), true);
    }

    @Override // me.mioclient.Helper_19
    public void do1804(BaritoneHelper_5 baritoneHelper_5) {
        do1806(baritoneHelper_5, false);
    }

    public void do1805(List<BaritoneHelper_5> list, boolean z) {
        Iterator<BaritoneHelper_5> it = list.iterator();
        while (it.hasNext()) {
            do1806(it.next(), z);
        }
    }

    public void do1806(BaritoneHelper_5 baritoneHelper_5, boolean z) {
        List<BaritoneHelper_5> list = this.map3.get(baritoneHelper_5.getClass2110());
        if (list != null) {
            if (!z) {
                list.remove(baritoneHelper_5);
            } else if (baritoneHelper_5.is2111()) {
                list.remove(baritoneHelper_5);
            }
        }
    }

    public List<BaritoneHelper_5> getList1807(Class<?> cls, Object obj) {
        Function<Object, List<BaritoneHelper_5>> function = obj2 -> {
            CopyOnWriteArrayList copyOnWriteArrayList = new CopyOnWriteArrayList();
            do1808(copyOnWriteArrayList, cls, obj);
            return copyOnWriteArrayList;
        };
        if (obj == null) {
            return this.map2.computeIfAbsent(cls, function);
        }
        Iterator<Object> it = this.map.keySet().iterator();
        while (it.hasNext()) {
            if (it.next() == obj) {
                return this.map.get(obj);
            }
        }
        List<BaritoneHelper_5> list = (List) function.apply(obj);
        this.map.put(obj, list);
        return list;
    }

    public void do1808(List<BaritoneHelper_5> list, Class<?> cls, Object obj) {
        for (Method method : cls.getDeclaredMethods()) {
            if (is1809(method)) {
                list.add(new BaritoneHelper_2(getInner1810(cls), cls, obj, method));
            }
        }
        if (cls.getSuperclass() != null) {
            do1808(list, cls.getSuperclass(), obj);
        }
    }

    public boolean is1809(Method method) {
        return method.isAnnotationPresent(Listen.class) && method.getReturnType() == Void.TYPE && method.getParameterCount() == 1 && !method.getParameters()[0].getType().isPrimitive();
    }

    public BaritoneHelper_2.Inner getInner1810(Class<?> cls) {
        synchronized (this.list) {
            for (Inner inner : this.list) {
                if (cls.getName().startsWith(inner.string)) {
                    return inner.inner;
                }
            }
            throw new RuntimeException(cls);
        }
    }
}
