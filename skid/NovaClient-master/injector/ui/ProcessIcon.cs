using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace NovaInjector;

internal static class ProcessIcon
{
    // Return pixels rather than XAML objects so native icon lookup can run off the UI thread.
    internal static byte[]? Read(IntPtr window, string? executablePath, int size, uint dpi)
    {
        IntPtr icon = IntPtr.Zero;
        if (window != IntPtr.Zero)
        {
            // WM_GETICON returns a borrowed handle. A hung window must not stall a refresh.
            foreach (int kind in new[] { 1, 2, 0 })
            {
                if (SendMessageTimeout(window, 0x007F, (UIntPtr)kind, (IntPtr)dpi,
                    0x0002, 100, out UIntPtr result) != IntPtr.Zero && result != UIntPtr.Zero)
                    icon = CopyIcon((IntPtr)result);
                if (icon != IntPtr.Zero)
                    break;
            }
            if (icon == IntPtr.Zero)
                icon = CopyIcon(GetClassLongPtr(window, -14)); // GCLP_HICON
            if (icon == IntPtr.Zero)
                icon = CopyIcon(GetClassLongPtr(window, -34)); // GCLP_HICONSM
        }
        if (icon == IntPtr.Zero && !string.IsNullOrEmpty(executablePath))
        {
            if (SHGetFileInfo(executablePath, 0, out ShellFileInfo info,
                (uint)Marshal.SizeOf<ShellFileInfo>(), 0x00000100) != IntPtr.Zero)
                icon = info.Icon;
        }
        if (icon == IntPtr.Zero)
            return null;
        try { return Render(icon, size); }
        finally { DestroyIcon(icon); }
    }

    private static byte[]? Render(IntPtr icon, int size)
    {
        var info = new BitmapInfo
        {
            Size = (uint)Marshal.SizeOf<BitmapInfo>(),
            Width = size,
            Height = -size,
            Planes = 1,
            BitCount = 32
        };
        IntPtr dc = CreateCompatibleDC(IntPtr.Zero);
        if (dc == IntPtr.Zero)
            return null;
        IntPtr bitmap = IntPtr.Zero;
        IntPtr previous = IntPtr.Zero;
        try
        {
            bitmap = CreateDIBSection(dc, ref info, 0, out IntPtr bits, IntPtr.Zero, 0);
            if (bitmap == IntPtr.Zero)
                return null;
            previous = SelectObject(dc, bitmap);
            var pixels = new byte[size * size * 4];
            Marshal.Copy(pixels, 0, bits, pixels.Length);
            if (!DrawIconEx(dc, 0, 0, icon, size, size, 0, IntPtr.Zero, 3))
                return null;
            GdiFlush();
            Marshal.Copy(bits, pixels, 0, pixels.Length);

            // Legacy masked icons have no alpha channel. Compare black and white
            // backgrounds to preserve their transparent edges in either Windows theme.
            if (!Enumerable.Range(0, size * size).Any(index => pixels[index * 4 + 3] != 0))
            {
                var white = new byte[pixels.Length];
                Array.Fill(white, (byte)255);
                Marshal.Copy(white, 0, bits, white.Length);
                if (!DrawIconEx(dc, 0, 0, icon, size, size, 0, IntPtr.Zero, 3))
                    return null;
                GdiFlush();
                Marshal.Copy(bits, white, 0, white.Length);
                for (int i = 0; i < pixels.Length; i += 4)
                {
                    int transparency = Math.Max(white[i] - pixels[i],
                        Math.Max(white[i + 1] - pixels[i + 1], white[i + 2] - pixels[i + 2]));
                    pixels[i + 3] = (byte)Math.Clamp(255 - transparency, 0, 255);
                }
            }
            return pixels;
        }
        finally
        {
            if (previous != IntPtr.Zero)
                SelectObject(dc, previous);
            if (bitmap != IntPtr.Zero)
                DeleteObject(bitmap);
            DeleteDC(dc);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct BitmapInfo
    {
        public uint Size;
        public int Width, Height;
        public ushort Planes, BitCount;
        public uint Compression, SizeImage;
        public int XPelsPerMeter, YPelsPerMeter;
        public uint ClrUsed, ClrImportant;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct ShellFileInfo
    {
        public IntPtr Icon;
        public int IconIndex;
        public uint Attributes;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)] public string DisplayName;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 80)] public string TypeName;
    }

    [DllImport("user32.dll", EntryPoint = "SendMessageTimeoutW")]
    private static extern IntPtr SendMessageTimeout(IntPtr window, uint message, UIntPtr wParam,
        IntPtr lParam, uint flags, uint timeout, out UIntPtr result);
    [DllImport("user32.dll", EntryPoint = "GetClassLongPtrW")]
    private static extern IntPtr GetClassLongPtr(IntPtr window, int index);
    [DllImport("user32.dll")] private static extern IntPtr CopyIcon(IntPtr icon);
    [DllImport("user32.dll")] private static extern bool DestroyIcon(IntPtr icon);
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    private static extern IntPtr SHGetFileInfo(string path, uint attributes, out ShellFileInfo info, uint size, uint flags);
    [DllImport("gdi32.dll")] private static extern IntPtr CreateCompatibleDC(IntPtr dc);
    [DllImport("gdi32.dll")]
    private static extern IntPtr CreateDIBSection(IntPtr dc, ref BitmapInfo info, uint usage,
        out IntPtr bits, IntPtr section, uint offset);
    [DllImport("gdi32.dll")] private static extern IntPtr SelectObject(IntPtr dc, IntPtr value);
    [DllImport("gdi32.dll")] private static extern bool DeleteObject(IntPtr value);
    [DllImport("gdi32.dll")] private static extern bool DeleteDC(IntPtr dc);
    [DllImport("gdi32.dll")] private static extern bool GdiFlush();
    [DllImport("user32.dll")]
    private static extern bool DrawIconEx(IntPtr dc, int x, int y, IntPtr icon, int width, int height,
        uint step, IntPtr background, uint flags);
}
