package dev.myth.validation;

import dev.myth.Myth;
import dev.myth.UI.setting.Setting;
import dev.myth.model.render.NoRender;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientLifecycleEvents;
import net.minecraft.core.particles.ParticleTypes;
import net.minecraft.world.entity.EntityTypes;

/**
 * Mixin injectors are resolved when their target class is transformed, so booting to the title
 * screen proves almost nothing. This force-loads every class NoRender injects into: a target that
 * moved, or a descriptor that no longer matches, throws right here instead of in a real session.
 */
public final class NoRenderValidation implements ClientModInitializer {
    /** Every class dev.myth.mixin touches for NoRender. */
    private static final List<String> TARGETS = List.of(
            "net.minecraft.client.renderer.blockentity.BlockEntityRenderDispatcher",
            "net.minecraft.client.gui.components.BossHealthOverlay",
            "net.minecraft.client.Camera",
            "net.minecraft.client.gui.components.ChatComponent$DrawingBackgroundGraphicsAccess",
            "net.minecraft.client.gui.components.ChatComponent$DrawingFocusedGraphicsAccess",
            "net.minecraft.client.multiplayer.ClientLevel",
            "net.minecraft.client.multiplayer.ClientPacketListener",
            "net.minecraft.world.entity.Entity",
            "net.minecraft.client.renderer.entity.EntityRenderer",
            "net.minecraft.client.renderer.fog.FogRenderer",
            "net.minecraft.client.renderer.fog.environment.BlindnessFogEnvironment",
            "net.minecraft.client.renderer.fog.environment.DarknessFogEnvironment",
            "net.minecraft.client.renderer.GameRenderer",
            "com.mojang.blaze3d.font.GlyphInfo",
            "net.minecraft.client.gui.Hud",
            "net.minecraft.client.renderer.entity.layers.HumanoidArmorLayer",
            "net.minecraft.client.renderer.extract.LevelExtractor",
            "net.minecraft.client.renderer.LightmapRenderStateExtractor",
            "net.minecraft.client.renderer.entity.LivingEntityRenderer",
            "net.minecraft.client.particle.ParticleEngine",
            "net.minecraft.client.gui.components.PlayerTabOverlay",
            "net.minecraft.client.gui.screens.Screen",
            "net.minecraft.client.renderer.ScreenEffectRenderer",
            "net.minecraft.world.level.lighting.SkyLightSectionStorage",
            "net.minecraft.client.gui.components.toasts.ToastManager",
            "net.minecraft.client.renderer.WorldBorderRenderer");

    /** The Mio tables, so a typo in a default shows up as a failure rather than a surprise in game. */
    private static final List<String> DEFAULTS = List.of(
            "self=false", "hurtCam=true", "potions=false", "darkness=true", "blindness=true",
            "newSneaking=false", "totemOverlay=false", "hands=false", "handsOpacity=0.0",
            "ui=false", "messageIndicator=true", "unsecureServer=true", "tutorialToast=true",
            "textShadow=false", "bossBars=false", "scoreBoard=false", "tabIcons=false",
            "heldTooltips=false", "tint=false",
            "world=false", "explosions=true", "worldBorder=true", "skyLight=false", "fog=false",
            "range=3.0", "sky=false", "particles=false", "particleTypes=[]",
            "particleSelection=BlackList", "noBlockBreaking=false",
            "entitiesPage=false", "hurt=true", "nameTags=false", "interpolation=false",
            "fire=true", "others=false", "armor=false", "armorOpacity=0.0",
            "tileEntities=false", "tileDistance=64", "wardens=false", "wardenDistance=32",
            "entities=false", "entityTypes=[]", "removal=Partial", "entitySelection=BlackList");

    private final List<String> results = new ArrayList<>();

    @Override
    public void onInitializeClient() {
        ClientLifecycleEvents.CLIENT_STARTED.register(client -> {
            try {
                checkMixinTargets();
                checkModule();
                checkDefaults();
                checkBehaviour();
            } catch (Throwable error) {
                fail("validation crashed", error.toString());
                Myth.LOGGER.error("NoRender validation crashed", error);
            }
            write();
            client.stop();
        });
    }

    private void checkMixinTargets() {
        ClassLoader loader = Myth.class.getClassLoader();
        for (String target : TARGETS) {
            try {
                Class.forName(target, true, loader);
                pass("mixin target applied: " + target);
            } catch (Throwable error) {
                fail("mixin target failed: " + target, error.toString());
            }
        }
    }

    private void checkModule() {
        NoRender model = NoRender.active();
        check(model == null, "active() is null while disabled", "active() returned a model");
        NoRender registered = Myth.MODEL_MANAGER.getModel(NoRender.class).orElse(null);
        check(registered != null, "NoRender is registered", "not found in the model manager");
        if (registered == null) return;
        check(!registered.isEnabled(), "NoRender defaults to disabled", "enabled by default");
        check(!registered.isDrawn(), "NoRender stays out of the module list", "isDrawn() is true");
        check(registered.getSettings().size() == DEFAULTS.size(),
                "NoRender exposes " + DEFAULTS.size() + " settings", "found " + registered.getSettings().size());
    }

    private void checkDefaults() {
        NoRender model = Myth.MODEL_MANAGER.getModel(NoRender.class).orElse(null);
        if (model == null) return;
        List<Setting<?>> settings = model.getSettings();
        for (int i = 0; i < Math.min(settings.size(), DEFAULTS.size()); i++) {
            Setting<?> setting = settings.get(i);
            String expected = DEFAULTS.get(i);
            String actual = setting.getId() + "=" + setting.get();
            check(actual.equals(expected), "setting " + expected, "got " + actual);
        }
    }

    private void checkBehaviour() {
        NoRender model = Myth.MODEL_MANAGER.getModel(NoRender.class).orElse(null);
        if (model == null) return;
        try {
            model.setEnabled(true, false);
            check(NoRender.active() == model, "active() resolves once enabled", "active() stayed null");
            check(model.handOpacity() == 1, "Hands off leaves the hand opaque", "got " + model.handOpacity());
            check(model.armorOpacity() == 1, "Armor off leaves armour opaque", "got " + model.armorOpacity());
            check(!model.hidesHands(), "Hands off keeps the hand pass", "hidesHands() is true");
            check(!model.hidesEntity(EntityTypes.ZOMBIE), "an empty entity set hides nothing", "a zombie was filtered");
            check(!model.hidesParticleType(ParticleTypes.FLAME), "an unrelated particle survives", "flame was filtered");
            check(NoRender.hidesParticleType(ParticleTypes.EXPLOSION),
                    "Explosions filters the explosion flash", "explosion survived");
            check(NoRender.hidesParticleType(ParticleTypes.EXPLOSION_EMITTER),
                    "Explosions filters the explosion emitter", "explosion_emitter survived");
            // 26.2 defers the smoke and debris cloud to ClientExplosionTracker, which respawns it a
            // tick later as plain block particles. Filtering by type cannot reach it, so the cut has
            // to happen in trackExplosionEffects: assert both halves of that split explicitly.
            check(!NoRender.hidesParticleType(ParticleTypes.SMOKE),
                    "the explosion cloud is not reachable by particle type", "smoke was filtered by type");
            check(hasExplosionCloudHook(),
                    "ClientLevel.trackExplosionEffects carries the cloud hook", "no Myth injection found");
            model.potions.set(true);
            check(NoRender.hidesParticleType(ParticleTypes.WITCH),
                    "Potions filters the spell particle family", "witch survived");
            model.potions.set(false);
            model.setEnabled(false, false);
            check(!NoRender.hidesParticleType(ParticleTypes.EXPLOSION),
                    "a disabled module filters nothing", "explosion was still filtered");
        } catch (RuntimeException error) {
            fail("exercising NoRender", error.toString());
        } finally {
            try { model.setEnabled(false, false); } catch (RuntimeException ignored) {}
        }
    }

    /** Mixin renames its handlers, so the injection shows up as a synthetic myth$ method. */
    private static boolean hasExplosionCloudHook() {
        for (var method : net.minecraft.client.multiplayer.ClientLevel.class.getDeclaredMethods())
            if (method.getName().contains("myth$explosionCloud")) return true;
        return false;
    }

    private void check(boolean condition, String label, String detail) {
        if (condition) pass(label); else fail(label, detail);
    }
    private void pass(String label) { results.add("PASS " + label); }
    private void fail(String label, String detail) { results.add("FAIL " + label + " (" + detail + ")"); }

    private void write() {
        long failures = results.stream().filter(line -> line.startsWith("FAIL")).count();
        results.add("SUMMARY " + (results.size() - failures) + " pass, " + failures + " fail");
        String report = String.join(System.lineSeparator(), results) + System.lineSeparator();
        Path root = Path.of(System.getProperty("myth.validation.root", "."));
        try {
            Files.createDirectories(root);
            Files.writeString(root.resolve("result.txt"), report, StandardCharsets.UTF_8);
        } catch (IOException error) {
            Myth.LOGGER.error("Could not write the NoRender validation report", error);
        }
        results.forEach(line -> Myth.LOGGER.info("[norender-validation] {}", line));
    }
}
