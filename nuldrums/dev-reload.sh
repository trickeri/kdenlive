#!/usr/bin/env bash
# Fast per-iteration reload of the Nuldrums stack on this Arch box.
#
# Refreshes whatever KRunner / the desktop launcher actually runs:
#   - fork binary  -> force-copied to ~/.local/bin/kdenlive
#                     (cmake --install silently skips on matching mtimes, so we copy)
#   - nulcaption   -> pipx snapshot reinstalled into ~/.local/bin/nulcaption
# then kills any running kdenlive so the next launch picks up fresh code.
#
# Usage:
#   ./dev-reload.sh            # both: build+install fork, reinstall engine
#   ./dev-reload.sh fork       # fork (C++) only
#   ./dev-reload.sh engine     # nulcaption (Python) only
#   ./dev-reload.sh --no-kill  # skip the pkill (append to any mode)
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
LOCAL_BIN="${LOCAL_BIN:-$HOME/.local/bin}"
NULCAPTION_REPO="${NULCAPTION_REPO:-$REPO_ROOT/../nulcaption}"

MODE="both"
DO_KILL=1
for arg in "$@"; do
  case "$arg" in
    fork|engine|both) MODE="$arg" ;;
    --no-kill)        DO_KILL=0 ;;
    *) echo "unknown arg: $arg" >&2; exit 2 ;;
  esac
done

reload_fork() {
  echo ">> [fork] building kdenlive ($BUILD_DIR)"
  cmake --build "$BUILD_DIR" --target kdenlive -j"${JOBS:-$(nproc)}"
  echo ">> [fork] force-installing -> $LOCAL_BIN/kdenlive"
  install -m755 "$BUILD_DIR/bin/kdenlive" "$LOCAL_BIN/kdenlive"
}

reload_engine() {
  echo ">> [engine] pipx reinstall nulcaption ($NULCAPTION_REPO)"
  # --force reinstalls if present, else first-time installs.
  # --system-site-packages lets the venv see the system PySide6 the settings
  # window (nulcaption-settings) needs, without downloading it into the venv.
  pipx install --force --system-site-packages "$NULCAPTION_REPO"
}

[[ "$MODE" == "both" || "$MODE" == "fork"   ]] && reload_fork
[[ "$MODE" == "both" || "$MODE" == "engine" ]] && reload_engine

if [[ "$DO_KILL" == 1 ]]; then
  if pgrep -x kdenlive >/dev/null; then
    echo ">> killing running kdenlive (relaunch via KRunner to test)"
    pkill -x kdenlive || true
  fi
fi

echo ">> done ($MODE). Launch from KRunner to test the installed copies."
