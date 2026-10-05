using Microsoft.UI.Xaml;

namespace NovaInjector;

public partial class App : Application
{
    public static Window? MainWindowInstance { get; private set; }

    public App() => InitializeComponent();

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        MainWindowInstance = new MainWindow();
        MainWindowInstance.Activate();
    }
}