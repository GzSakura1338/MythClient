param(
    [Parameter(Mandatory=$true)][string]$Dll,
    [string]$JavaHome = 'E:\MCLDownload\ext\jre-v64-220420\jdk17',
    [string]$Javac = 'D:\Program Files\Java\jdk-26.0.2.1\bin\javac.exe',
    [string]$JoinLoader
)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetTempPath()) ('nova-deferred-test-' + [guid]::NewGuid().ToString('N'))
$sources = Join-Path $root 'src'
$classes = Join-Path $root 'classes'
$appdata = Join-Path $root 'appdata'
New-Item -ItemType Directory -Path $classes,(Join-Path $appdata '.minecraft') -Force | Out-Null

# Minimal game-shaped fixtures exercise the real DLL's JNI readiness probe.
# They are never placed in a game JAR and cannot connect to any server.
$files = @{
    'net/minecraft/client/player/LocalPlayer.java' = 'package net.minecraft.client.player; public class LocalPlayer {}'
    'net/minecraft/client/multiplayer/ClientLevel.java' = 'package net.minecraft.client.multiplayer; public class ClientLevel {}'
    'net/minecraft/client/multiplayer/ClientPacketListener.java' = 'package net.minecraft.client.multiplayer; public class ClientPacketListener {}'
    'net/minecraft/client/gui/screens/Screen.java' = 'package net.minecraft.client.gui.screens; public class Screen {}'
    'net/minecraftforge/client/event/ClientPlayerNetworkEvent.java' = 'package net.minecraftforge.client.event; public class ClientPlayerNetworkEvent { public static class LoggingIn {} }'
    'JoinedFixture.java' = @'
import java.nio.file.*;
import java.lang.reflect.*;
import net.minecraft.client.Minecraft;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.client.multiplayer.*;
import net.minecraft.client.gui.screens.Screen;
import net.minecraftforge.client.event.ClientPlayerNetworkEvent.LoggingIn;
public class JoinedFixture {
    static Path log;
    static String text() throws Exception { return Files.exists(log) ? Files.readString(log) : ""; }
    static void absent(String state) throws Exception {
        Thread.sleep(400);
        if (Files.exists(log)) throw new AssertionError("Native runtime loaded before remote join: " + state);
        System.out.println("PASS DLL absent: " + state);
    }
    public static void main(String[] args) throws Exception {
        log = Path.of(args[1]);
        Minecraft mc = Minecraft.m_91087_();
        Class<?> loader = Class.forName(Files.readString(Path.of(args[0])));
        Object receiver = loader.getConstructor().newInstance();
        Method callback = java.util.Arrays.stream(loader.getDeclaredMethods())
            .filter(m -> m.getParameterCount() == 1 && m.getParameterTypes()[0] == LoggingIn.class).findFirst().orElseThrow();
        absent("startup without a join event");
        callback.invoke(receiver, new LoggingIn()); absent("no world / failed login");
        mc.f_91073_ = new ClientLevel(); mc.f_91074_ = new LocalPlayer(); mc.local = true;
        callback.invoke(receiver, new LoggingIn()); absent("singleplayer join event");
        mc.local = false; mc.listener = new ClientPacketListener(); mc.f_91080_ = new Screen();
        callback.invoke(receiver, new LoggingIn());
        for (int i = 0; i < 50 && !text().contains("waiting for a remote world"); i++) Thread.sleep(100);
        if (!text().contains("waiting for a remote world")) throw new AssertionError("Join did not load the DLL");
        for (int i = 0; i < 3; i++) callback.invoke(receiver, new LoggingIn());
        if (text().split("waiting for a remote world", -1).length != 2) throw new AssertionError("Duplicate DLL load");
        System.out.println("PASS real callback loads the DLL once after remote join");
        mc.f_91080_ = null;
        for (int i = 0; i < 50 && !text().contains("remote world ready"); i++) Thread.sleep(100);
        if (!text().contains("remote world ready")) throw new AssertionError("Runtime did not activate after loading screen closed");
        System.out.println("PASS runtime activates after the remote world becomes ready");
    }
}
'@
    'net/minecraft/client/Minecraft.java' = @'
package net.minecraft.client;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.client.multiplayer.*;
import net.minecraft.client.gui.screens.Screen;
public class Minecraft {
    private static final Minecraft INSTANCE = new Minecraft();
    public volatile LocalPlayer f_91074_;
    public volatile ClientLevel f_91073_;
    public volatile Screen f_91080_;
    public volatile boolean local;
    public volatile ClientPacketListener listener;
    public static Minecraft m_91087_() { return INSTANCE; }
    public boolean m_91090_() { return local; }
    public ClientPacketListener m_91403_() { return listener; }
}
'@
    'DeferredFixture.java' = @'
import java.nio.file.*;
import net.minecraft.client.Minecraft;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.client.multiplayer.*;
import net.minecraft.client.gui.screens.Screen;
public class DeferredFixture {
    static Path log;
    static String text() throws Exception { return Files.exists(log) ? Files.readString(log) : ""; }
    static void waiting(String phase) throws Exception {
        Thread.sleep(1300);
        String value = text();
        if (!value.contains("waiting for a remote world") || value.contains("remote world ready") ||
            value.contains("Bridge:") || value.contains("modules registered") ||
            value.contains("Transformer:") || value.contains("swap hook installed"))
            throw new AssertionError("Unexpected activation during " + phase + ": " + value);
        System.out.println("PASS inactive: " + phase);
    }
    public static void main(String[] args) throws Exception {
        log = Path.of(args[1]);
        Minecraft mc = Minecraft.m_91087_();
        System.load(args[0]);
        if (args[2].equals("ordinary")) {
            Thread.sleep(2000);
            String value = text();
            if (!value.contains("Profile:") || value.contains("HeyPixel:"))
                throw new AssertionError("Ordinary injection entered deferred mode: " + value);
            System.out.println("PASS ordinary load keeps immediate initialization");
            return;
        }
        if (args[2].equals("cancel")) {
            for (int i = 0; i < 100 && !text().contains("waiting cancelled"); i++) Thread.sleep(100);
            String value = text();
            if (!value.contains("waiting cancelled") || value.contains("remote world ready") ||
                value.contains("modules registered") || value.contains("swap hook installed"))
                throw new AssertionError("Waiting cancellation failed: " + value);
            System.out.println("PASS waiting cancellation keeps runtime inactive");
            return;
        }
        waiting("menu / authentication failure");
        mc.listener = new ClientPacketListener();
        waiting("connection without world");
        mc.f_91073_ = new ClientLevel();
        waiting("world without player");
        mc.f_91074_ = new LocalPlayer(); mc.local = true;
        waiting("singleplayer world");
        mc.local = false; mc.f_91080_ = new Screen();
        waiting("loading screen");
        mc.f_91080_ = null; mc.listener = null;
        waiting("missing server connection");
        mc.listener = new ClientPacketListener();
        Thread.sleep(350);
        mc.f_91073_ = null;
        waiting("join interrupted before readiness stabilizes");
        mc.f_91073_ = new ClientLevel();
        for (int i = 0; i < 60 && !text().contains("remote world ready"); i++) Thread.sleep(100);
        String value = text();
        if (value.split("remote world ready", -1).length != 2)
            throw new AssertionError("Expected exactly one activation: " + value);
        System.out.println("PASS remote world activates the real DLL exactly once");
        // All native workers are daemon threads. No rendering backend or real
        // Minecraft classes exist here; exit before bridge retry can time out.
    }
}
'@
}
foreach ($entry in $files.GetEnumerator()) {
    $path = Join-Path $sources $entry.Key
    New-Item -ItemType Directory -Path (Split-Path $path) -Force | Out-Null
    [IO.File]::WriteAllText($path, $entry.Value, [Text.UTF8Encoding]::new($false))
}
$javaFiles = @(Get-ChildItem $sources -Recurse -Filter '*.java' | ForEach-Object FullName)
& $Javac --release 17 -d $classes @javaFiles
if ($LASTEXITCODE -ne 0) { throw 'Fixture compilation failed' }
$modes = @('deferred','ordinary','cancel')
if ($JoinLoader) {
    $asm = Join-Path (Resolve-Path (Join-Path $PSScriptRoot '../../../skid/DEOBF/heypixel-nova/verify-libs')).Path '*'
    & $Javac --release 17 -cp $asm -d $classes (Join-Path $PSScriptRoot 'ExtractJoinFixture.java')
    if ($LASTEXITCODE -ne 0) { throw 'Join fixture extractor compilation failed' }
    & (Join-Path $JavaHome 'bin/java.exe') -cp "$classes;$asm" ExtractJoinFixture (Resolve-Path $JoinLoader).Path $classes
    if ($LASTEXITCODE -ne 0) { throw 'Join fixture extraction failed' }
    New-Item -ItemType Directory -Path (Join-Path $classes 'lib') -Force | Out-Null
    Copy-Item -LiteralPath (Resolve-Path $Dll).Path -Destination (Join-Path $classes 'lib/NovaClientBase.dll')
    $modes = @('event')
}
foreach ($mode in $modes) {
    $caseDir = Join-Path $root $mode
    $caseAppdata = Join-Path $caseDir 'appdata'
    New-Item -ItemType Directory -Path (Join-Path $caseAppdata '.minecraft') -Force | Out-Null
    $name = if ($mode -eq 'ordinary') { 'ordinary-client.dll' } else { 'nova-client-fixture.dll' }
    $library = Join-Path $caseDir $name
    Copy-Item -LiteralPath (Resolve-Path $Dll).Path -Destination $library
    $log = Join-Path $caseAppdata '.minecraft/nova-base.log'
    $info = [Diagnostics.ProcessStartInfo]::new((Join-Path $JavaHome 'bin/java.exe'))
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.Environment['APPDATA'] = $caseAppdata
    $fixture = if ($mode -eq 'event') { 'JoinedFixture' } else { 'DeferredFixture' }
    $inputArg = if ($mode -eq 'event') { Join-Path $classes 'fixture-owner.txt' } else { $library }
    foreach ($arg in @('-Xmx128M','-Xcheck:jni','-cp',$classes,$fixture,$inputArg,$log,$mode)) { $info.ArgumentList.Add($arg) }
    $process = [Diagnostics.Process]::Start($info)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if ($mode -eq 'cancel') {
        $until = [DateTime]::UtcNow.AddSeconds(10)
        while ([DateTime]::UtcNow -lt $until -and !$process.HasExited) {
            $logText = ''
            if (Test-Path -LiteralPath $log) {
                $stream = [IO.File]::Open($log, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
                $reader = [IO.StreamReader]::new($stream)
                try { $logText = $reader.ReadToEnd() } finally { $reader.Dispose() }
            }
            if ($logText -match 'waiting for a remote world') {
                $wake = [Threading.EventWaitHandle]::OpenExisting('Local\NovaUnload-' + $process.Id)
                try { [void]$wake.Set() } finally { $wake.Dispose() }
                break
            }
            Start-Sleep -Milliseconds 100
        }
    }
    if (!$process.WaitForExit(25000)) { $process.Kill(); throw "$mode fixture timed out" }
    $output = $stdout.GetAwaiter().GetResult()
    $errors = $stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $caseDir 'stdout.log'), $output)
    [IO.File]::WriteAllText((Join-Path $caseDir 'stderr.log'), $errors)
    $output -split '\r?\n' | Where-Object { $_ -match '^PASS|^\[Nova\]' } | Write-Output
    if ($process.ExitCode -ne 0 -or ($output + $errors) -match 'WARNING in native method|FATAL ERROR in native method') {
        throw "$mode fixture failed ($($process.ExitCode)): $errors"
    }
}
Write-Output "PASS: real DLL lifecycle with -Xcheck:jni; isolated artifacts: $root"
