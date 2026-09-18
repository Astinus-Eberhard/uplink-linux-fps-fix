#!/bin/bash
# Launches Uplink: Hacker Elite (GOG native Linux build) with the glBitmap
# performance fix. Does NOT modify the game installation — it only injects
# LD_PRELOAD/LD_LIBRARY_PATH and then execs the original start.sh.
#
# Usage: copy libbitmapcache.so and this script into your Uplink install
# directory (next to start.sh), then run ./launch-with-fix.sh instead of
# ./start.sh.
#
# NOTE: GOG installs commonly use directory names containing spaces (e.g.
# "GOG Games/Uplink Hacker Elite"). LD_PRELOAD is a space-separated list in
# ld.so, so a full path with spaces gets silently split into several
# nonexistent "libraries" and the fix fails to load with NO visible error —
# the game just runs unpatched. Workaround: add the install directory to
# LD_LIBRARY_PATH (colon-separated, spaces are fine there) and reference the
# library in LD_PRELOAD by bare filename only.
set -e
BINDIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$BINDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_PRELOAD="libbitmapcache.so${LD_PRELOAD:+:$LD_PRELOAD}"
exec "$BINDIR/start.sh" "$@"
