# Runs the same layout with several values of the steering ratio of the twin
# (scene\twin_params.txt) and prints what the referee recorded in each run.
param([double[]]$Gains = @(0.75, 0.80, 0.85), [string]$Layout = "A", [string]$Direction = "cw", [int]$MaxSeconds = 85)
$tools = $PSScriptRoot
$scene = Join-Path (Split-Path $tools -Parent) "scene"
foreach ($g in $Gains) {
    Set-Content -Path (Join-Path $scene "twin_params.txt") -Value "steer_gain $g" -Encoding ascii
    Write-Host "=== steer_gain $g"
    & (Join-Path $tools "run_episode.ps1") -Layout $Layout -Direction $Direction -MaxSeconds $MaxSeconds | Select-Object -Last 2
    $res = Get-ChildItem (Join-Path $scene "results") -Filter "run_*.json" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    $j = Get-Content $res.FullName -Raw | ConvertFrom-Json
    $j.events | ForEach-Object { Write-Host "   $_" }
}
