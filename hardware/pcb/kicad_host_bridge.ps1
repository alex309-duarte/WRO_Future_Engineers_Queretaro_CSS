param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('open_project', 'focus_kicad', 'status', 'send_keys', 'window_info', 'click_relative', 'capture_window', 'click_and_type', 'click_and_paste', 'click_dialog_button', 'debug_window_tree', 'debug_symbol_dialog')]
    [string]$Action,

    [string]$ProjectPath = 'c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\future_engineers_robotics_expansion.kicad_pro',

    [string]$Keys = '',

    [int]$DelayMs = 300,

    [int]$X = 0,

    [int]$Y = 0,

    [ValidateSet('auto', 'kicad', 'eeschema')]
    [string]$Target = 'auto',

    [string]$OutputPath = 'c:\Users\jesus\WRO_Future_Engineers_Queretaro_CSS\hardware\pcb\kicad_window_capture.png'
)

$kicadExe = 'C:\Program Files\KiCad\9.0\bin\kicad.exe'

if (-not ('Win32' -as [type])) {
    Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class Win32 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")]
    public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
}
"@
}

function Debug-KiCadSymbolDialog {
    $proc = Get-KiCadProcess -PreferredTarget $Target
    if (-not $proc) {
        Write-Output 'KICAD_NOT_RUNNING'
        return
    }

    $processId = [uint32]$proc.Id
    $dialogHandle = [IntPtr]::Zero

    $findDialog = [Win32+EnumWindowsProc]{
        param([IntPtr]$hWnd, [IntPtr]$lParam)

        $windowPid = [uint32]0
        [void][Win32]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($windowPid -ne $processId) {
            return $true
        }

        $classBuilder = New-Object System.Text.StringBuilder 256
        [void][Win32]::GetClassName($hWnd, $classBuilder, $classBuilder.Capacity)
        if ($classBuilder.ToString() -eq '#32770') {
            $script:dialogHandle = $hWnd
            return $false
        }

        return $true
    }

    $script:dialogHandle = [IntPtr]::Zero
    [void][Win32]::EnumWindows($findDialog, [IntPtr]::Zero)
    $dialogHandle = $script:dialogHandle
    $script:dialogHandle = [IntPtr]::Zero

    if ($dialogHandle -eq [IntPtr]::Zero) {
        Write-Output 'KICAD_SYMBOL_DIALOG_NOT_FOUND'
        return
    }

    $dialogTextBuilder = New-Object System.Text.StringBuilder 512
    [void][Win32]::GetWindowText($dialogHandle, $dialogTextBuilder, $dialogTextBuilder.Capacity)
    Write-Output ('DIALOG HWND=' + $dialogHandle + ' TEXT=' + $dialogTextBuilder.ToString())
    $childLines = New-Object System.Collections.Generic.List[string]

    $enumChild = [Win32+EnumWindowsProc]{
        param([IntPtr]$childHandle, [IntPtr]$lParam)

        $childClassBuilder = New-Object System.Text.StringBuilder 256
        [void][Win32]::GetClassName($childHandle, $childClassBuilder, $childClassBuilder.Capacity)
        $childTextBuilder = New-Object System.Text.StringBuilder 512
        [void][Win32]::GetWindowText($childHandle, $childTextBuilder, $childTextBuilder.Capacity)
        $null = $childLines.Add('CHILD HWND=' + $childHandle + ' CLASS=' + $childClassBuilder.ToString() + ' TEXT=' + $childTextBuilder.ToString())
        return $true
    }

    [void][Win32]::EnumChildWindows($dialogHandle, $enumChild, [IntPtr]::Zero)
    foreach ($childLine in $childLines) {
        Write-Output $childLine
    }
}

function Get-KiCadProcess {
    param(
        [string]$PreferredTarget = 'auto'
    )

    $kicadProc = Get-Process -Name 'kicad' -ErrorAction SilentlyContinue | Select-Object -First 1
    $eeschemaProc = Get-Process -Name 'eeschema' -ErrorAction SilentlyContinue | Select-Object -First 1

    switch ($PreferredTarget) {
        'kicad' {
            return $kicadProc
        }
        'eeschema' {
            return $eeschemaProc
        }
        default {
            if ($eeschemaProc) { return $eeschemaProc }
            return $kicadProc
        }
    }
}

function Focus-KiCadWindow {
    param(
        [string]$PreferredTarget = 'auto'
    )

    $proc = Get-KiCadProcess -PreferredTarget $PreferredTarget
    if (-not $proc) {
        Write-Output 'KICAD_NOT_RUNNING'
        return
    }

    $handle = $proc.MainWindowHandle
    if (-not $handle -or $handle -eq 0) {
        Write-Output 'KICAD_NO_WINDOW'
        return
    }

    [void][Win32]::ShowWindowAsync($handle, 9)
    Start-Sleep -Milliseconds 300
    [void][Win32]::SetForegroundWindow($handle)
    Write-Output 'KICAD_FOCUSED'
}

function Get-KiCadWindowInfo {
    $focusResult = Focus-KiCadWindow -PreferredTarget $Target
    if ($focusResult -ne 'KICAD_FOCUSED') {
        Write-Output $focusResult
        return
    }

    Start-Sleep -Milliseconds 300

    $proc = Get-KiCadProcess -PreferredTarget $Target
    $handle = $proc.MainWindowHandle

    $rect = New-Object Win32+RECT
    [void][Win32]::GetWindowRect($handle, [ref]$rect)
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    $title = $proc.MainWindowTitle
    Write-Output ("KICAD_WINDOW Title=`"$title`" Left=$($rect.Left) Top=$($rect.Top) Width=$width Height=$height")
}

function Invoke-KiCadDialogButton {
    param(
        [string]$ButtonText,
        [int]$PostDelayMs
    )

    if ([string]::IsNullOrWhiteSpace($ButtonText)) {
        Write-Error 'Keys must contain the button text for click_dialog_button action'
        exit 1
    }

    $focusResult = Focus-KiCadWindow -PreferredTarget $Target
    if ($focusResult -ne 'KICAD_FOCUSED') {
        Write-Output $focusResult
        return
    }

    $proc = Get-KiCadProcess -PreferredTarget $Target
    $handle = $proc.MainWindowHandle
    $processId = [uint32]$proc.Id
    $dialogHandle = [IntPtr]::Zero
    $matchedButton = [IntPtr]::Zero
    $normalizedTarget = $ButtonText.Trim().ToLowerInvariant()

    $findDialog = [Win32+EnumWindowsProc]{
        param([IntPtr]$hWnd, [IntPtr]$lParam)

        $windowPid = [uint32]0
        [void][Win32]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($windowPid -ne $processId) {
            return $true
        }

        $classBuilder = New-Object System.Text.StringBuilder 256
        [void][Win32]::GetClassName($hWnd, $classBuilder, $classBuilder.Capacity)
        if ($classBuilder.ToString() -eq '#32770') {
            $script:dialogHandle = $hWnd
            return $false
        }

        return $true
    }

    $script:dialogHandle = [IntPtr]::Zero
    [void][Win32]::EnumWindows($findDialog, [IntPtr]::Zero)
    if ($script:dialogHandle -ne [IntPtr]::Zero) {
        $handle = $script:dialogHandle
    }
    $script:dialogHandle = [IntPtr]::Zero

    $callback = [Win32+EnumWindowsProc]{
        param([IntPtr]$childHandle, [IntPtr]$lParam)

        $classBuilder = New-Object System.Text.StringBuilder 256
        [void][Win32]::GetClassName($childHandle, $classBuilder, $classBuilder.Capacity)
        if ($classBuilder.ToString() -ne 'Button') {
            return $true
        }

        $textBuilder = New-Object System.Text.StringBuilder 256
        [void][Win32]::GetWindowText($childHandle, $textBuilder, $textBuilder.Capacity)
        $childText = $textBuilder.ToString().Trim().ToLowerInvariant()
        if ($childText -eq $normalizedTarget) {
            $script:matchedButton = $childHandle
            return $false
        }

        return $true
    }

    $script:matchedButton = [IntPtr]::Zero
    [void][Win32]::EnumChildWindows($handle, $callback, [IntPtr]::Zero)
    $matchedButton = $script:matchedButton
    $script:matchedButton = [IntPtr]::Zero

    if ($matchedButton -eq [IntPtr]::Zero) {
        Write-Output ('KICAD_DIALOG_BUTTON_NOT_FOUND ' + $ButtonText)
        return
    }

    [void][Win32]::SendMessage($matchedButton, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero)
    Start-Sleep -Milliseconds $PostDelayMs
    Write-Output ('KICAD_DIALOG_BUTTON_CLICKED ' + $ButtonText)
}

function Debug-KiCadWindowTree {
    $proc = Get-KiCadProcess -PreferredTarget $Target
    if (-not $proc) {
        Write-Output 'KICAD_NOT_RUNNING'
        return
    }

    $processId = [uint32]$proc.Id
    $topWindows = New-Object System.Collections.Generic.List[System.IntPtr]

    $enumTop = [Win32+EnumWindowsProc]{
        param([IntPtr]$hWnd, [IntPtr]$lParam)

        $windowPid = [uint32]0
        [void][Win32]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($windowPid -eq $processId) {
            $null = $topWindows.Add($hWnd)
        }
        return $true
    }

    [void][Win32]::EnumWindows($enumTop, [IntPtr]::Zero)

    foreach ($windowHandle in $topWindows) {
        $classBuilder = New-Object System.Text.StringBuilder 256
        [void][Win32]::GetClassName($windowHandle, $classBuilder, $classBuilder.Capacity)
        $textBuilder = New-Object System.Text.StringBuilder 512
        [void][Win32]::GetWindowText($windowHandle, $textBuilder, $textBuilder.Capacity)
        Write-Output ('TOP HWND=' + $windowHandle + ' CLASS=' + $classBuilder.ToString() + ' TEXT=' + $textBuilder.ToString())

        $enumChild = [Win32+EnumWindowsProc]{
            param([IntPtr]$childHandle, [IntPtr]$lParam)

            $childClassBuilder = New-Object System.Text.StringBuilder 256
            [void][Win32]::GetClassName($childHandle, $childClassBuilder, $childClassBuilder.Capacity)
            $childTextBuilder = New-Object System.Text.StringBuilder 512
            [void][Win32]::GetWindowText($childHandle, $childTextBuilder, $childTextBuilder.Capacity)
            Write-Output ('CHILD HWND=' + $childHandle + ' CLASS=' + $childClassBuilder.ToString() + ' TEXT=' + $childTextBuilder.ToString())
            return $true
        }

        [void][Win32]::EnumChildWindows($windowHandle, $enumChild, [IntPtr]::Zero)
    }
}

function Send-KiCadKeys {
    param(
        [string]$KeySequence,
        [int]$PostDelayMs
    )

    if ([string]::IsNullOrWhiteSpace($KeySequence)) {
        Write-Error 'Keys must be provided for send_keys action'
        exit 1
    }

    $focusResult = Focus-KiCadWindow
    if ($focusResult -ne 'KICAD_FOCUSED') {
        Write-Output $focusResult
        return
    }

    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait($KeySequence)
    Start-Sleep -Milliseconds $PostDelayMs
    Write-Output ('KICAD_KEYS_SENT ' + $KeySequence)
}

function Click-KiCadRelative {
    param(
        [int]$RelX,
        [int]$RelY,
        [int]$PostDelayMs
    )

    $focusResult = Focus-KiCadWindow -PreferredTarget $Target
    if ($focusResult -ne 'KICAD_FOCUSED') {
        Write-Output $focusResult
        return
    }

    $proc = Get-KiCadProcess -PreferredTarget $Target
    $handle = $proc.MainWindowHandle
    $rect = New-Object Win32+RECT
    [void][Win32]::GetWindowRect($handle, [ref]$rect)

    $targetX = $rect.Left + $RelX
    $targetY = $rect.Top + $RelY

    [void][Win32]::SetCursorPos($targetX, $targetY)
    Start-Sleep -Milliseconds 100
    [Win32]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 50
    [Win32]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds $PostDelayMs
    Write-Output ("KICAD_CLICK X=$targetX Y=$targetY")
}

function Capture-KiCadWindow {
    param(
        [string]$CapturePath
    )

    $focusResult = Focus-KiCadWindow -PreferredTarget $Target
    if ($focusResult -ne 'KICAD_FOCUSED') {
        Write-Output $focusResult
        return
    }

    Add-Type -AssemblyName System.Drawing

    $proc = Get-KiCadProcess -PreferredTarget $Target
    $handle = $proc.MainWindowHandle
    $rect = New-Object Win32+RECT
    [void][Win32]::GetWindowRect($handle, [ref]$rect)

    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top

    if ($width -le 0 -or $height -le 0) {
        Write-Error 'Invalid window size for capture'
        exit 1
    }

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
    $bitmap.Save($CapturePath, [System.Drawing.Imaging.ImageFormat]::Png)
    $graphics.Dispose()
    $bitmap.Dispose()

    Write-Output ("KICAD_CAPTURE " + $CapturePath)
}

function Click-And-TypeKiCad {
    param(
        [int]$RelX,
        [int]$RelY,
        [string]$Text,
        [int]$PostDelayMs,
        [string]$CapturePath
    )

    if ([string]::IsNullOrWhiteSpace($Text)) {
        Write-Error 'Text must be provided for click_and_type action'
        exit 1
    }

    Click-KiCadRelative -RelX $RelX -RelY $RelY -PostDelayMs 250
    Start-Sleep -Milliseconds 150
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait($Text)
    Start-Sleep -Milliseconds $PostDelayMs

    if (-not [string]::IsNullOrWhiteSpace($CapturePath)) {
        Capture-KiCadWindow -CapturePath $CapturePath
    }

    Write-Output ('KICAD_CLICK_AND_TYPE ' + $Text)
}

function Click-And-PasteKiCad {
    param(
        [int]$RelX,
        [int]$RelY,
        [string]$Text,
        [int]$PostDelayMs,
        [string]$CapturePath
    )

    if ([string]::IsNullOrWhiteSpace($Text)) {
        Write-Error 'Text must be provided for click_and_paste action'
        exit 1
    }

    Click-KiCadRelative -RelX $RelX -RelY $RelY -PostDelayMs 250
    Start-Sleep -Milliseconds 150
    Set-Clipboard -Value $Text
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait('^v')
    Start-Sleep -Milliseconds $PostDelayMs

    if (-not [string]::IsNullOrWhiteSpace($CapturePath)) {
        Capture-KiCadWindow -CapturePath $CapturePath
    }

    Write-Output ('KICAD_CLICK_AND_PASTE ' + $Text)
}

switch ($Action) {
    'open_project' {
        if (-not (Test-Path $kicadExe)) {
            Write-Error "KiCad executable not found at $kicadExe"
            exit 1
        }
        if (-not (Test-Path $ProjectPath)) {
            Write-Error "Project not found at $ProjectPath"
            exit 1
        }

        Start-Process -FilePath $kicadExe -ArgumentList ('"' + $ProjectPath + '"')
        Start-Sleep -Seconds 2
        $proc = Get-KiCadProcess
        if ($proc) {
            Write-Output ('KICAD_OPENED PID=' + $proc.Id)
        } else {
            Write-Output 'KICAD_LAUNCH_REQUESTED'
        }
    }
    'focus_kicad' {
        Focus-KiCadWindow -PreferredTarget $Target
    }
    'status' {
        $proc = Get-KiCadProcess -PreferredTarget $Target
        if ($proc) {
            Write-Output ('KICAD_RUNNING PID=' + $proc.Id + ' NAME=' + $proc.ProcessName)
        } else {
            Write-Output 'KICAD_NOT_RUNNING'
        }
    }
    'send_keys' {
        Send-KiCadKeys -KeySequence $Keys -PostDelayMs $DelayMs
    }
    'window_info' {
        Get-KiCadWindowInfo
    }
    'click_relative' {
        Click-KiCadRelative -RelX $X -RelY $Y -PostDelayMs $DelayMs
    }
    'capture_window' {
        Capture-KiCadWindow -CapturePath $OutputPath
    }
    'click_and_type' {
        Click-And-TypeKiCad -RelX $X -RelY $Y -Text $Keys -PostDelayMs $DelayMs -CapturePath $OutputPath
    }
    'click_and_paste' {
        Click-And-PasteKiCad -RelX $X -RelY $Y -Text $Keys -PostDelayMs $DelayMs -CapturePath $OutputPath
    }
    'click_dialog_button' {
        Invoke-KiCadDialogButton -ButtonText $Keys -PostDelayMs $DelayMs
    }
    'debug_window_tree' {
        Debug-KiCadWindowTree
    }
    'debug_symbol_dialog' {
        Debug-KiCadSymbolDialog
    }
}
