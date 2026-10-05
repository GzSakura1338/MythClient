/*
 * Decompiled with CFR 0.2.2 (FabricMC 7c48b8c4).
 */
package nick;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;

public final class SettingModes {
    private static final Object MIN;
    private static final Object MAX;
    private static final Field _ENUM;
    private static final Map<String, Long> DATA;

    private static Enum<?> get(long id) {
        if (id == 30310109472L) {
            return (Enum)MIN;
        }
        if (id == 30310109400L) {
            return (Enum)MAX;
        }
        throw new RuntimeException("unknown id: " + Long.toHexString(id));
    }

    public static void apply(String setting, Object inst) {
        try {
            Long id = DATA.get(setting);
            if (id == null) {
                return;
            }
            Enum<?> e = SettingModes.get(id);
            _ENUM.set(inst, e);
        } catch (Throwable _t) {
            _t.printStackTrace(System.err);
            throw new RuntimeException();
        }
    }

    static {
        try {
            Class<?> k = Class.forName("me.mioclient.HoleSnapMode");
            Class<?> setting = Class.forName("me.mioclient.api.Setting");
            _ENUM = setting.getDeclaredField("holeSnapMode");
            _ENUM.setAccessible(true);
            MIN = k.getDeclaredField("MIN").get(null);
            MAX = k.getDeclaredField("MAX").get(null);
        } catch (Throwable _t) {
            _t.printStackTrace(System.err);
            throw new RuntimeException();
        }
        DATA = new HashMap<String, Long>();
        DATA.put("me/mioclient/module/combat/Offhand.health", 30310109472L);
        DATA.put("me/mioclient/module/render/Chams.speed", 30310109472L);
        DATA.put("me/mioclient/module/movement/Fireworks.delay", 30310109472L);
        DATA.put("me/mioclient/module/combat/Aura.limit", 30310109472L);
        DATA.put("me/mioclient/module/movement/HoleSnap.pitch", 30310109472L);
        DATA.put("me/mioclient/module/render/Waypoints.distance", 30310109400L);
        DATA.put("me/mioclient/module/misc/BetterChat.alpha", 30310109472L);
        DATA.put("me/mioclient/module/exploit/NewChunks.distance", 30310109472L);
        DATA.put("me/mioclient/module/combat/AutoClicker.delay", 30310109472L);
        DATA.put("me/mioclient/module/render/Borders.level", 30310109472L);
        DATA.put("me/mioclient/module/misc/UnfocusedFPS.fps", 30310109400L);
        DATA.put("me/mioclient/module/render/LogoutSpots.distance", 30310109400L);
        DATA.put("me/mioclient/module/combat/AutoCrystal.minDamage2", 30310109400L);
        DATA.put("me/mioclient/module/combat/AutoCrystal.ticks", 30310109472L);
        DATA.put("me/mioclient/module/player/SpeedMine.limit", 30310109400L);
        DATA.put("me/mioclient/module/movement/ElytraFly.minBoost", 30310109400L);
        DATA.put("me/mioclient/module/movement/ElytraFly.limit", 30310109400L);
        DATA.put("me/mioclient/module/player/AutoEat.health", 30310109472L);
        DATA.put("me/mioclient/module/player/AutoEat.hunger", 30310109472L);
        DATA.put("me/mioclient/module/combat/WebAura.ticks", 30310109472L);
        DATA.put("me/mioclient/module/combat/HoleFill.ticks", 30310109472L);
    }
}

