using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text.RegularExpressions;

namespace NovaInjector;

public sealed partial record ClientProfile(uint Id, string Key, string Label, string Game = "minecraft")
{
    public bool SupportsHeyPixel => Key == "forge-1.20.1";

    public override string ToString() => Label;

    public static IReadOnlyList<ClientProfile> Load()
    {
        using Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("Nova.ClientProfiles")
            ?? throw new InvalidOperationException("缺少客户端版本目录。");
        using var reader = new StreamReader(stream);
        var profiles = new List<ClientProfile>();
        var pattern = new Regex("^NOVA_PROFILE\\(\\w+,\\s*(\\d+),\\s*\"([^\"]+)\",\\s*\"([^\"]+)\",\\s*(\\w+)\\)$");
        while (reader.ReadLine() is string line)
        {
            if (string.IsNullOrWhiteSpace(line)) continue;
            Match match = pattern.Match(line.Trim());
            if (!match.Success) throw new InvalidOperationException("客户端版本目录格式错误。");
            profiles.Add(new ClientProfile(uint.Parse(match.Groups[1].Value), match.Groups[2].Value, match.Groups[3].Value, match.Groups[4].Value));
        }
        if (profiles.Count == 0) throw new InvalidOperationException("客户端版本目录为空。");
        return profiles;
    }
}
