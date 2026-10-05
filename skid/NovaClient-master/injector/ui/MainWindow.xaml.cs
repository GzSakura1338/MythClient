using Microsoft.UI.Composition.SystemBackdrops;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Windows.Graphics;
using Windows.UI.ViewManagement;
using WinRT.Interop;

namespace NovaInjector;

public sealed partial class MainWindow : Window
{
    private readonly UISettings uiSettings = new();
    private readonly AccessibilitySettings accessibilitySettings = new();
    private bool isClosed;
    private bool isRefreshing;
    private bool isBusy;
    private bool isDetectingHeyPixel;
    private bool isCs2;
    private bool HeyPixelEnabled => !isCs2 && ClientVersionBox.SelectedItem is ClientProfile { SupportsHeyPixel: true } && HeyPixelToggle.IsOn;
    private bool RemoteThreadEnabled => !isCs2 && !HeyPixelEnabled &&
        ClientVersionBox.SelectedItem is ClientProfile { SupportsHeyPixel: true } && RemoteThreadToggle.IsOn;

    [DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(IntPtr window);

    public MainWindow()
    {
        InitializeComponent();
        ClientVersionBox.ItemsSource = ClientProfile.Load().Where(profile => profile.Game == "minecraft").ToList();
        ClientVersionBox.SelectedIndex = 0;
        MinecraftTab.IsChecked = true;
        if (MicaController.IsSupported())
            SystemBackdrop = new MicaBackdrop();

        if (AppWindowTitleBar.IsCustomizationSupported())
        {
            ExtendsContentIntoTitleBar = true;
            AppWindow.TitleBar.PreferredHeightOption = TitleBarHeightOption.Tall;
            SetTitleBar(AppTitleBar);
        }
        else
            AppTitleBar.Visibility = Visibility.Collapsed;

        RootGrid.ActualThemeChanged += RootGrid_ActualThemeChanged;
        RootGrid.Loaded += RootGrid_Loaded;
        RootGrid.SizeChanged += RootGrid_SizeChanged;
        uiSettings.ColorValuesChanged += UISettings_ColorValuesChanged;
        Closed += MainWindow_Closed;
        ApplySystemTheme();
        // AppWindow sizes are physical pixels; keep the initial size consistent in DIPs.
        double scale = GetDpiForWindow(WindowNative.GetWindowHandle(this)) / 96.0;
        AppWindow.Resize(new SizeInt32((int)Math.Round(720 * scale), (int)Math.Round(580 * scale)));
        _ = RefreshProcessesAsync();
    }

    private void RootGrid_Loaded(object sender, RoutedEventArgs e)
    {
        UpdateWindowChrome();
        UpdateResponsiveLayout(RootGrid.ActualWidth);
    }

    private void RootGrid_SizeChanged(object sender, SizeChangedEventArgs e)
    {
        UpdateCaptionInset();
        UpdateResponsiveLayout(e.NewSize.Width);
    }

    private void UpdateResponsiveLayout(double width)
    {
        bool compact = width < 640;
        MainContent.Padding = compact ? new Thickness(16, 12, 16, 16) : new Thickness(20, 12, 20, 20);
        foreach (Button button in new[] { RefreshButton, PickDllButton })
        {
            Grid.SetRow(button, compact ? 1 : 0);
            Grid.SetColumn(button, compact ? 0 : 1);
            button.HorizontalAlignment = compact ? HorizontalAlignment.Stretch : HorizontalAlignment.Left;
        }
    }
    private void RootGrid_ActualThemeChanged(FrameworkElement sender, object args) => UpdateWindowChrome();
    private void UISettings_ColorValuesChanged(UISettings sender, object args) => QueueThemeUpdate();

    private void QueueThemeUpdate()
    {
        // Windows appearance notifications can arrive on a background thread.
        DispatcherQueue.TryEnqueue(() =>
        {
            if (!isClosed)
                ApplySystemTheme();
        });
    }

    private void ApplySystemTheme()
    {
        var background = uiSettings.GetColorValue(UIColorType.Background);
        RootGrid.RequestedTheme = 5 * background.G + 2 * background.R + background.B >= 8 * 128
            ? ElementTheme.Light : ElementTheme.Dark;
        UpdateWindowChrome();
    }

    private void UpdateWindowChrome()
    {
        RootGrid.Background = SystemBackdrop is not null && !accessibilitySettings.HighContrast
            ? new SolidColorBrush(Microsoft.UI.Colors.Transparent) : FallbackPalette.Background;
        if (!ExtendsContentIntoTitleBar)
            return;

        var titleBar = AppWindow.TitleBar;
        titleBar.ButtonBackgroundColor = Microsoft.UI.Colors.Transparent;
        titleBar.ButtonInactiveBackgroundColor = Microsoft.UI.Colors.Transparent;
        titleBar.ButtonForegroundColor = (TitleBarText.Foreground as SolidColorBrush)?.Color;
        titleBar.ButtonInactiveForegroundColor = (InactiveCaptionPalette.Foreground as SolidColorBrush)?.Color;
        titleBar.ButtonHoverForegroundColor = titleBar.ButtonForegroundColor;
        titleBar.ButtonPressedForegroundColor = titleBar.ButtonForegroundColor;
        titleBar.ButtonHoverBackgroundColor = (CaptionPalette.Background as SolidColorBrush)?.Color;
        titleBar.ButtonPressedBackgroundColor = (CaptionPalette.BorderBrush as SolidColorBrush)?.Color;
        UpdateCaptionInset();
    }

    private void UpdateCaptionInset()
    {
        if (ExtendsContentIntoTitleBar && RootGrid.XamlRoot is { } xamlRoot)
            CaptionInset.Width = new GridLength(AppWindow.TitleBar.RightInset / xamlRoot.RasterizationScale);
    }

    private void MainWindow_Closed(object sender, WindowEventArgs args)
    {
        isClosed = true;
        uiSettings.ColorValuesChanged -= UISettings_ColorValuesChanged;
        RootGrid.ActualThemeChanged -= RootGrid_ActualThemeChanged;
        RootGrid.Loaded -= RootGrid_Loaded;
        RootGrid.SizeChanged -= RootGrid_SizeChanged;
    }

    private async void RefreshProcesses_Click(object sender, RoutedEventArgs e) => await RefreshProcessesAsync();

    private async void GameTab_Click(object sender, RoutedEventArgs e)
    {
        bool nextCs2 = ReferenceEquals(sender, Cs2Tab);
        MinecraftTab.IsChecked = !nextCs2;
        Cs2Tab.IsChecked = nextCs2;
        if (isCs2 == nextCs2) return;
        isCs2 = nextCs2;
        ProcessBox.ItemsSource = null;
        ProcessBox.PlaceholderText = isCs2 ? "选择 CS2 进程" : "选择 Java 进程";
        Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(ProcessBox, isCs2 ? "目标 CS2 进程" : "目标 Java 进程");
        Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(RefreshButton, isCs2 ? "刷新 CS2 进程" : "刷新 Java 进程");
        ClientVersionPanel.Visibility = isCs2 ? Visibility.Collapsed : Visibility.Visible;
        Cs2Notice.Visibility = isCs2 ? Visibility.Visible : Visibility.Collapsed;
        HeyPixelOptions.Visibility = !isCs2 && ClientVersionBox.SelectedItem is ClientProfile { SupportsHeyPixel: true }
            ? Visibility.Visible : Visibility.Collapsed;
        StatusBar.IsOpen = false;
        UpdateHeyPixelMode();
        await RefreshProcessesAsync();
    }

    private async Task RefreshProcessesAsync()
    {
        if (isRefreshing || isClosed)
            return;
        isRefreshing = true;
        bool refreshingCs2 = isCs2;
        RefreshButton.IsEnabled = false;
        ProcessHint.Text = "正在读取进程…";
        uint dpi = GetDpiForWindow(WindowNative.GetWindowHandle(this));
        int iconSize = Math.Max(32, (int)Math.Ceiling(24 * dpi / 96.0));
        try
        {
            var snapshots = await Task.Run(() => ProcessSnapshot.ReadAll(iconSize, dpi, refreshingCs2));
            if (isClosed || refreshingCs2 != isCs2)
                return;
            // Read selection after lookup so a choice made while refreshing is preserved.
            int? selectedId = (ProcessBox.SelectedItem as JavaProcess)?.Id;
            var choices = snapshots.Select(snapshot => new JavaProcess(snapshot, iconSize)).ToList();
            ProcessBox.ItemsSource = choices;
            ProcessBox.SelectedItem = choices.FirstOrDefault(process => process.Id == selectedId);
            ProcessHint.Text = choices.Count == 0 ? (isCs2 ? "请先启动 CS2" : "请先启动 Minecraft")
                : $"{choices.Count} 个 {(isCs2 ? "CS2" : "Java")} 进程";
        }
        catch (Exception ex)
        {
            if (!isClosed)
            {
                ProcessHint.Text = "进程读取失败";
                ShowStatus("无法刷新进程", ex.Message, InfoBarSeverity.Warning);
            }
        }
        finally
        {
            isRefreshing = false;
            if (!isClosed)
                RefreshButton.IsEnabled = ProcessBox.IsEnabled;
            if (!isClosed && refreshingCs2 != isCs2)
                await RefreshProcessesAsync();
        }
    }

    private void PickDll_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            string? path = FileDialog.PickDll(WindowNative.GetWindowHandle(this), DllPathBox.Text);
            if (path is not null && !isClosed)
            {
                DllPathBox.Text = path;
                StatusBar.IsOpen = false;
            }
        }
        catch (Exception ex)
        {
            if (!isClosed)
                ShowStatus("无法选择文件", ex.Message, InfoBarSeverity.Error);
        }
    }

    private void ClientVersion_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        // Selection can change during XAML initialization, before later controls exist.
        if (HeyPixelOptions is null || PatchButtonText is null) return;
        bool supported = !isCs2 && ClientVersionBox.SelectedItem is ClientProfile { SupportsHeyPixel: true };
        HeyPixelOptions.Visibility = supported ? Visibility.Visible : Visibility.Collapsed;
        if (!supported)
        {
            HeyPixelToggle.IsOn = false;
            RemoteThreadToggle.IsOn = false;
        }
        UpdateHeyPixelMode();
    }

    private async void HeyPixel_Toggled(object sender, RoutedEventArgs e)
    {
        if (HeyPixelDirectoryPanel is null || PatchButtonText is null) return;
        if (HeyPixelToggle.IsOn) RemoteThreadToggle.IsOn = false;
        UpdateHeyPixelMode();
        StatusBar.IsOpen = false;
        if (HeyPixelEnabled && string.IsNullOrWhiteSpace(HeyPixelDirectoryBox.Text))
            await DetectHeyPixelAsync();
    }

    private void UpdateHeyPixelMode()
    {
        ProcessPanel.Visibility = HeyPixelEnabled ? Visibility.Collapsed : Visibility.Visible;
        HeyPixelDirectoryPanel.Visibility = HeyPixelEnabled ? Visibility.Visible : Visibility.Collapsed;
        SetBusy(isBusy);
    }

    private void RemoteThread_Toggled(object sender, RoutedEventArgs e)
    {
        if (HeyPixelDirectoryPanel is null || PatchButtonText is null) return;
        if (RemoteThreadToggle.IsOn) HeyPixelToggle.IsOn = false;
        UpdateHeyPixelMode();
        StatusBar.IsOpen = false;
    }

    private async void DetectHeyPixel_Click(object sender, RoutedEventArgs e) => await DetectHeyPixelAsync();

    private async Task DetectHeyPixelAsync()
    {
        if (isDetectingHeyPixel || isBusy) return;
        isDetectingHeyPixel = true;
        string initialPath = HeyPixelDirectoryBox.Text;
        DetectHeyPixelButton.IsEnabled = false;
        try
        {
            var directories = await Task.Run(ClientProfile.FindHeyPixelDirectories);
            if (isClosed || !HeyPixelEnabled || HeyPixelDirectoryBox.Text != initialPath || isBusy) return;
            if (directories.Count == 1)
            {
                HeyPixelDirectoryBox.Text = directories[0];
                ShowStatus("已识别布吉岛目录", "选择 Nova DLL 后，点击“应用 HeyPixel 模式”。", InfoBarSeverity.Informational);
            }
            else
            {
                OutputBox.Text = string.Join("\n", directories);
                ShowStatus(directories.Count == 0 ? "未找到布吉岛目录" : "找到多个布吉岛目录",
                    "请用“选择目录”指定要修改的 .minecraft 目录。", InfoBarSeverity.Warning);
            }
        }
        catch (Exception ex)
        {
            if (!isClosed) ShowStatus("自动识别失败", ex.Message, InfoBarSeverity.Warning);
        }
        finally
        {
            isDetectingHeyPixel = false;
            if (!isClosed) DetectHeyPixelButton.IsEnabled = !isBusy;
        }
    }

    private void PickGameDirectory_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            string? path = FileDialog.PickGameDirectory(WindowNative.GetWindowHandle(this), HeyPixelDirectoryBox.Text);
            if (path is not null) HeyPixelDirectoryBox.Text = path;
        }
        catch (Exception ex) { ShowStatus("无法选择目录", ex.Message, InfoBarSeverity.Error); }
    }

    private async void RestoreHeyPixel_Click(object sender, RoutedEventArgs e) => await RunHeyPixelAsync(restore: true);

    private async Task RunHeyPixelAsync(bool restore)
    {
        if (isBusy || !HeyPixelEnabled || ClientVersionBox.SelectedItem is not ClientProfile profile) return;
        string directory = HeyPixelDirectoryBox.Text.Trim();
        string javaDirectory = HeyPixelJavaBox.Text.Trim();
        string dll = DllPathBox.Text.Trim();
        if (!restore && !File.Exists(dll))
        {
            ShowStatus("请选择 Nova DLL", "使用“浏览”选择 Windows x64 版 Nova 客户端 DLL。", InfoBarSeverity.Warning);
            return;
        }
        SetBusy(true);
        ShowStatus(restore ? "正在恢复原版" : "正在应用 HeyPixel 模式", "正在校验 Java、客户端和代理备份…", InfoBarSeverity.Informational);
        OutputBox.Text = string.Empty;
        try
        {
            string result = await Task.Run(() => restore ? profile.RestoreHeyPixel(directory, javaDirectory) : profile.ApplyHeyPixel(directory, dll, javaDirectory));
            if (!isClosed)
            {
                OutputBox.Text = result;
                ShowStatus(restore ? "原版已恢复" : "HeyPixel 模式已应用", "下次从网易启动器启动布吉岛时生效。", InfoBarSeverity.Success);
            }
        }
        catch (Exception ex)
        {
            if (!isClosed)
            {
                OutputBox.Text = ex.Message;
                ShowStatus(restore ? "恢复失败" : "应用失败", ex.Message, InfoBarSeverity.Error);
            }
        }
        finally { if (!isClosed) SetBusy(false); }
    }

    private async void Patch_Click(object sender, RoutedEventArgs e)
    {
        if (HeyPixelEnabled)
        {
            await RunHeyPixelAsync(restore: false);
            return;
        }
        ClientProfile? profile = isCs2 ? ClientProfile.Load().FirstOrDefault(item => item.Game == "cs2")
            : ClientVersionBox.SelectedItem as ClientProfile;
        if (profile is null)
        {
            ShowStatus("请选择客户端版本", "选择与目标进程一致的客户端版本。", InfoBarSeverity.Warning);
            ClientVersionBox.Focus(FocusState.Programmatic);
            return;
        }
        if (ProcessBox.SelectedItem is not JavaProcess process)
        {
            ShowStatus("请选择目标进程", isCs2 ? "从列表中选择 cs2.exe。" : "从列表中选择 Minecraft 对应的 Java 进程。", InfoBarSeverity.Warning);
            ProcessBox.Focus(FocusState.Programmatic);
            return;
        }
        if (!File.Exists(DllPathBox.Text))
        {
            ShowStatus("请选择注入 DLL", "使用“浏览”选择一个有效的 DLL 文件。", InfoBarSeverity.Warning);
            PickDllButton.Focus(FocusState.Programmatic);
            return;
        }
        string injector = Path.Combine(AppContext.BaseDirectory, "NovaInjector.exe");
        if (!File.Exists(injector))
        {
            ShowStatus("缺少注入器", "未找到 NovaInjector.exe，请将注入器放到程序目录后重试。", InfoBarSeverity.Error);
            OutputBox.Text = $"找不到 {injector}\n请先编译 injector\\cli.c。";
            return;
        }

        SetBusy(true);
        bool remoteThread = RemoteThreadEnabled;
        ShowStatus("正在注入", $"正在向 {process.Name}（PID {process.Id}）注入 DLL…", InfoBarSeverity.Informational);
        OutputBox.Text = string.Empty;
        try
        {
            await Task.Run(BrowserRuntime.Ensure);
            var startInfo = new ProcessStartInfo(injector)
            {
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true
            };
            startInfo.ArgumentList.Add(process.Id.ToString(CultureInfo.InvariantCulture));
            startInfo.ArgumentList.Add(DllPathBox.Text);
            startInfo.ArgumentList.Add(profile.Key);
            if (remoteThread) startInfo.ArgumentList.Add("remote-thread");
            using var child = Process.Start(startInfo) ?? throw new InvalidOperationException("无法启动注入器。");
            Task<string> stdout = child.StandardOutput.ReadToEndAsync();
            Task<string> stderr = child.StandardError.ReadToEndAsync();
            await child.WaitForExitAsync();
            string[] output = await Task.WhenAll(stdout, stderr);
            if (!isClosed)
            {
                OutputBox.Text = $"退出码: {child.ExitCode}\n{output[0]}{output[1]}";
                ShowStatus(child.ExitCode == 0 ? "注入已完成" : "注入失败",
                    child.ExitCode == 0 ? $"{profile.Label} 运行时已加载，界面会在游戏开始绘制后初始化。" : "请查看运行日志中的详细信息。",
                    child.ExitCode == 0 ? InfoBarSeverity.Success : InfoBarSeverity.Error);
            }
        }
        catch (Exception ex)
        {
            if (!isClosed)
            {
                OutputBox.Text = ex.Message;
                ShowStatus("加载失败", ex.Message, InfoBarSeverity.Error);
            }
        }
        finally
        {
            if (!isClosed)
                SetBusy(false);
        }
    }

    private void ShowStatus(string title, string message, InfoBarSeverity severity)
    {
        StatusBar.Title = title;
        StatusBar.Message = message;
        StatusBar.Severity = severity;
        StatusBar.IsOpen = true;
    }

    private void SetBusy(bool busy)
    {
        isBusy = busy;
        PatchButton.IsEnabled = !busy;
        MinecraftTab.IsEnabled = !busy;
        Cs2Tab.IsEnabled = !busy;
        ProcessBox.IsEnabled = !busy;
        ClientVersionBox.IsEnabled = !busy;
        RefreshButton.IsEnabled = !busy && !isRefreshing;
        PickDllButton.IsEnabled = !busy;
        DllPathBox.IsEnabled = !busy;
        HeyPixelToggle.IsEnabled = !busy;
        RemoteThreadToggle.IsEnabled = !busy;
        HeyPixelDirectoryBox.IsEnabled = !busy;
        HeyPixelJavaBox.IsEnabled = !busy;
        DetectHeyPixelButton.IsEnabled = !busy && !isDetectingHeyPixel;
        PickGameDirectoryButton.IsEnabled = !busy;
        RestoreHeyPixelButton.IsEnabled = !busy;
        BusyRing.IsActive = busy;
        BusyRing.Visibility = busy ? Visibility.Visible : Visibility.Collapsed;
        PatchButtonText.Text = busy ? "正在处理…" : HeyPixelEnabled ? "应用 HeyPixel 模式" : "注入";
        Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(PatchButton, PatchButtonText.Text);
        StatusBar.IsClosable = !busy;
    }
}
