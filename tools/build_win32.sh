#!/bin/sh
# Cross-build the native Win32 binary (no SDL2) with mingw-w64.
#
# Needs gcc-mingw-w64-i686; for the 64-bit target set
#   MINGW_PREFIX=x86_64-w64-mingw32 sh tools/build_win32.sh
# The result runs under Wine as well as on Windows:
#   cd build-win32 && wine ffrace.exe
set -e

MINGW_PREFIX=${MINGW_PREFIX:-i686-w64-mingw32}
OUT=${OUT:-build-win32}

if ! command -v "$MINGW_PREFIX-gcc" >/dev/null 2>&1; then
    echo "no $MINGW_PREFIX-gcc - install gcc-mingw-w64-i686 (or set MINGW_PREFIX)" >&2
    exit 1
fi

cd "$(dirname "$0")/.."

cmake -S . -B "$OUT" -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=tools/toolchain-mingw-w64.cmake \
      -DMINGW_PREFIX="$MINGW_PREFIX" \
      -DFFRACE_BACKEND=win32
cmake --build "$OUT" --parallel

echo "BUILD OK: $OUT/ffrace.exe"
