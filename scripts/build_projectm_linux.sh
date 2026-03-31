#!/usr/bin/env bash
set -euo pipefail

# Downloads the current projectM master checkout into ofxProjectM/_projectm_build,
# builds the static Linux libraries, and copies the results into the addon's
# libs/projectM folder structure.
#
# Run this inside WSL, for example:
#   bash scripts/build_projectm_linux.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADDON_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECTM_ROOT="${ADDON_ROOT}/_projectm_build"
BUILD_DIR="${PROJECTM_ROOT}/build-linux-static"
STAGE_DIR="${PROJECTM_ROOT}/build-linux-stage"
TARGET_LIB_DIR="${ADDON_ROOT}/libs/projectM/lib/linux64"
TARGET_INCLUDE_DIR="${ADDON_ROOT}/libs/projectM/include/projectM-4"
PROJECTM_REPO_URL="https://github.com/projectM-visualizer/projectm.git"
PROJECTM_REPO_BRANCH="master"

require_command() {
	local command_name="$1"
	if ! command -v "${command_name}" >/dev/null 2>&1; then
		echo "Required command not found: ${command_name}" >&2
		exit 1
	fi
}

require_command git
require_command cmake

echo "==> Fetching current projectM master"
rm -rf "${PROJECTM_ROOT}"
git clone --branch "${PROJECTM_REPO_BRANCH}" --depth 1 --recursive "${PROJECTM_REPO_URL}" "${PROJECTM_ROOT}"

echo "==> Configuring Linux static build"
rm -rf "${BUILD_DIR}"
cmake -S "${PROJECTM_ROOT}" -B "${BUILD_DIR}" \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_SHARED_LIBS=OFF \
	-DBUILD_TESTING=OFF \
	-DENABLE_SDL_UI=OFF \
	-DENABLE_INSTALL=ON

echo "==> Building"
cmake --build "${BUILD_DIR}" --parallel

echo "==> Installing into staging dir"
rm -rf "${STAGE_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${STAGE_DIR}"

MAIN_LIB="$(find "${STAGE_DIR}" -type f -name 'libprojectM-4.a' | head -n 1)"
PLAYLIST_LIB="$(find "${STAGE_DIR}" -type f -name 'libprojectM-4-playlist.a' | head -n 1)"
INSTALLED_INCLUDE_DIR="${STAGE_DIR}/include/projectM-4"

if [[ -z "${MAIN_LIB}" || ! -f "${MAIN_LIB}" ]]; then
	echo "Could not find built libprojectM-4.a in ${STAGE_DIR}" >&2
	exit 1
fi

if [[ -z "${PLAYLIST_LIB}" || ! -f "${PLAYLIST_LIB}" ]]; then
	echo "Could not find built libprojectM-4-playlist.a in ${STAGE_DIR}" >&2
	exit 1
fi

if [[ ! -d "${INSTALLED_INCLUDE_DIR}" ]]; then
	echo "Could not find installed headers in ${INSTALLED_INCLUDE_DIR}" >&2
	exit 1
fi

echo "==> Copying libs and headers into addon"
mkdir -p "${TARGET_LIB_DIR}"
mkdir -p "${TARGET_INCLUDE_DIR}"

cp -f "${MAIN_LIB}" "${TARGET_LIB_DIR}/libprojectM-4.a"
cp -f "${PLAYLIST_LIB}" "${TARGET_LIB_DIR}/libprojectM-4-playlist.a"
rm -rf "${TARGET_INCLUDE_DIR}"
mkdir -p "${TARGET_INCLUDE_DIR}"
cp -a "${INSTALLED_INCLUDE_DIR}/." "${TARGET_INCLUDE_DIR}/"

echo "==> Cleaning temporary build files"
rm -rf "${PROJECTM_ROOT}"

echo "==> Done"
echo "Copied:"
echo "  ${TARGET_LIB_DIR}/libprojectM-4.a"
echo "  ${TARGET_LIB_DIR}/libprojectM-4-playlist.a"
echo "  ${TARGET_INCLUDE_DIR}"
