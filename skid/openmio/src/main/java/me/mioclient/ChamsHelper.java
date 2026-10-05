package me.mioclient;

import java.awt.Color;
import me.mioclient.module.client.Colors;
import me.mioclient.module.render.Chams;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChamsHelper.class */
public interface ChamsHelper extends SearchHelper_4 {
    public static final Colors colors = (Colors) BaritoneHelper_3.baritoneHelper_4.getModule117(Colors.class);
    public static final Color color = new Color(0, 0, 0, 0);

    default void do591(Chams chams, Entity entity, MatrixStack matrixStack) {
        Color[] colorArray593 = getColorArray593(chams);
        if (chams.fade.getValue().booleanValue()) {
            float f = chams.get2044(entity);
            colorArray593 = new Color[]{MixinMessageIndicatorHelper_2.getColor816(colorArray593[0], (int) (f * colorArray593[0].getAlpha())), MixinMessageIndicatorHelper_2.getColor816(colorArray593[1], (int) (f * colorArray593[1].getAlpha()))};
        }
        ChamsHelper_2.do612(colorArray593[0], colorArray593[1]);
        if (chams.friends.getValue().booleanValue() && (entity instanceof PlayerEntity)) {
            PlayerEntity playerEntity = (PlayerEntity) entity;
            if (BaritoneHelper_3.searchHelper4_14.is520(playerEntity)) {
                do592(chams, colorArray593, colors.friendColor.getValue());
            }
            if (BaritoneHelper_3.searchHelper4_14.is522(playerEntity)) {
                do592(chams, colorArray593, colors.enemyColor.getValue());
            }
        }
        ChamsHelper_2.do613(chams.extraLayer.getValue().booleanValue());
        if (entity instanceof PlayerEntity) {
            PlayerEntity playerEntity2 = (PlayerEntity) entity;
            if (chams.is2048()) {
                playerEntity2.limbAnimator.pos = 0.0f;
                playerEntity2.limbAnimator.speed = 0.0f;
                playerEntity2.limbAnimator.prevSpeed = 0.0f;
            }
        }
        ChamsHelper_2.do615(matrixStack, entity);
    }

    default void do592(Chams chams, Color[] colorArr, Color color2) {
        ChamsHelper_2.do612(MixinMessageIndicatorHelper_2.getColor816(color2, colorArr[0].getAlpha()), MixinMessageIndicatorHelper_2.getColor816(color2, colorArr[1].getAlpha()));
    }

    default Color[] getColorArray593(Chams chams) {
        return new Color[]{chams.outline.getValue(), chams.fill.getValue()};
    }

    default boolean is594() {
        return false;
    }
}
