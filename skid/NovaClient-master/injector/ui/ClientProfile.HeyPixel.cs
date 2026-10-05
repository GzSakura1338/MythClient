using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text.RegularExpressions;
using System.Text;
using System.Text.Json;
using System.Runtime.InteropServices;

namespace NovaInjector;

public sealed partial record ClientProfile
{
    // Legacy hashes are used only to migrate old JAR patches back to the original.
    internal const string HeyPixelOriginalJarHash = "BD500EA2DD0E2806450A252671D406336A93A3FCD6A076D63E2B55BE196DE192";
    private const string LegacyPreloadLoaderHash = "592FDCBAA23F3B6C9C816E7EAD511B87B92293EF2D280DE6B7FAB4B09DCB83CA";
    private const string PatchedLoaderHash = "43EB1EFEB6CD46D2566D12DC8D7D189A91EDC9D380925C63A7B05D54EF05BAB0";
    private const string NovaDllEntry = "lib/NovaClientBase.dll";
    private static readonly object HeyPixelLock = new();
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool QueryFullProcessImageName(IntPtr process, uint flags, StringBuilder path, ref uint length);
    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(IntPtr handle);

    internal static string FindHeyPixelJavaDirectory(string gameDirectory)
    {
        string root = Path.GetFullPath(Path.Combine(gameDirectory, "..", ".."));
        string bin = Path.Combine(root, "ext", "jre-v64-220420", "jdk17", "bin");
        return File.Exists(Path.Combine(bin, "java.exe")) && File.Exists(Path.Combine(bin, "server", "jvm.dll"))
            ? bin : string.Empty;
    }

    private static string RequireJavaDirectory(string game, string? javaDirectory)
    {
        string bin = string.IsNullOrWhiteSpace(javaDirectory) ? FindHeyPixelJavaDirectory(game) : Path.GetFullPath(javaDirectory.Trim());
        if (string.IsNullOrEmpty(bin) || !File.Exists(Path.Combine(bin, "java.exe")) ||
            !File.Exists(Path.Combine(bin, "server", "jvm.dll")))
            throw new DirectoryNotFoundException("请选择网易启动器实际使用的 Java bin 目录（含 java.exe 和 server/jvm.dll）。");
        ValidatePe(File.ReadAllBytes(Path.Combine(bin, "java.exe")), false);
        ValidatePe(File.ReadAllBytes(Path.Combine(bin, "server", "jvm.dll")), true);
        RequireGameStopped(bin);
        return Path.TrimEndingDirectorySeparator(bin);
    }

    private static void ValidatePe(byte[] bytes, bool dll)
    {
        using var pe = new PEReader(new MemoryStream(bytes));
        if (pe.PEHeaders.CoffHeader.Machine != Machine.Amd64 ||
            (dll && (pe.PEHeaders.CoffHeader.Characteristics & Characteristics.Dll) == 0))
            throw new InvalidDataException("需要 Windows x64 Java 和客户端 DLL。");
    }

    private static bool HasExport(byte[] bytes, string required)
    {
        using var pe = new PEReader(new MemoryStream(bytes));
        var directory = pe.PEHeaders.PEHeader?.ExportTableDirectory ?? default;
        if (directory.Size < 40) return false;
        var table = pe.GetSectionData(directory.RelativeVirtualAddress).GetReader(0, 40);
        table.Offset = 24;
        uint count = table.ReadUInt32();
        table.Offset = 32;
        int names = table.ReadInt32();
        if (count > 65536) return false;
        var entries = pe.GetSectionData(names).GetReader(0, checked((int)count * 4));
        for (uint i = 0; i < count; ++i)
        {
            int address = entries.ReadInt32();
            var name = pe.GetSectionData(address).GetReader();
            if (name.ReadUTF8(required.Length + 1) == required + "\0") return true;
        }
        return false;
    }

    private static string? ExistingHash(string path) => File.Exists(path) ? FileHash(path) : null;

    private static void ReplaceFromFile(string source, string target)
    {
        string temporary = target + ".nova-" + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            File.Copy(source, temporary);
            if (File.Exists(target)) File.Replace(temporary, target, null);
            else File.Move(temporary, target);
        }
        finally { if (File.Exists(temporary)) File.Delete(temporary); }
    }

    // Keep recovery files outside mods. Transactions restore every touched file
    // on ordinary failures; retain rollback files if Windows prevents recovery.
    private static void CommitFiles(string stateDirectory, (string Path, byte[]? Bytes)[] changes,
        Dictionary<string, string?> expected)
    {
        string stage = Path.Combine(stateDirectory, "transaction-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(stage);
        var originals = new Dictionary<string, string?>();
        var applied = new List<string>();
        bool retain = false;
        try
        {
            for (int i = 0; i < changes.Length; ++i)
            {
                var change = changes[i];
                if (ExistingHash(change.Path) != expected[change.Path])
                    throw new IOException("文件在校验后发生变化，未覆盖：" + change.Path);
                string? backup = null;
                if (File.Exists(change.Path))
                {
                    backup = Path.Combine(stage, i + ".before");
                    File.Copy(change.Path, backup);
                    if (FileHash(backup) != expected[change.Path]) throw new IOException("备份期间文件发生变化。");
                }
                originals.Add(change.Path, backup);
                if (change.Bytes is not null) File.WriteAllBytes(Path.Combine(stage, i + ".after"), change.Bytes);
            }
            for (int i = 0; i < changes.Length; ++i)
            {
                var change = changes[i];
                string? before = originals[change.Path];
                if (ExistingHash(change.Path) != (before is null ? null : FileHash(before)))
                    throw new IOException("文件在安装期间发生变化，已停止：" + change.Path);
                if (change.Bytes is null) File.Delete(change.Path);
                else ReplaceFromFile(Path.Combine(stage, i + ".after"), change.Path);
                applied.Add(change.Path);
            }
        }
        catch (Exception failure)
        {
            var recoveryErrors = new List<Exception>();
            foreach (string path in applied.AsEnumerable().Reverse())
            {
                try
                {
                    if (originals[path] is not { } backup) File.Delete(path);
                    else ReplaceFromFile(backup, path);
                }
                catch (Exception ex) { recoveryErrors.Add(ex); }
            }
            if (recoveryErrors.Count != 0)
            {
                retain = true;
                throw new IOException("操作失败，部分文件未能自动恢复；请保留恢复目录：" + stage,
                    new AggregateException(new[] { failure }.Concat(recoveryErrors)));
            }
            throw;
        }
        finally { if (!retain) Directory.Delete(stage, recursive: true); }
    }

    private static Dictionary<string, string>? ReadProxyState(string path, string game)
    {
        if (!File.Exists(path)) return null;
        var state = JsonSerializer.Deserialize<Dictionary<string, string>>(File.ReadAllText(path))
            ?? throw new InvalidDataException("HeyPixel 安装记录损坏。");
        foreach (string key in new[] { "Game", "ProxyHash", "ClientHash", "Client", "ConfigHash", "OriginalProxyHash" })
            if (!state.ContainsKey(key)) throw new InvalidDataException("HeyPixel 安装记录缺少字段。");
        if (!string.Equals(state["Game"], game, StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("这个 Java 已由另一个游戏目录使用 HeyPixel 模式：" + state["Game"]);
        foreach (string key in new[] { "ProxyHash", "ClientHash", "ConfigHash" })
            if (!Regex.IsMatch(state[key], "^[A-F0-9]{64}$")) throw new InvalidDataException("安装记录哈希损坏。");
        if (state["OriginalProxyHash"].Length != 0 && !Regex.IsMatch(state["OriginalProxyHash"], "^[A-F0-9]{64}$"))
            throw new InvalidDataException("原始代理备份记录损坏。");
        if (state["Client"] != "nova-client-proxy-" + state["ClientHash"] + ".dll")
            throw new InvalidDataException("客户端路径记录损坏。");
        return state;
    }

    private static void VerifyProxyState(string bin, string folder, Dictionary<string, string> state)
    {
        foreach (var item in new[] { (Path.Combine(bin, "winmm.dll"), "ProxyHash"),
                     (Path.Combine(folder, state["Client"]), "ClientHash"), (Path.Combine(folder, "loader.ini"), "ConfigHash") })
            if (ExistingHash(item.Item1) != state[item.Item2])
                throw new IOException("已安装文件被外部修改或删除，请先检查：" + item.Item1);
        if (state["OriginalProxyHash"].Length != 0 &&
            ExistingHash(Path.Combine(folder, state["OriginalProxyHash"] + ".winmm.bak")) != state["OriginalProxyHash"])
            throw new IOException("原 winmm.dll 备份缺失或损坏，未修改安装。");
    }

    // Unknown original mod versions are fine: proxy loading does not edit Java
    // bytecode. Only known legacy Nova patches need a verified original backup.
    private static byte[]? LegacyOriginal(string jarPath, string game)
    {
        using var jar = ZipFile.OpenRead(jarPath);
        EnsureUniqueEntries(jar);
        var loader = jar.GetEntry(ReadHeyPixelEntryName());
        string? hash = loader is null ? null : EntryHash(loader);
        bool patched = hash == PatchedLoaderHash || hash == LegacyPreloadLoaderHash;
        if (!patched && jar.GetEntry(NovaDllEntry) is null) return null;
        if (!patched) throw new InvalidDataException("发现未知 Nova JAR 补丁，请先恢复原版模组。");
        string backup = BackupPath(game, HeyPixelOriginalJarHash);
        if (ExistingHash(backup) != HeyPixelOriginalJarHash)
            throw new InvalidDataException("旧 JAR 补丁需要完整原版备份后才能迁移：" + backup);
        using var original = ZipFile.OpenRead(backup);
        foreach (var entry in jar.Entries)
        {
            if (entry.FullName == ReadHeyPixelEntryName() || entry.FullName == NovaDllEntry) continue;
            if (original.GetEntry(entry.FullName) is not { } old || EntryHash(old) != EntryHash(entry))
                throw new InvalidDataException("旧补丁含有额外修改，不能自动恢复：" + entry.FullName);
        }
        if (jar.Entries.Count != original.Entries.Count + (jar.GetEntry(NovaDllEntry) is null ? 0 : 1))
            throw new InvalidDataException("旧补丁条目不完整，不能自动迁移。");
        string patchedBackup = BackupPath(game, FileHash(jarPath));
        if (!File.Exists(patchedBackup)) File.Copy(jarPath, patchedBackup);
        else if (FileHash(patchedBackup) != FileHash(jarPath)) throw new IOException("旧补丁备份损坏。");
        return File.ReadAllBytes(backup);
    }

    internal string ApplyHeyPixel(string gameDirectory, string dllPath, string? javaDirectory = null)
    {
        RequireHeyPixelProfile();
        lock (HeyPixelLock)
        {
            string game = RequireGameDirectory(gameDirectory);
            RequireGameStopped(game);
            string bin = RequireJavaDirectory(game, javaDirectory);
            byte[] client = File.ReadAllBytes(Path.GetFullPath(dllPath));
            ValidatePe(client, true);
            if (!HasExport(client, "NovaHeyPixelProxyABI_v1"))
                throw new InvalidDataException("请选择新版 Nova DLL：缺少 HeyPixel 启动代理接口。");
            byte[] proxy = ReadHeyPixelResource("Nova.HeyPixel.Winmm");
            ValidatePe(proxy, true);
            string folder = Path.Combine(bin, "nova-heypixel");
            Directory.CreateDirectory(folder);
            using var installLock = new FileStream(Path.Combine(folder, "install.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
            string manifest = Path.Combine(folder, "install.json");
            string? manifestHash = ExistingHash(manifest);
            var prior = ReadProxyState(manifest, game);
            if (prior is not null) VerifyProxyState(bin, folder, prior);
            else if (File.Exists(Path.Combine(folder, "loader.ini"))) throw new IOException("存在无安装记录的配置，未覆盖。");
            var changes = new List<(string Path, byte[]? Bytes)>();
            var expected = new Dictionary<string, string?>();
            AddLegacyMigrations(game, changes, expected);
            string proxyPath = Path.Combine(bin, "winmm.dll");
            string originalHash = prior is null ? ExistingHash(proxyPath) ?? "" : prior["OriginalProxyHash"];
            if (prior is null && originalHash.Length != 0)
            {
                string backup = Path.Combine(folder, originalHash + ".winmm.bak");
                if (!File.Exists(backup)) File.Copy(proxyPath, backup);
                if (FileHash(backup) != originalHash) throw new IOException("原 winmm.dll 备份校验失败。");
            }
            string clientHash = Convert.ToHexString(SHA256.HashData(client));
            string clientName = "nova-client-proxy-" + clientHash + ".dll";
            byte[] config = new UnicodeEncoding(false, true).GetPreamble().Concat(Encoding.Unicode.GetBytes(
                "[Nova]\r\nEnabled=1\r\nGameDirectory=\"" + game + "\"\r\nClient=" + clientName + "\r\n")).ToArray();
            var state = new Dictionary<string, string> {
                ["Game"] = game, ["ProxyHash"] = Convert.ToHexString(SHA256.HashData(proxy)),
                ["ClientHash"] = clientHash, ["Client"] = clientName,
                ["ConfigHash"] = Convert.ToHexString(SHA256.HashData(config)), ["OriginalProxyHash"] = originalHash
            };
            if (prior is not null && state.All(x => prior.TryGetValue(x.Key, out var value) && value == x.Value) && changes.Count == 0)
                return "HeyPixel 启动代理已安装，所选 DLL 相同，无需重复修改。\nJava：" + bin;
            string payload = Path.Combine(folder, clientName);
            if (File.Exists(payload) && FileHash(payload) != clientHash) throw new IOException("同名客户端文件内容不一致。");
            if (!File.Exists(payload)) changes.Add((payload, client));
            changes.Add((Path.Combine(folder, "loader.ini"), config));
            changes.Add((manifest, JsonSerializer.SerializeToUtf8Bytes(state)));
            changes.Add((proxyPath, proxy));
            expected[payload] = ExistingHash(payload);
            expected[manifest] = manifestHash;
            expected[Path.Combine(folder, "loader.ini")] = prior is null ? null : prior["ConfigHash"];
            expected[proxyPath] = prior is null ? (originalHash.Length == 0 ? null : originalHash) : prior["ProxyHash"];
            CommitFiles(folder, changes.ToArray(), expected);
            return $"HeyPixel winmm 启动代理已安装。\nJava：{bin}\n客户端：{payload}\n原代理备份：{(originalHash.Length == 0 ? "原先不存在" : Path.Combine(folder, originalHash + ".winmm.bak"))}\n从网易启动器重新启动布吉岛后，Nova 在游戏接口就绪时初始化，不需要进入服务器。";
        }
    }

    internal string RestoreHeyPixel(string gameDirectory, string? javaDirectory = null)
    {
        RequireHeyPixelProfile();
        lock (HeyPixelLock)
        {
            string game = RequireGameDirectory(gameDirectory);
            RequireGameStopped(game);
            string bin = RequireJavaDirectory(game, javaDirectory);
            string folder = Path.Combine(bin, "nova-heypixel");
            Directory.CreateDirectory(folder);
            using var installLock = new FileStream(Path.Combine(folder, "install.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
            string manifest = Path.Combine(folder, "install.json");
            string? manifestHash = ExistingHash(manifest);
            var state = ReadProxyState(manifest, game);
            var changes = new List<(string Path, byte[]? Bytes)>();
            var expected = new Dictionary<string, string?>();
            AddLegacyMigrations(game, changes, expected);
            if (state is not null)
            {
                VerifyProxyState(bin, folder, state);
                byte[]? original = state["OriginalProxyHash"].Length == 0 ? null :
                    File.ReadAllBytes(Path.Combine(folder, state["OriginalProxyHash"] + ".winmm.bak"));
                changes.Add((Path.Combine(folder, "loader.ini"), null));
                changes.Add((Path.Combine(bin, "winmm.dll"), original));
                changes.Add((manifest, null));
                // Keep content-addressed payloads and backups for recovery; with
                // the proxy/config removed they cannot auto-load.
            }
            if (changes.Count == 0) return "没有已安装的 Nova HeyPixel 加载器，无需恢复。";
            expected[manifest] = manifestHash;
            expected[Path.Combine(folder, "loader.ini")] = state is null ? null : state["ConfigHash"];
            expected[Path.Combine(bin, "winmm.dll")] = state is null ? null : state["ProxyHash"];
            CommitFiles(folder, changes.ToArray(), expected);
            return "已移除 Nova 启动代理并恢复原先的 winmm.dll 状态；旧 JAR 补丁也已恢复。备份与客户端文件保留在：" + folder;
        }
    }

    internal static IReadOnlyList<string> FindHeyPixelDirectories()
    {
        var roots = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        // Prefer the launcher's configured download location, including custom drives.
        foreach (RegistryHive hive in new[] { RegistryHive.CurrentUser, RegistryHive.LocalMachine })
        foreach (RegistryView view in new[] { RegistryView.Registry64, RegistryView.Registry32 })
        {
            try
            {
                using RegistryKey key = RegistryKey.OpenBaseKey(hive, view);
                using RegistryKey? launcher = key.OpenSubKey(@"Software\Netease\MCLauncher");
                if (launcher?.GetValue("DownloadPath") is string download && !string.IsNullOrWhiteSpace(download))
                    roots.Add(Path.GetFullPath(Path.Combine(download, "Game", ".minecraft")));
            }
            catch (Exception ex) when (ex is System.Security.SecurityException or UnauthorizedAccessException or IOException or ArgumentException) { }
        }
        foreach (DriveInfo drive in DriveInfo.GetDrives())
        {
            if (drive.DriveType != DriveType.Fixed || !drive.IsReady) continue;
            roots.Add(Path.Combine(drive.RootDirectory.FullName, "MCLDownload", "Game", ".minecraft"));
        }
        return roots.Where(Directory.Exists).ToArray();
    }

    internal static IReadOnlyList<string> FindHeyPixelJars(string gameDirectory)
    {
        string mods = Path.Combine(Path.GetFullPath(gameDirectory), "mods");
        if (!Directory.Exists(mods)) return Array.Empty<string>();
        var matches = new List<string>();
        foreach (string path in Directory.EnumerateFiles(mods, "*.jar", SearchOption.TopDirectoryOnly))
        {
            try
            {
                using var jar = ZipFile.OpenRead(path);
                ZipArchiveEntry? entry = jar.GetEntry("META-INF/mods.toml");
                if (entry is null || entry.Length > 1024 * 1024) continue;
                using var reader = new StreamReader(entry.Open());
                // Restrict matching to [[mods]] blocks, not dependency declarations.
                foreach (string section in Regex.Split(reader.ReadToEnd(), @"(?m)^\s*\[\[mods\]\]\s*(?:#.*)?$" ).Skip(1))
                {
                    string mod = Regex.Split(section, @"(?m)^\s*\[")[0];
                    if (!Regex.IsMatch(mod, "(?m)^\\s*modId\\s*=\\s*[\"']heypixel[\"']\\s*(?:#.*)?$")) continue;
                    matches.Add(path);
                    break;
                }
            }
            catch (InvalidDataException) { /* Unrelated non-ZIP files are not mods. */ }
        }
        return matches;
    }

    private void RequireHeyPixelProfile()
    {
        if (!SupportsHeyPixel) throw new InvalidOperationException("HeyPixel 模式仅支持 Minecraft 1.20.1 (Forge)。");
    }

    private static string RequireGameDirectory(string directory)
    {
        if (string.IsNullOrWhiteSpace(directory)) throw new DirectoryNotFoundException("请选择网易游戏的 .minecraft 目录。");
        string game = Path.TrimEndingDirectorySeparator(Path.GetFullPath(directory));
        if (!Directory.Exists(game)) throw new DirectoryNotFoundException("游戏目录不存在：" + game);
        if (game.IndexOfAny(new[] { '\r', '\n', '"' }) >= 0) throw new ArgumentException("游戏目录包含不支持的字符。");
        return game;
    }

    private static void AddLegacyMigrations(string game, List<(string Path, byte[]? Bytes)> changes,
        Dictionary<string, string?> expected)
    {
        // JARs are optional: inspect any that exist only for old Nova patches.
        foreach (string jar in FindHeyPixelJars(game))
        {
            string hash = FileHash(jar);
            if (LegacyOriginal(jar, game) is not { } original) continue;
            changes.Add((jar, original));
            expected.Add(jar, hash);
        }
    }

    private static void RequireGameStopped(string directory)
    {
        string root = Path.GetFullPath(directory).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        foreach (string name in new[] { "java", "javaw" })
        foreach (Process process in Process.GetProcessesByName(name))
        {
            using (process)
            {
                bool loaded = false;
                try
                {
                    // Query limited information works for elevated Java without
                    // requesting module-memory access. The runtime-bin check
                    // blocks all games using that Java before replacing its DLL.
                    IntPtr handle = OpenProcess(0x1000, false, process.Id);
                    if (handle == IntPtr.Zero) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
                    try
                    {
                        var path = new StringBuilder(32768);
                        uint length = (uint)path.Capacity;
                        if (!QueryFullProcessImageName(handle, 0, path, ref length))
                            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
                        loaded = path.ToString().StartsWith(root, StringComparison.OrdinalIgnoreCase);
                    }
                    finally { CloseHandle(handle); }
                }
                catch (Exception ex) when (ex is System.ComponentModel.Win32Exception or InvalidOperationException or NotSupportedException)
                {
                    // Exited/protected unrelated Java processes cannot be
                    // inspected. File replacement still enforces Windows locks.
                }
                if (loaded) throw new IOException("请先退出此目录启动的 Minecraft，再应用或恢复 HeyPixel 模式。");
            }
        }
    }

    private static string BackupPath(string directory, string hash) =>
        Path.Combine(Path.GetFullPath(directory), "nova-backups", "heypixel", hash + ".jar.bak");

    private static byte[] ReadHeyPixelResource(string name)
    {
        using Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(name)
            ?? throw new InvalidOperationException("缺少 HeyPixel 预加载资源。");
        using var bytes = new MemoryStream();
        stream.CopyTo(bytes);
        return bytes.ToArray();
    }

    private static string ReadHeyPixelEntryName() =>
        System.Text.Encoding.UTF8.GetString(ReadHeyPixelResource("Nova.HeyPixel.Entry")).Trim('\uFEFF', '\r', '\n', ' ');

    private static void EnsureUniqueEntries(ZipArchive jar)
    {
        if (jar.Entries.Select(e => e.FullName).Distinct(StringComparer.Ordinal).Count() != jar.Entries.Count)
            throw new InvalidDataException("JAR 存在重复条目，未修改文件。");
    }

    private static string EntryHash(ZipArchiveEntry entry)
    {
        using Stream stream = entry.Open();
        return Hash(stream);
    }

    private static string FileHash(string path)
    {
        using Stream stream = File.OpenRead(path);
        return Hash(stream);
    }

    private static string Hash(Stream stream) => Convert.ToHexString(SHA256.HashData(stream));
}
