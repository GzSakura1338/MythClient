/*
 * Decompiled with CFR 0.2.2 (FabricMC 7c48b8c4).
 */
package nick;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.List;

public final class Commands {
    private static final List<String> COMMANDS = List.of("me/mioclient/Feature_5", "me/mioclient/Feature_13", "me/mioclient/Feature_22", "me/mioclient/Feature_14", "me/mioclient/Feature_30", "me/mioclient/Feature_2", "me/mioclient/Feature_25", "me/mioclient/Feature_17", "me/mioclient/Feature_23", "me/mioclient/Feature_16", "me/mioclient/Feature_6", "me/mioclient/Feature_11", "me/mioclient/Feature_29", "me/mioclient/Feature_37", "me/mioclient/PresetHelperFeature", "me/mioclient/StopwatchFeature", "me/mioclient/Feature_3", "me/mioclient/Feature_33", "me/mioclient/Feature_20", "me/mioclient/Feature_18", "me/mioclient/Feature_27", "me/mioclient/Feature_4", "me/mioclient/CompletableFutureFeature", "me/mioclient/Feature_28", "me/mioclient/Feature_39", "me/mioclient/CompletableFutureFeature_2", "me/mioclient/Feature_24", "me/mioclient/Feature_8", "me/mioclient/Feature_34", "me/mioclient/Feature_31", "me/mioclient/Feature_19", "me/mioclient/Feature_7", "me/mioclient/Feature_38", "me/mioclient/Feature_21", "me/mioclient/ExecutorServiceFeature_4", "me/mioclient/ExecutorServiceFeature_2", "me/mioclient/ExecutorServiceFeature_3", "me/mioclient/Feature_9", "me/mioclient/Feature_12");

    public static void initialize(Object manager) {
        try {
            Class<?> k = manager.getClass();
            Method m = k.getDeclaredMethod("register", Class.forName("me.mioclient.Feature"));
            for (String cmd : COMMANDS) {
                Class<?> klass = Class.forName(cmd.replace('/', '.'));
                Constructor<?>[] constructors = klass.getDeclaredConstructors();
                if (constructors.length != 1) {
                    throw new RuntimeException(cmd);
                }
                m.invoke(manager, constructors[0].newInstance(new Object[0]));
            }
            Class<?> en = Class.forName("me.mioclient.NameTagsHelperMode");
            Object e1 = en.getDeclaredField("FRIEND").get(null);
            Object e2 = en.getDeclaredField("ENEMY").get(null);
            Class<?> klass = Class.forName("me.mioclient.Feature_10");
            Constructor<?>[] constructors = klass.getDeclaredConstructors();
            if (constructors.length != 1) {
                throw new RuntimeException();
            }
            m.invoke(manager, constructors[0].newInstance(e1));
            m.invoke(manager, constructors[0].newInstance(e2));
        } catch (Throwable _t) {
            _t.printStackTrace(System.err);
        }
    }
}

