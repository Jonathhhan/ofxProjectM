#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADDON_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DEFAULT_VLC_ADDON_ROOT="$(cd "${ADDON_ROOT}/../ofxVlc4" 2>/dev/null && pwd || true)"
VLC_ADDON_ROOT="${DEFAULT_VLC_ADDON_ROOT}"
TARGET_EXAMPLE_NAME="ofxProjectMExample"

write_step() {
	printf '==> %s\n' "$1"
}

die() {
	printf '%s\n' "$1" >&2
	exit 1
}

ensure_dir() {
	mkdir -p "$1"
}

reset_dir() {
	rm -rf "$1"
	mkdir -p "$1"
}

copy_dir_contents() {
	local source_dir="$1"
	local target_dir="$2"
	if [[ ! -d "$source_dir" ]]; then
		return 0
	fi
	reset_dir "$target_dir"
	cp -R "$source_dir"/. "$target_dir"/
}

usage() {
	cat <<'EOF'
Usage:
  bash scripts/sync_vlc_runtime.sh [options]

Options:
  --vlc-addon-root PATH   Path to the sibling ofxVlc4 addon
  --help                  Show this help
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--vlc-addon-root)
			VLC_ADDON_ROOT="${2:-}"
			shift 2
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

[[ -n "${VLC_ADDON_ROOT}" && -d "${VLC_ADDON_ROOT}" ]] || die "Could not find ofxVlc4 addon. Pass it with --vlc-addon-root."

TARGET_EXAMPLE="${ADDON_ROOT}/${TARGET_EXAMPLE_NAME}"
[[ -d "${TARGET_EXAMPLE}" ]] || die "Missing target example at ${TARGET_EXAMPLE}."

SOURCE_RUNTIME="${VLC_ADDON_ROOT}/libs/libvlc/runtime/vs/x64"
[[ -d "${SOURCE_RUNTIME}" ]] || die "Missing VLC runtime at ${SOURCE_RUNTIME}. Run ofxVlc4/scripts/install-libvlc.sh first."

BIN_DIR="${TARGET_EXAMPLE}/bin"
DATA_DIR="${BIN_DIR}/data"
DLL_DIR="${TARGET_EXAMPLE}/dll/x64"

write_step "Syncing VLC runtime from ${SOURCE_RUNTIME}"
ensure_dir "${BIN_DIR}"
ensure_dir "${DATA_DIR}"
reset_dir "${DLL_DIR}"

cp -f "${SOURCE_RUNTIME}/libvlc.dll" "${DLL_DIR}/libvlc.dll"
cp -f "${SOURCE_RUNTIME}/libvlccore.dll" "${DLL_DIR}/libvlccore.dll"
cp -f "${SOURCE_RUNTIME}/libvlc.dll" "${BIN_DIR}/libvlc.dll"
cp -f "${SOURCE_RUNTIME}/libvlccore.dll" "${BIN_DIR}/libvlccore.dll"
find "${BIN_DIR}" -maxdepth 1 -type f -name '*_plugin.dll' -delete
copy_dir_contents "${SOURCE_RUNTIME}/plugins" "${BIN_DIR}/plugins"
if [[ -d "${SOURCE_RUNTIME}/lua" ]]; then
	copy_dir_contents "${SOURCE_RUNTIME}/lua" "${BIN_DIR}/lua"
else
	rm -rf "${BIN_DIR}/lua"
fi

write_step "Done"
printf 'Synced runtime into:\n'
printf '  %s\n' "${BIN_DIR}"
printf '  %s\n' "${DLL_DIR}"
