package me.mioclient.event;

import java.util.List;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.tooltip.TooltipComponent;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/RenderWithTooltipEvent.class */
public class RenderWithTooltipEvent extends Event {
    public static int num;
    public static int num2;
    public final Screen screen;
    public List<TooltipComponent> list;
    public int x;
    public int y;

    public RenderWithTooltipEvent(Screen screen, List<TooltipComponent> list, int i, int i2, int i3, int i4) {
        this.screen = screen;
        this.list = list;
        this.x = i;
        this.y = i2;
        num = i3;
        num2 = i4;
    }

    public Screen getScreen247() {
        return this.screen;
    }

    public List<TooltipComponent> getList248() {
        return this.list;
    }

    public void do249(List<TooltipComponent> list) {
        this.list = list;
    }

    public int getX() {
        return this.x;
    }

    public void setX(int i) {
        this.x = i;
    }

    public int getY() {
        return this.y;
    }

    public void setY(int i) {
        this.y = i;
    }
}
