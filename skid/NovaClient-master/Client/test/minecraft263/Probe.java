import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.Gui;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.OptionInstance;

public class Probe {
    static native String init();
    static native void shutdown();
    static native boolean mouse(boolean capture);
    static native boolean sprint(boolean down);
    static native double gamma();
    static native boolean setGamma(double value);
    static native long window();
    static native boolean vulkanHandles();
    static native boolean motion(Object packet,int id,int x,int y,int z);
    static native net.minecraft.network.protocol.game.ServerboundMovePlayerPacket.PosRot posRot(Object packet);
    static native boolean click(int container);
    static native net.minecraft.world.item.ItemStack stack(int index,boolean armor);
    static native boolean replay(net.minecraft.network.protocol.Packet packet);
    static void check(boolean pass, String message) { if (!pass) throw new AssertionError(message); }
    public static void main(String[] args) {
        System.load(args[0]);
        System.setProperty("nova.version", "1.20.1");
        check(init().contains("Expected Minecraft 26.3"), "reject wrong version");
        shutdown();
        System.setProperty("nova.version", "26.3");
        System.setProperty("nova.backend", "UnknownBackend");
        check(init().contains("Unsupported"), "reject unsupported renderer");
        shutdown();
        System.setProperty("nova.backend", "OpenGL");
        check(init() == null, "initialize 26.3");
        check(window() == 1234, "SDL handle");
        Minecraft mc = Minecraft.getInstance();
        var movement=new net.minecraft.network.protocol.game.ClientboundSetEntityMotionPacket(42,new net.minecraft.world.phys.Vec3(0.125,0.4,-0.25));
        check(motion(movement,42,1000,3200,-2000),"26.3 Vec3 velocity conversion");
        check(!motion(new Object(),0,0,0,0),"reject foreign packet");
        check(!motion(new net.minecraft.network.protocol.game.ClientboundSetEntityMotionPacket(42,new net.minecraft.world.phys.Vec3(Double.NaN,0,0)),42,0,0,0),"reject non-finite velocity");
        var position=new net.minecraft.network.protocol.game.ServerboundMovePlayerPacket.PosRot(1.5,64,-3,90,-20,true,true);
        var replaced=posRot(position);
        check(replaced!=null&&replaced.getX(0)==1.5&&replaced.getY(0)==64&&replaced.getZ(0)==-3&&replaced.getYRot(0)==100&&replaced.getXRot(0)==-20&&replaced.isOnGround()&&replaced.horizontalCollision(),"rotation preserves position and collision flags");
        check(!click(8)&&mc.gameMode.clicks==0,"reject stale container");
        check(click(7)&&mc.gameMode.clicks==1,"quick move through current menu");
        check(stack(35,false).marker()==35&&stack(0,true).marker()==36&&stack(3,true).marker()==39,"modern inventory slot layout");
        check(stack(-1,false)==null&&stack(36,false)==null&&stack(4,true)==null,"reject inventory bounds");
        check(replay(listener -> {}),"replay packet");
        check(replay(listener -> {throw new net.minecraft.server.RunningOnDifferentThreadException();}),"thread handoff counts as queued");
        check(!replay(listener -> {throw new IllegalArgumentException();}),"real replay failure is reported");
        for(int i=0;i<2000;i++)check(motion(movement,42,1000,3200,-2000)&&posRot(position)!=null&&stack(0,true)!=null,"adapter local-reference lifetime");
        check(gamma() == 0.5, "read gamma");
        check(setGamma(10) && gamma() == 10, "write gamma");
        mc.options.brightness = new OptionInstance<>(0.75);
        check(gamma() == 0.75, "do not cache replaced option object");
        check(sprint(true) && mc.options.keySprint.down, "sprint in world");
        mc.gui = new Gui();
        mc.gui.current = new Screen();
        check(sprint(true) && !mc.options.keySprint.down, "suppress sprint in screen");
        mc.gui.current = null;
        check(mouse(true) && !mc.mouseHandler.grabbed, "release mouse");
        check(mouse(true) && mouse(false) && mc.mouseHandler.grabbed, "restore original grab after repeated capture");
        check(mouse(true), "capture before game opens screen");
        mc.gui.current = new Screen();
        check(mouse(false) && !mc.mouseHandler.grabbed, "do not grab over game screen");
        mc.gui.current = null;
        check(mouse(true) && mouse(false) && !mc.mouseHandler.grabbed, "preserve originally released cursor");
        // Detect leaked locals on an already attached JVM thread under -Xcheck:jni.
        for (int i = 0; i < 2000; ++i) {
            check(sprint(false) && setGamma(0.5) && gamma() == 0.5, "JNI loop");
        }
        shutdown();
        check(init() == null, "reinitialize after cleanup");
        shutdown();
        System.setProperty("nova.backend", "Vulkan");
        check(init() == null && vulkanHandles(), "Vulkan pointer chain and queue family");
        check(sprint(true) && mc.options.keySprint.down && gamma() == 0.5, "common APIs on Vulkan");
        shutdown();
        System.out.println("PASS: version/backend, Vulkan handles, SDL, screen, mouse, sprint, gamma, velocity, PosRot flags, container clicks, inventory, packet replay, JNI lifetime");
    }
}
