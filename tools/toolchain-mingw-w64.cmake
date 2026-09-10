# CMake toolchain for cross-building the Windows binary with mingw-w64.
#
#   cmake -S . -B build-win32 -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_TOOLCHAIN_FILE=tools/toolchain-mingw-w64.cmake \
#         -DFFRACE_BACKEND=win32
#
# Defaults to the 32-bit target, which is what runs on the widest range of
# Windows versions.  For 64-bit pass -DMINGW_PREFIX=x86_64-w64-mingw32.
#
# FFRACE_BACKEND=sdl2 works through this file too, provided a mingw-w64 build
# of SDL2 is on CMAKE_PREFIX_PATH.
set(CMAKE_SYSTEM_NAME Windows)

if(NOT DEFINED MINGW_PREFIX)
    set(MINGW_PREFIX i686-w64-mingw32)
endif()

if(MINGW_PREFIX MATCHES "^x86_64")
    set(CMAKE_SYSTEM_PROCESSOR AMD64)
else()
    set(CMAKE_SYSTEM_PROCESSOR x86)
endif()

set(CMAKE_C_COMPILER   ${MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER  ${MINGW_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
