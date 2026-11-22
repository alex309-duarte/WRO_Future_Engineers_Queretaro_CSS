# One complete Obstacle Challenge run in the digital twin (Windows PowerShell).
#
#   .\run_episode.ps1 -Layout A -Direction cw [-Record] [-NoDisplay]
#   .\run_episode.ps1 -Layout A -Direction cw -Modify "1000,2400,150" -Surprise -Record
#   .\run_episode.ps1 -Layout A -Direction cw -Adapted -Record -UntilLaps 2.95 -Title "Layout A"
#
# Builds the scene for the layout, starts the simulation, runs the robot
# program in WSL (it presses the start button by itself after 3 s and ends
# 5 s after the mission; at -MaxSeconds it is told to end, so it closes its
# camera video properly), stops the simulation and prints the referee's
# result (scene\results\run_*.json). With -Record the video of the run is
# rendered afterwards from its record (tools\replay_video.py), so the
# simulation keeps real time during the run.
param(
    [string]$Layout = "A",
    [ValidateSet("cw", "ccw")][string]$Direction = "cw",
    [int]$Seed = 1,
    [string]$Modify = "",          # surprise rule: "X_MM,Y_MM,HEIGHT_MM" replaces that sign
    [switch]$Surprise,             # run the surprise-rule prototype (team plan) in the program
    [switch]$Adapted,              # use the build with proposed\mission_changes.patch
    [switch]$Record,
    [string]$Title = "",           # title written on the video
    [double]$UntilLaps = 0,        # video ends when this many laps are done (2.95 = back in the starting section after lap 3)
    [switch]$NoDisplay,            # the program does not open its own window with the camera image
    [switch]$Watch,                # keep CoppeliaSim's 3D view drawing during the run (the simulation may fall behind real time)
    [int]$MaxSeconds = 300,
    [string]$Python = "C:\Python312\python.exe"
)
# Native programs that write to stderr must not end the script (Windows PowerShell 5)
$ErrorActionPreference = "Continue"
$tools = $PSScriptRoot
$sim = Split-Path $tools -Parent
$scene = Join-Path $sim "scene"
$logs = Join-Path $sim "logs"
New-Item -ItemType Directory -Force $logs | Out-Null
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$tag = "$Layout`_$Direction`_$stamp"

$buildArgs = @((Join-Path $scene "build_scene.py"), "--layout", $Layout, "--direction", $Direction, "--seed", $Seed)
if ($Modify) { $buildArgs += @("--modify", $Modify); $tag = "$tag`_S" + ($Modify -replace ',', 'x') }
if ($Surprise) { $tag = "$tag`_plan" }
if ($Adapted) { $tag = "$tag`_adapted" }
& $Python @buildArgs
if ($LASTEXITCODE -ne 0) { throw "build_scene.py failed" }
$ttt = Get-ChildItem $scene -Filter "wro_twin_*_$Direction.ttt" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
& $Python (Join-Path $tools "sim_ctl.py") load $ttt.FullName
& $Python (Join-Path $tools "sim_ctl.py") start
# The 3D view costs about 4 ms per 10 ms step; without it the simulation keeps
# real time while the camera, the LiDAR and the board are being served.
if (-not $Watch) { & $Python (Join-Path $tools "sim_ctl.py") display off | Out-Null }

# WSL path of the tools folder
$wslTools = (wsl -d Ubuntu -- wslpath -a ($tools -replace '\\', '/')).Trim()
$extra = if ($NoDisplay) { "--no-display" } else { "" }
$envs = "WRO_SIM_AUTOSTART=3 WRO_SIM_EXIT_AFTER_RUN=5"
if ($Record) {
    # the program saves its camera video (with the detections) and the time of each frame
    New-Item -ItemType Directory -Force (Join-Path $logs "cam_$tag") | Out-Null
    # (written inside WSL and copied afterwards: the video file is only
    # complete when the program closes it)
    $camTmp = "/tmp/wro_cam_$tag"
    $extra = "$extra -s -o '$camTmp'"
    $envs = "$envs WRO_SIM_FRAME_LOG='$camTmp/frames.csv'"
}
if ($Surprise) { $envs = "$envs WRO_SURPRISE_RULE=1" }
if ($Adapted) { $envs = "$envs WRO_TWIN_BUILD=`$HOME/wro_twin_build_adapted" }
$log = "$wslTools/../logs/run_$tag.log"
$pre = if ($Record) { "mkdir -p '$camTmp' && " } else { "" }
wsl -d Ubuntu -- bash -lc "cd '$wslTools' && $pre$envs timeout -s USR1 -k 10 $MaxSeconds bash run_twin.sh $extra > '$log' 2>&1"
if ($Record) { wsl -d Ubuntu -- bash -lc "cp '$camTmp'/* '$wslTools/../logs/cam_$tag/' && rm -rf '$camTmp'" }

& $Python (Join-Path $tools "sim_ctl.py") stop
& $Python (Join-Path $tools "sim_ctl.py") display on | Out-Null
$res = Get-ChildItem (Join-Path $scene "results") -Filter "run_*.json" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($res) { Copy-Item $res.FullName (Join-Path $logs "run_$tag.json") }
if ($Record -and $res) {
    # the video is rendered now, from the record of the run
    $video = Join-Path $logs "run_$tag.mp4"
    $renderArgs = @((Join-Path $tools "replay_video.py"), $res.FullName, $video, "--camera", (Join-Path $logs "cam_$tag"))
    if ($Title) { $renderArgs += @("--title", $Title) }
    if ($UntilLaps -gt 0) { $renderArgs += @("--until-laps", $UntilLaps.ToString([cultureinfo]::InvariantCulture)) }
    & $Python @renderArgs
}
Write-Host "Log:    $log"
if ($Record) { Write-Host "Video:  $video" }
if ($res) {
    $j = Get-Content $res.FullName -Raw | ConvertFrom-Json
    Write-Host ("Result: {0} | {1} | laps {2} | time {3} s | signs touched {4} | wall contacts {5} | simulation speed {6}" -f `
        $j.result, $j.direction, $j.laps, $j.time_s, @($j.signs_touched).Count, $j.wall_contacts, $j.sim_speed)
    Write-Host "File:   $($res.FullName)"
}
