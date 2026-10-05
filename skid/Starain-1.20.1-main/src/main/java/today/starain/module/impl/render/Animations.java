package today.starain.module.impl.render;

import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.math.Axis;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.item.SwordItem;
import net.minecraft.world.item.UseAnim;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.combat.KillAura;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;


public class Animations extends ClientModule {
    /** 供其它模块（如 NoSlow）读取当前是否在做格挡动画。 */
    public static boolean blocking = false;

    private final ModeValue mode = new ModeValue("Block Mods", "1.7", new String[]{"None", "1.7", "Push"}, this);
    private final BooleanValue auraAutoBlock = new BooleanValue("Aura Auto Block", true, this, () -> !mode.isMode("None"));
    private final BooleanValue onlyAura = new BooleanValue("Only Aura", false, this, () -> !mode.isMode("None"));
    private final NumberValue blockingX = new NumberValue("Blocking-X", -2.0, 2.0, 0.5, 0.05, this, () -> !mode.isMode("None"));
    private final NumberValue blockingY = new NumberValue("Blocking-Y", -2.0, 2.0, -0.5, 0.05, this, () -> !mode.isMode("None"));

    public Animations() {
        setName("Animations");
        setCategory(Category.RENDER);
    }

    public boolean shouldBlock() {
        if (!isEnabled() || mode.isMode("None")) {
            return false;
        }
        LocalPlayer player = mc.player;
        if (player == null) {
            return false;
        }
        if (!(player.getMainHandItem().getItem() instanceof SwordItem)) {
            return false;
        }

        boolean offhandUsing = false;
        if (player.isUsingItem() && player.getUsedItemHand() == InteractionHand.OFF_HAND
                && player.getOffhandItem().getUseAnimation() != UseAnim.BLOCK) {
            offhandUsing = true;
        }

        boolean auraBlocking = auraAutoBlock.get() && getAuraTarget() != null;

        boolean result;
        if (onlyAura.get()) {
            result = auraBlocking;
        } else if (auraBlocking) {
            result = true;
        } else if (offhandUsing) {
            result = false;
        } else {
            result = mc.options.keyUse.isDown();
        }

        blocking = result;
        return result;
    }


    public void applyBlockPose(PoseStack poseStack, int i, float swingProgress) {
        float bx = blockingX.get().floatValue();
        float by = blockingY.get().floatValue();
        final float deg = (float) Math.PI / 180.0F;

        if (mode.isMode("1.7")) {
            poseStack.translate((float) i * bx, by, -0.72F);
            float f17 = Mth.sin(swingProgress * swingProgress * (float) Math.PI);
            float f22 = Mth.sin(Mth.sqrt(swingProgress) * (float) Math.PI);
            poseStack.mulPose(Axis.YP.rotation((float) i * (45.0F + f17 * -20.0F) * deg));
            poseStack.mulPose(Axis.ZP.rotation((float) i * f22 * -10.0F * deg));
            poseStack.mulPose(Axis.XP.rotation(f22 * -80.0F * deg));
            poseStack.mulPose(Axis.YP.rotation((float) i * -45.0F * deg));
            poseStack.scale(0.9F, 0.9F, 0.9F);
            poseStack.translate(-0.2F, 0.126F, 0.2F);
            poseStack.mulPose(Axis.XP.rotation(-102.25F * deg));
            poseStack.mulPose(Axis.YP.rotation((float) i * 15.0F * deg));
            poseStack.mulPose(Axis.ZP.rotation((float) i * 80.0F * deg));
        } else if (mode.isMode("Push")) {
            poseStack.translate((float) i * bx, by, -0.82F);
            poseStack.translate((float) i * -0.1414214F, 0.08F, 0.1414214F);
            poseStack.mulPose(Axis.XP.rotation(-102.25F * deg));
            poseStack.mulPose(Axis.YP.rotation((float) i * 13.365F * deg));
            poseStack.mulPose(Axis.ZP.rotation((float) i * 78.05F * deg));
            float f15 = Mth.sin(swingProgress * swingProgress * (float) Math.PI);
            float f3 = Mth.sin(Mth.sqrt(swingProgress) * (float) Math.PI);
            poseStack.mulPose(Axis.XP.rotation(f15 * -10.0F * deg));
            poseStack.mulPose(Axis.YP.rotation(f15 * -10.0F * deg));
            poseStack.mulPose(Axis.ZP.rotation(f15 * -10.0F * deg));
            poseStack.mulPose(Axis.XP.rotation(f3 * -10.0F * deg));
            poseStack.mulPose(Axis.YP.rotation(f3 * -10.0F * deg));
            poseStack.mulPose(Axis.ZP.rotation(f3 * -10.0F * deg));
        }
    }

    private LivingEntity getAuraTarget() {
        KillAura aura = Starain.getInstance().getModuleManager().getModule(KillAura.class);
        if (aura == null || !aura.isEnabled() || KillAura.targets == null || KillAura.targets.isEmpty()) {
            return null;
        }
        int index = Math.min(Math.max(KillAura.targetNumber, 0), KillAura.targets.size() - 1);
        Entity target = KillAura.targets.get(index);
        return target instanceof LivingEntity living ? living : null;
    }
}
