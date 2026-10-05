package me.mioclient;

import it.unimi.dsi.fastutil.objects.ObjectArrayList;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.net.http.HttpResponse;
import java.nio.file.Path;
import java.time.Duration;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ThreadLocalRandom;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import net.minecraft.util.InvalidIdentifierException;
import net.minecraft.util.math.Vec3d;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_11.class */
public final class SearchHelper4_11 implements SearchHelper_4 {
    public final Map<SearchIdentifier, SearchHelper4_4> map = Collections.synchronizedMap(new HashMap());
    public ObjectArrayList<SearchHelper4_4> objectArrayList = new ObjectArrayList<>();
    public ObjectArrayList<SearchHelper4_4> objectArrayList2 = new ObjectArrayList<>();
    public ObjectArrayList<SearchHelper4_4> objectArrayList3 = new ObjectArrayList<>();
    public ObjectArrayList<SearchHelper4_4> objectArrayList4 = new ObjectArrayList<>();
    public ObjectArrayList<SearchHelper4_4> objectArrayList5 = new ObjectArrayList<>();
    public ObjectArrayList<SearchHelper4_4> objectArrayList6 = new ObjectArrayList<>();
    public ObjectArrayList<SearchHelper4_4> objectArrayList7 = new ObjectArrayList<>();
    public static final String string = "https://mioclient.me/assets/killstreaks.zip";
    public static final Logger logger = LogManager.getLogger("Mio Sound System");

    public SearchHelper4_11() {
        try {
            Path resolve = PresetHelper.path8.resolve("killstreaks");
            File file = resolve.toFile();
            if (!file.exists() || !file.isDirectory()) {
                logger.info("Downloading killstreak sounds");
                if (file.exists() && !file.delete()) {
                    throw new java.lang.RuntimeException("Failed to delete invalid killstreaks file");
                }
                if (!file.mkdirs()) {
                    throw new java.lang.RuntimeException("Failed to create the killstreaks folder");
                }
                HttpResponse httpResponse2960 = DiscordNotifsHelperSearchHelper4.getHttpResponse2960(DiscordNotifsHelperSearchHelper4.getBuilder2963("https://mioclient.me/assets/killstreaks.zip").header("User-Agent", "MioClient/2.0").timeout(Duration.ofSeconds(5L)).build(), HttpResponse.BodyHandlers.ofByteArray());
                if (httpResponse2960.statusCode() != 200) {
                    throw new java.lang.RuntimeException("Got invalid response code while downloading killstreak sounds (%d)".formatted(Integer.valueOf(httpResponse2960.statusCode())));
                }
                ZipInputStream zipInputStream = new ZipInputStream(new ByteArrayInputStream((byte[]) httpResponse2960.body()));
                while (true) {
                    ZipEntry nextEntry = zipInputStream.getNextEntry();
                    if (nextEntry == null) {
                        break;
                    } else if (nextEntry.isDirectory()) {
                        resolve.resolve(nextEntry.getName()).toFile().mkdirs();
                    } else {
                        PresetHelper_4.do1568(resolve.resolve(nextEntry.getName()), zipInputStream.readAllBytes());
                    }
                }
                logger.info("Finished downloading killstreak sounds");
            }
        } catch (Exception e) {
            logger.error("Failed to download killstreak sounds");
            e.printStackTrace();
        }
    }

    public ObjectArrayList<SearchHelper4_4> getObjectArrayList2967(String str) {
        ObjectArrayList<SearchHelper4_4> objectArrayList = new ObjectArrayList<>();
        File file = PresetHelper.path8.resolve("killstreaks").resolve(str).toFile();
        if (!file.exists()) {
            logger.warn("Folder %s doesn't exist, ignoring".formatted(file.getName()));
            try {
                file.mkdir();
            } catch (Exception e) {
            }
            return objectArrayList;
        }
        if (!file.isDirectory()) {
            throw new IllegalArgumentException("mio-fabric/killstreaks/%s isn't a folder".formatted(file.getName()));
        }
        for (File file2 : (File[]) Objects.requireNonNull(file.listFiles())) {
            if (!file2.isDirectory()) {
                if (file2.getName().endsWith(".ogg")) {
                    objectArrayList.add(new SearchHelper4_4(PresetHelper_4.getByteArray1569(file2.toPath())));
                } else {
                    logger.warn("Ignoring mio-fabric/killstreaks/%s/%s. Only .ogg sounds are supported".formatted(str, file2.getName()));
                }
            }
        }
        if (objectArrayList.isEmpty()) {
            logger.warn(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("No sounds were registered for \u0001"));
        }
        return objectArrayList;
    }

    public SearchHelper4_4 getSearchHelper4_42968(int i) {
        if (i < 2) {
            return null;
        }
        switch (i) {
            case 2:
                ObjectArrayList<SearchHelper4_4> objectArrayList = this.objectArrayList;
                return (SearchHelper4_4) objectArrayList.get(ThreadLocalRandom.current().nextInt(this.objectArrayList.size()));
            case 3:
                ObjectArrayList<SearchHelper4_4> objectArrayList2 = this.objectArrayList2;
                return (SearchHelper4_4) objectArrayList2.get(ThreadLocalRandom.current().nextInt(this.objectArrayList2.size()));
            case 4:
                ObjectArrayList<SearchHelper4_4> objectArrayList3 = this.objectArrayList3;
                return (SearchHelper4_4) objectArrayList3.get(ThreadLocalRandom.current().nextInt(this.objectArrayList3.size()));
            case 5:
                ObjectArrayList<SearchHelper4_4> objectArrayList4 = this.objectArrayList4;
                return (SearchHelper4_4) objectArrayList4.get(ThreadLocalRandom.current().nextInt(this.objectArrayList4.size()));
            case 6:
                ObjectArrayList<SearchHelper4_4> objectArrayList5 = this.objectArrayList5;
                return (SearchHelper4_4) objectArrayList5.get(ThreadLocalRandom.current().nextInt(this.objectArrayList5.size()));
            case 7:
                ObjectArrayList<SearchHelper4_4> objectArrayList6 = this.objectArrayList6;
                return (SearchHelper4_4) objectArrayList6.get(ThreadLocalRandom.current().nextInt(this.objectArrayList6.size()));
            case 8:
                ObjectArrayList<SearchHelper4_4> objectArrayList7 = this.objectArrayList7;
                return (SearchHelper4_4) objectArrayList7.get(ThreadLocalRandom.current().nextInt(this.objectArrayList7.size()));
            default:
                if (i % 4 != 0) {
                    return null;
                }
                ObjectArrayList<SearchHelper4_4> objectArrayList8 = this.objectArrayList7;
                return (SearchHelper4_4) objectArrayList8.get(ThreadLocalRandom.current().nextInt(this.objectArrayList7.size()));
        }
    }

    public Set<SearchIdentifier> getSet2969() {
        return this.map.keySet();
    }

    public SearchHelper4_4 getSearchHelper4_42970(SearchIdentifier searchIdentifier) {
        return (SearchHelper4_4) this.map.entrySet().stream().filter(entry -> {
            return (entry.getKey() == null || ((SearchIdentifier) entry.getKey()).getName() == null || ((SearchIdentifier) entry.getKey()).getString1610() == null || !((SearchIdentifier) entry.getKey()).getName().equals(searchIdentifier.getName()) || !((SearchIdentifier) entry.getKey()).getString1610().equals(searchIdentifier.getString1610())) ? false : true;
        }).map((v0) -> {
            return v0.getValue();
        }).findAny().orElse(SearchHelper4_4.searchHelper4_4);
    }

    public void do2971(SearchIdentifier searchIdentifier, float f) {
        getSearchHelper4_42970(searchIdentifier).do1820(f);
    }

    public void do2972(SearchIdentifier searchIdentifier, Vec3d vec3d, float f) {
        getSearchHelper4_42970(searchIdentifier).do1821(vec3d, f);
    }

    public void do2973() {
        do2975();
        this.map.clear();
        for (Mode_9 mode_9 : Mode_9.values()) {
            try {
                String string2921 = new ArgumentTypeHelper().getArgumentTypeHelper2919(mode_9.getSearchIdentifier1837().getName()).getString2921("/assets/mio/sounds/\u0001.ogg");
                InputStream resourceAsStream = getClass().getResourceAsStream(string2921);
                if (resourceAsStream == null) {
                    throw new FileNotFoundException(string2921);
                }
                this.map.put(mode_9.getSearchIdentifier1837(), new SearchHelper4_4(resourceAsStream.readAllBytes()));
            } catch (Exception e) {
            }
        }
        for (File file : PresetHelper.path8.toFile().listFiles()) {
            if (file.isDirectory() && !file.getName().equalsIgnoreCase("killstreaks")) {
                do2974(file.getName());
            }
        }
    }

    public void do2974(String str) {
        for (File file : PresetHelper.path8.resolve(str).toFile().listFiles()) {
            if (!file.isDirectory() && file.getName().endsWith(".ogg")) {
                try {
                    this.map.put(new SearchIdentifier(str, file.getName().replace(".ogg", "")), new SearchHelper4_4(PresetHelper_4.getByteArray1569(file.toPath())));
                } catch (Exception e) {
                    if (e instanceof InvalidIdentifierException) {
                        logger.warn("Failed to load sound: %s".formatted(e.getMessage()));
                    } else {
                        logger.warn("Failed to load sound %s: %s".formatted(file.getName(), e.toString()));
                    }
                }
            }
        }
    }

    public void do2975() {
        try {
            this.objectArrayList = getObjectArrayList2967("doublekill_2");
            this.objectArrayList2 = getObjectArrayList2967("triplekill_3");
            this.objectArrayList3 = getObjectArrayList2967("dominating_4");
            this.objectArrayList4 = getObjectArrayList2967("megakill_5");
            this.objectArrayList5 = getObjectArrayList2967("unstoppable_6");
            this.objectArrayList6 = getObjectArrayList2967("wickedsick_7");
            this.objectArrayList7 = getObjectArrayList2967("monsterkill_8up");
        } catch (Exception e) {
            logger.warn("Failed to load killstreak sounds");
        }
    }
}
