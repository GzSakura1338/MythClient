package me.mioclient;

import com.mojang.authlib.GameProfile;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import me.mioclient.mixin.ducks.DuckLivingEntity;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.client.util.SkinTextures;
import net.minecraft.client.world.ClientWorld;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.PlayerModelPart;
import net.minecraft.util.Hand;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/OtherClientPlayerEntity.class */
public class OtherClientPlayerEntity extends net.minecraft.client.network.OtherClientPlayerEntity {
    public final Set<PlayerModelPart> set;
    public SkinTextures skinTextures;
    public boolean flag;
    public Vec3d vec3d;

    public OtherClientPlayerEntity(ClientWorld clientWorld) {
        super(clientWorld, new GameProfile(UUID.randomUUID(), ""));
        this.set = new HashSet();
    }

    public void tickMovement() {
    }

    /* JADX WARN: Multi-variable type inference failed */
    public void do146(PlayerEntity playerEntity) {
        this.vec3d = new Vec3d(playerEntity.getX(), playerEntity.getY(), playerEntity.getZ());
        super.copyFrom(playerEntity);
        copyPositionAndRotation((Entity) playerEntity);
        this.prevYaw = playerEntity.getYaw();
        setYaw(playerEntity.getYaw());
        this.prevPitch = playerEntity.getPitch();
        setPitch(playerEntity.getPitch());
        this.bodyYaw = playerEntity.bodyYaw;
        this.prevBodyYaw = playerEntity.bodyYaw;
        this.headYaw = playerEntity.headYaw;
        this.prevHeadYaw = playerEntity.headYaw;
        this.limbAnimator.speed = playerEntity.limbAnimator.speed;
        this.limbAnimator.prevSpeed = playerEntity.limbAnimator.speed;
        this.limbAnimator.pos = playerEntity.limbAnimator.pos;
        this.deathTime = 0;
        this.lastVelocity = new Vec3d(0.0d, 0.0d, 0.0d);
        setVelocity(0.0d, 0.0d, 0.0d);
        setPose(playerEntity.getPose());
        setSneaking(playerEntity.isSneaking());
        if (playerEntity instanceof AbstractClientPlayerEntity) {
            AbstractClientPlayerEntity abstractClientPlayerEntity = (AbstractClientPlayerEntity) playerEntity;
            this.skinTextures = abstractClientPlayerEntity.getSkinTextures();
            for (PlayerModelPart playerModelPart : PlayerModelPart.values()) {
                if (playerModelPart != PlayerModelPart.CAPE && abstractClientPlayerEntity.isPartVisible(playerModelPart)) {
                    this.set.add(playerModelPart);
                }
            }
        }
        for (int i = 0; i < getInventory().size(); i++) {
            getInventory().setStack(i, playerEntity.getInventory().getStack(i).copy());
        }
        setStackInHand(Hand.MAIN_HAND, playerEntity.getMainHandStack().copy());
        setStackInHand(Hand.OFF_HAND, playerEntity.getOffHandStack().copy());
        getInventory().selectedSlot = playerEntity.getInventory().selectedSlot;
        ((DuckLivingEntity) this).mio$setLeaningPitch(((DuckLivingEntity) playerEntity).mio$getLeaningPitch());
        ((DuckLivingEntity) this).mio$setLastLeaningPitch(((DuckLivingEntity) playerEntity).mio$getLastLeaningPitch());
    }

    public void setPosition(double d, double d2, double d3) {
        super.setPosition(d, d2, d3);
        this.lastRenderX = d;
        this.lastRenderY = d2;
        this.lastRenderZ = d3;
    }

    public Vec3d getVec3d147() {
        return this.vec3d;
    }

    public SkinTextures getSkinTextures() {
        return this.skinTextures != null ? this.skinTextures : super.getSkinTextures();
    }

    public boolean isPartVisible(PlayerModelPart playerModelPart) {
        return this.set.contains(playerModelPart);
    }

    public boolean is148() {
        return this.flag;
    }

    public void do149(boolean z) {
        this.flag = z;
    }
}
