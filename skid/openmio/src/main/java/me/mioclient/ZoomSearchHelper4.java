package me.mioclient;

import me.mioclient.MatrixStackEvent;
import me.mioclient.event.Listen;
import me.mioclient.module.render.Zoom;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ZoomSearchHelper4.class */
public class ZoomSearchHelper4 implements SearchHelper_4 {
    public final Zoom zoom;

    public ZoomSearchHelper4(Zoom zoom) {
        this.zoom = zoom;
    }

    @Listen
    public void onEvent(MatrixStackEvent.Inner_3 inner_3) {
        int i = this.zoom.get917();
        this.zoom.do916(i);
        minecraftClient.options.smoothCameraEnabled = this.zoom.isToggled() && this.zoom.smoothCamera.getValue().booleanValue();
        if (i < this.zoom.num || this.zoom.isToggled()) {
            this.zoom.flag2 = true;
        } else {
            baritoneHelper.do1802(this);
            this.zoom.flag2 = false;
        }
    }
}
