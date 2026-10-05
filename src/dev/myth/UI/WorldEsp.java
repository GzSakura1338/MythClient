package dev.myth.UI;

import imgui.ImDrawList;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.client.Minecraft;
import net.minecraft.world.phys.AABB;
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

    /** Fade off keeps only the newest box, matching Mio's single-target highlight. */
    public void submit(Object owner, Style style, AABB box) {
        if (box == null) return;
        styles.put(owner, style);
        List<Entry> list = boxes.computeIfAbsent(owner, key -> new ArrayList<>());
        if (!style.fade()) { list.clear(); list.add(new Entry(box)); return; }
        for (Entry entry : list) if (entry.box.equals(box)) { entry.trigger(); return; }
        list.add(new Entry(box));
    }

    public void clear(Object owner) { boxes.remove(owner); }

    public boolean hasContent() { return boxes.values().stream().anyMatch(list -> !list.isEmpty()); }

    public void render(ImDrawList draw, float width, float height, float opacity) {
        if (boxes.isEmpty() || Minecraft.getInstance().level == null) return;
        for (var owner : boxes.entrySet()) {
            Style style = styles.get(owner.getKey());
            if (style == null) continue;
            float fadeMillis = style.fadeSeconds() * 1000;
            owner.getValue().removeIf(entry -> entry.alpha(fadeMillis) <= 0);
            for (Entry entry : owner.getValue()) {
                float alpha = (style.multiFade() ? entry.alpha(fadeMillis) : 1) * opacity;
                box(draw, entry.box, MioDraw.packed(style.fill(), alpha),
                        MioDraw.packed(style.outline(), alpha), style.lineWidth(), width, height);
            }
        }
    }

    private static void box(ImDrawList draw, AABB box, int fill, int outline, float lineWidth, float width, float height) {
        float[][] corner = new float[8][];
        for (int i = 0; i < 8; i++) {
            double x = (i == 1 || i == 2 || i == 5 || i == 6) ? box.maxX : box.minX;
            double y = i < 4 ? box.minY : box.maxY;
            double z = (i == 2 || i == 3 || i == 6 || i == 7) ? box.maxZ : box.minZ;
            // A corner behind the eye has no screen position; Mio clips in 3D, this skips the box.
            if ((corner[i] = project(x, y, z, width, height)) == null) return;
        }
        int[][] faces = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
        if ((fill >>> 24) != 0) for (int[] face : faces)
            draw.addQuadFilled(corner[face[0]][0], corner[face[0]][1], corner[face[1]][0], corner[face[1]][1],
                    corner[face[2]][0], corner[face[2]][1], corner[face[3]][0], corner[face[3]][1], fill);
        int[][] edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        if ((outline >>> 24) != 0 && lineWidth > 0) for (int[] edge : edges)
            draw.addLine(corner[edge[0]][0], corner[edge[0]][1], corner[edge[1]][0], corner[edge[1]][1], outline, lineWidth);
    }

    /** World space to ImGui space through the matrices the frame was actually rendered with. */
    public static float[] project(double x, double y, double z, float width, float height) {
        var camera = Minecraft.getInstance().gameRenderer.gameRenderState().levelRenderState.cameraRenderState;
        Vector4f position = new Vector4f((float) (x - camera.pos.x), (float) (y - camera.pos.y), (float) (z - camera.pos.z), 1);
        camera.viewRotationMatrix.transform(position);
        camera.projectionMatrix.transform(position);
        if (position.w <= 1.0E-4f) return null;
        float ndcX = position.x / position.w, ndcY = position.y / position.w;
        if (!Float.isFinite(ndcX) || !Float.isFinite(ndcY)) return null;
        return new float[]{(ndcX * .5f + .5f) * width, (1 - (ndcY * .5f + .5f)) * height};
    }
}
