$ErrorActionPreference = 'Stop'
$testRoot = $PSScriptRoot
$clientRoot = (Resolve-Path "$testRoot/../..").Path
$output = Join-Path $clientRoot 'build-vs2026/minecraft263-test'
New-Item -ItemType Directory -Force $output | Out-Null
# Minimal fixtures exercise JNI descriptors and lifetime. They are not Minecraft binaries.
$fixtures = @{
 'org/lwjgl/system/Pointer' = 'public interface Pointer { long address(); }'
 'org/lwjgl/vulkan/VkDevice' = 'public class VkDevice implements org.lwjgl.system.Pointer { public long address() { return 11; } public VkPhysicalDevice getPhysicalDevice() { return new VkPhysicalDevice(); } }'
 'org/lwjgl/vulkan/VkPhysicalDevice' = 'public class VkPhysicalDevice implements org.lwjgl.system.Pointer { public long address() { return 22; } public VkInstance getInstance() { return new VkInstance(); } }'
 'org/lwjgl/vulkan/VkInstance' = 'public class VkInstance implements org.lwjgl.system.Pointer { public long address() { return 33; } }'
 'org/lwjgl/vulkan/VkQueue' = 'public class VkQueue implements org.lwjgl.system.Pointer { public long address() { return 44; } }'
 'com/mojang/renderpearl/api/device/GpuSurface' = 'public interface GpuSurface {}'
 'com/mojang/renderpearl/backend/api/GpuSurfaceBackend' = 'public interface GpuSurfaceBackend {}'
 'com/mojang/renderpearl/frontend/FrontendGpuSurface' = 'public class FrontendGpuSurface implements com.mojang.renderpearl.api.device.GpuSurface { private final com.mojang.renderpearl.backend.api.GpuSurfaceBackend backend = new com.mojang.renderpearl.backend.vulkan.VulkanGpuSurface(); }'
 'com/mojang/renderpearl/backend/vulkan/VulkanGpuSurface' = 'public class VulkanGpuSurface implements com.mojang.renderpearl.backend.api.GpuSurfaceBackend { private final VulkanDevice device = new VulkanDevice(); private final long surface = 55; private long swapchain = 66; }'
 'com/mojang/renderpearl/backend/vulkan/VulkanDevice' = 'public class VulkanDevice { public org.lwjgl.vulkan.VkDevice vkDevice() { return new org.lwjgl.vulkan.VkDevice(); } public VulkanQueue graphicsQueue() { return new VulkanQueue(); } }'
 'com/mojang/renderpearl/backend/vulkan/VulkanQueue' = 'public class VulkanQueue { public org.lwjgl.vulkan.VkQueue vkQueue() { return new org.lwjgl.vulkan.VkQueue(); } public int queueFamilyIndex() { return 2; } }'
 'net/minecraft/WorldVersion' = 'public interface WorldVersion { String id(); }'
 'net/minecraft/SharedConstants' = 'public class SharedConstants { public static WorldVersion getCurrentVersion() { return () -> System.getProperty("nova.version"); } }'
 'com/mojang/renderpearl/api/device/GpuBackend' = 'public interface GpuBackend { String getName(); }'
 'com/mojang/blaze3d/systems/RenderSystem' = 'public class RenderSystem { private static final com.mojang.renderpearl.api.device.GpuBackend BACKEND = () -> System.getProperty("nova.backend"); }'
 'com/mojang/blaze3d/platform/Window' = 'public class Window { public long handle() { return 1234; } }'
 'net/minecraft/client/gui/screens/Screen' = 'public class Screen {}'
 'net/minecraft/client/gui/Gui' = 'public class Gui { public net.minecraft.client.gui.screens.Screen current; public net.minecraft.client.gui.screens.Screen screen() { return current; } }'
 'net/minecraft/client/MouseHandler' = 'public class MouseHandler { public boolean grabbed = true; public boolean isMouseGrabbed() { return grabbed; } public void grabMouse() { grabbed = true; } public void releaseMouse() { grabbed = false; } }'
 'net/minecraft/client/KeyMapping' = 'public class KeyMapping { public boolean down; public void setDown(boolean value) { down = value; } }'
 'net/minecraft/client/OptionInstance' = 'public class OptionInstance<T> { private T value; public OptionInstance(T value) { this.value = value; } }'
 'net/minecraft/client/Options' = 'public class Options { public KeyMapping keySprint = new KeyMapping(); public OptionInstance<Double> brightness = new OptionInstance<>(0.5); public OptionInstance<Double> gamma() { return brightness; } }'
 'net/minecraft/client/Minecraft' = 'public class Minecraft { private static final Minecraft INSTANCE = new Minecraft(); public static Minecraft getInstance() { return INSTANCE; } private final com.mojang.renderpearl.api.device.GpuSurface windowSurface = new com.mojang.renderpearl.frontend.FrontendGpuSurface(); public net.minecraft.client.gui.Gui gui = new net.minecraft.client.gui.Gui(); public MouseHandler mouseHandler = new MouseHandler(); public Options options = new Options(); public com.mojang.blaze3d.platform.Window getWindow() { return new com.mojang.blaze3d.platform.Window(); } }'
}
$fixtures['net/minecraft/world/phys/Vec3'] = 'public class Vec3 { public final double x,y,z; public Vec3(double x,double y,double z){this.x=x;this.y=y;this.z=z;} }'
$fixtures['net/minecraft/network/protocol/game/ClientboundSetEntityMotionPacket'] = 'public record ClientboundSetEntityMotionPacket(int id, net.minecraft.world.phys.Vec3 movement) {}'
$fixtures['net/minecraft/network/protocol/game/ServerboundMovePlayerPacket'] = 'public class ServerboundMovePlayerPacket { protected double x,y,z; protected float yaw,pitch; protected boolean ground,collision; public double getX(double fallback){return x;} public double getY(double fallback){return y;} public double getZ(double fallback){return z;} public float getYRot(float fallback){return yaw;} public float getXRot(float fallback){return pitch;} public boolean isOnGround(){return ground;} public boolean horizontalCollision(){return collision;} public static class PosRot extends ServerboundMovePlayerPacket { public PosRot(double x,double y,double z,float yaw,float pitch,boolean ground,boolean collision){this.x=x;this.y=y;this.z=z;this.yaw=yaw;this.pitch=pitch;this.ground=ground;this.collision=collision;} } }'
$fixtures['net/minecraft/world/item/ItemStack'] = 'public record ItemStack(int marker) {}'
$fixtures['net/minecraft/world/entity/player/Inventory'] = 'public class Inventory { public net.minecraft.world.item.ItemStack getItem(int index){return new net.minecraft.world.item.ItemStack(index);} }'
$fixtures['net/minecraft/world/inventory/AbstractContainerMenu'] = 'public class AbstractContainerMenu { public int containerId=7; }'
$fixtures['net/minecraft/world/inventory/ContainerInput'] = 'public enum ContainerInput { PICKUP,QUICK_MOVE,SWAP,THROW }'
$fixtures['net/minecraft/world/entity/player/Player'] = 'public class Player { public net.minecraft.world.inventory.AbstractContainerMenu containerMenu=new net.minecraft.world.inventory.AbstractContainerMenu(); private Inventory inventory=new Inventory(); public Inventory getInventory(){return inventory;} }'
$fixtures['net/minecraft/client/player/LocalPlayer'] = 'public class LocalPlayer extends net.minecraft.world.entity.player.Player {}'
$fixtures['net/minecraft/client/multiplayer/MultiPlayerGameMode'] = 'public class MultiPlayerGameMode { public int clicks; public void handleContainerInput(int id,int slot,int button,net.minecraft.world.inventory.ContainerInput input,net.minecraft.world.entity.player.Player player){if(id!=7||slot!=4||button!=0||input!=net.minecraft.world.inventory.ContainerInput.QUICK_MOVE)throw new AssertionError("wrong container request");clicks++;} }'
$fixtures['net/minecraft/network/PacketListener'] = 'public interface PacketListener {}'
$fixtures['net/minecraft/network/protocol/Packet'] = 'public interface Packet { void handle(net.minecraft.network.PacketListener listener); }'
$fixtures['net/minecraft/client/multiplayer/ClientPacketListener'] = 'public class ClientPacketListener implements net.minecraft.network.PacketListener {}'
$fixtures['net/minecraft/server/RunningOnDifferentThreadException'] = 'public class RunningOnDifferentThreadException extends RuntimeException {}'
$fixtures['net/minecraft/client/Minecraft'] = $fixtures['net/minecraft/client/Minecraft'].Replace('public MouseHandler mouseHandler', 'public net.minecraft.client.player.LocalPlayer player=new net.minecraft.client.player.LocalPlayer(); public net.minecraft.client.multiplayer.MultiPlayerGameMode gameMode=new net.minecraft.client.multiplayer.MultiPlayerGameMode(); public net.minecraft.client.multiplayer.ClientPacketListener getConnection(){return new net.minecraft.client.multiplayer.ClientPacketListener();} public MouseHandler mouseHandler')
$sources = @("$testRoot/Probe.java")
foreach ($entry in $fixtures.GetEnumerator()) {
    $path = Join-Path $output ($entry.Key + '.java')
    New-Item -ItemType Directory -Force (Split-Path $path) | Out-Null
    $package = ($entry.Key.Substring(0, $entry.Key.LastIndexOf('/'))).Replace('/', '.')
    [IO.File]::WriteAllText($path, "package $package;`n" + $entry.Value)
    $sources += $path
}
& javac -d "$output/classes" @sources
if ($LASTEXITCODE -ne 0) { throw 'Java fixture compilation failed' }
$build = @"
@echo off
call "D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /W4 /WX /LD /Gy /I"$clientRoot/src" /I"$clientRoot/include" "$testRoot/probe.cpp" "$clientRoot/src/minecraft/compat/minecraft263/Minecraft263.cpp" "$clientRoot/src/minecraft/compat/minecraft263/Minecraft263Access.cpp" "$clientRoot/src/minecraft/compat/minecraft263/Minecraft263Actions.cpp" "$clientRoot/src/minecraft/compat/minecraft263/Minecraft263Inventory.cpp" /Fo"$output\\" /Fe"$output/probe.dll" /link /OPT:REF
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$output/build.cmd", $build)
& cmd /c "$output/build.cmd"
if ($LASTEXITCODE -ne 0) { throw 'JNI probe compilation failed' }
& java --enable-native-access=ALL-UNNAMED -Xcheck:jni -cp "$output/classes" Probe "$output/probe.dll"
if ($LASTEXITCODE -ne 0) { throw 'JNI probe failed' }
