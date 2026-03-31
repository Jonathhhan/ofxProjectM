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

function Remove-DirectoryIfExists {
	param([string]$Path)
	if (Test-Path $Path) {
		Remove-Item -LiteralPath $Path -Recurse -Force
	}
}

function Invoke-NativeCommand {
	param(
		[string]$Name,
		[string[]]$Arguments
	)

	& $Name @Arguments
	if ($LASTEXITCODE -ne 0) {
		throw "Command failed ($LASTEXITCODE): $Name $($Arguments -join ' ')"
	}
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$addonRoot = Split-Path -Parent $scriptDir
$targetLibDir = Join-Path $addonRoot "libs\projectM\lib\vs"
$targetIncludeDir = Join-Path $addonRoot "libs\projectM\include\projectM-4"
$projectmRepoUrl = "https://github.com/projectM-visualizer/projectm.git"
$projectmRepoBranch = "master"
# Build outside the addon tree so OneDrive does not interfere with Git metadata.
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("ofxProjectM_" + [System.Guid]::NewGuid().ToString("N"))
$projectmRoot = Join-Path $tempRoot "projectm"
$buildDir = Join-Path $tempRoot "build-windows-static"
$stageDir = Join-Path $tempRoot "build-windows-stage"

Require-Command git
Require-Command cmake

try {
	Write-Host "==> Using temporary build root: $tempRoot"
	New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null

	Write-Host "==> Fetching current projectM master"
	Invoke-NativeCommand git @(
		"clone",
		"--branch", $projectmRepoBranch,
		"--depth", "1",
		"--recursive",
		$projectmRepoUrl,
		$projectmRoot
	)

	Write-Host "==> Configuring Windows static build"
	Invoke-NativeCommand cmake @(
		"-S", $projectmRoot,
		"-B", $buildDir,
		"-G", $Generator,
		"-A", $Platform,
		"-DBUILD_SHARED_LIBS=OFF",
		"-DBUILD_TESTING=OFF",
		"-DENABLE_SDL_UI=OFF",
		"-DENABLE_INSTALL=ON"
	)

	Write-Host "==> Building"
	Invoke-NativeCommand cmake @(
		"--build", $buildDir,
		"--config", $Configuration,
		"--parallel"
	)

	Write-Host "==> Installing into staging dir"
	Invoke-NativeCommand cmake @(
		"--install", $buildDir,
		"--config", $Configuration,
		"--prefix", $stageDir
	)

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
	Remove-DirectoryIfExists $targetIncludeDir
	New-Item -ItemType Directory -Force -Path $targetIncludeDir | Out-Null

	Copy-Item -LiteralPath $mainLib -Destination (Join-Path $targetLibDir "libprojectM-4.lib") -Force
	Copy-Item -LiteralPath $playlistLib -Destination (Join-Path $targetLibDir "libprojectM-4-playlist.lib") -Force
	Copy-Item -Path (Join-Path $installedIncludeDir "*") -Destination $targetIncludeDir -Recurse -Force

	Write-Host "==> Done"
	Write-Host "Copied:"
	Write-Host "  $(Join-Path $targetLibDir 'libprojectM-4.lib')"
	Write-Host "  $(Join-Path $targetLibDir 'libprojectM-4-playlist.lib')"
	Write-Host "  $targetIncludeDir"
}
finally {
	Write-Host "==> Cleaning temporary build files"
	Remove-DirectoryIfExists $tempRoot
}
