/*
 * Decompiled with CFR 0.2.2 (FabricMC 7c48b8c4).
 * 
 * Could not load the following classes:
 *  net.minecraft.registry.Registry
 *  net.minecraft.registry.Registries
 */
package nick;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;
import net.minecraft.registry.Registry;
import net.minecraft.registry.Registries;

public final class SettingSubdata {
    private static final Map<String, Subdata> DATA = new HashMap<String, Subdata>();

    public static void apply(String setting, Object inst) {
        Subdata data = DATA.get(setting);
        if (data == null) {
            return;
        }
        data.apply(inst);
    }

    static {
        DATA.put("me/mioclient/module/render/HoleESP.fill", new ConverterData("me.mioclient.module.render.HoleESP$HoleESPMode"));
        DATA.put("me/mioclient/module/render/HoleESP.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/HoleESP.height", new NumberData("m"));
        DATA.put("me/mioclient/module/render/HoleESP.radius", new NumberData("m"));
        DATA.put("me/mioclient/module/render/HoleESP.fadeRadius", new NumberData("m"));
        DATA.put("me/mioclient/module/render/HoleESP.safeFill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/HoleESP.safeOutline", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/HoleESP.unsafeFill", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/HoleESP.unsafeOutline", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/HoleESP.trappedFill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/HoleESP.trappedOutline", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/NoRender.opacity2", new NumberData(""));
        DATA.put("me/mioclient/module/render/NoRender.range", new NumberData(""));
        DATA.put("me/mioclient/module/render/NoRender.particleTypes", new RegistryData(30155557376L));
        DATA.put("me/mioclient/module/render/NoRender.selection2", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/render/NoRender.opacity", new NumberData(""));
        DATA.put("me/mioclient/module/render/NoRender.clusterAlpha", new NumberData(""));
        DATA.put("me/mioclient/module/render/NoRender.tileDistance", new NumberData("m"));
        DATA.put("me/mioclient/module/render/NoRender.wardenDistance", new NumberData("m"));
        DATA.put("me/mioclient/module/render/NoRender.entityTypes", new RegistryData(30153979672L));
        DATA.put("me/mioclient/module/render/NoRender.removal", new ConverterData("me.mioclient.module.render.NoRender$NoRenderMode"));
        DATA.put("me/mioclient/module/render/NoRender.selection", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/player/AutoFarm.mode", new ConverterData("me.mioclient.module.player.AutoFarm$AutoFarmMode"));
        DATA.put("me/mioclient/module/player/AutoFarm.range", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoFarm.bpt", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoFarm.removeDelay", new NumberData("s"));
        DATA.put("me/mioclient/module/movement/FastSwim.speed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/FastSwim.speed2", new NumberData(""));
        DATA.put("me/mioclient/module/movement/FastSwim.vSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/FastSwim.glide", new NumberData(""));
        DATA.put("me/mioclient/module/movement/FastSwim.accelMin", new NumberData(""));
        DATA.put("me/mioclient/module/movement/FastSwim.accelTime", new NumberData("s"));
        DATA.put("me/mioclient/module/combat/Offhand.item", new ConverterData("me.mioclient.OffhandMode"));
        DATA.put("me/mioclient/module/combat/Offhand.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/Offhand.health", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/Offhand.safe", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/Offhand.custom", new RegistryData(30154396840L));
        DATA.put("me/mioclient/module/movement/Warp.mode", new ConverterData("me.mioclient.module.movement.Warp$WarpMode"));
        DATA.put("me/mioclient/module/movement/Warp.boost", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Warp.charge", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Warp.chargeSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Warp.recharge", new ConverterData("me.mioclient.WarpHelperMode"));
        DATA.put("me/mioclient/module/combat/AutoBowRelease.delay", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/PingSpoof.delay", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/PingSpoof.jitter", new NumberData(""));
        DATA.put("me/mioclient/module/client/IRC.volume2", new NumberData(""));
        DATA.put("me/mioclient/module/client/IRC.info", new ConverterData("me.mioclient.module.client.IRC$IRCMode"));
        DATA.put("me/mioclient/module/client/IRC.volume", new NumberData(""));
        DATA.put("me/mioclient/module/movement/FastWeb.mode", new ConverterData("me.mioclient.module.movement.FastWeb$FastWebPredicateMode"));
        DATA.put("me/mioclient/module/movement/FastWeb.speed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/FastWeb.vSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/misc/VisualRange.volume", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoExp.mode", new ConverterData("me.mioclient.module.combat.AutoExp$SpeedMineMode"));
        DATA.put("me/mioclient/module/combat/AutoExp.delay", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoExp.frequency", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoExp.stopAt", new NumberData("%"));
        DATA.put("me/mioclient/module/exploit/FastProjectile.packets", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/FastProjectile.chargeDelay", new NumberData("s"));
        DATA.put("me/mioclient/module/misc/ExtraScreenshot.mode", new ConverterData("me.mioclient.module.misc.ExtraScreenshot$ExtraScreenshotMode"));
        DATA.put("me/mioclient/module/misc/ExtraScreenshot.volume", new NumberData(""));
        DATA.put("me/mioclient/module/combat/SelfFill.mode", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/combat/SelfFill.whitelist", new RegistryData(30089655776L));
        DATA.put("me/mioclient/module/combat/AutoArmor.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/AutoArmor.safeAmount", new NumberData("%"));
        DATA.put("me/mioclient/module/combat/AutoArmor.range", new NumberData("m"));
        DATA.put("me/mioclient/module/render/Chams.range", new NumberData("m"));
        DATA.put("me/mioclient/module/render/Chams.fadeRadius", new NumberData("%"));
        DATA.put("me/mioclient/module/render/Chams.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Chams.opacity", new NumberData("%"));
        DATA.put("me/mioclient/module/render/Chams.speed", new NumberData(""));
        DATA.put("me/mioclient/module/render/Chams.strength", new NumberData(""));
        DATA.put("me/mioclient/module/render/Chams.progress", new NumberData("%"));
        DATA.put("me/mioclient/module/render/Chams.animals", new ConverterData("me.mioclient.ChamsMode"));
        DATA.put("me/mioclient/module/render/Chams.hostiles", new ConverterData("me.mioclient.ChamsMode"));
        DATA.put("me/mioclient/module/render/Chams.players", new ConverterData("me.mioclient.ChamsMode"));
        DATA.put("me/mioclient/module/render/Chams.self", new ConverterData("me.mioclient.ChamsMode"));
        DATA.put("me/mioclient/module/render/Chams.crystals", new ConverterData("me.mioclient.ChamsMode"));
        DATA.put("me/mioclient/module/render/Chams.wireframe", new ConverterData("me.mioclient.ChamsMode_2"));
        DATA.put("me/mioclient/module/render/Chams.time", new NumberData(""));
        DATA.put("me/mioclient/module/render/Chams.motion", new NumberData(""));
        DATA.put("me/mioclient/module/render/Chams.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Chams.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Chams.shine", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Chams.popFill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Chams.popLine", new ColorData(false, true));
        DATA.put("me/mioclient/module/movement/Sprint.mode", new ConverterData("me.mioclient.module.movement.Sprint$SprintPredicateMode"));
        DATA.put("me/mioclient/module/movement/Sprint.boostSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/render/Highlight.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Highlight.color", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Highlight.fillColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/client/UI.msaa", new ConverterData("me.mioclient.module.client.UI$Mode"));
        DATA.put("me/mioclient/module/client/UI.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/client/UI.buttonHeight", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.frameWidth", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.modulePadding", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.animSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.guiScale", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.shadowColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/client/UI.shadowSize", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.volume", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.volume3", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.volume2", new NumberData(""));
        DATA.put("me/mioclient/module/client/UI.textColor", new ColorData(false, false));
        DATA.put("me/mioclient/module/client/UI.enabledColor", new ColorData(false, false));
        DATA.put("me/mioclient/module/client/UI.color", new ColorData(false, true));
        DATA.put("me/mioclient/module/client/UI.bgColor", new ColorData(false, false));
        DATA.put("me/mioclient/module/client/UI.bgButton", new ColorData(false, true));
        DATA.put("me/mioclient/module/client/UI.bgEnabled", new ColorData(false, true));
        DATA.put("me/mioclient/module/client/UI.tint", new ColorData(false, false));
        DATA.put("me/mioclient/module/client/UI.gradientColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/Criticals.mode", new ConverterData("me.mioclient.module.combat.Criticals$CriticalsMode"));
        DATA.put("me/mioclient/module/movement/NoSlow.mode", new ConverterData("me.mioclient.module.movement.NoSlow$NoSlowMode"));
        DATA.put("me/mioclient/module/player/NoInteract.mode", new ConverterData("me.mioclient.module.player.NoInteract$Mode"));
        DATA.put("me/mioclient/module/misc/PMSound.volume", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Speed.mode", new ConverterData("me.mioclient.module.movement.Speed$SpeedPredicateMode"));
        DATA.put("me/mioclient/module/movement/Speed.speed", new NumberData(""));
        DATA.put("me/mioclient/module/misc/DiscordNotifs.timeout", new NumberData("s"));
        DATA.put("me/mioclient/module/player/ChestStealer.whitelist", new RegistryData(30154396840L));
        DATA.put("me/mioclient/module/player/ChestStealer.mode", new ConverterData("me.mioclient.ChestStealerMode"));
        DATA.put("me/mioclient/module/player/ChestStealer.select", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/player/ChestStealer.lookFor", new ConverterData("me.mioclient.ChestStealerMode_2"));
        DATA.put("me/mioclient/module/player/ChestStealer.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/player/ChestStealer.timeout", new NumberData("ms"));
        DATA.put("me/mioclient/module/player/ChestStealer.frequency", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Fireworks.boostFactor", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Fireworks.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/movement/Fireworks.ticks", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Fireworks.horizontal", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Fireworks.vertical", new NumberData(""));
        DATA.put("me/mioclient/module/misc/CustomDeathText.mode", new ConverterData("me.mioclient.module.misc.CustomDeathText$CustomDeathTextMode"));
        DATA.put("me/mioclient/module/combat/Aura.sorting", new ConverterData("me.mioclient.module.combat.Aura$AuraMode_3"));
        DATA.put("me/mioclient/module/combat/Aura.fov", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/combat/Aura.delay", new NumberData(""));
        DATA.put("me/mioclient/module/combat/Aura.range", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/Aura.wallRange", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/Aura.limit", new NumberData(""));
        DATA.put("me/mioclient/module/combat/Aura.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/Aura.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/Aura.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/combat/Aura.weapon", new ConverterData("me.mioclient.module.combat.Aura$AuraPredicateMode"));
        DATA.put("me/mioclient/module/combat/Aura.swap", new ConverterData("me.mioclient.module.combat.Aura$AuraMode"));
        DATA.put("me/mioclient/module/combat/Aura.mace", new ConverterData("me.mioclient.module.combat.Aura$AuraMode_2"));
        DATA.put("me/mioclient/module/movement/HoleSnap.height", new NumberData("m"));
        DATA.put("me/mioclient/module/movement/HoleSnap.timeout", new NumberData("s"));
        DATA.put("me/mioclient/module/movement/HoleSnap.range", new NumberData("m"));
        DATA.put("me/mioclient/module/movement/HoleSnap.speed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/HoleSnap.pitch", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/render/Glint.color", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Particles.totemsScale", new NumberData(""));
        DATA.put("me/mioclient/module/render/Particles.totemsVelocity", new NumberData(""));
        DATA.put("me/mioclient/module/render/Particles.totemsColor1", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Particles.totemsColor2", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Particles.rocketScale", new NumberData(""));
        DATA.put("me/mioclient/module/render/Particles.rocketColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Particles.damageScale", new NumberData(""));
        DATA.put("me/mioclient/module/render/Particles.damageVelocity", new NumberData(""));
        DATA.put("me/mioclient/module/render/Particles.damageColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Particles.portalScale", new NumberData(""));
        DATA.put("me/mioclient/module/render/Particles.portalColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Particles.from", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Particles.to", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Waypoints.distance", new NumberData("k"));
        DATA.put("me/mioclient/module/render/Waypoints.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Waypoints.info", new ConverterData("me.mioclient.module.render.Waypoints$WaypointsMode"));
        DATA.put("me/mioclient/module/render/Waypoints.textScale", new NumberData(""));
        DATA.put("me/mioclient/module/render/Waypoints.color", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Waypoints.beamAlpha", new NumberData(""));
        DATA.put("me/mioclient/module/render/Animations.floatFactor", new NumberData(""));
        DATA.put("me/mioclient/module/render/Animations.rotationSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/render/Animations.crystalScale", new NumberData(""));
        DATA.put("me/mioclient/module/render/Animations.playerScale", new NumberData(""));
        DATA.put("me/mioclient/module/combat/SelfWeb.enemyRange", new NumberData("m"));
        DATA.put("me/mioclient/module/misc/KillEffects.volume", new NumberData(""));
        DATA.put("me/mioclient/module/misc/KillEffects.volume3", new NumberData(""));
        DATA.put("me/mioclient/module/misc/KillEffects.volume2", new NumberData(""));
        DATA.put("me/mioclient/module/misc/KillEffects.lightnings", new NumberData(""));
        DATA.put("me/mioclient/module/misc/KillEffects.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Trails.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Trails.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Trails.fadeDelay", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Trails.fadeDuration", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Trails.color", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Trails.threshold", new NumberData(""));
        DATA.put("me/mioclient/module/client/Notifications.watermark", new ColorData(false, true));
        DATA.put("me/mioclient/module/client/Notifications.brackets", new ColorData(false, true));
        DATA.put("me/mioclient/module/client/Notifications.primary", new ColorData(false, false));
        DATA.put("me/mioclient/module/client/Notifications.secondary", new ColorData(false, true));
        DATA.put("me/mioclient/module/client/Notifications.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Blur.radius", new NumberData(""));
        DATA.put("me/mioclient/module/render/Skeleton.color", new ColorData(false, true));
        DATA.put("me/mioclient/module/movement/EntityControl.speed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/EntityControl.vertical", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.shader", new ConverterData("me.mioclient.module.render.Shader$MixinHeldItemRendererMode"));
        DATA.put("me/mioclient/module/render/Shader.range2", new NumberData("m"));
        DATA.put("me/mioclient/module/render/Shader.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.glow", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.quality", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.step", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.speed", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.fillSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.outlineSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.fillStrength", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.outlineStrength", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.overlayAlpha", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.type", new ConverterData("me.mioclient.module.render.Shader$Mode"));
        DATA.put("me/mioclient/module/render/Shader.radius", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.alpha", new NumberData(""));
        DATA.put("me/mioclient/module/render/Shader.fill", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Shader.fillSecond", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Shader.outline2", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Shader.outlineSecond", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Shader.range", new NumberData("m"));
        DATA.put("me/mioclient/module/render/Shader.range3", new NumberData("m"));
        DATA.put("me/mioclient/module/misc/Announcer.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.rotations", new ConverterData("me.mioclient.module.exploit.RaytraceBypass$RaytraceBypassMode"));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.keep", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.offset", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.bruteOffset", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.iterations", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.bruteStep", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.basePitch", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/RaytraceBypass.backUpPitch", new NumberData(""));
        DATA.put("me/mioclient/module/render/Trajectories.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Trajectories.color", new ColorData(false, true));
        DATA.put("me/mioclient/module/exploit/Phase.mode", new ConverterData("me.mioclient.module.exploit.Phase$PhasePredicateMode"));
        DATA.put("me/mioclient/module/exploit/Phase.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/misc/SkinFlicker.mode", new ConverterData("me.mioclient.module.misc.SkinFlicker$SkinFlickerMode"));
        DATA.put("me/mioclient/module/misc/SkinFlicker.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/player/AutoMount.range", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoMount.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/movement/NoFall.mode", new ConverterData("me.mioclient.module.movement.NoFall$NoFallMode"));
        DATA.put("me/mioclient/module/movement/NoFall.groundDistance", new NumberData("m"));
        DATA.put("me/mioclient/module/movement/FakeLag.mode", new ConverterData("me.mioclient.module.movement.FakeLag$FakeLagMode"));
        DATA.put("me/mioclient/module/movement/FakeLag.timeout", new NumberData("ms"));
        DATA.put("me/mioclient/module/movement/FakeLag.disableTime", new NumberData("s"));
        DATA.put("me/mioclient/module/movement/FakeLag.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/movement/FakeLag.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/movement/FakeLag.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoNameTag.mode", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/player/AutoNameTag.whitelist", new RegistryData(30153979672L));
        DATA.put("me/mioclient/module/player/AutoNameTag.range", new NumberData("m"));
        DATA.put("me/mioclient/module/player/AutoNameTag.frequency", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoNameTag.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/player/AutoNameTag.swap", new ConverterData("me.mioclient.AutoCrystalMode_2"));
        DATA.put("me/mioclient/module/player/AutoFish.castDelay", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Crosshair.distance", new NumberData(""));
        DATA.put("me/mioclient/module/render/Crosshair.amplitude", new NumberData(""));
        DATA.put("me/mioclient/module/render/Crosshair.gap", new NumberData(""));
        DATA.put("me/mioclient/module/render/Crosshair.length", new NumberData(""));
        DATA.put("me/mioclient/module/render/Crosshair.width", new NumberData(""));
        DATA.put("me/mioclient/module/render/Crosshair.color", new ColorData(false, true));
        DATA.put("me/mioclient/module/player/Freecam.speed", new NumberData(""));
        DATA.put("me/mioclient/module/player/Freecam.verticalSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/player/RotationLock.value2", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/player/RotationLock.value", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/render/Zoom.amount", new NumberData(""));
        DATA.put("me/mioclient/module/render/Hitmarker.length", new NumberData(""));
        DATA.put("me/mioclient/module/render/Hitmarker.time", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Hitmarker.fadeTime", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Hitmarker.color", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Hitmarker.outlineColor", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Hitmarker.volume", new NumberData(""));
        DATA.put("me/mioclient/module/misc/AutoReconnect.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/render/Search.whitelist", new RegistryData(30089655776L));
        DATA.put("me/mioclient/module/render/Search.entityTypes", new RegistryData(30153979672L));
        DATA.put("me/mioclient/module/render/Search.fillOpacity", new NumberData(""));
        DATA.put("me/mioclient/module/render/Search.tracerOpacity", new NumberData(""));
        DATA.put("me/mioclient/module/render/Search.volume", new NumberData(""));
        DATA.put("me/mioclient/module/misc/ExtraTab.latency", new ConverterData("me.mioclient.module.misc.ExtraTab$MixinPlayerListHudMode"));
        DATA.put("me/mioclient/module/misc/ExtraTab.scale", new NumberData(""));
        DATA.put("me/mioclient/module/misc/ExtraTab.self", new ColorData(false, true));
        DATA.put("me/mioclient/module/misc/ExtraTab.mode", new ConverterData("me.mioclient.module.misc.ExtraTab$ExtraTabMode"));
        DATA.put("me/mioclient/module/movement/Velocity.mode", new ConverterData("me.mioclient.module.movement.Velocity$VelocityMode"));
        DATA.put("me/mioclient/module/movement/Velocity.horizontal", new NumberData("%"));
        DATA.put("me/mioclient/module/movement/Velocity.vertical", new NumberData("%"));
        DATA.put("me/mioclient/module/misc/AntiCyrillic.mode", new ConverterData("me.mioclient.module.misc.AntiCyrillic$AntiCyrillicMode"));
        DATA.put("me/mioclient/module/render/SkyColor.type", new ConverterData("me.mioclient.module.render.SkyColor$MixinClientWorldMode"));
        DATA.put("me/mioclient/module/render/SkyColor.fog", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/SkyColor.sky", new ColorData(false, false));
        DATA.put("me/mioclient/module/combat/AutoMine.logic", new ConverterData("me.mioclient.Mode_12"));
        DATA.put("me/mioclient/module/combat/AutoMine.grimDelay", new NumberData(""));
        DATA.put("me/mioclient/module/player/Nuker.mode", new ConverterData("me.mioclient.module.player.Nuker$NukerMode_2"));
        DATA.put("me/mioclient/module/player/Nuker.shape", new ConverterData("me.mioclient.module.player.Nuker$NukerMode_3"));
        DATA.put("me/mioclient/module/player/Nuker.sort", new ConverterData("me.mioclient.module.player.Nuker$NukerMode"));
        DATA.put("me/mioclient/module/player/Nuker.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/player/Nuker.range", new NumberData("m"));
        DATA.put("me/mioclient/module/player/Nuker.wallRange", new NumberData("m"));
        DATA.put("me/mioclient/module/player/Nuker.whitelist", new RegistryData(30089655776L));
        DATA.put("me/mioclient/module/client/Fonts.style", new ConverterData("me.mioclient.Mode_8"));
        DATA.put("me/mioclient/module/client/Fonts.size", new NumberData(""));
        DATA.put("me/mioclient/module/client/Fonts.shift", new NumberData(""));
        DATA.put("me/mioclient/module/client/Fonts.shadow", new NumberData(""));
        DATA.put("me/mioclient/module/client/Fonts.translate", new NumberData(""));
        DATA.put("me/mioclient/module/misc/BetterChat.format", new ConverterData("me.mioclient.module.misc.BetterChat$BetterChatMode"));
        DATA.put("me/mioclient/module/misc/BetterChat.textColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/misc/BetterChat.bracketsColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/misc/BetterChat.rainbowAmount", new NumberData(""));
        DATA.put("me/mioclient/module/misc/BetterChat.volume", new NumberData(""));
        DATA.put("me/mioclient/module/misc/BetterChat.type", new ConverterData("me.mioclient.module.misc.BetterChat$BetterChatMode_2"));
        DATA.put("me/mioclient/module/misc/BetterChat.speed", new NumberData(""));
        DATA.put("me/mioclient/module/misc/BetterChat.alpha", new NumberData("ms"));
        DATA.put("me/mioclient/module/misc/BetterChat.color", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/PhaseESP.safe", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/PhaseESP.semiSafe", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/PhaseESP.unsafe", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/PhaseESP.alpha", new NumberData(""));
        DATA.put("me/mioclient/module/render/Tracers.hitbox", new ConverterData("me.mioclient.module.render.Tracers$TracersMode"));
        DATA.put("me/mioclient/module/render/Tracers.maxDistance", new NumberData("m"));
        DATA.put("me/mioclient/module/render/Tracers.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Tracers.color", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/VoidESP.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/VoidESP.height", new NumberData("m"));
        DATA.put("me/mioclient/module/render/VoidESP.radius", new NumberData("m"));
        DATA.put("me/mioclient/module/render/VoidESP.fadeRadius", new NumberData("m"));
        DATA.put("me/mioclient/module/render/VoidESP.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/VoidESP.line", new ColorData(false, true));
        DATA.put("me/mioclient/module/player/FastPlace.delay", new NumberData(""));
        DATA.put("me/mioclient/module/misc/StashFinder.chestCount", new NumberData(""));
        DATA.put("me/mioclient/module/misc/StashFinder.volume", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/ChorusControl.yaw", new ConverterData("me.mioclient.ChorusControlMode"));
        DATA.put("me/mioclient/module/exploit/ChorusControl.range", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/ChorusControl.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/exploit/ChorusControl.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/exploit/ChorusControl.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/RocketExtender.timeout", new NumberData("s"));
        DATA.put("me/mioclient/module/player/AutoCraft.items", new RegistryData(30154396840L));
        DATA.put("me/mioclient/module/player/AutoCraft.recipeBlackList", new RegistryData(30154396840L));
        DATA.put("me/mioclient/module/player/AutoCraft.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/player/AutoCraft.frequency", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoCraft.drop", new ConverterData("me.mioclient.module.player.AutoCraft$AutoCraftMode"));
        DATA.put("me/mioclient/module/player/AutoCraft.range", new NumberData("m"));
        DATA.put("me/mioclient/module/player/AutoCraft.max", new NumberData(""));
        DATA.put("me/mioclient/module/combat/Pusher.range", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/Pusher.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/Pusher.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/Pusher.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/Pusher.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/combat/Pusher.fadeTime", new NumberData("s"));
        DATA.put("me/mioclient/module/exploit/NewChunks.distance", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/NewChunks.height", new NumberData("m"));
        DATA.put("me/mioclient/module/exploit/NewChunks.blocks", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/NewChunks.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/NewChunks.fill2", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.outline", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/exploit/NewChunks.line3", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.fill3", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.line2", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.fill4", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.line4", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.blocks2", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/NewChunks.fill5", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.line", new ColorData(false, false));
        DATA.put("me/mioclient/module/exploit/NewChunks.volume", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoBreed.range", new NumberData("m"));
        DATA.put("me/mioclient/module/player/AutoBreed.frequency", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoBreed.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/player/AutoUpgrade.items", new ConverterData("me.mioclient.module.player.AutoUpgrade$AutoUpgradeMode"));
        DATA.put("me/mioclient/module/player/AutoUpgrade.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/render/Ambience.brightness", new ConverterData("me.mioclient.module.render.Ambience$MixinEntityRendererMode"));
        DATA.put("me/mioclient/module/render/Ambience.color", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Ambience.lightLevel", new NumberData(""));
        DATA.put("me/mioclient/module/render/Ambience.time", new NumberData("h"));
        DATA.put("me/mioclient/module/render/Ambience.weather", new ConverterData("me.mioclient.module.render.Ambience$AmbiencePredicateMode"));
        DATA.put("me/mioclient/module/render/Ambience.amount", new NumberData(""));
        DATA.put("me/mioclient/module/misc/Spammer.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/misc/AntiAim.yaw", new ConverterData("me.mioclient.module.misc.AntiAim$AntiAimPredicateMode"));
        DATA.put("me/mioclient/module/misc/AntiAim.speed", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/misc/AntiAim.static_2", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/misc/AntiAim.flip", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/misc/AntiAim.jitterRange", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/misc/AntiAim.base", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/misc/AntiAim.pitch2", new ConverterData("me.mioclient.module.misc.AntiAim$AntiAimMode"));
        DATA.put("me/mioclient/module/misc/AntiAim.static_", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/misc/AntiAim.look", new ConverterData("me.mioclient.module.misc.AntiAim$AntiAimMode_2"));
        DATA.put("me/mioclient/module/combat/AnchorAura.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/AnchorAura.range", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AnchorAura.wallRange", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AnchorAura.targetRange", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AnchorAura.minDamage", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AnchorAura.maxSelfDamage", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AnchorAura.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/AnchorAura.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/AnchorAura.fadeTime", new NumberData("s"));
        DATA.put("me/mioclient/module/combat/AnchorAura.health", new NumberData("hp"));
        DATA.put("me/mioclient/module/misc/Swing.hand", new ConverterData("me.mioclient.module.misc.Swing$ScaffoldHelperMode"));
        DATA.put("me/mioclient/module/misc/Swing.type", new ConverterData("me.mioclient.module.misc.Swing$MixinLivingEntityMode"));
        DATA.put("me/mioclient/module/misc/Swing.speed", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AntiPhase.material", new ConverterData("me.mioclient.AntiPhaseMode"));
        DATA.put("me/mioclient/module/combat/AntiPhase.range", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AntiPhase.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/AntiPhase.bpt", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AntiPhase.fill", new ColorData(false, false));
        DATA.put("me/mioclient/module/combat/AntiPhase.outline", new ColorData(false, false));
        DATA.put("me/mioclient/module/combat/AntiPhase.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AntiPhase.fadeTime", new NumberData("s"));
        DATA.put("me/mioclient/module/player/AutoTame.action", new ConverterData("me.mioclient.module.player.AutoTame$AutoTameMode"));
        DATA.put("me/mioclient/module/player/AutoTame.range", new NumberData("m"));
        DATA.put("me/mioclient/module/player/AutoTame.frequency", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoTame.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/exploit/Timer.mode", new ConverterData("me.mioclient.module.exploit.Timer$TimerPredicateMode"));
        DATA.put("me/mioclient/module/exploit/Timer.shift", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/Timer.pulseMin", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/Timer.pulseMax", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/Timer.minDelay", new NumberData("s"));
        DATA.put("me/mioclient/module/exploit/Timer.maxDelay", new NumberData("s"));
        DATA.put("me/mioclient/module/exploit/Timer.disableDelay", new NumberData("s"));
        DATA.put("me/mioclient/module/render/NoBob.multiplier", new NumberData(""));
        DATA.put("me/mioclient/module/render/ESP.fill5", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.outline2", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.fill6", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.outline4", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.fill2", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.outline6", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.fill3", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.mode", new ConverterData("me.mioclient.ESPPredicateMode"));
        DATA.put("me/mioclient/module/render/ESP.range", new NumberData("m"));
        DATA.put("me/mioclient/module/render/ESP.itemsText", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.background", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/ESP.scale", new NumberData(""));
        DATA.put("me/mioclient/module/render/ESP.fill4", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.outline3", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.outline5", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.chorusText", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/ESP.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/movement/AutoWalk.volume", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoClicker.mode", new ConverterData("me.mioclient.module.combat.AutoClicker$AutoClickerMode"));
        DATA.put("me/mioclient/module/combat/AutoClicker.cps", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoClicker.delay", new NumberData("x"));
        DATA.put("me/mioclient/module/movement/HighJump.strength", new NumberData(""));
        DATA.put("me/mioclient/module/render/Xray.whitelist", new RegistryData(30089655776L));
        DATA.put("me/mioclient/module/render/Borders.chunk", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Borders.map", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/Borders.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Borders.mapSize", new NumberData(""));
        DATA.put("me/mioclient/module/render/Borders.level", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/FastLatency.mode", new ConverterData("me.mioclient.module.exploit.FastLatency$FastLatencyMode"));
        DATA.put("me/mioclient/module/exploit/FastLatency.timeout", new NumberData("s"));
        DATA.put("me/mioclient/module/exploit/Reach.modifier", new NumberData("m"));
        DATA.put("me/mioclient/module/misc/UnfocusedFPS.fps", new NumberData(""));
        DATA.put("me/mioclient/module/combat/BedAura.delay2", new NumberData(""));
        DATA.put("me/mioclient/module/combat/BedAura.range2", new NumberData(""));
        DATA.put("me/mioclient/module/combat/BedAura.delay", new NumberData(""));
        DATA.put("me/mioclient/module/combat/BedAura.range", new NumberData(""));
        DATA.put("me/mioclient/module/combat/BedAura.ticks", new NumberData(""));
        DATA.put("me/mioclient/module/combat/BedAura.slot", new NumberData(""));
        DATA.put("me/mioclient/module/combat/BedAura.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/BedAura.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/BedAura.fadeTime", new NumberData("s"));
        DATA.put("me/mioclient/module/combat/BedAura.minDamage", new NumberData(""));
        DATA.put("me/mioclient/module/combat/BedAura.maxSelfDamage", new NumberData(""));
        DATA.put("me/mioclient/module/render/LogoutSpots.distance", new NumberData("m"));
        DATA.put("me/mioclient/module/render/LogoutSpots.model", new ConverterData("me.mioclient.module.render.LogoutSpots$LogoutSpotsMode_2"));
        DATA.put("me/mioclient/module/render/LogoutSpots.alpha", new NumberData(""));
        DATA.put("me/mioclient/module/render/LogoutSpots.width", new NumberData(""));
        DATA.put("me/mioclient/module/render/LogoutSpots.width2", new NumberData(""));
        DATA.put("me/mioclient/module/render/LogoutSpots.textScale", new NumberData(""));
        DATA.put("me/mioclient/module/render/LogoutSpots.position", new ConverterData("me.mioclient.module.render.LogoutSpots$LogoutSpotsMode"));
        DATA.put("me/mioclient/module/render/LogoutSpots.modelFill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/LogoutSpots.modelLine", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/LogoutSpots.boxFill", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/LogoutSpots.boxLine", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/LogoutSpots.nameTagColor", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/LogoutSpots.fillColor", new ColorData(false, false));
        DATA.put("me/mioclient/module/combat/AutoCrystal.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.range", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.wallRange", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.minDamage2", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.maxSelfDamage", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.damageRatio", new NumberData("x"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.sequential", new ConverterData("me.mioclient.AutoCrystalMode_5"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.delay2", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.factor", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoCrystal.range2", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.wallRange2", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.minDamage", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.maxSelfDamage2", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.instant", new ConverterData("me.mioclient.AutoCrystalMode"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.ticksExisted", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoCrystal.weakness", new ConverterData("me.mioclient.ScaffoldHelperMode"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.health2", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.armorThreshold", new NumberData("%"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.ticks", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoCrystal.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/AutoCrystal.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/AutoCrystal.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoCrystal.fadeTime", new NumberData("s"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.health", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.targetRange", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.crystalRange", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.timeout", new NumberData("s"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.autoSwap", new ConverterData("me.mioclient.AutoCrystalMode_2"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.silent", new ConverterData("me.mioclient.AutoCrystalMode_4"));
        DATA.put("me/mioclient/module/combat/AutoCrystal.swapDelay", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoCrystal.swapPenalty", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoCrystal.multipoint", new ConverterData("me.mioclient.AutoCraftMode"));
        DATA.put("me/mioclient/module/movement/Step.mode", new ConverterData("me.mioclient.module.movement.Step$StepMode"));
        DATA.put("me/mioclient/module/movement/Step.height", new NumberData("m"));
        DATA.put("me/mioclient/module/movement/Step.timerSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/player/SpeedMine.mode", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/player/SpeedMine.damage", new NumberData(""));
        DATA.put("me/mioclient/module/player/SpeedMine.range", new NumberData("m"));
        DATA.put("me/mioclient/module/player/SpeedMine.rebreak2", new ConverterData("me.mioclient.SpeedMineMode_2"));
        DATA.put("me/mioclient/module/player/SpeedMine.instantDelay", new NumberData("s"));
        DATA.put("me/mioclient/module/player/SpeedMine.autoSwap", new ConverterData("me.mioclient.SpeedMineMode_3"));
        DATA.put("me/mioclient/module/player/SpeedMine.f121", new ConverterData("me.mioclient.SpeedMineMode_4"));
        DATA.put("me/mioclient/module/player/SpeedMine.limit", new NumberData(""));
        DATA.put("me/mioclient/module/player/SpeedMine.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/player/SpeedMine.renderMode", new ConverterData("me.mioclient.SpeedMineMode"));
        DATA.put("me/mioclient/module/player/SpeedMine.colorMode", new ConverterData("me.mioclient.SpeedMineMode_5"));
        DATA.put("me/mioclient/module/player/SpeedMine.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/player/SpeedMine.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/player/SpeedMine.blocks", new RegistryData(30089655776L));
        DATA.put("me/mioclient/module/combat/Arrows.fov", new NumberData(""));
        DATA.put("me/mioclient/module/combat/Arrows.assumeDuration", new NumberData("%"));
        DATA.put("me/mioclient/module/movement/SafeWalk.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/movement/ElytraFly.mode", new ConverterData("me.mioclient.module.movement.ElytraFly$ElytraFlyPredicateMode"));
        DATA.put("me/mioclient/module/movement/ElytraFly.takeoff", new ConverterData("me.mioclient.module.movement.ElytraFly$ElytraFlyMode_2"));
        DATA.put("me/mioclient/module/movement/ElytraFly.minBoost", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.verticalBoost", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.limit", new NumberData("km/h"));
        DATA.put("me/mioclient/module/movement/ElytraFly.speed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.minY", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.add", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.boostPitch", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.speed3", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.vertical", new ConverterData("me.mioclient.module.movement.ElytraFly$ElytraFlyMode"));
        DATA.put("me/mioclient/module/movement/ElytraFly.vSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.vPitch", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.glide", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.pitch", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.verAccelMin", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.accelMin", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.accelTime", new NumberData("s"));
        DATA.put("me/mioclient/module/movement/ElytraFly.start", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.speed2", new NumberData(""));
        DATA.put("me/mioclient/module/movement/ElytraFly.deployTime", new NumberData("ms"));
        DATA.put("me/mioclient/module/movement/ElytraFly.pitch2", new NumberData(""));
        DATA.put("me/mioclient/module/player/Replenish.items", new RegistryData(30154396840L));
        DATA.put("me/mioclient/module/player/Replenish.selection", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/player/Replenish.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/player/Replenish.threshold", new NumberData(""));
        DATA.put("me/mioclient/module/movement/LongJump.jumpSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/misc/SoundBlocker.sounds", new RegistryData(30107510864L));
        DATA.put("me/mioclient/module/movement/Flight.mode", new ConverterData("me.mioclient.module.movement.Flight$FlightMode_2"));
        DATA.put("me/mioclient/module/movement/Flight.speed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Flight.vSpeed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Flight.glide", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Flight.antiKick", new ConverterData("me.mioclient.module.movement.Flight$FlightMode"));
        DATA.put("me/mioclient/module/movement/Flight.accelMin", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Flight.accelTime", new NumberData("s"));
        DATA.put("me/mioclient/module/render/ViewModel.mainX", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.mainY", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.mainZ", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.mainScaleX", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.mainScaleY", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.mainScaleZ", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.mainRotateX", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/render/ViewModel.mainRotateY", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/render/ViewModel.mainRotateZ", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/render/ViewModel.offX", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.offY", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.offZ", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.offScaleX", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.offScaleY", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.offScaleZ", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.offRotateX", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/render/ViewModel.offRotateY", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/render/ViewModel.offRotateZ", new NumberData(new String(new byte[]{-62, -80})));
        DATA.put("me/mioclient/module/render/ViewModel.eatMultiplier", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.swingProgressAmount2", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.swingProgressAmount", new NumberData(""));
        DATA.put("me/mioclient/module/render/ViewModel.fovAmount", new NumberData(""));
        DATA.put("me/mioclient/module/render/Tunnels.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/render/Tunnels.height", new NumberData("m"));
        DATA.put("me/mioclient/module/render/Tunnels.minLength", new NumberData("m"));
        DATA.put("me/mioclient/module/render/Tunnels.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/Tunnels.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/Mainhand.slot", new NumberData(""));
        DATA.put("me/mioclient/module/combat/Mainhand.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/Mainhand.delay2", new NumberData("ms"));
        DATA.put("me/mioclient/module/combat/Mainhand.health", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/Mainhand.safe", new NumberData("hp"));
        DATA.put("me/mioclient/module/player/AutoEat.health", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoEat.hunger", new NumberData(""));
        DATA.put("me/mioclient/module/player/AutoEat.sort", new ConverterData("me.mioclient.module.player.AutoEat$AutoEatMode"));
        DATA.put("me/mioclient/module/exploit/Trident.boost", new NumberData(""));
        DATA.put("me/mioclient/module/exploit/Trident.delay", new NumberData(""));
        DATA.put("me/mioclient/module/player/Scaffold.selection", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/player/Scaffold.blocks", new RegistryData(30089655776L));
        DATA.put("me/mioclient/module/player/Scaffold.swap", new ConverterData("me.mioclient.ScaffoldMode_3"));
        DATA.put("me/mioclient/module/player/Scaffold.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/player/Scaffold.offset", new NumberData(""));
        DATA.put("me/mioclient/module/player/Scaffold.lineWidth", new NumberData(""));
        DATA.put("me/mioclient/module/player/Scaffold.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/player/Scaffold.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/player/Scaffold.fadeTime", new NumberData("s"));
        DATA.put("me/mioclient/module/render/BreakHighlight.range", new NumberData("m"));
        DATA.put("me/mioclient/module/render/BreakHighlight.fill", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/BreakHighlight.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/combat/AutoLog.health", new NumberData("hp"));
        DATA.put("me/mioclient/module/combat/AutoLog.invincTimeout", new NumberData("s"));
        DATA.put("me/mioclient/module/combat/AutoLog.totemCount", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AutoLog.durability", new NumberData("%"));
        DATA.put("me/mioclient/module/combat/AutoLog.y", new NumberData("m"));
        DATA.put("me/mioclient/module/render/NameTags.range", new NumberData("m"));
        DATA.put("me/mioclient/module/render/NameTags.scale", new NumberData(""));
        DATA.put("me/mioclient/module/render/NameTags.durability", new ConverterData("me.mioclient.module.render.NameTags$NameTagsMode"));
        DATA.put("me/mioclient/module/render/NameTags.fill", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/NameTags.outline", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/NameTags.hole", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/NameTags.phase", new ColorData(false, true));
        DATA.put("me/mioclient/module/render/NameTags.text", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/NameTags.invisibles", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/NameTags.sneak", new ColorData(false, false));
        DATA.put("me/mioclient/module/render/NameTags.eating", new ColorData(false, false));
        DATA.put("me/mioclient/module/movement/AntiVoid.mode", new ConverterData("me.mioclient.module.movement.AntiVoid$AntiVoidMode"));
        DATA.put("me/mioclient/module/movement/AntiVoid.height", new NumberData("m"));
        DATA.put("me/mioclient/module/render/ViewClip.range", new NumberData(""));
        DATA.put("me/mioclient/module/player/InventoryCleaner.whitelist", new RegistryData(30154396840L));
        DATA.put("me/mioclient/module/player/InventoryCleaner.selection", new ConverterData("me.mioclient.ScaffoldMode_2"));
        DATA.put("me/mioclient/module/player/InventoryCleaner.delay", new NumberData("ms"));
        DATA.put("me/mioclient/module/player/InventoryCleaner.frequency", new NumberData(""));
        DATA.put("me/mioclient/module/movement/FastFall.mode", new ConverterData("me.mioclient.module.movement.FastFall$FastFallMode"));
        DATA.put("me/mioclient/module/movement/FastFall.height", new NumberData("m"));
        DATA.put("me/mioclient/module/movement/FastFall.speed", new NumberData(""));
        DATA.put("me/mioclient/module/misc/AntiAFK.delay", new NumberData("s"));
        DATA.put("me/mioclient/module/movement/FastLadder.speed", new NumberData(""));
        DATA.put("me/mioclient/module/movement/Jesus.mode", new ConverterData("me.mioclient.module.movement.Jesus$JesusMode"));
        DATA.put("me/mioclient/module/movement/Jesus.ascending", new NumberData(""));
        DATA.put("me/mioclient/module/combat/AnvilAura.targetRange", new NumberData("m"));
        DATA.put("me/mioclient/module/client/Colors.friendColor", new ColorData(false, false));
        DATA.put("me/mioclient/module/client/Colors.enemyColor", new ColorData(false, false));
        DATA.put("me/mioclient/module/client/Colors.themeColor", new ColorData(false, false));
        DATA.put("me/mioclient/module/client/Colors.scheme", new ConverterData("me.mioclient.SearchMode"));
        DATA.put("me/mioclient/module/client/AntiCheat.yawStep", new NumberData(""));
        DATA.put("me/mioclient/module/client/AntiCheat.rotations", new ConverterData("me.mioclient.AutoCrystalMode_6"));
        DATA.put("me/mioclient/module/client/AntiCheat.strictDirection", new ConverterData("me.mioclient.Mode_6"));
        DATA.put("me/mioclient/module/combat/WebAura.range", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/WebAura.ticks", new NumberData(""));
        DATA.put("me/mioclient/module/combat/Blocker.offsets", new ConverterData("me.mioclient.BlockerPredicateMode"));
        DATA.put("me/mioclient/module/combat/Blocker.blockDamage", new NumberData(""));
        DATA.put("me/mioclient/module/combat/HoleFill.whitelist", new RegistryData(30089655776L));
        DATA.put("me/mioclient/module/combat/HoleFill.mode", new ConverterData("me.mioclient.module.combat.HoleFill$HoleFillPredicateMode"));
        DATA.put("me/mioclient/module/combat/HoleFill.smartRange", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/HoleFill.range", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/HoleFill.verticalRange", new NumberData("m"));
        DATA.put("me/mioclient/module/combat/HoleFill.ticks", new NumberData(""));
    }

    private static abstract class Subdata {
        private Subdata() {
        }

        public abstract void apply(Object var1);
    }

    private static final class ConverterData
    extends Subdata {
        private static final Field CONVERTER_FIELD;
        private final Object c;

        public ConverterData(String s) {
            try {
                Class<?> k = Class.forName(s);
                Class<?> converter = Class.forName("me.mioclient.EnumSettingConverter");
                this.c = converter.getDeclaredConstructors()[0].newInstance(k);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }

        @Override
        public void apply(Object setting) {
            try {
                CONVERTER_FIELD.set(setting, this.c);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }

        static {
            try {
                CONVERTER_FIELD = Class.forName("me.mioclient.EnumSetting").getDeclaredField("enumSettingConverter");
                CONVERTER_FIELD.setAccessible(true);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }
    }

    private static final class NumberData
    extends Subdata {
        private static final Field STR_FIELD;
        private final String s;

        public NumberData(String s) {
            this.s = s;
        }

        @Override
        public void apply(Object setting) {
            try {
                STR_FIELD.set(setting, this.s);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }

        static {
            try {
                STR_FIELD = Class.forName("me.mioclient.NumberSetting").getDeclaredField("string");
                STR_FIELD.setAccessible(true);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }
    }

    private static final class ColorData
    extends Subdata {
        private static final Field A1;
        private static final Field B1;
        private static final Field B2;
        private final Object c;
        private final boolean _b1;
        private final boolean _b2;

        public ColorData(boolean b1, boolean b2) {
            try {
                this.c = A1.getType().getDeclaredConstructors()[0].newInstance(new Object[0]);
                this._b1 = b1;
                this._b2 = b2;
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }

        @Override
        public void apply(Object setting) {
            try {
                A1.set(setting, this.c);
                B1.set(setting, this._b1);
                B2.set(setting, this._b2);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }

        static {
            try {
                Class<?> k = Class.forName("me.mioclient.ColorSetting");
                A1 = k.getDeclaredField("converter");
                B1 = k.getDeclaredField("flag");
                B2 = k.getDeclaredField("flag2");
                A1.setAccessible(true);
                B1.setAccessible(true);
                B2.setAccessible(true);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }
    }

    private static final class RegistryData
    extends Subdata {
        private static final Field REGISTRY_FIELD;
        private final Registry<?> registry;

        public RegistryData(long id) {
            if (id == 30089655776L) {
                this.registry = Registries.BLOCK;
            } else if (id == 30154396840L) {
                this.registry = Registries.ITEM;
            } else if (id == 30153979672L) {
                this.registry = Registries.ENTITY_TYPE;
            } else if (id == 30155557376L) {
                this.registry = Registries.PARTICLE_TYPE;
            } else if (id == 30107510864L) {
                this.registry = Registries.SOUND_EVENT;
            } else {
                throw new RuntimeException("unknown registry identifier: 0x" + Long.toHexString(id));
            }
        }

        @Override
        public void apply(Object setting) {
            try {
                REGISTRY_FIELD.set(setting, this.registry);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }

        static {
            try {
                REGISTRY_FIELD = Class.forName("me.mioclient.EntityListObjectSetting").getDeclaredField("registry");
                REGISTRY_FIELD.setAccessible(true);
            } catch (Throwable _t) {
                _t.printStackTrace(System.err);
                throw new RuntimeException();
            }
        }
    }
}

