param(
	[string]$Generator = "Visual Studio 18 2026",
	[string]$Platform = "x64",
	[string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

function Require-Command {
	param([string]$Name)
	if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
		throw "Required command not found: $Name"
	}
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$addonRoot = Split-Path -Parent $scriptDir
$projectmRoot = Join-Path $addonRoot "_projectm_build"
$buildDir = Join-Path $projectmRoot "build-windows-static"
$stageDir = Join-Path $projectmRoot "build-windows-stage"
$targetLibDir = Join-Path $addonRoot "libs\projectM\lib\vs"
$targetIncludeDir = Join-Path $addonRoot "libs\projectM\include\projectM-4"
$projectmRepoUrl = "https://github.com/projectM-visualizer/projectm.git"
$projectmRepoBranch = "master"

Require-Command git
Require-Command cmake

Write-Host "==> Fetching current projectM master"
if (Test-Path $projectmRoot) {
	Remove-Item -LiteralPath $projectmRoot -Recurse -Force
}
& git clone --branch $projectmRepoBranch --depth 1 --recursive $projectmRepoUrl $projectmRoot

Write-Host "==> Configuring Windows static build"
if (Test-Path $buildDir) {
	Remove-Item -LiteralPath $buildDir -Recurse -Force
}

& cmake -S $projectmRoot -B $buildDir `
	-G $Generator `
	-A $Platform `
	-DBUILD_SHARED_LIBS=OFF `
	-DBUILD_TESTING=OFF `
	-DENABLE_SDL_UI=OFF `
	-DENABLE_INSTALL=ON

Write-Host "==> Building"
& cmake --build $buildDir --config $Configuration --parallel

Write-Host "==> Installing into staging dir"
if (Test-Path $stageDir) {
	Remove-Item -LiteralPath $stageDir -Recurse -Force
}

& cmake --install $buildDir --config $Configuration --prefix $stageDir

$mainLib = Get-ChildItem -Path $stageDir -Recurse -Filter "libprojectM-4.lib" | Select-Object -First 1 -ExpandProperty FullName
$playlistLib = Get-ChildItem -Path $stageDir -Recurse -Filter "libprojectM-4-playlist.lib" | Select-Object -First 1 -ExpandProperty FullName
$installedIncludeDir = Join-Path $stageDir "include\projectM-4"

if (-not $mainLib -or -not (Test-Path $mainLib)) {
	throw "Could not find built libprojectM-4.lib in $stageDir"
}

if (-not $playlistLib -or -not (Test-Path $playlistLib)) {
	throw "Could not find built libprojectM-4-playlist.lib in $stageDir"
}

if (-not (Test-Path $installedIncludeDir)) {
	throw "Could not find installed headers in $installedIncludeDir"
}

Write-Host "==> Copying libs and headers into addon"
New-Item -ItemType Directory -Force -Path $targetLibDir | Out-Null
if (Test-Path $targetIncludeDir) {
	Remove-Item -LiteralPath $targetIncludeDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $targetIncludeDir | Out-Null

Copy-Item -LiteralPath $mainLib -Destination (Join-Path $targetLibDir "libprojectM-4.lib") -Force
Copy-Item -LiteralPath $playlistLib -Destination (Join-Path $targetLibDir "libprojectM-4-playlist.lib") -Force
Copy-Item -Path (Join-Path $installedIncludeDir "*") -Destination $targetIncludeDir -Recurse -Force

Write-Host "==> Cleaning temporary build files"
if (Test-Path $projectmRoot) {
	Remove-Item -LiteralPath $projectmRoot -Recurse -Force
}

Write-Host "==> Done"
Write-Host "Copied:"
Write-Host "  $(Join-Path $targetLibDir 'libprojectM-4.lib')"
Write-Host "  $(Join-Path $targetLibDir 'libprojectM-4-playlist.lib')"
Write-Host "  $targetIncludeDir"
