param([string]$Executable, [string]$ExpectedHeyPixelDirectory)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = if ($Executable) { [IO.Path]::GetFullPath($Executable) } else { Join-Path $repo 'build/Injector.WinUI.exe' }
if (!(Test-Path -LiteralPath $source -PathType Leaf)) { throw 'Published injector not found' }
# Only this EXE is copied below; unrelated DLLs/older builds beside the source
# must not invalidate the single-file isolation test.
$folder = Join-Path $env:TEMP ('nova-injector-smoke-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $folder | Out-Null
$exe = Join-Path $folder 'Injector.WinUI.exe'
Copy-Item -LiteralPath $source -Destination $exe
$start = [Diagnostics.ProcessStartInfo]::new($exe)
$start.UseShellExecute = $false
$start.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
$start.WorkingDirectory = $folder
# This test only opens the UI and invokes CLI usage. It never injects, so no
# elevation is needed. The distributed executable retains its normal manifest.
$start.EnvironmentVariables['__COMPAT_LAYER'] = 'RunAsInvoker'
$start.EnvironmentVariables['DOTNET_BUNDLE_EXTRACT_BASE_DIR'] = Join-Path $folder 'cache'
$process = [Diagnostics.Process]::Start($start)
try {
    $null = $process.WaitForInputIdle(20000)
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        $process.Refresh()
        if ($process.HasExited) { throw "Standalone UI exited: $($process.ExitCode)" }
        if ($process.MainWindowHandle -ne 0) { break }
        Start-Sleep -Milliseconds 200
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($process.MainWindowHandle -eq 0) { throw 'Standalone UI did not create a window' }
    $helper = @(Get-ChildItem -LiteralPath (Join-Path $folder 'cache') -Filter NovaInjector.exe -Recurse)
    if ($helper.Count -ne 1) { throw 'Embedded native injector was not extracted' }
    $cli = [Diagnostics.ProcessStartInfo]::new($helper[0].FullName)
    $cli.UseShellExecute = $false
    $cli.CreateNoWindow = $true
    $cli.RedirectStandardError = $true
    $cli.RedirectStandardOutput = $true
    $cli.EnvironmentVariables['__COMPAT_LAYER'] = 'RunAsInvoker'
    $child = [Diagnostics.Process]::Start($cli)
    $usage = $child.StandardError.ReadToEnd()
    $child.WaitForExit()
    if ($child.ExitCode -ne 2 -or $usage -notmatch 'remote-thread' -or $usage -notmatch 'minecraft-26.3' -or $usage -notmatch 'lunar-1.8.9' -or $usage -notmatch 'lunar-1.7.10' -or $usage -notmatch 'easecation' -or $usage -notmatch 'neoforge-1.21.10') {
        throw 'Embedded native injector usage/profile check failed'
    }
    Add-Type -AssemblyName UIAutomationClient
    Add-Type -AssemblyName UIAutomationTypes
    $window = [Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
    function Find-Control([string]$id) {
        $condition = [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::AutomationIdProperty, $id)
        return $window.FindFirst([Windows.Automation.TreeScope]::Descendants, $condition)
    }
    function Select-Profile([string]$label) {
        $box = Find-Control 'ClientVersionBox'
        $box.GetCurrentPattern([Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
        $condition = [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::NameProperty, $label)
        $until = [DateTime]::UtcNow.AddSeconds(5)
        do {
            $item = $window.FindFirst([Windows.Automation.TreeScope]::Descendants, $condition)
            if ($item) { break }
            Start-Sleep -Milliseconds 100
        } while ([DateTime]::UtcNow -lt $until)
        if (!$item) { throw "Profile missing: $label" }
        $item.GetCurrentPattern([Windows.Automation.SelectionItemPattern]::Pattern).Select()
        $until = [DateTime]::UtcNow.AddSeconds(5)
        do {
            $selection = $box.GetCurrentPattern([Windows.Automation.SelectionPattern]::Pattern).Current.GetSelection()
            if ($selection.Count -eq 1 -and $selection[0].Current.Name -eq $label) { break }
            Start-Sleep -Milliseconds 100
        } while ([DateTime]::UtcNow -lt $until)
        if ($selection.Count -ne 1 -or $selection[0].Current.Name -ne $label) { throw "Profile selection failed: $label" }
        # Closing the popup immediately after UIA Select can cancel WinUI's
        # pending selection. Wait for selection processing before collapsing.
        Start-Sleep -Milliseconds 250
        $box.GetCurrentPattern([Windows.Automation.ExpandCollapsePattern]::Pattern).Collapse()
        Start-Sleep -Milliseconds 250
    }
    # The native window may exist before WinUI publishes the XAML accessibility tree.
    $until = [DateTime]::UtcNow.AddSeconds(15)
    do {
        $process.Refresh()
        $window = [Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
        $minecraftTab = Find-Control 'MinecraftTab'
        $cs2Tab = Find-Control 'Cs2Tab'
        if ($minecraftTab -and $cs2Tab) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $until)
    if (!$minecraftTab -or !$cs2Tab) {
        $controls = $window.FindAll([Windows.Automation.TreeScope]::Descendants, [Windows.Automation.Condition]::TrueCondition)
        $details = @($controls | Select-Object -First 12 | ForEach-Object { "$($_.Current.AutomationId):$($_.Current.Name)" }) -join '; '
        throw "Game selector missing (window=$($window.Current.Name), offscreen=$($window.Current.IsOffscreen), controls=$details)"
    }
    $cs2Tab.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Toggle()
    $until = [DateTime]::UtcNow.AddSeconds(5)
    do {
        $action = Find-Control 'PatchButton'
        if ($action.Current.Name -eq '注入' -and $action.Current.IsEnabled) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $until)
    if (!$action.Current.IsEnabled -or $action.Current.Name -ne '注入') { throw 'CS2 injection entry is unavailable' }
    if ((Find-Control 'ClientVersionBox') -or (Find-Control 'HeyPixelToggle') -or (Find-Control 'RemoteThreadToggle')) { throw 'Minecraft settings visible in CS2' }
    if ((Find-Control 'ProcessBox').Current.Name -ne '目标 CS2 进程') { throw 'CS2 process selector not active' }
    $minecraftTab.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Toggle()
    $until = [DateTime]::UtcNow.AddSeconds(5)
    do {
        $version = Find-Control 'ClientVersionBox'
        if ($version -and (Find-Control 'PatchButton').Current.IsEnabled) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $until)
    if (!$version -or (Find-Control 'ProcessBox').Current.Name -ne '目标 Java 进程') { throw 'Minecraft selection was not restored' }
    $toggle = Find-Control 'HeyPixelToggle'
    if (!$toggle) { throw 'Forge 1.20.1 must expose HeyPixel mode' }
    $remote = Find-Control 'RemoteThreadToggle'
    if (!$remote) { throw 'Forge 1.20.1 must expose remote-thread loading' }
    if ($remote.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Current.ToggleState -ne [Windows.Automation.ToggleState]::Off) {
        throw 'Reflective injection must remain the default'
    }
    $remote.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Toggle()
    Start-Sleep -Milliseconds 250
    if ($toggle.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Current.ToggleState -ne [Windows.Automation.ToggleState]::Off -or !(Find-Control 'ProcessBox')) {
        throw 'Remote loading must keep the process selector and disable HeyPixel'
    }
    $toggle.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Toggle()
    Start-Sleep -Milliseconds 250
    if ($remote.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Current.ToggleState -ne [Windows.Automation.ToggleState]::Off) {
        throw 'HeyPixel must disable remote-thread loading'
    }
    $deadline = [DateTime]::UtcNow.AddSeconds(20)
    do {
        $directory = Find-Control 'HeyPixelDirectoryBox'
        if ($directory) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    if (!$directory -or (Find-Control 'PatchButton').Current.Name -notmatch 'HeyPixel') {
        throw 'HeyPixel mode did not switch to prelaunch installation'
    }
    if (!(Find-Control 'HeyPixelJavaBox')) { throw 'HeyPixel proxy Java directory field missing' }
    if ($ExpectedHeyPixelDirectory) {
        do {
            $detected = $directory.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value
            if ($detected -eq $ExpectedHeyPixelDirectory) { break }
            Start-Sleep -Milliseconds 100
        } while ([DateTime]::UtcNow -lt $deadline)
        if ($detected -ne $ExpectedHeyPixelDirectory) { throw "Directory detection mismatch: $detected" }
    }
    # Never invoke Apply or Restore: this smoke test only exercises UI state.
    Select-Profile 'Lunar Client 1.8.9'
    $until = [DateTime]::UtcNow.AddSeconds(5)
    do {
        $hiddenToggle = Find-Control 'HeyPixelToggle'
        $actionName = (Find-Control 'PatchButton').Current.Name
        if (!$hiddenToggle -and $actionName -eq '注入') { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $until)
    if ($hiddenToggle -or (Find-Control 'RemoteThreadToggle') -or $actionName -ne '注入') {
        $selectedName = (Find-Control 'ClientVersionBox').GetCurrentPattern([Windows.Automation.SelectionPattern]::Pattern).Current.GetSelection()[0].Current.Name
        throw "HeyPixel mode must disappear for Lunar (toggle=$([bool]$hiddenToggle), action=$actionName, profile=$selectedName)"
    }
    Select-Profile 'Lunar Client 1.7.10'
    if (Find-Control 'HeyPixelToggle') { throw 'Lunar 1.7.10 must not expose HeyPixel mode' }
    Select-Profile 'EaseCation'
    if (Find-Control 'HeyPixelToggle') { throw 'EaseCation must not expose HeyPixel mode' }
    Select-Profile 'Minecraft 1.21.10 (NeoForge)'
    if (Find-Control 'HeyPixelToggle') { throw 'NeoForge 1.21.10 must not expose HeyPixel mode' }
    Select-Profile 'Minecraft 1.20.1 (Forge)'
    $toggle = Find-Control 'HeyPixelToggle'
    if (!$toggle) { throw 'Returning to Forge did not restore the HeyPixel option' }
    if ($toggle.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Current.ToggleState -ne [Windows.Automation.ToggleState]::Off) {
        throw 'Returning to Forge must not silently reactivate HeyPixel mode'
    }
    [pscustomobject]@{ Result='PASS'; Window=$process.MainWindowTitle; Helper='PASS'; GameTabs='PASS'; HeyPixelUI='PASS'; RemoteThreadUI='PASS'; IsolatedFolder=$folder }
} finally {
    if (!$process.HasExited) {
        $null = $process.CloseMainWindow()
        if (!$process.WaitForExit(5000)) { $process.Kill() }
    }
}
