package dev.myth.model.misc;

import com.mojang.authlib.GameProfile;
import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.MotionEvent;
import dev.myth.event.impl.PacketEvent;
import dev.myth.event.impl.RespawnEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.UUID;
import net.minecraft.client.Minecraft;
import net.minecraft.client.multiplayer.ClientLevel;
import net.minecraft.client.player.RemotePlayer;
import net.minecraft.core.particles.ParticleTypes;
import net.minecraft.network.protocol.game.ServerboundAttackPacket;
import net.minecraft.sounds.SoundEvents;
import net.minecraft.sounds.SoundSource;
import net.minecraft.world.effect.MobEffectInstance;
import net.minecraft.world.effect.MobEffects;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.ai.attributes.Attributes;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.level.Explosion;
import net.minecraft.world.phys.Vec3;

/** Mio's {@code .fakeplayer} command (Feature_14) rebuilt as a module. */
public final class FakePlayer extends Model {
    // Mio reserves this client-only id so the copy can never collide with a tracked entity.
    private static final int ENTITY_ID = -9344;
    public final Setting<String> name = addSetting(Setting.text("name", "Name", "Herobrine"));
    public final Setting<Boolean> record = addSetting(Setting.bool("record", "Record", false));
    public final Setting<Boolean> play = addSetting(Setting.bool("play", "Play", false));
    private final List<Frame> frames = new ArrayList<>();
    private Copy copy;
    private int current;

    public FakePlayer() {
        super("fakeplayer", "FakePlayer", Category.MISC);
        setDescription("Spawns a client-side copy of you that survives on totems.");
        name.onChange(value -> {
            if (value.isBlank() || value.length() > 16) throw new IllegalArgumentException("Name must be 1 to 16 characters");
            if (isEnabled()) spawn();
        });
        record.onChange(value -> { if (value) frames.clear(); });
        play.onChange(value -> current = 0);
    }

    @Override public String getInfo() { return copy == null ? null : name.get(); }
    @Override protected void onEnable() { spawn(); }
    @Override protected void onDisable() {
        remove(); frames.clear(); record.set(false); play.set(false);
    }

    private void spawn() {
        var client = Minecraft.getInstance();
        remove();
        if (client.level == null || client.player == null) return;
        copy = new Copy(client.level, new GameProfile(UUID.randomUUID(), name.get()), client.player);
        copy.setId(ENTITY_ID);
        client.level.addEntity(copy);
        copy.setOnGround(client.player.onGround());
    }

    private void remove() {
        var client = Minecraft.getInstance();
        if (copy != null && client.level != null) client.level.removeEntity(ENTITY_ID, Entity.RemovalReason.DISCARDED);
        copy = null; current = 0;
    }

    @EventTarget private void onMotion(MotionEvent event) {
        var player = Minecraft.getInstance().player;
        if (player == null) return;
        if (event.getType() == EventType.POST) {
            if (record.get()) frames.add(Frame.of(player));
            return;
        }
        if (copy == null || !play.get() || frames.isEmpty()) return;
        current = (current + 1) % frames.size();
        frames.get(current).apply(copy);
    }

    /** The old level is gone by the time the player ticks again, so re-spawn into the new one. */
    @EventTarget private void onRespawn(RespawnEvent event) {
        copy = null; current = 0; frames.clear(); spawn();
    }

    /** The copy exists only here; letting its id reach the server would be a bogus attack. */
    @EventTarget private void onPacket(PacketEvent event) {
        if (copy == null || event.getType() != EventType.OUTGOING) return;
        if (!(event.getPacket() instanceof ServerboundAttackPacket packet) || packet.entityId() != ENTITY_ID) return;
        event.setCancelled(true);
        // 26.2 routes client-side damage through hurtClient, which moves no health, so Mio's
        // hit has to be applied here for the totem and hurt animation to happen at all.
        var player = Minecraft.getInstance().player;
        if (player != null) copy.hit(player);
    }

    private record Frame(Vec3 position, float yaw, float pitch, float headYaw, Vec3 velocity,
                         boolean sneaking, boolean onGround) {
        static Frame of(Player player) {
            return new Frame(player.position(), player.getYRot(), player.getXRot(), player.yHeadRot,
                    player.getDeltaMovement(), player.isShiftKeyDown(), player.onGround());
        }
        void apply(Copy copy) {
            copy.moveOrInterpolateTo(Optional.of(position), Optional.of(yaw), Optional.of(pitch));
            copy.setDeltaMovement(velocity);
            copy.setYRot(yaw); copy.setXRot(pitch); copy.setYHeadRot(headYaw);
            copy.setShiftKeyDown(sneaking); copy.setOnGround(onGround);
            copy.setPose(sneaking ? net.minecraft.world.entity.Pose.CROUCHING : net.minecraft.world.entity.Pose.STANDING);
        }
    }

    /** Mio's inner OtherClientPlayerEntity: a totem-backed dummy that never finishes dying. */
    private static final class Copy extends RemotePlayer {
        private int ticks;

        Copy(ClientLevel level, GameProfile profile, Player source) {
            super(level, profile);
            copyPosition(source);
            yRotO = getYRot(); xRotO = getXRot();
            yHeadRot = source.yHeadRot; yHeadRotO = yHeadRot;
            yBodyRot = source.yBodyRot; yBodyRotO = yBodyRot;
            setPose(source.getPose());
            // 26.2 clamps absorption to MAX_ABSORPTION, and a client-only entity never receives
            // the effect's attribute modifier, so Mio's 16 point shield needs the base value here.
            var absorption = getAttribute(Attributes.MAX_ABSORPTION);
            if (absorption != null) absorption.setBaseValue(16);
            setHealth(20);
            getInventory().replaceWith(source.getInventory());
        }

        /** Vanilla's cooldown ramp against the attacker's attack damage, absorption first. */
        void hit(Player attacker) {
            float scale = attacker.getAttackStrengthScale(.5f);
            float damage = (float) attacker.getAttributeValue(Attributes.ATTACK_DAMAGE) * (.2f + scale * scale * .8f);
            float absorbed = Math.min(getAbsorptionAmount(), damage);
            setAbsorptionAmount(getAbsorptionAmount() - absorbed);
            hurtTime = hurtDuration = 10;
            setHealth(getHealth() - (damage - absorbed));
        }

        @Override public void setHealth(float health) {
            super.setHealth(health);
            if (getHealth() > 0) return;
            // Mio pops a totem instead of dying: 9 hearts back, regeneration and the vanilla cue.
            super.setHealth(9);
            removeAllEffects();
            addEffect(new MobEffectInstance(MobEffects.REGENERATION, 900, 1));
            var client = Minecraft.getInstance();
            client.particleEngine.createTrackingEmitter(this, ParticleTypes.TOTEM_OF_UNDYING, 30);
            if (client.level != null) client.level.playLocalSound(this, SoundEvents.TOTEM_USE, SoundSource.PLAYERS, 1, 1);
        }

        @Override public void tick() {
            if (++ticks % 80 == 0) {
                addEffect(new MobEffectInstance(MobEffects.REGENERATION, 400, 1));
                addEffect(new MobEffectInstance(MobEffects.RESISTANCE, 6000, 0));
                addEffect(new MobEffectInstance(MobEffects.FIRE_RESISTANCE, 6000, 0));
                addEffect(new MobEffectInstance(MobEffects.ABSORPTION, 2400, 3));
                setAbsorptionAmount(16);
            }
            if (!getOffhandItem().is(Items.TOTEM_OF_UNDYING))
                getInventory().setItem(40, new ItemStack(Items.TOTEM_OF_UNDYING));
            super.tick();
        }

        @Override public boolean ignoreExplosion(Explosion explosion) { return false; }
        @Override public boolean isAttackable() { return true; }
        @Override public boolean fireImmune() { return true; }
        @Override public boolean isPushable() { return false; }
        @Override public boolean isAlive() { return true; }
        @Override protected void tickDeath() { deathTime = 0; }
        @Override public void push(Entity entity) {}
    }
}
