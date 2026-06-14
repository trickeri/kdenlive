#!/usr/bin/env bash
# Build the Nuldrums Kdenlive fork on Arch Linux (Phase 1: clean build).
# See nuldrums/BUILD_ARCH.md for details. Not yet validated on hardware —
# review before running on the Arch box.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo ">> Nuldrums Kdenlive fork build (Arch)"
echo "   repo: $REPO_ROOT"
echo "   branch: $(git branch --show-current 2>/dev/null || echo '?')"

DEPS=(
  base-devel git cmake ninja extra-cmake-modules
  qt6-base qt6-multimedia qt6-svg qt6-declarative qt6-tools
  kcoreaddons kguiaddons ki18n kxmlgui kconfig kconfigwidgets
  kwidgetsaddons knewstuff knotifications knotifyconfig kio
  karchive kbookmarks kcrash kdbusaddons kiconthemes ktextwidgets
  kdeclarative kfilemetadata purpose
  mlt ffmpeg
  vulkan-icd-loader vulkan-headers shaderc glslang
)

if [[ "${SKIP_DEPS:-0}" != "1" ]]; then
  echo ">> Installing dependencies (sudo pacman). Set SKIP_DEPS=1 to skip."
  sudo pacman -S --needed "${DEPS[@]}"
fi

BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
echo ">> Configuring ($BUILD_DIR)"
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}" \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX:-$REPO_ROOT/install}"

echo ">> Building"
cmake --build "$BUILD_DIR" -j"${JOBS:-$(nproc)}"

echo
echo ">> Done. Run it with:"
echo "   $BUILD_DIR/bin/kdenlive"
