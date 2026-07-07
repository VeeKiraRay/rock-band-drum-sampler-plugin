$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $RepoRoot
try {
    cmake -B build
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    cmake --build build --config Debug --target RBDrumSamplerTests
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & "build/RBDrumSamplerTests_artefacts/Debug/RBDrumSamplerTests.exe"
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
