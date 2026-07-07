$ErrorActionPreference = "Stop"

$RepoRoot   = Resolve-Path (Join-Path $PSScriptRoot "..")
$ToolsDir   = Join-Path $RepoRoot "build\tools"
$Pluginval  = Join-Path $ToolsDir "pluginval.exe"
$LogDir     = Join-Path $RepoRoot "build\pluginval-logs"
$PluginPath = Join-Path $RepoRoot "build\RBDrumSampler_artefacts\Debug\VST3\Rock Band Drum Sampler.vst3"

$PluginvalVersion = "v1.0.4"
$PluginvalUrl = "https://github.com/Tracktion/pluginval/releases/download/$PluginvalVersion/pluginval_Windows.zip"

if (-not (Test-Path $Pluginval)) {
    Write-Host "pluginval not found - downloading $PluginvalVersion..."
    New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null

    $zipPath = Join-Path $ToolsDir "pluginval_Windows.zip"
    Invoke-WebRequest -Uri $PluginvalUrl -OutFile $zipPath
    Expand-Archive -Path $zipPath -DestinationPath $ToolsDir -Force
    Remove-Item $zipPath

    if (-not (Test-Path $Pluginval)) {
        throw "pluginval.exe not found after extracting $PluginvalUrl into $ToolsDir"
    }
}

if (-not (Test-Path $PluginPath)) {
    throw "Plugin build not found at '$PluginPath'. Build it first: cmake --build build --config Debug --target RBDrumSampler"
}

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

& $Pluginval --strictness-level 5 --validate-in-process --output-dir $LogDir "$PluginPath"
exit $LASTEXITCODE
