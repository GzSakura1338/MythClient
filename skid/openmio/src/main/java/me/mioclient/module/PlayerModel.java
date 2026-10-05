package me.mioclient.module;

import me.mioclient.feature.Size;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.ingame.InventoryScreen;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.entity.LivingEntity;
import org.joml.Quaternionf;
import org.joml.Quaternionfc;
import org.joml.Vector3f;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/PlayerModel.class */
public class PlayerModel extends me.mioclient.ModuleList {
    public static boolean flag = false;

    public PlayerModel() {
        super("PlayerModel", new String[0]);
        Size size = new Size(this);
        size.do2637(this);
        do3019(size);
    }

    @Override // me.mioclient.ModuleList
    public void do364(DrawContext drawContext) {
        LivingEntity livingEntity = minecraftClient.player;
        Quaternionf rotateZ = new Quaternionf().rotateZ(Float.intBitsToFloat(1078530011));
        Quaternionf rotateX = new Quaternionf().rotateX(0.0f);
        rotateZ.mul((Quaternionfc) rotateX);
        Vector3f vector3f = new Vector3f(0.0f, ((ClientPlayerEntity) livingEntity).getHeight() / Float.intBitsToFloat(1073741824), 0.0f);
        flag = true;
        InventoryScreen.drawEntity(drawContext, Float.intBitsToFloat(1103626240), Float.intBitsToFloat(1109393408), Float.intBitsToFloat(1108082688), vector3f, rotateZ, rotateX, livingEntity);
        flag = false;
    }

    @Override // me.mioclient.ModuleList
    public float[] getFloatArray365() {
        return new float[]{Float.intBitsToFloat(1112014848), Float.intBitsToFloat(1117782016)};
    }
}
