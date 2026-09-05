@echo off
setlocal
set VCVARS=D:\VS\BuildTools\VC\Auxiliary\Build\vcvars32.bat
set CMAKE=D:\vcpkg\downloads\tools\cmake-3.30.1-windows\cmake-3.30.1-windows-i386\bin\cmake.exe
set NINJA=D:/vcpkg/downloads/tools/ninja/1.12.1-windows/ninja.exe
set TOOLCHAIN=D:/vcpkg/scripts/buildsystems/vcpkg.cmake

if not exist "%VCVARS%" echo missing %VCVARS% && exit /b 1
if not exist "%CMAKE%" echo missing %CMAKE% && exit /b 1

call "%VCVARS%" >nul || exit /b 1
cd /d "%~dp0"

"%CMAKE%" -S . -B build-cmake -G Ninja -DCMAKE_MAKE_PROGRAM=%NINJA% ^
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN% ^
    -DVCPKG_TARGET_TRIPLET=x86-windows || exit /b 1
"%CMAKE%" --build build-cmake || exit /b 1

echo CMAKE BUILD OK
