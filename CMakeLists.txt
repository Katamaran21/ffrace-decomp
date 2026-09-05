cmake_minimum_required(VERSION 3.16)
project(ffrace C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

option(FFRACE_WARNINGS "Enable the full warning set" ON)
option(FFRACE_COPY_ASSETS "Copy BITMAP/Sounds/Musics next to the binary" ON)

set(FFRACE_SOURCES
    ff_appassets.c
    ff_assets.c
    ff_audio.c
    ff_audio_sdl2.c
    ff_bmp.c
    ff_gfx.c
    ff_gfx_tile.c
    ff_hud.c
    ff_ingame.c
    ff_lap.c
    ff_main.c
    ff_menu.c
    ff_menu_panels.c
    ff_menu_screens.c
    ff_mod.c
    ff_mod_effects.c
    ff_opponent.c
    ff_physics.c
    ff_platform_sdl2.c
    ff_race.c
    ff_racer.c
    ff_rand.c
    ff_render.c
    ff_road.c
    ff_screen.c
    ff_settings.c
    ff_ship.c
    ff_text.c
    ff_touch_sdl2.c)

find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    find_package(SDL2 QUIET)
endif()
if(NOT SDL2_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
    endif()
endif()
if(NOT SDL2_FOUND)
    message(FATAL_ERROR
            "SDL2 not found.  Install libsdl2-dev / sdl2 / vcpkg's sdl2, or pass "
            "-DCMAKE_PREFIX_PATH=<sdl2 prefix>.")
endif()

add_executable(ffrace WIN32 ${FFRACE_SOURCES})

if(TARGET SDL2::SDL2)
    target_link_libraries(ffrace PRIVATE SDL2::SDL2)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(ffrace PRIVATE SDL2::SDL2main)
    endif()
elseif(TARGET PkgConfig::SDL2)
    target_link_libraries(ffrace PRIVATE PkgConfig::SDL2)
else()
    target_include_directories(ffrace PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ffrace PRIVATE ${SDL2_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(ffrace PRIVATE shell32)
endif()

if(MSVC)
    target_compile_definitions(ffrace PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(NOT WIN32)
    include(CheckLibraryExists)
    check_library_exists(m sin "" FFRACE_NEED_LIBM)
    if(FFRACE_NEED_LIBM)
        target_link_libraries(ffrace PRIVATE m)
    endif()
endif()

if(FFRACE_WARNINGS)
    if(MSVC)
        target_compile_options(ffrace PRIVATE /W4)
    else()
        target_compile_options(ffrace PRIVATE -Wall -Wextra)
    endif()
endif()

if(FFRACE_COPY_ASSETS)
    foreach(dir BITMAP Sounds Musics)
        add_custom_command(TARGET ffrace POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy_directory
                                   ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
                                   $<TARGET_FILE_DIR:ffrace>/${dir})
    endforeach()
endif()

install(TARGETS ffrace RUNTIME DESTINATION bin)
install(DIRECTORY BITMAP Sounds Musics DESTINATION bin)
