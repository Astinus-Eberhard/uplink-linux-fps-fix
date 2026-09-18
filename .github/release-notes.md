Prebuilt `LD_PRELOAD` shim for the GOG native Linux build of *Uplink: Hacker Elite*.

## Install

1. Download `libbitmapcache.so` and `launch-with-fix.sh` below — or the `.tar.gz`, which also contains the README and LICENSE.
2. Copy both files into your Uplink install directory, next to `start.sh`.
3. `chmod +x launch-with-fix.sh`, then run `./launch-with-fix.sh` instead of `./start.sh`.

Your game installation is not modified: the script only sets `LD_PRELOAD`/`LD_LIBRARY_PATH` and execs the original `start.sh`.

On startup the shim prints `[bitmapcache] glBitmap intercepted, caching glyphs as textures` to stderr. **If that line does not appear, the fix is not loaded** and the game is running unpatched — see the note about GOG install paths containing spaces in the README.

## Binary details

- `x86_64` only, built on Debian 12 — requires glibc 2.34 or newer (Ubuntu 22.04+, Debian 12+, Fedora 35+, current Arch/SteamOS).
- No dependencies beyond libc/libdl; no OpenGL headers or libraries needed at build or run time.
- Built from this tag by GitHub Actions. Verify the download with `sha256sum -c SHA256SUMS`, or just run `make` and build it yourself.
