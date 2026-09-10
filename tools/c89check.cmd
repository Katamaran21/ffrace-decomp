@echo off
rem Strict ANSI C (C89) conformance pass with the clang frontend bundled in
rem Visual Studio Build Tools. Compiles nothing - parses every .c the port
rem builds and reports anything MSVC accepts but a strict C89 compiler rejects.
setlocal
set VCVARS=D:\VS\BuildTools\VC\Auxiliary\Build\vcvars32.bat
set CLANG=D:\VS\BuildTools\VC\Tools\Llvm\bin\clang-tidy.exe
set SDL=D:\vcpkg\installed\x86-windows

call "%VCVARS%" >nul 2>&1
cd /d "%~dp0.."

set RC=0
for %%F in (ff_*.c) do (
  rem The native backend includes windows.h, and the SDK headers are not C89:
  rem they use inline, long long and unnamed unions, so -pedantic-errors fires
  rem inside the SDK before it ever reaches our code.  MSVC and mingw-w64 still
  rem compile those two files in the normal C90 build.
  echo %%F | findstr /i /c:"_win32.c" >nul
  if errorlevel 1 (
    "%CLANG%" --quiet --checks=clang-diagnostic-* "%%F" -- -std=c89 -pedantic-errors -Wall ^
      -isystem "%SDL%\include" -isystem "%SDL%\include\SDL2" -D_WIN32 ^
      -D_CRT_SECURE_NO_WARNINGS || set RC=1
  )
)
if "%RC%"=="0" echo C89 OK
exit /b %RC%
