package me.mioclient;

import com.mojang.blaze3d.systems.RenderSystem;
import java.util.LinkedList;
import org.joml.Vector4i;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_17.class */
public class SearchHelper4_17 implements SearchHelper_4 {
    public static final LinkedList<Vector4i> linkedList = new LinkedList<>();

    public static void do1106(int i, int i2, int i3, int i4) {
        double scaleFactor = minecraftClient.getWindow().getScaleFactor();
        Vector4i vector4i = new Vector4i((int) (i * scaleFactor), (int) ((minecraftClient.getWindow().getScaledHeight() - (i2 + i4)) * scaleFactor), (int) (i3 * scaleFactor), (int) (i4 * scaleFactor));
        RenderSystem.enableScissor(vector4i.x, vector4i.y, vector4i.z, vector4i.w);
        linkedList.addLast(vector4i);
    }

    public static void do1107() {
        RenderSystem.disableScissor();
        linkedList.removeLast();
        if (linkedList.isEmpty()) {
            return;
        }
        Vector4i last = linkedList.getLast();
        RenderSystem.enableScissor(last.x, last.y, last.z, last.w);
    }
}
