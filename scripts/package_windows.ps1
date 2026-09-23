param(
    [string]$BuildDirectory = "build-release",
    [string]$OutputDirectory = "dist"
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$build = (Resolve-Path (Join-Path $root $BuildDirectory)).Path
$output = Join-Path $root $OutputDirectory
$stage = Join-Path $output "windows-stage"
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item (Join-Path $build "NebiPadPlus.exe") $stage -Force

$deploy = Get-Command windeployqt.exe -ErrorAction Stop
& $deploy.Source --release --compiler-runtime --no-translations (Join-Path $stage "NebiPadPlus.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

$installer = Join-Path $output "NebiPadPlusSuite-Windows-x64.exe"
$nsis = Get-Command makensis.exe -ErrorAction Stop
& $nsis.Source "/DSTAGING=$stage" "/DOUTPUT=$installer" (Join-Path $root "packaging/windows/NebiPadPlus.nsi")
if ($LASTEXITCODE -ne 0) { throw "NSIS failed" }
