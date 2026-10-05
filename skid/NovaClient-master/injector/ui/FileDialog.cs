using System;
using System.IO;
using System.Runtime.InteropServices;

namespace NovaInjector;

/// <summary>
/// Win32 common item dialog. The WinRT FileOpenPicker is hosted by a
/// medium-integrity broker, which cannot hand a selection back to an elevated
/// process, so the injector (which always runs elevated) uses
/// IFileOpenDialog instead: it runs in-process and works at any integrity level.
/// </summary>
internal static class FileDialog
{
    private const uint FOS_NOCHANGEDIR     = 0x00000008;
    private const uint FOS_PICKFOLDERS     = 0x00000020;
    private const uint FOS_FORCEFILESYSTEM = 0x00000040;
    private const uint FOS_PATHMUSTEXIST   = 0x00000800;
    private const uint FOS_FILEMUSTEXIST   = 0x00001000;
    private const uint SIGDN_FILESYSPATH   = 0x80058000;
    private const int  ERROR_CANCELLED     = unchecked((int)0x800704C7);

    private static readonly Guid FileOpenDialogClsid = new("DC1C5A9C-E88A-4dde-A5A1-60F82A20AEF7");

    /// <summary>
    /// Prompts for a DLL, starting in the folder of <paramref name="currentPath"/>
    /// when it is usable. Returns null if the user cancels.
    /// </summary>
    internal static string? PickDll(IntPtr owner, string? currentPath)
    {
        string? startIn = null;
        if (!string.IsNullOrWhiteSpace(currentPath))
        {
            try
            {
                startIn = Path.GetDirectoryName(currentPath);
            }
            catch (ArgumentException) { /* Malformed path: keep the default folder. */ }
        }

        return PickFile(owner, "选择注入 DLL", startIn, false,
                        ("DLL 文件", "*.dll"), ("所有文件", "*.*"));
    }

    internal static string? PickGameDirectory(IntPtr owner, string? currentPath) =>
        PickFile(owner, "选择网易游戏的 .minecraft 目录", currentPath, true);

    private static string? PickFile(IntPtr owner, string title, string? startIn, bool pickFolder,
                                    params (string Name, string Spec)[] filters)
    {
        Type? comType = Type.GetTypeFromCLSID(FileOpenDialogClsid);
        if (comType is null)
            throw new InvalidOperationException("FileOpenDialog is not registered on this system.");

        var dialog = (IFileOpenDialog)Activator.CreateInstance(comType)!;
        IntPtr filterBuffer = IntPtr.Zero;
        int filterSize = 0;

        try
        {
            dialog.GetOptions(out uint options);
            dialog.SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST |
                                       (pickFolder ? FOS_PICKFOLDERS : FOS_FILEMUSTEXIST) | FOS_NOCHANGEDIR);
            dialog.SetTitle(title);
            if (filters.Length > 0)
            {
                filterSize = Marshal.SizeOf<FilterSpec>();
                filterBuffer = Marshal.AllocCoTaskMem(filterSize * filters.Length);
                for (int i = 0; i < filters.Length; i++)
                {
                    Marshal.StructureToPtr(new FilterSpec(filters[i].Name, filters[i].Spec),
                                           IntPtr.Add(filterBuffer, i * filterSize), false);
                }
                dialog.SetFileTypes((uint)filters.Length, filterBuffer);
            }
            SetStartFolder(dialog, startIn);

            int hr = dialog.Show(owner);
            if (hr == ERROR_CANCELLED)
                return null;
            if (hr != 0)
                Marshal.ThrowExceptionForHR(hr);

            dialog.GetResult(out IShellItem item);
            try
            {
                item.GetDisplayName(SIGDN_FILESYSPATH, out string path);
                return path;
            }
            finally
            {
                Marshal.ReleaseComObject(item);
            }
        }
        finally
        {
            // The dialog reads the filter array for as long as it is open, so it
            // is only released once Show has returned.
            for (int i = 0; filterBuffer != IntPtr.Zero && i < filters.Length; i++)
                Marshal.DestroyStructure<FilterSpec>(IntPtr.Add(filterBuffer, i * filterSize));
            if (filterBuffer != IntPtr.Zero)
                Marshal.FreeCoTaskMem(filterBuffer);
            Marshal.ReleaseComObject(dialog);
        }
    }

    private static void SetStartFolder(IFileOpenDialog dialog, string? folder)
    {
        if (string.IsNullOrWhiteSpace(folder) || !Directory.Exists(folder))
            return;

        Guid iid = new("43826d1e-e718-42ee-bc55-a1e261c37bfe");
        try
        {
            SHCreateItemFromParsingName(folder, IntPtr.Zero, ref iid, out IShellItem item);
            try
            {
                dialog.SetFolder(item);
            }
            finally
            {
                Marshal.ReleaseComObject(item);
            }
        }
        catch (COMException) { /* Unreadable folder: keep the default one. */ }
    }

    [DllImport("shell32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, PreserveSig = false)]
    private static extern void SHCreateItemFromParsingName(
        string pszPath, IntPtr pbc, ref Guid riid,
        [MarshalAs(UnmanagedType.Interface)] out IShellItem ppv);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct FilterSpec
    {
        [MarshalAs(UnmanagedType.LPWStr)] public string Name;
        [MarshalAs(UnmanagedType.LPWStr)] public string Spec;

        public FilterSpec(string name, string spec)
        {
            Name = name;
            Spec = spec;
        }
    }

    [ComImport, Guid("43826d1e-e718-42ee-bc55-a1e261c37bfe"),
     InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IShellItem
    {
        void BindToHandler(IntPtr pbc, IntPtr bhid, IntPtr riid, out IntPtr ppv);
        void GetParent(out IShellItem ppsi);
        void GetDisplayName(uint sigdnName, [MarshalAs(UnmanagedType.LPWStr)] out string ppszName);
        void GetAttributes(uint sfgaoMask, out uint psfgaoAttribs);
        void Compare(IShellItem psi, uint hint, out int piOrder);
    }

    // IFileOpenDialog derives from IFileDialog, but inherited members are not
    // reliable in COM interop vtables, so the base methods are declared inline
    // here in declaration order to keep the slots aligned.
    [ComImport, Guid("d57c7288-d4ad-4768-be02-9d969532d960"),
     InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IFileOpenDialog
    {
        [PreserveSig] int Show(IntPtr parent);
        void SetFileTypes(uint cFileTypes, IntPtr rgFilterSpec);
        void SetFileTypeIndex(uint iFileType);
        void GetFileTypeIndex(out uint piFileType);
        void Advise(IntPtr pfde, out uint pdwCookie);
        void Unadvise(uint dwCookie);
        void SetOptions(uint fos);
        void GetOptions(out uint pfos);
        void SetDefaultFolder(IShellItem psi);
        void SetFolder(IShellItem psi);
        void GetFolder(out IShellItem ppsi);
        void GetCurrentSelection(out IShellItem ppsi);
        void SetFileName([MarshalAs(UnmanagedType.LPWStr)] string pszName);
        void GetFileName([MarshalAs(UnmanagedType.LPWStr)] out string pszName);
        void SetTitle([MarshalAs(UnmanagedType.LPWStr)] string pszTitle);
        void SetOkButtonLabel([MarshalAs(UnmanagedType.LPWStr)] string pszText);
        void SetFileNameLabel([MarshalAs(UnmanagedType.LPWStr)] string pszLabel);
        void GetResult(out IShellItem ppsi);
        void AddPlace(IShellItem psi, int fdap);
        void SetDefaultExtension([MarshalAs(UnmanagedType.LPWStr)] string pszDefaultExtension);
        void Close([MarshalAs(UnmanagedType.Error)] int hr);
        void SetClientGuid(IntPtr guid);
        void ClearClientData();
        void SetFilter(IntPtr pFilter);
        void GetResults(out IntPtr ppenum);
        void GetSelectedItems(out IntPtr ppsai);
    }
}
