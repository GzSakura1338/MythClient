package me.mioclient;

import me.mioclient.mixin.ducks.DuckAnimalModel;
import me.mioclient.mixin.ducks.DuckLivingEntityRenderer;
import me.mioclient.module.render.Animations;
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.model.AnimalModel;
import net.minecraft.client.render.entity.model.BipedEntityModel;
import net.minecraft.client.render.entity.model.CompositeEntityModel;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.client.render.entity.model.PlayerEntityModel;
import net.minecraft.client.render.entity.model.SinglePartEntityModel;
import net.minecraft.client.render.entity.model.VillagerResemblingModel;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.EntityPose;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Direction;
import net.minecraft.util.math.MathHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_2.class */
public class SearchHelper4_2 implements SearchHelper_4, Helper_6<LivingEntity> {
    public static Animations animations = (Animations) BaritoneHelper_3.baritoneHelper_4.getModule117(Animations.class);
    public boolean flag;
    public boolean flag2 = false;
    public boolean flag3 = false;

    @Override // me.mioclient.Helper_6
    /* renamed from: do1539, reason: merged with bridge method [inline-methods] */
    public void do721(LivingEntity livingEntity, float f, MatrixStack matrixStack) {
        Direction sleepingDirection;
        this.flag = minecraftClient.player == livingEntity;
        this.flag2 = livingEntity instanceof OtherClientPlayerEntity;
        if (this.flag2) {
            f = 1.0f;
        }
        if (livingEntity instanceof OtherClientPlayerEntity) {
            this.flag3 = ((OtherClientPlayerEntity) livingEntity).is148();
        } else {
            this.flag3 = false;
        }
        LivingEntityRenderer renderer = (LivingEntityRenderer) minecraftClient.getEntityRenderDispatcher().getRenderer(livingEntity);
        if (renderer instanceof LivingEntityRenderer) {
            DuckLivingEntityRenderer duckLivingEntityRenderer = (DuckLivingEntityRenderer) renderer;
            EntityModel model = (EntityModel)(renderer.getModel());
            matrixStack.push();
            ((EntityModel) model).handSwingProgress = livingEntity.getHandSwingProgress(f);
            ((EntityModel) model).riding = livingEntity.hasVehicle();
            ((EntityModel) model).child = livingEntity.isBaby();
            float lerpAngleDegrees = MathHelper.lerpAngleDegrees(f, livingEntity.prevBodyYaw, livingEntity.bodyYaw);
            float lerpAngleDegrees2 = MathHelper.lerpAngleDegrees(f, livingEntity.prevHeadYaw, livingEntity.headYaw);
            if (this.flag) {
                lerpAngleDegrees = MathHelper.lerpAngleDegrees(f, rotations().get2500(), rotations().get2497());
                lerpAngleDegrees2 = MathHelper.lerpAngleDegrees(f, rotations().get2501(), rotations().get2502());
            }
            float f2 = lerpAngleDegrees2 - lerpAngleDegrees;
            if (livingEntity.hasVehicle()) {
                LivingEntity vehicle = (livingEntity.getVehicle()) instanceof LivingEntity ? (LivingEntity) (livingEntity.getVehicle()) : null;
                if (vehicle instanceof LivingEntity) {
                    LivingEntity livingEntity2 = vehicle;
                    float clamp = MathHelper.clamp(MathHelper.wrapDegrees(lerpAngleDegrees2 - (this.flag ? MathHelper.lerpAngleDegrees(f, rotations().get2500(), rotations().get2497()) : MathHelper.lerpAngleDegrees(f, livingEntity2.prevBodyYaw, livingEntity2.bodyYaw))), -85.0f, 85.0f);
                    lerpAngleDegrees = lerpAngleDegrees2 - clamp;
                    if (clamp * clamp > 2500.0f) {
                        lerpAngleDegrees += clamp * 0.2f;
                    }
                    f2 = lerpAngleDegrees2 - lerpAngleDegrees;
                }
            }
            float lerp = MathHelper.lerp(f, livingEntity.prevPitch, livingEntity.getPitch());
            if (this.flag) {
                lerp = MathHelper.lerp(f, rotations().get2499(), rotations().get2496());
            }
            if (LivingEntityRenderer.shouldFlipUpsideDown(livingEntity)) {
                lerp *= -1.0f;
                f2 *= -1.0f;
            }
            if (livingEntity.isInPose(EntityPose.SLEEPING) && (sleepingDirection = livingEntity.getSleepingDirection()) != null) {
                float eyeHeight = livingEntity.getEyeHeight(EntityPose.STANDING) - 0.1f;
                matrixStack.translate((-sleepingDirection.getOffsetX()) * eyeHeight, 0.0f, (-sleepingDirection.getOffsetZ()) * eyeHeight);
            }
            if (model instanceof BipedEntityModel) {
                BipedEntityModel bipedEntityModel = (BipedEntityModel) model;
                if ((livingEntity instanceof PlayerEntity) && ((PlayerEntity) livingEntity) != minecraftClient.player && animations.is1001()) {
                    bipedEntityModel.sneaking = true;
                } else {
                    bipedEntityModel.sneaking = livingEntity.isSneaking();
                }
            }
            float mio$getAnimationProgress = duckLivingEntityRenderer.mio$getAnimationProgress(livingEntity, f);
            duckLivingEntityRenderer.mio$setupTransforms(livingEntity, matrixStack, mio$getAnimationProgress, lerpAngleDegrees, f, livingEntity.getScale());
            matrixStack.scale(-1.0f, -1.0f, 1.0f);
            duckLivingEntityRenderer.mio$scale(livingEntity, matrixStack, f);
            matrixStack.translate(0.0f, -1.501f, 0.0f);
            float f3 = 0.0f;
            float f4 = 0.0f;
            if (!livingEntity.hasVehicle() && livingEntity.isAlive()) {
                if (livingEntity instanceof OtherClientPlayerEntity) {
                    f3 = Math.min(livingEntity.limbAnimator.getSpeed(), 1.0f);
                    f4 = livingEntity.limbAnimator.getPos();
                } else {
                    f3 = Math.min(livingEntity.limbAnimator.getSpeed(f), 1.0f);
                    f4 = livingEntity.limbAnimator.getPos(f);
                }
                if (livingEntity.isBaby()) {
                    f4 *= 3.0f;
                }
            }
            model.animateModel(livingEntity, f4, f3, f);
            model.setAngles(livingEntity, f4, f3, mio$getAnimationProgress, f2, lerp);
            MatrixStackData matrixStackData = new MatrixStackData(matrixStack, MatrixStackDataMode.BOTH);
            if (model instanceof VillagerResemblingModel) {
                ((VillagerResemblingModel) model).hatRim.visible = false;
            }
            if (model instanceof AnimalModel) {
                do1541(matrixStackData, (AnimalModel) model);
            } else if (model instanceof SinglePartEntityModel) {
                ChamsHelper_2.do617(matrixStackData, ((SinglePartEntityModel) model).getPart());
            } else if (model instanceof CompositeEntityModel) {
                ((CompositeEntityModel<?>) model).getParts().forEach(modelPart -> {
                    ChamsHelper_2.do617(matrixStackData, modelPart);
                });
            } else {
                do1540(matrixStackData, model);
            }
            matrixStack.pop();
        }
    }

    public void do1540(MatrixStackData matrixStackData, EntityModel<?> entityModel) {
    }

    public void do1541(MatrixStackData matrixStackData, AnimalModel<?> animalModel) {
        DuckAnimalModel duckAnimalModel = (DuckAnimalModel) animalModel;
        if (animalModel instanceof BipedEntityModel) {
            do1542((BipedEntityModel) animalModel);
        }
        if (!animalModel.child) {
            duckAnimalModel.mio$getHeadParts().forEach(modelPart -> {
                ChamsHelper_2.do617(matrixStackData, modelPart);
            });
            duckAnimalModel.mio$getBodyParts().forEach(modelPart2 -> {
                ChamsHelper_2.do617(matrixStackData, modelPart2);
            });
            return;
        }
        matrixStackData.getMatrixStack1013().push();
        if (duckAnimalModel.mio$isHeadScaled()) {
            float mio$getInvertedChildHeadScale = 1.5f / duckAnimalModel.mio$getInvertedChildHeadScale();
            matrixStackData.getMatrixStack1013().scale(mio$getInvertedChildHeadScale, mio$getInvertedChildHeadScale, mio$getInvertedChildHeadScale);
        }
        matrixStackData.getMatrixStack1013().translate(0.0f, duckAnimalModel.mio$getChildHeadYOffset() / 16.0f, duckAnimalModel.mio$getChildHeadZOffset() / 16.0f);
        duckAnimalModel.mio$getHeadParts().forEach(modelPart3 -> {
            ChamsHelper_2.do617(matrixStackData, modelPart3);
        });
        matrixStackData.getMatrixStack1013().pop();
        matrixStackData.getMatrixStack1013().push();
        float mio$getInvertedChildBodyScale = 1.0f / duckAnimalModel.mio$getInvertedChildBodyScale();
        matrixStackData.getMatrixStack1013().scale(mio$getInvertedChildBodyScale, mio$getInvertedChildBodyScale, mio$getInvertedChildBodyScale);
        matrixStackData.getMatrixStack1013().translate(0.0f, duckAnimalModel.mio$getChildBodyYOffset() / 16.0f, 0.0f);
        duckAnimalModel.mio$getBodyParts().forEach(modelPart4 -> {
            ChamsHelper_2.do617(matrixStackData, modelPart4);
        });
        matrixStackData.getMatrixStack1013().pop();
    }

    public void do1542(BipedEntityModel<?> bipedEntityModel) {
        boolean z = ChamsHelper_2.is614() && !this.flag2;
        bipedEntityModel.hat.visible = z;
        if (bipedEntityModel instanceof PlayerEntityModel) {
            PlayerEntityModel playerEntityModel = (PlayerEntityModel) bipedEntityModel;
            playerEntityModel.leftPants.visible = z;
            playerEntityModel.rightPants.visible = z;
            playerEntityModel.leftSleeve.visible = z;
            playerEntityModel.rightSleeve.visible = z;
            playerEntityModel.jacket.visible = z;
            if (this.flag3) {
                playerEntityModel.rightArmPose = BipedEntityModel.ArmPose.EMPTY;
                playerEntityModel.leftArmPose = BipedEntityModel.ArmPose.EMPTY;
            }
        }
    }

    public <T> T getObject1543(T t, T t2) {
        return this.flag ? t2 : t;
    }

    public static SearchHelper4_8 rotations() {
        return BaritoneHelper_3.searchHelper4_8;
    }
}
