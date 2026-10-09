package dev.myth.UI;

import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.math.Axis;
import dev.myth.mixin.access.GameRendererAccess;
import imgui.ImDrawList;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.state.OptionsRenderState;
import net.minecraft.client.renderer.state.level.CameraEntityRenderState;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import net.minecraft.util.Mth;
import net.minecraft.world.effect.MobEffects;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;
import org.joml.Matrix4f;
import org.joml.Vector3f;
import org.joml.Vector4f;

/** Mio AntiPhaseSearchHelper4: fading world boxes, projected and drawn by the ImGui backend. */
public final class WorldEsp {
    /** Mio holds a box at full alpha for 150 ms after its last trigger, then fades it. */
    private static final long HOLD = 150;

    public record Style(int fill, int outline, float lineWidth, float fadeSeconds, boolean fade, boolean multiFade) {}

    private static final class Entry {
        final AABB box;
        long shownAt;
        Entry(AABB box) { this.box = box; trigger(); }
        void trigger() { shownAt = System.currentTimeMillis(); }
        float alpha(float fadeMillis) {
            long over = Math.max(System.currentTimeMillis() - shownAt - HOLD, 0);
            if (over == 0) return 1;
            return fadeMillis <= 0 ? 0 : Math.clamp(1 - over / fadeMillis, 0, 1);
        }
    }

    private final Map<Object, Style> styles = new LinkedHashMap<>();
    private final Map<Object, List<Entry>> boxes = new LinkedHashMap<>();

    /** Mio do2133: MultiFade keeps every box alive, otherwise only the newest one is shown. */
    public void submit(Object owner, Style style, AABB box) {
        if (box == null) return;
        styles.put(owner, style);
        List<Entry> list = boxes.computeIfAbsent(owner, key -> new ArrayList<>());
        if (!style.multiFade()) { list.clear(); list.add(new Entry(box)); return; }
        for (Entry entry : list) if (entry.box.equals(box)) { entry.trigger(); return; }
        list.add(new Entry(box));
    }

    public void clear(Object owner) { boxes.remove(owner); }

    public boolean hasContent() { return boxes.values().stream().anyMatch(list -> !list.isEmpty()); }

    public void render(ImDrawList draw, float width, float height, float opacity) {
        if (boxes.isEmpty() || Minecraft.getInstance().level == null) return;
        Projection projection = Projection.capture();
        if (projection == null) return;
        for (var owner : boxes.entrySet()) {
            Style style = styles.get(owner.getKey());
            if (style == null) continue;
            // Mio get100: Fade off leaves no fade window, so a box simply drops after the hold.
            float fadeMillis = style.fade() ? style.fadeSeconds() * 1000 : 0;
            owner.getValue().removeIf(entry -> entry.alpha(fadeMillis) <= 0);
            for (Entry entry : owner.getValue()) {
                float alpha = (style.fade() ? entry.alpha(fadeMillis) : 1) * opacity;
                box(draw, projection, entry.box, MioDraw.packed(style.fill(), alpha),
                        MioDraw.packed(style.outline(), alpha), style.lineWidth(), width, height);
            }
        }
    }

    private static void box(ImDrawList draw, Projection projection, AABB box, int fill, int outline,
                            float lineWidth, float width, float height) {
        float[][] corner = new float[8][];
        for (int i = 0; i < 8; i++) {
            double x = (i == 1 || i == 2 || i == 5 || i == 6) ? box.maxX : box.minX;
            double y = i < 4 ? box.minY : box.maxY;
            double z = (i == 2 || i == 3 || i == 6 || i == 7) ? box.maxZ : box.minZ;
            // A corner behind the eye has no screen position; Mio clips in 3D, this skips the box.
            if ((corner[i] = projection.project(x, y, z, width, height)) == null) return;
        }
        int[][] faces = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
        if ((fill >>> 24) != 0) for (int[] face : faces)
            draw.addQuadFilled(corner[face[0]][0], corner[face[0]][1], corner[face[1]][0], corner[face[1]][1],
                    corner[face[2]][0], corner[face[2]][1], corner[face[3]][0], corner[face[3]][1], fill);
        int[][] edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        if ((outline >>> 24) != 0 && lineWidth > 0) for (int[] edge : edges)
            draw.addLine(corner[edge[0]][0], corner[edge[0]][1], corner[edge[1]][0], corner[edge[1]][1], outline, lineWidth);
    }

    /**
     * World space to ImGui space through the exact transform the level was drawn with this frame.
     *
     * <p>{@code cameraRenderState.projectionMatrix} alone is not that transform:
     * {@code GameRenderer.renderLevel} multiplies the view bob and the portal / nausea skew into it
     * before handing it to {@code RenderSystem}. Leaving them out is what made boxes drift away from
     * their block while walking — the terrain bobbed and the overlay did not.
     */
    public record Projection(Matrix4f matrix, Vec3 cameraPos) {
        public static Projection capture() {
            Minecraft mc = Minecraft.getInstance();
            var state = mc.gameRenderer.gameRenderState();
            CameraRenderState camera = state.levelRenderState.cameraRenderState;
            OptionsRenderState options = state.optionsRenderState;

            Matrix4f matrix = new Matrix4f(camera.projectionMatrix);
            PoseStack bob = new PoseStack();
            bobHurt(camera, options, bob);
            if (options.bobView) bobView(camera, bob);
            matrix.mul(bob.last().pose());
            spin(mc, options, matrix);
            return new Projection(matrix.mul(camera.viewRotationMatrix), camera.pos);
        }

        public float[] project(double x, double y, double z, float width, float height) {
            Vector4f position = new Vector4f((float) (x - cameraPos.x), (float) (y - cameraPos.y),
                    (float) (z - cameraPos.z), 1);
            matrix.transform(position);
            if (position.w <= 1.0E-4f) return null;
            float ndcX = position.x / position.w, ndcY = position.y / position.w;
            if (!Float.isFinite(ndcX) || !Float.isFinite(ndcY)) return null;
            return new float[]{(ndcX * .5f + .5f) * width, (1 - (ndcY * .5f + .5f)) * height};
        }

        /** GameRenderer bobHurt: the damage tilt and the death roll. */
        private static void bobHurt(CameraRenderState camera, OptionsRenderState options, PoseStack poseStack) {
            CameraEntityRenderState entity = camera.entityRenderState;
            if (!entity.isLiving) return;
            float hurt = entity.hurtTime;
            if (entity.isDeadOrDying) {
                float duration = Math.min(entity.deathTime, 20);
                poseStack.mulPose(Axis.ZP.rotationDegrees(40 - 8000 / (duration + 200)));
            }
            if (hurt < 0) return;
            hurt /= entity.hurtDuration;
            // Mth.sin table-looks-up through the 0/0 NaN vanilla also walks into here, landing on 0.
            hurt = Mth.sin(hurt * hurt * hurt * hurt * (float) Math.PI);
            float dir = entity.hurtDir;
            poseStack.mulPose(Axis.YP.rotationDegrees(-dir));
            poseStack.mulPose(Axis.ZP.rotationDegrees((float) (-hurt * 14.0 * options.damageTiltStrength)));
            poseStack.mulPose(Axis.YP.rotationDegrees(dir));
        }

        /** GameRenderer bobView: the walk bob, the part that was making the boxes drift. */
        private static void bobView(CameraRenderState camera, PoseStack poseStack) {
            CameraEntityRenderState entity = camera.entityRenderState;
            if (!entity.isPlayer) return;
            float walk = entity.backwardsInterpolatedWalkDistance;
            float bob = entity.bob;
            poseStack.translate(Mth.sin(walk * (float) Math.PI) * bob * .5f,
                    -Math.abs(Mth.cos(walk * (float) Math.PI) * bob), 0);
            poseStack.mulPose(Axis.ZP.rotationDegrees(Mth.sin(walk * (float) Math.PI) * bob * 3));
            poseStack.mulPose(Axis.XP.rotationDegrees(Math.abs(Mth.cos(walk * (float) Math.PI - .2f) * bob) * 5));
        }

        /** GameRenderer renderLevel: the portal and nausea screen skew. */
        private static void spin(Minecraft mc, OptionsRenderState options, Matrix4f matrix) {
            var player = mc.player;
            if (player == null) return;
            float partialTick = mc.getDeltaTracker().getGameTimeDeltaPartialTick(false);
            float portal = Mth.lerp(partialTick, player.oPortalEffectIntensity, player.portalEffectIntensity);
            float nausea = player.getEffectBlendFactor(MobEffects.NAUSEA, partialTick);
            float intensity = Math.max(portal, nausea) * (options.screenEffectScale * options.screenEffectScale);
            if (intensity <= 0) return;
            float skew = 5 / (intensity * intensity + 5) - intensity * .04f;
            skew *= skew;
            Vector3f axis = new Vector3f(0, Mth.SQRT_OF_TWO / 2, Mth.SQRT_OF_TWO / 2);
            GameRendererAccess renderer = (GameRendererAccess) mc.gameRenderer;
            float angle = (renderer.myth$spinningEffectTime()
                    + partialTick * renderer.myth$spinningEffectSpeed()) * (float) (Math.PI / 180.0);
            matrix.rotate(angle, axis);
            matrix.scale(1 / skew, 1, 1);
            matrix.rotate(-angle, axis);
        }
    }
}
