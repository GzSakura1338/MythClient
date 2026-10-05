using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using NovaInjector;

// Filesystem integration only. Installed payloads are never executed here.
if (args.Length != 3) throw new ArgumentException("Usage: <original HeyPixel jar> <new Nova DLL> <fresh output directory>");
string original = Path.GetFullPath(args[0]), dll = Path.GetFullPath(args[1]), root = Path.GetFullPath(args[2]);
if (Directory.Exists(root)) throw new IOException("Use a fresh test directory.");
string game = Path.Combine(root, "网易 游戏", "Game", ".minecraft");
string bin = Path.Combine(root, "网易 游戏", "ext", "jre-v64-220420", "jdk17", "bin");
string mods = Path.Combine(game, "mods");
Directory.CreateDirectory(mods); Directory.CreateDirectory(Path.Combine(bin, "server"));
const string javaSource = @"E:\MCLDownload\ext\jre-v64-220420\jdk17\bin";
File.Copy(Path.Combine(javaSource, "java.exe"), Path.Combine(bin, "java.exe"));
File.Copy(Path.Combine(javaSource, "server", "jvm.dll"), Path.Combine(bin, "server", "jvm.dll"));
string jar = Path.Combine(mods, "4690287328241054167@2@8.jar");
File.Copy(original, jar);
string proxy = Path.Combine(bin, "winmm.dll"), folder = Path.Combine(bin, "nova-heypixel");
File.WriteAllText(proxy, "user original winmm bytes");
string previousProxyHash = Hash(proxy);
var forge = ClientProfile.Load().Single(p => p.SupportsHeyPixel);
static string Hash(string path) { using var s = File.OpenRead(path); return Convert.ToHexString(SHA256.HashData(s)); }
static void Check(bool value, string label) { if (!value) throw new Exception(label); Console.WriteLine("PASS " + label); }
void Reject(Action action, string label)
{
    string beforeJar = Hash(jar), beforeProxy = Hash(proxy);
    bool failed = false;
    try { action(); } catch (Exception ex) when (ex is IOException or InvalidDataException or InvalidOperationException or BadImageFormatException) { failed = true; }
    Check(failed && Hash(jar) == beforeJar && Hash(proxy) == beforeProxy, "rejected without changing targets: " + label);
}
Check(ClientProfile.FindHeyPixelJavaDirectory(game) == bin, "Java detection from download root");
Check(ClientProfile.FindHeyPixelJars(game).Single() == jar, "modId discovery");
Reject(() => ClientProfile.Load().Single(p => p.Key == "lunar-1.8.9").ApplyHeyPixel(game, dll), "unsupported profile");
string badDll = Path.Combine(root, "bad.dll");
File.WriteAllText(badDll, "not PE");
Reject(() => forge.ApplyHeyPixel(game, badDll), "invalid DLL");
Reject(() => forge.ApplyHeyPixel(game, Path.Combine(javaSource, "server", "jvm.dll")), "non-Nova DLL missing proxy ABI");
string duplicate = Path.Combine(mods, "duplicate.jar");
File.Copy(jar, duplicate);
forge.ApplyHeyPixel(game, dll); forge.RestoreHeyPixel(game);
Check(Hash(jar) == Hash(original) && Hash(duplicate) == Hash(original), "mod count does not restrict proxy installation");
File.Delete(duplicate);
using (var held = new FileStream(proxy, FileMode.Open, FileAccess.Read, FileShare.Read))
    Reject(() => forge.ApplyHeyPixel(game, dll), "locked original proxy rolls back manifest/config/payload");
Check(!File.Exists(Path.Combine(folder, "install.json")) && !File.Exists(Path.Combine(folder, "loader.ini")), "failed install leaves no active config");
Console.WriteLine(forge.ApplyHeyPixel(game, dll));
Check(Hash(jar) == Hash(original), "proxy install leaves original JAR byte-identical");
Check(Hash(Path.Combine(folder, previousProxyHash + ".winmm.bak")) == previousProxyHash, "existing proxy backup");
string installed = Hash(proxy);
DateTime write = File.GetLastWriteTimeUtc(proxy);
forge.ApplyHeyPixel(game, dll);
Check(Hash(proxy) == installed && File.GetLastWriteTimeUtc(proxy) == write, "idempotent reapply");
using (var held = new FileStream(Path.Combine(folder, "install.lock"), FileMode.Open, FileAccess.ReadWrite, FileShare.None))
    Reject(() => forge.ApplyHeyPixel(game, dll), "cross-process installation lock");
string updated = Path.Combine(root, "updated.dll"); File.Copy(dll, updated);
using (var stream = new FileStream(updated, FileMode.Append)) stream.WriteByte(0);
forge.ApplyHeyPixel(game, updated);
Check(Hash(Path.Combine(folder, "nova-client-proxy-" + Hash(updated) + ".dll")) == Hash(updated), "client update");
string backup = Path.Combine(folder, previousProxyHash + ".winmm.bak");
byte[] backupBytes = File.ReadAllBytes(backup); File.WriteAllText(backup, "broken");
Reject(() => forge.RestoreHeyPixel(game), "corrupt original proxy backup");
File.WriteAllBytes(backup, backupBytes);
byte[] proxyBytes = File.ReadAllBytes(proxy); File.WriteAllText(proxy, "launcher replacement");
Reject(() => forge.RestoreHeyPixel(game), "externally replaced installed proxy");
File.WriteAllBytes(proxy, proxyBytes);
forge.RestoreHeyPixel(game);
Check(Hash(proxy) == previousProxyHash && Hash(jar) == Hash(original), "byte-identical restore");
forge.RestoreHeyPixel(game);
Check(Hash(proxy) == previousProxyHash, "idempotent restore");
using (var changed = ZipFile.Open(jar, ZipArchiveMode.Update))
using (var writer = new StreamWriter(changed.CreateEntry("new-version.txt").Open())) writer.Write("new mod revision");
string newJarHash = Hash(jar);
forge.ApplyHeyPixel(game, dll); forge.RestoreHeyPixel(game);
Check(Hash(jar) == newJarHash, "new original mod version stays untouched");
File.Copy(original, jar, true);
string loaderName;
using (var reader = new StreamReader(typeof(ClientProfile).Assembly.GetManifestResourceStream("Nova.HeyPixel.Entry")!))
    loaderName = reader.ReadToEnd().Trim('\uFEFF', '\r', '\n', ' ');
using (var patched = ZipFile.Open(jar, ZipArchiveMode.Update))
{
    patched.GetEntry(loaderName)!.Delete();
    using (var output = patched.CreateEntry(loaderName).Open())
    using (var input = typeof(ClientProfile).Assembly.GetManifestResourceStream("Nova.HeyPixel.Loader")!) input.CopyTo(output);
    using (var output = patched.CreateEntry("lib/NovaClientBase.dll").Open()) output.Write(File.ReadAllBytes(dll));
}
Reject(() => forge.ApplyHeyPixel(game, dll), "legacy migration without original backup");
string originalBackup = Path.Combine(game, "nova-backups", "heypixel", ClientProfile.HeyPixelOriginalJarHash + ".jar.bak");
Directory.CreateDirectory(Path.GetDirectoryName(originalBackup)!); File.Copy(original, originalBackup);
using (var held = new FileStream(proxy, FileMode.Open, FileAccess.Read, FileShare.Read))
    Reject(() => forge.ApplyHeyPixel(game, dll), "legacy JAR rollback after proxy replace failure");
forge.ApplyHeyPixel(game, dll);
Check(Hash(jar) == Hash(original), "legacy join patch restored before proxy activation");
forge.RestoreHeyPixel(game);
File.Delete(proxy);
forge.ApplyHeyPixel(game, dll); forge.RestoreHeyPixel(game);
Check(!File.Exists(proxy), "restore removes proxy when none existed before installation");
File.Move(jar, Path.Combine(root, "saved-mod.jar"));
foreach (bool noModsDirectory in new[] { false, true })
{
    if (noModsDirectory) Directory.Delete(mods);
    forge.ApplyHeyPixel(game, dll);
    forge.ApplyHeyPixel(game, dll);
    Check(File.Exists(proxy), "install/reapply without HeyPixel mod; mods missing=" + noModsDirectory);
    forge.RestoreHeyPixel(game);
    forge.RestoreHeyPixel(game);
    Check(!File.Exists(proxy), "restore without HeyPixel mod; mods missing=" + noModsDirectory);
}
Console.WriteLine("PASS HeyPixel filesystem integration: " + root);
