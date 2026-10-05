using System;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;

namespace NovaInjector;

internal static class BrowserRuntime
{
    // Immutable directories permit an older browser to keep running during an
    // injector update. The pointer is published only after verified extraction.
    internal static void Ensure() => Ensure(null);

    internal static void Ensure(string? dataRoot)
    {
        var assembly = Assembly.GetExecutingAssembly();
        using var hashStream = assembly.GetManifestResourceStream("Nova.Browser.Hash")
            ?? throw new InvalidOperationException("缺少浏览器运行时校验文件。");
        using var reader = new StreamReader(hashStream);
        string expected = reader.ReadToEnd().Trim().ToLowerInvariant();
        if (expected.Length != 64) throw new InvalidDataException("浏览器校验格式不正确。");
        string root = Path.Combine(dataRoot ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "Nova", "Browser"), "runtime", "webview2-154.0.4258.37-v1");
        Directory.CreateDirectory(root);
        string version = expected[..16];
        string target = Path.Combine(root, version);
        if (!File.Exists(Path.Combine(target, "complete.sha256")) ||
            File.ReadAllText(Path.Combine(target, "complete.sha256")).Trim() != expected ||
            !File.Exists(Path.Combine(target, "NovaBrowserHost.exe")) || !File.Exists(Path.Combine(target, "runtime", "msedgewebview2.exe")))
        {
            using var resource = assembly.GetManifestResourceStream("Nova.Browser.Runtime")
                ?? throw new InvalidOperationException("缺少自带 WebView2 固定版运行时。");
            // Resource streams are seekable; hash before unpacking executable code.
            string actual = Convert.ToHexString(SHA256.HashData(resource)).ToLowerInvariant();
            if (actual != expected) throw new InvalidDataException("浏览器运行时校验失败。");
            resource.Position = 0;
            string temporary = Path.Combine(root, "prepare-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(temporary);
            try
            {
                using (var zip = new ZipArchive(resource, ZipArchiveMode.Read, leaveOpen: true))
                    zip.ExtractToDirectory(temporary);
                if (!File.Exists(Path.Combine(temporary, "NovaBrowserHost.exe")) || !File.Exists(Path.Combine(temporary, "runtime", "msedgewebview2.exe")))
                    throw new InvalidDataException("浏览器包不完整。");
                File.WriteAllText(Path.Combine(temporary, "complete.sha256"), expected);
                if (!Directory.Exists(target)) Directory.Move(temporary, target);
                else if (!File.Exists(Path.Combine(target, "complete.sha256")))
                    throw new IOException("浏览器版本目录不完整，请关闭浏览器后清理该版本目录。");
            }
            finally { if (Directory.Exists(temporary)) Directory.Delete(temporary, recursive: true); }
        }
        string pointer = Path.Combine(root, "current-" + Guid.NewGuid().ToString("N") + ".tmp");
        File.WriteAllText(pointer, version);
        File.Move(pointer, Path.Combine(root, "current.txt"), overwrite: true);
    }
}
