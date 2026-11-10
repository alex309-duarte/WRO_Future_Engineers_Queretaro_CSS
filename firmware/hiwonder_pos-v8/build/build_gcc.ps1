param(
    [string]$ToolchainRoot = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "RosRobotControllerM4-armclang")).Path
$uvProject = Join-Path $projectRoot "MDK-ARM\RosRobotControllerM4.uvprojx"
$buildRoot = Join-Path $PSScriptRoot "build-gcc"
$objectRoot = Join-Path $buildRoot "obj"

if ([string]::IsNullOrWhiteSpace($ToolchainRoot)) {
    $ToolchainRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..\downloads\arm-gnu-toolchain-15.2.rel1\extracted")).Path
}
$gcc = Join-Path $ToolchainRoot "bin\arm-none-eabi-gcc.exe"
$objcopy = Join-Path $ToolchainRoot "bin\arm-none-eabi-objcopy.exe"
$size = Join-Path $ToolchainRoot "bin\arm-none-eabi-size.exe"
foreach ($tool in @($gcc, $objcopy, $size)) {
    if (-not (Test-Path $tool)) { throw "Tool not found: $tool" }
}

function Invoke-NativeTool {
    param([string]$FilePath, [string[]]$Arguments)
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $FilePath
    $start.UseShellExecute = $false
    foreach ($argument in $Arguments) { [void]$start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($start)
    $process.WaitForExit()
    return $process.ExitCode
}

if ($Clean -and (Test-Path $buildRoot)) {
    Remove-Item -LiteralPath $buildRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $objectRoot | Out-Null

[xml]$project = Get-Content $uvProject
$includeText = $project.Project.Targets.Target.TargetOption.TargetArmAds.Cads.VariousControls.IncludePath | Select-Object -First 1
$includeDirs = @()
foreach ($entry in ($includeText -split ";")) {
    if ([string]::IsNullOrWhiteSpace($entry)) { continue }
    $relative = $entry.Trim().Replace("/", "\")
    if ($relative.StartsWith("..\")) {
        $candidate = Join-Path $projectRoot $relative.Substring(3)
    } else {
        $candidate = Join-Path (Join-Path $projectRoot "MDK-ARM") $relative
    }
    $candidate = $candidate.Replace("portable\RVDS\ARM_CM4F", "portable\GCC\ARM_CM4F")
    if (Test-Path $candidate) { $includeDirs += (Resolve-Path $candidate).Path }
}
foreach ($extra in @("Drivers\CMSIS\Core\Include", "Drivers\CMSIS\Device\ST\STM32F4xx\Include")) {
    $candidate = Join-Path $projectRoot $extra
    if (Test-Path $candidate) { $includeDirs += (Resolve-Path $candidate).Path }
}
$includeDirs = $includeDirs | Sort-Object -Unique

$sources = @()
foreach ($node in $project.SelectNodes("//FilePath")) {
    $entry = $node.InnerText.Replace("/", "\")
    if ([IO.Path]::GetExtension($entry).ToLowerInvariant() -ne ".c") { continue }
    # Arm Compiler-only adapter; GCC uses newlib-nano + nosys.specs.
    if ($entry.Replace("/", "\").EndsWith("Hiwonder\Misc\syscall.c")) { continue }
    if ($entry.StartsWith("..\")) {
        $candidate = Join-Path $projectRoot $entry.Substring(3)
    } else {
        $candidate = Join-Path (Join-Path $projectRoot "MDK-ARM") $entry
    }
    $candidate = $candidate.Replace("portable\RVDS\ARM_CM4F\port.c", "portable\GCC\ARM_CM4F\port.c")
    if (-not (Test-Path $candidate)) { throw "Source not found: $entry -> $candidate" }
    $sources += (Resolve-Path $candidate).Path
}
$sources = $sources | Sort-Object -Unique
$sources += (Resolve-Path (Join-Path $projectRoot "Core\Startup\startup_stm32f407vetx.s")).Path

$common = @(
    "-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard",
    "-DUSE_HAL_DRIVER", "-DSTM32F407xx", "-DLV_CONF_INCLUDE_SIMPLE", "-DLV_LVGL_H_INCLUDE_SIMPLE",
    "-ffunction-sections", "-fdata-sections", "-fcommon", "-fno-strict-aliasing",
    "-Os", "-g3", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-sign-compare"
)
foreach ($dir in $includeDirs) { $common += "-I$dir" }

$objects = @()
$index = 0
foreach ($source in $sources) {
    $index++
    $object = Join-Path $objectRoot (("{0:D3}_" -f $index) + [IO.Path]::GetFileNameWithoutExtension($source) + ".o")
    if ((Test-Path $object) -and ((Get-Item $object).LastWriteTimeUtc -ge (Get-Item $source).LastWriteTimeUtc)) {
        $objects += $object
        continue
    }
    $language = if ([IO.Path]::GetExtension($source).ToLowerInvariant() -eq ".c") { @("-std=gnu11") } else { @("-x", "assembler-with-cpp") }
    Write-Host ("[{0}/{1}] {2}" -f $index, $sources.Count, [IO.Path]::GetFileName($source))
    $compileArgs = $common + $language + @("-c", $source, "-o", $object)
    if ((Invoke-NativeTool $gcc $compileArgs) -ne 0) { throw "Compilation failed: $source" }
    $objects += $object
}

$elf = Join-Path $buildRoot "RosRobotControllerM4-extended-m1.elf"
$map = Join-Path $buildRoot "RosRobotControllerM4-extended-m1.map"
$hex = Join-Path $buildRoot "RosRobotControllerM4-extended-m1.hex"
$bin = Join-Path $buildRoot "RosRobotControllerM4-extended-m1.bin"
$linker = (Resolve-Path (Join-Path $PSScriptRoot "STM32F407VETX_FLASH.ld")).Path
$linkArgs = @(
    "-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard",
    "-T$linker", "-Wl,--gc-sections", "-Wl,-Map=$map", "--specs=nano.specs", "--specs=nosys.specs",
    "-o", $elf
) + $objects + @("-Wl,--start-group", "-lc", "-lm", "-Wl,--end-group")
$responseFile = Join-Path $buildRoot "linker-arguments.rsp"
$responseLines = $linkArgs | ForEach-Object {
    $arg = $_.Replace('\', '/')
    if ($arg -match '[\s"]') { '"' + $arg.Replace('"', '\"') + '"' } else { $arg }
}
[IO.File]::WriteAllLines($responseFile, $responseLines)
if ((Invoke-NativeTool $gcc @("@$responseFile")) -ne 0) { throw "Firmware link failed" }
if ((Invoke-NativeTool $objcopy @("-O", "ihex", $elf, $hex)) -ne 0) { throw "HEX creation failed" }
if ((Invoke-NativeTool $objcopy @("-O", "binary", $elf, $bin)) -ne 0) { throw "BIN creation failed" }
if ((Invoke-NativeTool $size @($elf)) -ne 0) { throw "Could not measure the image size" }
Get-FileHash -Algorithm SHA256 $hex, $bin | Format-Table Path, Hash
