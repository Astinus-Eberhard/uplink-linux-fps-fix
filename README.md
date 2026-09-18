# Uplink: Hacker Elite — Linux native performance fix

An `LD_PRELOAD` shim that fixes severe FPS collapse / CPU saturation / memory
growth in the **GOG native Linux build** of *Uplink: Hacker Elite* (v1.55,
compiled Aug 29 2011) on modern Mesa drivers (tested on `iris`, Intel Iris Xe /
Alder Lake-P, Mesa 26.0.3).

Not affiliated with Introversion Software or GOG. Unofficial community fix,
provided as-is (see [LICENSE](LICENSE)).

## Symptoms

- FPS collapses to less than 1 frame/s after roughly 10–15 minutes of play.
- One CPU core pinned near 100%.
- RAM usage grows continuously (observed: 77 MB → 404 MB over 18 minutes).
- On some driver overrides (e.g. `zink`), the CPU issue improves but UI
  textures get corrupted and RAM usage becomes erratic instead (up to 2.7 GB).

## Root cause

The game draws essentially all of its UI text (`FTGLBitmapFont` /
`GciDrawText`) using legacy `glBitmap()` calls — one unaccelerated
rasterization operation per glyph, per frame. On modern tile-based GPUs
through Mesa's `iris` driver this path is extremely expensive; as more UI
elements accumulate on screen during a session, per-frame cost grows until
the game becomes unplayable.

This is **not** a bug in `iris` or a missing driver feature — `glBitmap` is a
legacy immediate-mode OpenGL 1.x call that was never designed to be fast on
GPU architectures built for shader/texture pipelines. It just happens to be
the whole rendering strategy this 2001-era game engine uses for text.

## The fix

`libbitmapcache.so` intercepts `glBitmap()` via `LD_PRELOAD`:

1. Each unique glyph bitmap is converted **once** into a small `GL_ALPHA`
   texture and cached (keyed by a hash of the pixel data + all relevant
   `GL_UNPACK_*` pixel-store state).
2. Subsequent draws of the same glyph render a textured quad instead of
   re-rasterizing — with an explicit identity + pixel-exact ortho matrix
   swap around the draw, since `GL_CURRENT_RASTER_POSITION` is already in
   window space and must not be pushed through the game's active
   modelview/projection matrices a second time.
3. The real `glBitmap(0, 0, …)` (empty bitmap) is still called afterward, to
   advance the raster position exactly as the GL spec requires.

Two non-obvious bugs had to be fixed to get clean glyph rendering (both
documented in comments in `bitmapcache.c`):

- **Decode-side stride bug**: when `GL_UNPACK_ROW_LENGTH != 0`, it — not the
  `width` argument to `glBitmap` — determines the row stride of the source
  buffer per the GL spec. The game's small button font is packed in a
  fixed-cell sheet with `ROW_LENGTH` wider than the actual glyph width;
  ignoring this produced an alternating blank/real-row pattern in every
  decoded glyph.
- **Upload-side stride bug**: after fixing decoding, the freshly-built,
  tightly-packed alpha buffer was still uploaded via `glTexImage2D` while
  `GL_UNPACK_ROW_LENGTH` was left at the game's leftover value — the same
  class of bug, now on the upload path, producing visible texture noise even
  though the decoded data was already correct.

## Measured results

**Idle (main menu), 12 minutes, native `iris`:**

| | Baseline (unpatched) | `zink` override | This fix |
|---|---|---|---|
| CPU | 39% → 98% (climbing) | 47–57% (erratic) | flat 43.2–43.9% |
| RAM | 77 → 404 MB (18 min) | 386 MB – 2.7 GB (erratic) | flat 65788 KB, zero growth |

**Active gameplay soak test, 15 minutes, with the fix loaded**, measuring
*actual* frames/second (hooked at `SDL_GL_SwapBuffers`, not a CPU/RAM proxy):

- FPS: stayed in the **140–330 fps** range for the entire 900+ second run,
  never dropped anywhere near the original "&lt;1 fps" symptom.
- CPU: climbed from ~42% to a ~70% plateau within the first 5 minutes, then
  held flat (69–72%) for the remainder of the run — bounded, not runaway.
- RAM: grew from 66 MB to ~87 MB in the first few minutes (new glyphs/screens
  being cached for the first time), then held flat — an order of magnitude
  less than the original leak, and consistent with normal cache fill rather
  than a leak.

Rejected alternatives:
- `MESA_LOADER_DRIVER_OVERRIDE=zink` — fixes the CPU issue but corrupts
  JPEG/PNG textures (visible noise on the game's logo) and RAM usage becomes
  erratic (up to 2.7 GB).
- `LIBGL_ALWAYS_SOFTWARE=1` (llvmpipe) — no measurable improvement.
- `gamescope --framerate-limit` — does not help; it only throttles the
  compositor's presentation, not the game's internal render loop (confirmed
  via `strace -c` on the relevant ioctls).

## Download

Prebuilt `x86_64` binaries are attached to every release:

**[→ Latest release](https://github.com/Astinus-Eberhard/uplink-linux-fps-fix/releases/latest)**

Each release contains `libbitmapcache.so`, `launch-with-fix.sh`, a `.tar.gz`
with both plus this README and the LICENSE, and `SHA256SUMS` to verify the
download (`sha256sum -c SHA256SUMS`). The binaries are built from the tagged
source by GitHub Actions (see [`.github/workflows/release.yml`](.github/workflows/release.yml))
on Debian 12, so they require **glibc 2.34 or newer** — Ubuntu 22.04+, Debian
12+, Fedora 35+, current Arch/SteamOS. On anything older, build from source
(see [Building](#building)); there are no dependencies to hunt down.

## Usage

1. Download `libbitmapcache.so` and `launch-with-fix.sh` from the latest
   release, or build the library yourself (see below).
2. Copy `libbitmapcache.so` and `launch-with-fix.sh` into your Uplink install
   directory, next to `start.sh` (this is a GOG Linux installer target,
   typically something like `.../GOG Games/Uplink Hacker Elite/`).
3. `chmod +x launch-with-fix.sh`, then run `./launch-with-fix.sh` instead of
   `./start.sh`.

On startup the shim prints `[bitmapcache] glBitmap intercepted, caching glyphs
as textures` to stderr. **If that line is missing, the fix did not load** and
the game is running unpatched — most likely the install-path-with-spaces
problem described below.

The script does **not** modify your game installation — it sets
`LD_PRELOAD`/`LD_LIBRARY_PATH` and then `exec`s the original, unmodified
`start.sh`.

### ⚠️ GOG install paths with spaces

GOG's default install directories commonly contain spaces (e.g. `GOG
Games/Uplink Hacker Elite`). `LD_PRELOAD` is parsed by `ld.so` as a
space-separated list, so a full path containing spaces gets **silently**
split into several nonexistent "libraries" — the dynamic linker prints a
warning to stderr (easy to miss) and the game runs completely unpatched, with
no other symptom besides the fix simply not working.

`launch-with-fix.sh` works around this by adding the install directory to
`LD_LIBRARY_PATH` (colon-separated — spaces in the path are fine there) and
referencing the library in `LD_PRELOAD` by bare filename only, which `ld.so`
then resolves through `LD_LIBRARY_PATH`. If you're scripting your own
launcher instead of using the one here, keep this in mind.

## Building

```sh
make
# or directly:
gcc -Wall -shared -fPIC -O2 -o libbitmapcache.so bitmapcache.c -ldl
```

No dependencies beyond `libdl` — all GL types/constants used are declared by
hand in `bitmapcache.c`, so `libgl-dev` is not required.

## Tested environment

- Uplink: Hacker Elite v1.55 (RELEASE), GOG native Linux build, compiled Aug
  29 2011.
- Intel Alder Lake-P (Iris Xe integrated GPU), Mesa 26.0.3, `iris` driver.
- KDE Plasma, X11.

Other driver/GPU combinations are untested; issue reports and PRs welcome.
