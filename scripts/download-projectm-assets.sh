#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADDON_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PRESETS_REPO_URL="https://github.com/projectM-visualizer/presets-cream-of-the-crop.git"
TEXTURES_REPO_URL="https://github.com/projectM-visualizer/presets-milkdrop-texture-pack.git"
DOWNLOAD_PRESETS=0
DOWNLOAD_TEXTURES=0
KEEP_TEMP=0

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

ensure_dir() {
	mkdir -p "$1"
}

reset_dir() {
	rm -rf "$1"
	mkdir -p "$1"
}

copy_repo_contents() {
	local source_dir="$1"
	local target_dir="$2"
	reset_dir "$target_dir"
	find "$source_dir" -mindepth 1 -maxdepth 1 ! -name '.git' -exec cp -a {} "$target_dir"/ \;
}

example_targets() {
	local example_root="${ADDON_ROOT}/ofxProjectMExample"
	if [[ -d "$example_root" ]]; then
		printf '%s\n' "$example_root"
	fi
}

usage() {
	cat <<'EOF'
Usage:
  bash scripts/download-projectm-assets.sh [options]

Options:
  --presets                   Download the Cream of the Crop preset pack
  --textures                  Download the full Milkdrop texture pack
  --keep-temp                 Keep the temporary clone directory

Examples:
  bash scripts/download-projectm-assets.sh
  bash scripts/download-projectm-assets.sh --presets
  bash scripts/download-projectm-assets.sh --textures
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--presets)
			DOWNLOAD_PRESETS=1
			shift
			;;
		--textures)
			DOWNLOAD_TEXTURES=1
			shift
			;;
		--keep-temp)
			KEEP_TEMP=1
			shift
			;;
		--help|-h)
			usage
			exit 0
			;;
		*)
			die "Unknown argument: $1"
			;;
		esac
done

require_command git
require_command mktemp

TEMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/ofxProjectM-assets.XXXXXXXX")"
PRESETS_CLONE_DIR="${TEMP_ROOT}/presets"
TEXTURES_CLONE_DIR="${TEMP_ROOT}/textures"

cleanup() {
	if [[ "$KEEP_TEMP" -eq 0 ]]; then
		rm -rf "$TEMP_ROOT"
	else
		write_step "Keeping temporary files at ${TEMP_ROOT}"
	fi
}
trap cleanup EXIT

write_step "Using temporary download root: ${TEMP_ROOT}"

if [[ "$DOWNLOAD_PRESETS" -eq 0 && "$DOWNLOAD_TEXTURES" -eq 0 ]]; then
	DOWNLOAD_PRESETS=1
	DOWNLOAD_TEXTURES=1
	write_step "No asset selection provided, downloading the full preset and texture packs"
fi

if [[ "$DOWNLOAD_PRESETS" -eq 1 ]]; then
	write_step "Cloning Cream of the Crop preset pack"
	git clone --depth 1 "$PRESETS_REPO_URL" "$PRESETS_CLONE_DIR"
fi

if [[ "$DOWNLOAD_TEXTURES" -eq 1 ]]; then
	write_step "Cloning full Milkdrop texture pack"
	git clone --depth 1 "$TEXTURES_REPO_URL" "$TEXTURES_CLONE_DIR"
fi

FOUND_TARGET=0
while IFS= read -r example_root; do
	[[ -z "$example_root" ]] && continue
	FOUND_TARGET=1

	local_data_root="${example_root}/bin/data"
	ensure_dir "$local_data_root"

	if [[ "$DOWNLOAD_PRESETS" -eq 1 ]]; then
		write_step "Installing presets into ${example_root}/bin/data/presets"
		copy_repo_contents "$PRESETS_CLONE_DIR" "${local_data_root}/presets"
	fi

	if [[ "$DOWNLOAD_TEXTURES" -eq 1 ]]; then
		write_step "Installing textures into ${example_root}/bin/data/textures"
		copy_repo_contents "$TEXTURES_CLONE_DIR" "${local_data_root}/textures"
	fi
done < <(example_targets)

if [[ "$FOUND_TARGET" -eq 0 ]]; then
	die "No matching example folders found under ${ADDON_ROOT}"
fi

write_step "Done"
if [[ "$DOWNLOAD_PRESETS" -eq 1 ]]; then
	printf 'Installed preset pack from %s\n' "$PRESETS_REPO_URL"
fi
if [[ "$DOWNLOAD_TEXTURES" -eq 1 ]]; then
	printf 'Installed texture pack from %s\n' "$TEXTURES_REPO_URL"
fi
