#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADDON_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECTM_REPO_URL="https://github.com/projectM-visualizer/projectm.git"
PROJECTM_REPO_BRANCH="master"
WINDOWS_GENERATOR="Visual Studio 18 2026"
WINDOWS_PLATFORM="x64"
WINDOWS_CONFIGURATION=""
BUILD_TARGET="${BUILD_TARGET:-auto}"
OSX_ARCHITECTURES="${OSX_ARCHITECTURES:-arm64;x86_64}"
OSX_DEPLOYMENT_TARGET="${OSX_DEPLOYMENT_TARGET:-}"

write_step() {
	printf '==> %s\n' "$1"
}

die() {
	printf '%s\n' "$1" >&2
	exit 1
}

require_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		die "Required command not found: $1"
	fi
}

is_wsl() {
	if [[ -n "${WSL_INTEROP:-}" || -n "${WSL_DISTRO_NAME:-}" ]]; then
		return 0
	fi

	if [[ -r /proc/sys/kernel/osrelease ]] && grep -qi 'microsoft' /proc/sys/kernel/osrelease; then
		return 0
	fi

	return 1
}

resolve_build_target() {
	local uname_s
	uname_s="$(uname -s)"

	case "${BUILD_TARGET}" in
		auto)
			case "${uname_s}" in
				Darwin)
					printf '%s\n' "macos"
					;;
				Linux)
					if is_wsl; then
						printf '%s\n' "windows"
					else
						printf '%s\n' "linux"
					fi
					;;
				MINGW*|MSYS*|CYGWIN*)
					printf '%s\n' "windows"
					;;
				*)
					printf '%s\n' "unsupported"
					;;
			esac
			;;
		windows|linux|macos)
			printf '%s\n' "${BUILD_TARGET}"
			;;
		*)
			die "Unknown target '${BUILD_TARGET}'. Supported values: auto, windows, linux, macos"
			;;
	esac
}

to_windows_path() {
	local input_path="$1"
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -w "$input_path"
	elif command -v wslpath >/dev/null 2>&1; then
		wslpath -w "$input_path"
	else
		die "Could not convert path to Windows form: $input_path"
	fi
}

run_unix_build() {
	local target_lib_dir="$1"
	local build_dir_name="$2"
	local stage_dir_name="$3"
	shift 3

	local temp_root projectm_root build_dir stage_dir
	temp_root="$(mktemp -d "${TMPDIR:-/tmp}/ofxProjectM.XXXXXXXX")"
	projectm_root="${temp_root}/projectm"
	build_dir="${temp_root}/${build_dir_name}"
	stage_dir="${temp_root}/${stage_dir_name}"
	trap 'write_step "Cleaning temporary build files"; rm -rf "'"${temp_root}"'"' EXIT

	require_command git
	require_command cmake

	write_step "Using temporary build root: ${temp_root}"
	write_step "Fetching current projectM master"
	git clone --branch "${PROJECTM_REPO_BRANCH}" --depth 1 --recursive "${PROJECTM_REPO_URL}" "${projectm_root}"

	write_step "Configuring build"
	cmake "$@" -S "${projectm_root}" -B "${build_dir}"

	write_step "Building"
	cmake --build "${build_dir}" --parallel

	write_step "Installing into staging dir"
	rm -rf "${stage_dir}"
	cmake --install "${build_dir}" --prefix "${stage_dir}"

	local main_lib playlist_lib installed_include_dir target_include_dir
	main_lib="$(find "${stage_dir}" -type f -name 'libprojectM-4.a' | head -n 1)"
	playlist_lib="$(find "${stage_dir}" -type f -name 'libprojectM-4-playlist.a' | head -n 1)"
	installed_include_dir="${stage_dir}/include/projectM-4"
	target_include_dir="${ADDON_ROOT}/libs/projectM/include/projectM-4"

	[[ -n "${main_lib}" && -f "${main_lib}" ]] || die "Could not find built libprojectM-4.a in ${stage_dir}"
	[[ -n "${playlist_lib}" && -f "${playlist_lib}" ]] || die "Could not find built libprojectM-4-playlist.a in ${stage_dir}"
	[[ -d "${installed_include_dir}" ]] || die "Could not find installed headers in ${installed_include_dir}"

	write_step "Copying libs and headers into addon"
	mkdir -p "${target_lib_dir}"
	cp -f "${main_lib}" "${target_lib_dir}/libprojectM-4.a"
	cp -f "${playlist_lib}" "${target_lib_dir}/libprojectM-4-playlist.a"
	rm -rf "${target_include_dir}"
	mkdir -p "${target_include_dir}"
	cp -a "${installed_include_dir}/." "${target_include_dir}/"

	write_step "Done"
	printf 'Copied:\n'
	printf '  %s\n' "${target_lib_dir}/libprojectM-4.a"
	printf '  %s\n' "${target_lib_dir}/libprojectM-4-playlist.a"
	printf '  %s\n' "${target_include_dir}"
}

run_windows_build() {
	require_command powershell.exe

	local temp_root ps_script ps_script_win addon_root_win
	temp_root="$(mktemp -d "${TMPDIR:-/tmp}/ofxProjectM-windows-shell.XXXXXXXX")"
	ps_script="${temp_root}/build_projectm_windows.ps1"
	trap 'rm -rf "'"${temp_root}"'"' EXIT

	cat > "${ps_script}" <<'EOF'
param(
	[string]$AddonRoot = "",
	[string]$Generator = "Visual Studio 18 2026",
	[string]$Platform = "x64",
	[string[]]$Configurations = @("Debug", "Release")
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

function Find-StagedLibrary {
	param(
		[string]$StageDir,
		[string[]]$CandidateNames
	)

	foreach ($Candidate in $CandidateNames) {
		$Match = Get-ChildItem -Path $StageDir -Recurse -Filter $Candidate | Select-Object -First 1 -ExpandProperty FullName
		if ($Match -and (Test-Path $Match)) {
			return $Match
		}
	}

	return $null
}

if ([string]::IsNullOrWhiteSpace($AddonRoot)) {
	$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
	$AddonRoot = Split-Path -Parent $scriptDir
}

$targetLibDir = Join-Path $AddonRoot "libs\projectM\lib\vs"
$targetIncludeDir = Join-Path $AddonRoot "libs\projectM\include\projectM-4"
$projectmRepoUrl = "https://github.com/projectM-visualizer/projectm.git"
$projectmRepoBranch = "master"
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("ofxProjectM_" + [System.Guid]::NewGuid().ToString("N"))
$projectmRoot = Join-Path $tempRoot "projectm"
$buildDir = Join-Path $tempRoot "build-windows-static"
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

	$copiedInclude = $false
	foreach ($Configuration in $Configurations) {
		$stageDir = Join-Path $tempRoot ("build-windows-stage-" + $Configuration)

		Write-Host "==> Building $Configuration"
		Invoke-NativeCommand cmake @(
			"--build", $buildDir,
			"--config", $Configuration,
			"--parallel"
		)

		Write-Host "==> Installing $Configuration into staging dir"
		Invoke-NativeCommand cmake @(
			"--install", $buildDir,
			"--config", $Configuration,
			"--prefix", $stageDir
		)

		$mainLibCandidates = @("libprojectM-4.lib")
		$playlistLibCandidates = @("libprojectM-4-playlist.lib")
		if ($Configuration -ieq "Debug") {
			$mainLibCandidates = @("libprojectM-4d.lib", "libprojectM-4.lib")
			$playlistLibCandidates = @("libprojectM-4-playlistd.lib", "libprojectM-4-playlist.lib")
		}

		$mainLib = Find-StagedLibrary -StageDir $stageDir -CandidateNames $mainLibCandidates
		$playlistLib = Find-StagedLibrary -StageDir $stageDir -CandidateNames $playlistLibCandidates
		$installedIncludeDir = Join-Path $stageDir "include\projectM-4"
		$configTargetLibDir = Join-Path $targetLibDir $Configuration

		if (-not $mainLib -or -not (Test-Path $mainLib)) {
			throw "Could not find built libprojectM-4(.d).lib in $stageDir"
		}

		if (-not $playlistLib -or -not (Test-Path $playlistLib)) {
			throw "Could not find built libprojectM-4-playlist(.d).lib in $stageDir"
		}

		if (-not (Test-Path $installedIncludeDir)) {
			throw "Could not find installed headers in $installedIncludeDir"
		}

		Write-Host "==> Copying $Configuration libs into addon"
		New-Item -ItemType Directory -Force -Path $configTargetLibDir | Out-Null
		Copy-Item -LiteralPath $mainLib -Destination (Join-Path $configTargetLibDir "libprojectM-4.lib") -Force
		Copy-Item -LiteralPath $playlistLib -Destination (Join-Path $configTargetLibDir "libprojectM-4-playlist.lib") -Force

		if (-not $copiedInclude) {
			Remove-DirectoryIfExists $targetIncludeDir
			New-Item -ItemType Directory -Force -Path $targetIncludeDir | Out-Null
			Copy-Item -Path (Join-Path $installedIncludeDir "*") -Destination $targetIncludeDir -Recurse -Force
			$copiedInclude = $true
		}

		Write-Host "Copied:"
		Write-Host "  $(Join-Path $configTargetLibDir 'libprojectM-4.lib')"
		Write-Host "  $(Join-Path $configTargetLibDir 'libprojectM-4-playlist.lib')"
	}

	Write-Host "  $targetIncludeDir"
}
finally {
	Write-Host "==> Cleaning temporary build files"
	Remove-DirectoryIfExists $tempRoot
}
EOF

	ps_script_win="$(to_windows_path "${ps_script}")"
	addon_root_win="$(to_windows_path "${ADDON_ROOT}")"
	if [[ -n "${WINDOWS_CONFIGURATION}" ]]; then
		powershell.exe -ExecutionPolicy Bypass -File "${ps_script_win}" \
			-AddonRoot "${addon_root_win}" \
			-Generator "${WINDOWS_GENERATOR}" \
			-Platform "${WINDOWS_PLATFORM}" \
			-Configurations "${WINDOWS_CONFIGURATION}"
	else
		powershell.exe -ExecutionPolicy Bypass -File "${ps_script_win}" \
			-AddonRoot "${addon_root_win}" \
			-Generator "${WINDOWS_GENERATOR}" \
			-Platform "${WINDOWS_PLATFORM}"
	fi
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--target)
			BUILD_TARGET="${2:-}"
			shift 2
			;;
		--generator)
			WINDOWS_GENERATOR="${2:-}"
			shift 2
			;;
		--platform)
			WINDOWS_PLATFORM="${2:-}"
			shift 2
			;;
		--configuration)
			WINDOWS_CONFIGURATION="${2:-}"
			shift 2
			;;
		*)
			die "Unknown argument: $1"
			;;
	esac
done

SELECTED_BUILD_TARGET="$(resolve_build_target)"

if [[ "${BUILD_TARGET}" == "auto" && "${SELECTED_BUILD_TARGET}" == "windows" ]] && is_wsl; then
	write_step "Detected WSL; using the Windows/Visual Studio build path. Pass --target linux to force Linux output."
fi

case "${SELECTED_BUILD_TARGET}" in
	macos)
		mac_cmake_args=(
			-DCMAKE_BUILD_TYPE=Release
			-DCMAKE_OSX_ARCHITECTURES="${OSX_ARCHITECTURES}"
			-DBUILD_SHARED_LIBS=OFF
			-DBUILD_TESTING=OFF
			-DENABLE_SDL_UI=OFF
			-DENABLE_INSTALL=ON
		)
		if [[ -n "${OSX_DEPLOYMENT_TARGET}" ]]; then
			mac_cmake_args+=(-DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_DEPLOYMENT_TARGET}")
		fi
		run_unix_build \
			"${ADDON_ROOT}/libs/projectM/lib/osx" \
			"build-macos-static" \
			"build-macos-stage" \
			"${mac_cmake_args[@]}"
		;;
	linux)
		run_unix_build \
			"${ADDON_ROOT}/libs/projectM/lib/linux64" \
			"build-linux-static" \
			"build-linux-stage" \
			-DCMAKE_BUILD_TYPE=Release \
			-DBUILD_SHARED_LIBS=OFF \
			-DBUILD_TESTING=OFF \
			-DENABLE_SDL_UI=OFF \
			-DENABLE_INSTALL=ON
		;;
	windows)
		run_windows_build
		;;
	*)
		die "Unsupported platform/target combination: $(uname -s) / ${SELECTED_BUILD_TARGET}"
		;;
esac
