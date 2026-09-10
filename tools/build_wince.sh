#!/bin/sh
# Windows CE (Pocket PC / Windows Mobile) build with cegcc's mingw32ce.
#
# The native backend is the only option here: there is no SDL2 for Windows CE.
# ff_platform_win32.c draws through GDI and ff_audio_win32.c plays through
# waveOut, and both live in coredll.dll on the device.
#
# The toolchain prefix defaults to the usual cegcc install; override it with
#   CEGCC=/path/to/mingw32ce sh tools/build_wince.sh
# and pick the target arch with TARGET=arm-mingw32ce (or i386-mingw32ce).
set -e

CEGCC=${CEGCC:-/opt/mingw32ce}
TARGET=${TARGET:-arm-mingw32ce}
CC=${CC:-$CEGCC/bin/$TARGET-gcc}
STRIP=${STRIP:-$CEGCC/bin/$TARGET-strip}
OUT=${OUT:-build-wince}

if [ ! -x "$CC" ]; then
    echo "no $CC - set CEGCC to the cegcc prefix or TARGET to the arch" >&2
    exit 1
fi

cd "$(dirname "$0")/.."

# cegcc's windows.h uses "inline" in kfuncs.h, which -std=c89 rejects, so the
# port is compiled as gnu89: the same C89 code, with the keyword still a
# keyword.  tools/c89check.cmd and the Linux CI job cover strict conformance.
CFLAGS="-O2 -Wall -Wextra -std=gnu89 -DFF_BACKEND_WIN32 -fno-strict-aliasing"
LDFLAGS="-lcoredll -lm"

SRC="ff_appassets.c ff_assets.c ff_audio.c ff_bmp.c ff_gfx.c ff_gfx_tile.c \
ff_hud.c ff_ingame.c ff_lap.c ff_main.c ff_menu.c ff_menu_panels.c \
ff_menu_screens.c ff_mod.c ff_mod_effects.c ff_opponent.c ff_physics.c \
ff_race.c ff_racer.c ff_rand.c ff_render.c ff_results.c ff_road.c \
ff_screen.c ff_settings.c ff_ship.c ff_text.c ff_platform_win32.c \
ff_audio_win32.c"

rm -rf "$OUT"
mkdir -p "$OUT"

# shellcheck disable=SC2086
"$CC" $CFLAGS -o "$OUT/ffrace.exe" $SRC $LDFLAGS
"$STRIP" "$OUT/ffrace.exe"

for dir in BITMAP Sounds Musics; do
    mkdir -p "$OUT/$dir"
    cp "$dir"/* "$OUT/$dir/"
done

echo "BUILD OK: $OUT/ffrace.exe"
echo "Copy $OUT to the device, e.g. \\Program Files\\FFRace, and run"
echo "ffrace.exe there - it looks for BITMAP/Sounds/Musics next to itself."
