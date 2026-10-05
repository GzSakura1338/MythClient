using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;

namespace NovaInjector;

public sealed class JavaProcess
{
    public int Id { get; }
    public string Name { get; }
    public string DisplayName { get; }
    public string Details => $"{Name}.exe  ·  PID {Id}";
    public string ToolTip { get; }
    public ImageSource? Icon { get; }
    public Visibility IconVisibility => Icon is null ? Visibility.Collapsed : Visibility.Visible;
    public Visibility FallbackIconVisibility => Icon is null ? Visibility.Visible : Visibility.Collapsed;

    internal JavaProcess(ProcessSnapshot snapshot, int iconSize)
    {
        Id = snapshot.Id;
        Name = snapshot.Name;
        DisplayName = snapshot.DisplayName;
        ToolTip = string.IsNullOrEmpty(snapshot.ExecutablePath)
            ? $"{DisplayName}\n{Details}" : $"{DisplayName}\n{Details}\n{snapshot.ExecutablePath}";
        if (snapshot.IconPixels is { } pixels)
        {
            var bitmap = new WriteableBitmap(iconSize, iconSize);
            using Stream stream = bitmap.PixelBuffer.AsStream();
            stream.Write(pixels, 0, pixels.Length);
            bitmap.Invalidate();
            Icon = bitmap;
        }
    }

    public override string ToString() => $"{DisplayName}  ·  {Details}";
}

internal sealed record ProcessSnapshot(int Id, string Name, string DisplayName,
    string? ExecutablePath, byte[]? IconPixels)
{
    internal static List<ProcessSnapshot> ReadAll(int iconSize, uint dpi, bool cs2 = false)
    {
        var choices = new List<ProcessSnapshot>();
        foreach (Process process in cs2 ? Process.GetProcessesByName("cs2")
            : Process.GetProcessesByName("java").Concat(Process.GetProcessesByName("javaw")))
        {
            using (process)
            {
                try
                {
                    if (process.HasExited)
                        continue;
                    int id = process.Id;
                    string name = process.ProcessName;
                    string title = process.MainWindowTitle.Trim();
                    IntPtr window = process.MainWindowHandle;
                    string? path = null;
                    string? description = null;
                    try
                    {
                        path = process.MainModule?.FileName;
                        if (string.IsNullOrWhiteSpace(title) && !string.IsNullOrEmpty(path))
                            description = FileVersionInfo.GetVersionInfo(path).FileDescription;
                    }
                    catch (Win32Exception) { /* Retain processes whose executable metadata is inaccessible. */ }
                    catch (InvalidOperationException) { }
                    catch (IOException) { }
                    catch (UnauthorizedAccessException) { }

                    string displayName = !string.IsNullOrWhiteSpace(title) ? title
                        : !string.IsNullOrWhiteSpace(description) ? description.Trim() : $"{name}.exe";
                    choices.Add(new ProcessSnapshot(id, name, displayName, path,
                        ProcessIcon.Read(window, path, iconSize, dpi)));
                }
                catch (InvalidOperationException) { /* Process exited during refresh. */ }
                catch (Win32Exception) { /* Process status is inaccessible. */ }
            }
        }
        return choices.OrderBy(process => process.Id).ToList();
    }
}
