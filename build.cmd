@echo off
setlocal
set VCVARS=D:\VS\BuildTools\VC\Auxiliary\Build\vcvars32.bat
set SDL=D:\vcpkg\installed\x86-windows

if not exist "%VCVARS%" echo missing %VCVARS% && exit /b 1
if not exist "%SDL%\lib\SDL2.lib" echo missing %SDL%\lib\SDL2.lib && exit /b 1

call "%VCVARS%" >nul || exit /b 1
cd /d "%~dp0"
if not exist build mkdir build

cl /nologo /W4 /O2 /TC /MT /D_CRT_SECURE_NO_WARNINGS /I"%SDL%\include\SDL2" /Fobuild\ /Febuild\ffrace.exe ^
    ff_gfx.c ff_gfx_tile.c ff_bmp.c ff_text.c ff_mod.c ff_mod_effects.c ff_screen.c ff_settings.c ff_menu.c ff_menu_screens.c ff_menu_panels.c ff_rand.c ff_race.c ff_racer.c ff_physics.c ff_ingame.c ff_hud.c ff_assets.c ff_appassets.c ff_platform_sdl2.c ff_audio_sdl2.c ff_audio.c ff_touch_sdl2.c ff_main.c ff_render.c ff_road.c ff_ship.c ff_opponent.c ff_lap.c ^
    /link /SUBSYSTEM:WINDOWS /NODEFAULTLIB:msvcrt.lib "%SDL%\lib\SDL2.lib" "%SDL%\lib\manual-link\SDL2main.lib" shell32.lib
if errorlevel 1 exit /b 1

copy /y "%SDL%\bin\SDL2.dll" build\ >nul

if not exist build\BITMAP mkdir build\BITMAP
copy /y BITMAP\*.bmp build\BITMAP\ >nul
if exist Sounds\*.wav (
    if not exist build\Sounds mkdir build\Sounds
    copy /y Sounds\*.wav build\Sounds\ >nul
)
if exist Musics\*.tkm (
    if not exist build\Musics mkdir build\Musics
    copy /y Musics\*.tkm build\Musics\ >nul
)

echo BUILD OK: build\ffrace.exe
