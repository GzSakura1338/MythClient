package today.starain.module.impl.render;

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.BufferBuilder;
import com.mojang.blaze3d.vertex.DefaultVertexFormat;
import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.blaze3d.vertex.Tesselator;
import com.mojang.blaze3d.vertex.VertexFormat;
import io.github.humbleui.skija.Font;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.core.Direction;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.entity.projectile.AbstractArrow;
import net.minecraft.world.entity.projectile.Projectile;
import net.minecraft.world.entity.projectile.Snowball;
import net.minecraft.world.entity.projectile.ThrownEgg;
import net.minecraft.world.entity.projectile.ThrownEnderpearl;
import net.minecraft.world.entity.projectile.ThrownExperienceBottle;
import net.minecraft.world.entity.projectile.ThrownPotion;
import net.minecraft.world.item.BowItem;
import net.minecraft.world.item.CrossbowItem;
import net.minecraft.world.item.FishingRodItem;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.level.ClipContext;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.HitResult;
import net.minecraft.world.phys.Vec3;
import org.joml.Matrix4f;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.event.impl.RenderWorldEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.client.HUD;
import today.starain.ui.Design;
import today.starain.ui.Theme;
import today.starain.utils.render.ProjectionUtils;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Fonts;
import today.starain.utils.vector.Vector2f;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.NumberValue;

import java.awt.Color;
import java.util.ArrayList;
import java.util.List;

public class Trajectories extends ClientModule {

    private final BooleanValue syncColor = new BooleanValue("Sync Color", false, this);
    private final BooleanValue otherPlayers = new BooleanValue("Other Players", true, this);
    private final BooleanValue background = new BooleanValue("Background", true, this);
    private final BooleanValue pearls = new BooleanValue("Pearls", true, this);
    private final BooleanValue snowballs = new BooleanValue("Snowballs", false, this);
    private final BooleanValue eggs = new BooleanValue("Eggs", false, this);
    private final BooleanValue potions = new BooleanValue("Potions", false, this);
    private final BooleanValue expBottles = new BooleanValue("Exp Bottles", false, this);
    private final BooleanValue arrows = new BooleanValue("Arrows", false, this);
    private final BooleanValue fishingRod = new BooleanValue("Fishing Rod", false, this);
    private final NumberValue lineWidth = new NumberValue("Line Width", 1.0, 10.0, 3.0, 0.5, this);

    private static final Color NORMAL = new Color(206, 89, 255);
    private static final Color TOP_HIT = new Color(50, 255, 50);
    private static final Color ENTITY_HIT = new Color(255, 60, 60);

    private final List<Landing> landings = new ArrayList<>();

    public Trajectories() {
        setName("Trajectories");
        setCategory(Category.RENDER);
    }

    @Override
    public void onDisable() {
        landings.clear();
    }

    @EventTarget
    public void onRenderWorld(RenderWorldEvent event) {
        if (mc.player == null || mc.level == null) return;

        Vec3 camera = mc.gameRenderer.getMainCamera().getPosition();
        PoseStack poseStack = event.getPoseStack();
        Matrix4f matrix = poseStack.last().pose();
        float partialTick = event.getPartialTick();

        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.disableCull();
        RenderSystem.disableDepthTest();
        RenderSystem.depthMask(false);
        RenderSystem.setShader(GameRenderer::getPositionColorShader);

        drawForPlayer(mc.player, camera, matrix, partialTick);

        if (otherPlayers.get()) {
            for (Player player : mc.level.players()) {
                if (player == mc.player || !player.isAlive()) continue;
                drawForPlayer(player, camera, matrix, partialTick);
            }
        }

        for (Entity entity : mc.level.entitiesForRendering()) {
            Sim sim = simForEntity(entity);
            if (sim == null) continue;
            if (!entity.isAlive()) continue;
            Vec3 motion = entity.getDeltaMovement();
            if (motion.lengthSqr() < 0.01) continue;
            Vec3 start = new Vec3(entity.getX(), entity.getY(), entity.getZ());
            String owner = null;
            if (entity instanceof Projectile projectile && projectile.getOwner() != null) {
                owner = projectile.getOwner().getName().getString();
            }
            drawTrajectory(entity, start, motion, sim, camera, matrix, true, owner);
        }

        RenderSystem.enableDepthTest();
        RenderSystem.depthMask(true);
        RenderSystem.enableCull();
        RenderSystem.disableBlend();
    }

    private void drawForPlayer(Player player, Vec3 camera, Matrix4f matrix, float partialTick) {
        Sim sim = simFor(player, player.getMainHandItem());
        if (sim == null) sim = simFor(player, player.getOffhandItem());
        if (sim == null) return;

        float pitch = player.getViewXRot(partialTick) + sim.pitchOffset;
        float yaw = player.getViewYRot(partialTick);
        Vec3 dir = Vec3.directionFromRotation(pitch, yaw);

        double ipx = Mth.lerp(partialTick, player.xo, player.getX());
        double ipy = Mth.lerp(partialTick, player.yo, player.getY());
        double ipz = Mth.lerp(partialTick, player.zo, player.getZ());
        Vec3 pos = new Vec3(
                ipx - Math.cos(Math.toRadians(yaw)) * 0.16,
                ipy + player.getEyeHeight() - 0.1,
                ipz - Math.sin(Math.toRadians(yaw)) * 0.16
        );
        Vec3 motion = dir.scale(sim.velocity);
        drawTrajectory(player, pos, motion, sim, camera, matrix, false, null);
    }

    private void drawTrajectory(Entity shooter, Vec3 pos, Vec3 motion, Sim sim, Vec3 camera, Matrix4f matrix, boolean collectLanding, String owner) {
        List<Vec3> path = new ArrayList<>();
        path.add(pos);
        Direction hitFace = null;
        boolean hitEntity = false;

        for (int i = 0; i < 600 && pos.y > mc.level.getMinBuildHeight() - 8; i++) {
            Vec3 next = pos.add(motion);

            Entity entityHit = clipEntity(shooter, pos, next, sim.size);
            if (entityHit != null) {
                Vec3 ep = entityHit.getBoundingBox().inflate(sim.size).clip(pos, next).orElse(next);
                path.add(ep);
                pos = ep;
                hitEntity = true;
                break;
            }

            BlockHitResult block = mc.level.clip(new ClipContext(pos, next,
                    ClipContext.Block.COLLIDER, ClipContext.Fluid.NONE, shooter));
            if (block.getType() != HitResult.Type.MISS) {
                path.add(block.getLocation());
                pos = block.getLocation();
                hitFace = block.getDirection();
                break;
            }

            path.add(next);
            pos = next;
            boolean inWater = !mc.level.getFluidState(net.minecraft.core.BlockPos.containing(pos)).isEmpty();
            motion = motion.scale(inWater ? 0.6 : sim.drag);
            motion = new Vec3(motion.x, motion.y - sim.gravity, motion.z);
        }

        if (path.size() < 2) return;

        boolean landed = hitEntity || hitFace != null;
        Color base = syncColor.get() ? Theme.accent(0) : NORMAL;
        Color markColor = hitEntity ? ENTITY_HIT : (hitFace == Direction.UP ? TOP_HIT : base);

        Tesselator tesselator = Tesselator.getInstance();
        BufferBuilder buffer = tesselator.getBuilder();

        List<Vec3> rel = new ArrayList<>(path.size());
        for (Vec3 p : path) rel.add(p.subtract(camera));

        float pixels = lineWidth.get().floatValue();
        RenderSystem.setShader(GameRenderer::getPositionColorShader);
        buffer.begin(VertexFormat.Mode.TRIANGLE_STRIP, DefaultVertexFormat.POSITION_COLOR);
        for (int i = 0; i < rel.size(); i++) {
            Vec3 cur = rel.get(i);
            Vec3 prev = rel.get(Math.max(i - 1, 0));
            Vec3 nxt = rel.get(Math.min(i + 1, rel.size() - 1));
            Vec3 tangent = nxt.subtract(prev);
            if (tangent.lengthSqr() < 1.0E-7) tangent = new Vec3(0, 1, 0);
            tangent = tangent.normalize();
            double dist = cur.length();
            Vec3 view = dist < 1.0E-4 ? new Vec3(0, 0, 1) : cur.scale(1.0 / dist);
            Vec3 side = tangent.cross(view);
            if (side.lengthSqr() < 1.0E-7) side = tangent.cross(new Vec3(0, 1, 0));
            double half = pixels * 0.0013 * Math.max(dist, 0.5);
            side = side.normalize().scale(half);
            Color sc = syncColor.get() ? Theme.accent(-i) : base;
            float cr = sc.getRed() / 255f, cg = sc.getGreen() / 255f, cb = sc.getBlue() / 255f;
            Vec3 a1 = cur.subtract(side);
            Vec3 a2 = cur.add(side);
            buffer.vertex(matrix, (float) a1.x, (float) a1.y, (float) a1.z).color(cr, cg, cb, 1.0f).endVertex();
            buffer.vertex(matrix, (float) a2.x, (float) a2.y, (float) a2.z).color(cr, cg, cb, 1.0f).endVertex();
        }
        tesselator.end();

        if (landed) {
            Vec3 land = path.get(path.size() - 1);
            drawMarker(matrix, buffer, tesselator,
                    (float) (land.x - camera.x), (float) (land.y - camera.y), (float) (land.z - camera.z),
                    hitEntity ? null : hitFace, markColor);

            if (collectLanding) {
                landings.add(new Landing(land, owner));
            }
        }
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {
        if (landings.isEmpty() || mc.player == null) return;

        Font font = Fonts.getNormal(7.0f);
        float pad = 3.0f;
        float lineH = 9.0f;

        for (Landing landing : landings) {
            Vec3 land = landing.pos;
            Vector2f screen = ProjectionUtils.project(land.x, land.y, land.z, event.getPartialTick());
            if (screen.x >= Float.MAX_VALUE) continue;

            boolean hasOwner = landing.owner != null && !landing.owner.isEmpty();
            String coord = Mth.floor(land.x) + ", " + Mth.floor(land.y) + ", " + Mth.floor(land.z);
            double dist = mc.player.position().distanceTo(land);
            String distText = String.format("%.1fm", dist);

            int lines = hasOwner ? 3 : 2;
            float maxW = Math.max(Skia.getTextWidth(coord, font), Skia.getTextWidth(distText, font));
            if (hasOwner) maxW = Math.max(maxW, Skia.getTextWidth(landing.owner, font));
            float w = maxW + pad * 2;
            float h = lineH * lines + pad * 2;
            float x = screen.x - w / 2;
            float y = screen.y - h / 2;

            if (background.get()) {
                if (HUD.isShadowEnabled()) {
                    Skia.drawShadow(x, y, w, h, 4.0f);
                }
                if (HUD.isBlurEnabled()) {
                    Skia.drawGuiRoundedBlur(x, y, w, h, 4.0f);
                }
                Skia.drawRoundedRect(x, y, w, h, 4.0f, Design.BACKGROUND_COLOR);
            }
            float ty = y + pad + lineH / 2f;
            if (hasOwner) {
                Skia.drawFullCenteredTextPlain(landing.owner, screen.x, ty, Design.TEXT_COLOR, font);
                ty += lineH;
            }
            Skia.drawFullCenteredTextPlain(coord, screen.x, ty, Design.TEXT_COLOR, font);
            Skia.drawFullCenteredTextPlain(distText, screen.x, ty + lineH, new Color(180, 180, 180), font);
        }
        landings.clear();
    }

    private Entity clipEntity(Entity shooter, Vec3 from, Vec3 to, double size) {
        AABB box = new AABB(from, to).inflate(size + 1.0);
        Entity best = null;
        double bestDist = Double.MAX_VALUE;
        for (Entity e : mc.level.getEntities(shooter, box, en -> en != shooter && en.isPickable() && en.isAlive())) {
            var hit = e.getBoundingBox().inflate(size).clip(from, to);
            if (hit.isPresent()) {
                double d = from.distanceToSqr(hit.get());
                if (d < bestDist) {
                    bestDist = d;
                    best = e;
                }
            }
        }
        return best;
    }

    private void drawMarker(Matrix4f matrix, BufferBuilder buffer, Tesselator tesselator,
                            float x, float y, float z, Direction face, Color color) {
        float r = color.getRed() / 255f, g = color.getGreen() / 255f, b = color.getBlue() / 255f;
        float radius = 0.5f;

        int axis = face == null ? 1 : switch (face.getAxis()) {
            case X -> 0;
            case Y -> 1;
            case Z -> 2;
        };

        buffer.begin(VertexFormat.Mode.DEBUG_LINE_STRIP, DefaultVertexFormat.POSITION_COLOR);
        int segments = 40;
        for (int i = 0; i <= segments; i++) {
            double a = Math.PI * 2 * i / segments;
            float u = (float) (Math.cos(a) * radius);
            float v = (float) (Math.sin(a) * radius);
            float px = x, py = y, pz = z;
            if (axis == 1) { px += u; pz += v; }
            else if (axis == 0) { py += u; pz += v; }
            else { px += u; py += v; }
            buffer.vertex(matrix, px, py, pz).color(r, g, b, 1.0f).endVertex();
        }
        tesselator.end();

        buffer.begin(VertexFormat.Mode.DEBUG_LINES, DefaultVertexFormat.POSITION_COLOR);
        float[][] cross;
        if (axis == 1) {
            cross = new float[][]{{x - radius, y, z, x + radius, y, z}, {x, y, z - radius, x, y, z + radius}};
        } else if (axis == 0) {
            cross = new float[][]{{x, y - radius, z, x, y + radius, z}, {x, y, z - radius, x, y, z + radius}};
        } else {
            cross = new float[][]{{x - radius, y, z, x + radius, y, z}, {x, y - radius, z, x, y + radius, z}};
        }
        for (float[] c : cross) {
            buffer.vertex(matrix, c[0], c[1], c[2]).color(r, g, b, 1.0f).endVertex();
            buffer.vertex(matrix, c[3], c[4], c[5]).color(r, g, b, 1.0f).endVertex();
        }
        tesselator.end();
    }

    private Sim simForEntity(Entity entity) {
        if (entity instanceof ThrownEnderpearl) return pearls.get() ? new Sim(0f, 0.03f, 0.99, 0f, 0.25) : null;
        if (entity instanceof Snowball) return snowballs.get() ? new Sim(0f, 0.03f, 0.99, 0f, 0.25) : null;
        if (entity instanceof ThrownEgg) return eggs.get() ? new Sim(0f, 0.03f, 0.99, 0f, 0.25) : null;
        if (entity instanceof ThrownPotion) return potions.get() ? new Sim(0f, 0.05f, 0.99, 0f, 0.25) : null;
        if (entity instanceof ThrownExperienceBottle) return expBottles.get() ? new Sim(0f, 0.07f, 0.99, 0f, 0.25) : null;
        if (entity instanceof AbstractArrow) return arrows.get() ? new Sim(0f, 0.05f, 0.99, 0f, 0.3) : null;
        return null;
    }

    private Sim simFor(Player player, ItemStack stack) {
        if (stack.isEmpty()) return null;
        Item item = stack.getItem();

        if (item == Items.ENDER_PEARL) return pearls.get() ? new Sim(1.5f, 0.03f, 0.99, 0.0f, 0.25) : null;
        if (item == Items.SNOWBALL) return snowballs.get() ? new Sim(1.5f, 0.03f, 0.99, 0.0f, 0.25) : null;
        if (item == Items.EGG) return eggs.get() ? new Sim(1.5f, 0.03f, 0.99, 0.0f, 0.25) : null;
        if (item == Items.SPLASH_POTION || item == Items.LINGERING_POTION) {
            return potions.get() ? new Sim(0.5f, 0.05f, 0.99, -20.0f, 0.25) : null;
        }
        if (item == Items.EXPERIENCE_BOTTLE) {
            return expBottles.get() ? new Sim(0.7f, 0.07f, 0.99, -20.0f, 0.25) : null;
        }
        if (item instanceof BowItem) {
            if (!arrows.get() || !player.isUsingItem() || player.getUseItem() != stack) return null;
            float power = BowItem.getPowerForTime(player.getTicksUsingItem());
            if (power < 0.1f) return null;
            return new Sim(power * 3.0f, 0.05f, 0.99, 0.0f, 0.3);
        }
        if (item instanceof CrossbowItem && CrossbowItem.isCharged(stack)) {
            return arrows.get() ? new Sim(3.15f, 0.05f, 0.99, 0.0f, 0.3) : null;
        }
        if (item instanceof FishingRodItem) {
            return fishingRod.get() ? new Sim(1.5f, 0.04f, 0.92, 0.0f, 0.25) : null;
        }
        return null;
    }

    private record Landing(Vec3 pos, String owner) {
    }

    private static final class Sim {
        final float velocity;
        final float gravity;
        final double drag;
        final float pitchOffset;
        final double size;

        Sim(float velocity, float gravity, double drag, float pitchOffset, double size) {
            this.velocity = velocity;
            this.gravity = gravity;
            this.drag = drag;
            this.pitchOffset = pitchOffset;
            this.size = size;
        }
    }
}
