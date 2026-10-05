package me.mioclient;

import java.util.Locale;
import me.mioclient.HoleSnapSearchHelper4_2;
import me.mioclient.feature.Items;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BaritoneHelper_3.class */
public class BaritoneHelper_3 {
    public static BaritoneHelper_4 baritoneHelper_4 = new BaritoneHelper_4();
    public static HoleSnapSearchHelper4_2 holeSnapSearchHelper4_2;
    public static PresetHelper presetHelper;
    public static SearchHelper4_8 searchHelper4_8;
    public static KeyPearlSearchHelper4 keyPearlSearchHelper4;
    public static ChatFilterSearchHelper4_2 chatFilterSearchHelper4_2;
    public static HitmarkerSearchHelper4 hitmarkerSearchHelper4;
    public static HoleSnapSearchHelper4_4 holeSnapSearchHelper4_4;
    public static SearchHelper4_14 searchHelper4_14;
    public static WaypointsSearchHelper4 waypointsSearchHelper4;
    public static HoleSnapSearchHelper4_5 holeSnapSearchHelper4_5;
    public static SearchHelper4_11 searchHelper4_11;
    public static SearchHelper4_12 searchHelper4_12;
    public static SearchHelper4_5 searchHelper4_5;
    public static WelcomerHelper welcomerHelper;
    public static NameTagsSearchHelper4 nameTagsSearchHelper4;
    public static FireworksHelperSearchHelper4 fireworksHelperSearchHelper4;
    public static MainhandHelper_2 mainhandHelper_2;
    public static LogoutSpotsHelper logoutSpotsHelper;
    public static FeetPlaceSearchHelper4 feetPlaceSearchHelper4;
    public static AntiPhaseSearchHelper4_2 antiPhaseSearchHelper4_2;
    public static AntiPhaseSearchHelper4 antiPhaseSearchHelper4;
    public static HoleSnapSearchHelper4_2.Inner inner;
    public static SearchHelper4_15 searchHelper4_15;
    public static TooltipsSearchHelper4_2 tooltipsSearchHelper4_2;
    public static SearchHelper4_20 searchHelper4_20;
    public static DiscordNotifsHelper discordNotifsHelper;
    public static StashFinderSearchHelper4 stashFinderSearchHelper4;
    public static ScaffoldHelper scaffoldHelper;
    public static TooltipsSearchHelper4 tooltipsSearchHelper4;
    public static ChestStealerSearchHelper4_3 chestStealerSearchHelper4_3;
    public static NotificationsHelper notificationsHelper;
    public static ChatFilterSearchHelper4 chatFilterSearchHelper4;
    public static Items items;
    public static BreakHighlightSearchHelper4 breakHighlightSearchHelper4;
    public static ObstaclePasserHelper obstaclePasserHelper;
    public static MixinTitleScreenSearchHelper4 mixinTitleScreenSearchHelper4;
    public static HUDSearchHelper4 hUDSearchHelper4;

    public static void do2214() {
        try {
            Class.forName("baritone.api.BaritoneAPI");
            obstaclePasserHelper = new ObjectSetSearchHelper4();
        } catch (Throwable th) {
            obstaclePasserHelper = new ObstaclePasserHelper_3();
        }
        welcomerHelper = new WelcomerHelper();
        antiPhaseSearchHelper4_2 = new AntiPhaseSearchHelper4_2();
        holeSnapSearchHelper4_4 = new HoleSnapSearchHelper4_4();
        breakHighlightSearchHelper4 = new BreakHighlightSearchHelper4();
        holeSnapSearchHelper4_2 = new HoleSnapSearchHelper4_2();
        inner = new HoleSnapSearchHelper4_2.Inner();
        mainhandHelper_2 = new MainhandHelper_2();
        holeSnapSearchHelper4_5 = new HoleSnapSearchHelper4_5();
        logoutSpotsHelper = new LogoutSpotsHelper();
        feetPlaceSearchHelper4 = new FeetPlaceSearchHelper4();
        searchHelper4_8 = new SearchHelper4_8();
        waypointsSearchHelper4 = new WaypointsSearchHelper4();
        antiPhaseSearchHelper4 = new AntiPhaseSearchHelper4();
        tooltipsSearchHelper4_2 = new TooltipsSearchHelper4_2();
        keyPearlSearchHelper4 = new KeyPearlSearchHelper4();
        searchHelper4_20 = new SearchHelper4_20();
        nameTagsSearchHelper4 = new NameTagsSearchHelper4();
        chatFilterSearchHelper4_2 = new ChatFilterSearchHelper4_2();
        hitmarkerSearchHelper4 = new HitmarkerSearchHelper4();
        searchHelper4_12 = new SearchHelper4_12();
        searchHelper4_14 = new SearchHelper4_14();
        searchHelper4_11 = new SearchHelper4_11();
        fireworksHelperSearchHelper4 = new FireworksHelperSearchHelper4();
        searchHelper4_5 = new SearchHelper4_5();
        searchHelper4_15 = new SearchHelper4_15();
        discordNotifsHelper = new DiscordNotifsHelper();
        stashFinderSearchHelper4 = new StashFinderSearchHelper4();
        scaffoldHelper = new ScaffoldHelper();
        tooltipsSearchHelper4 = new TooltipsSearchHelper4();
        chestStealerSearchHelper4_3 = new ChestStealerSearchHelper4_3();
        notificationsHelper = new NotificationsHelper();
        chatFilterSearchHelper4 = new ChatFilterSearchHelper4();
        items = new Items();
        Locale.setDefault(Locale.Category.FORMAT, Locale.US);
        do2215();
    }

    public static void do2215() {
        baritoneHelper_4.do1478();
        SearchHelper_4.baritoneHelper.getObject1794(new ClientEvent());
        searchHelper4_11.do2973();
        presetHelper = new PresetHelper();
        presetHelper.do72();
    }

    public static MixinTitleScreenSearchHelper4 getMixinTitleScreenSearchHelper42216() {
        if (mixinTitleScreenSearchHelper4 == null) {
            mixinTitleScreenSearchHelper4 = new MixinTitleScreenSearchHelper4();
        }
        return mixinTitleScreenSearchHelper4;
    }

    public static HUDSearchHelper4 getHUDSearchHelper42217() {
        if (hUDSearchHelper4 == null) {
            hUDSearchHelper4 = new HUDSearchHelper4();
        }
        return hUDSearchHelper4;
    }
}
