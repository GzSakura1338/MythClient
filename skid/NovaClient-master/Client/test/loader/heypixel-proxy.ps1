param(
    [Parameter(Mandatory=$true)][string]$Dll,
    [Parameter(Mandatory=$true)][string]$Proxy,
    [string]$JavaHome = 'E:\MCLDownload\ext\jre-v64-220420\jdk17',
    [string]$Javac = 'D:\Program Files\Java\jdk-26.0.2.1\bin\javac.exe'
)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetTempPath()) ('nova-proxy-test-' + [guid]::NewGuid().ToString('N'))
$runtime = Join-Path $root 'jdk17'
$game = Join-Path $root '游戏 空间'
$classes = Join-Path $root 'classes'
New-Item -ItemType Directory -Path $runtime,$game,$classes -Force | Out-Null
# Clone the runtime. Never load or overwrite the reference proxy in the real JDK.
& robocopy $JavaHome $runtime /E /NFL /NDL /NJH /NJS /NP /XF winmm.dll '*.i64' '*.id0' '*.id1' '*.id2' '*.nam' '*.til' /XD nova-heypixel | Out-Null
if ($LASTEXITCODE -ge 8) { throw 'Could not clone Java runtime' }
$folder = Join-Path $runtime 'bin/nova-heypixel'
New-Item -ItemType Directory -Path $folder -Force | Out-Null
Copy-Item -LiteralPath (Resolve-Path $Proxy).Path -Destination (Join-Path $runtime 'bin/winmm.dll')
Copy-Item -LiteralPath (Resolve-Path $Dll).Path -Destination (Join-Path $folder 'nova-client-proxy-fixture.dll')
$marker = Join-Path $root 'glfw.dll'
$markerSource = Join-Path $root 'graphics.c'
[IO.File]::WriteAllText($markerSource, '__declspec(dllexport) int NovaFixtureGraphics(void) { return 1; }')
$markerBuild = Join-Path $root 'build-marker.cmd'
[IO.File]::WriteAllText($markerBuild, "@echo off`r`ncall `"D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat`" >nul`r`ncl /nologo /LD /MT /Fo`"$root\graphics.obj`" `"$markerSource`" /link /NOENTRY /OUT:`"$marker`" /IMPLIB:`"$root\graphics.lib`"`r`n")
& $markerBuild
if ($LASTEXITCODE -ne 0) { throw 'Graphics marker compilation failed' }
$sources = @{
    'net/minecraft/client/player/LocalPlayer.java' = 'package net.minecraft.client.player; public class LocalPlayer {}'
    'net/minecraft/client/multiplayer/ClientLevel.java' = 'package net.minecraft.client.multiplayer; public class ClientLevel {}'
    'net/minecraft/client/gui/screens/Screen.java' = 'package net.minecraft.client.gui.screens; public class Screen {}'
    'net/minecraft/client/Minecraft.java' = @'
package net.minecraft.client;
public class Minecraft {
    private static final Minecraft INSTANCE = new Minecraft();
    public net.minecraft.client.player.LocalPlayer f_91074_;
    public net.minecraft.client.multiplayer.ClientLevel f_91073_;
    public net.minecraft.client.gui.screens.Screen f_91080_;
    public static Minecraft m_91087_() { return INSTANCE; }
}
'@
    'StartupFixture.java' = @'
import java.nio.file.*;
public class StartupFixture {
    static Path log;
    static String text() throws Exception { return Files.exists(log) ? Files.readString(log) : ""; }
    public static void main(String[] args) throws Exception {
        log = Path.of(args[0]);
        if (args[1].equals("unrelated")) {
            Thread.sleep(1800);
            if (Files.exists(log)) throw new AssertionError("Nova loaded for unrelated working directory");
            System.out.println("PASS unrelated Java working directory does not load Nova");
            return;
        }
        for (int i = 0; i < 120 && !text().contains("waiting for Java graphics startup"); i++) Thread.sleep(100);
        if (!text().contains("waiting for Java graphics startup") || text().contains("attaching native runtime"))
            throw new AssertionError("JNI attached before Java graphics startup: " + text());
        // Simulate LWJGL loading GLFW from live Java. This loads only the tiny
        // fixture marker; Nova itself is loaded exclusively by the proxy.
        System.load(args[2]);
        for (int i = 0; i < 120 && !text().contains("waiting for initialized Minecraft"); i++) Thread.sleep(100);
        if (!text().contains("waiting for initialized Minecraft")) throw new AssertionError("Proxy failed to start Nova: " + text());
        if (text().contains("game interfaces ready")) throw new AssertionError("Premature game initialization");
        System.out.println("PASS proxy loaded Nova before Minecraft classes without System.load");
        Class<?> mc = Class.forName("net.minecraft.client.Minecraft");
        Object instance = mc.getMethod("m_91087_").invoke(null);
        if (mc.getField("f_91073_").get(instance) != null || mc.getField("f_91074_").get(instance) != null)
            throw new AssertionError("Fixture unexpectedly joined a world");
        for (int i = 0; i < 120 && !text().contains("starting Nova runtime without waiting for a world"); i++) Thread.sleep(100);
        if (!text().contains("starting Nova runtime without waiting for a world") || text().contains("waiting for a remote world"))
            throw new AssertionError("Nova did not initialize before joining: " + text());
        System.out.println("PASS real Nova startup proceeds with no world/player/server connection");
    }
}
'@
}
foreach ($item in $sources.GetEnumerator()) {
    $path = Join-Path $root ('src/' + $item.Key)
    New-Item -ItemType Directory -Path (Split-Path $path) -Force | Out-Null
    [IO.File]::WriteAllText($path, $item.Value)
}
$javaFiles = @(Get-ChildItem (Join-Path $root 'src') -Filter '*.java' -Recurse | ForEach-Object FullName)
& $Javac --release 17 -d $classes @javaFiles
if ($LASTEXITCODE -ne 0) { throw 'Fixture compilation failed' }
[IO.File]::WriteAllText((Join-Path $folder 'loader.ini'), "[Nova]`r`nEnabled=1`r`nGameDirectory=`"$game`"`r`nClient=nova-client-proxy-fixture.dll`r`n", [Text.Encoding]::Unicode)
foreach ($mode in @('startup','unrelated')) {
    $appdata = Join-Path $root ($mode + '-appdata')
    New-Item -ItemType Directory -Path (Join-Path $appdata '.minecraft') -Force | Out-Null
    $log = Join-Path $appdata '.minecraft/nova-base.log'
    $info = [Diagnostics.ProcessStartInfo]::new((Join-Path $runtime 'bin/java.exe'))
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.WorkingDirectory = if ($mode -eq 'startup') { $game } else { $root }
    $info.Environment['APPDATA'] = $appdata
    foreach ($arg in @('-Xmx128M','-Xcheck:jni','-cp',$classes,'StartupFixture',$log,$mode,$marker)) { $info.ArgumentList.Add($arg) }
    $process = [Diagnostics.Process]::Start($info)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (!$process.WaitForExit(30000)) { $process.Kill(); throw "$mode timed out; artifacts: $root" }
    $output = $stdout.GetAwaiter().GetResult(); $errors = $stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $root ($mode + '-stdout.log')), $output)
    [IO.File]::WriteAllText((Join-Path $root ($mode + '-stderr.log')), $errors)
    $output -split '\r?\n' | Where-Object { $_ -match '^PASS' } | Write-Output
    if ($process.ExitCode -ne 0 -or ($output + $errors) -match 'WARNING in native method|FATAL ERROR in native method') {
        throw "$mode failed ($($process.ExitCode)): $errors; artifacts: $root"
    }
}
Write-Output "PASS isolated JVM proxy lifecycle; artifacts: $root"
