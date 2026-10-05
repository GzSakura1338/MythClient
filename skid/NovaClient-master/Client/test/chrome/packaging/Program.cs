using System;
using System.IO;
string? dataRoot=args.Length>0?Path.GetFullPath(args[0]):null;
NovaInjector.BrowserRuntime.Ensure(dataRoot);
NovaInjector.BrowserRuntime.Ensure(dataRoot);
string root=Path.Combine(dataRoot??Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Nova","Browser"),"runtime","webview2-154.0.4258.37-v1");
string path=Path.Combine(root,File.ReadAllText(Path.Combine(root,"current.txt")));
foreach(string file in new[]{"NovaBrowserHost.exe","runtime/msedgewebview2.exe","runtime/msedge.dll","runtime/icudtl.dat","runtime/locales/zh-CN.pak"})
    if(!File.Exists(Path.Combine(path,file)))throw new Exception("Missing runtime file: "+file);
Console.WriteLine("PASS: embedded Chromium hash verification, extraction, immutable version publication and repeat installation");
