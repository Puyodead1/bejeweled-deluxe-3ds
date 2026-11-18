cmake_minimum_required(VERSION 3.20)

# Bejeweled Deluxe 1.87 for Nintendo Switch (homebrew .nro).
# Configure with the devkitPro toolchain from the devkitPro msys2 shell; build.ps1 does that for you:
#   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake
project(Bejeweled VERSION 1.87.0 LANGUAGES C CXX)

if(NOT NINTENDO_SWITCH)
    message(FATAL_ERROR "Configure with -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake (see build.ps1)")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PORTLIBS ${DEVKITPRO}/portlibs/switch)

# SDL2 and libmodplug from dkp-pacman (switch-sdl2, switch-libmodplug). Their pkg-config files work too, but the
# link line is short and fixed, so it is spelled out here.
add_library(nx_sdl2 INTERFACE)
target_include_directories(nx_sdl2 INTERFACE ${PORTLIBS}/include ${PORTLIBS}/include/SDL2)
target_link_directories(nx_sdl2 INTERFACE ${PORTLIBS}/lib)
target_link_libraries(nx_sdl2 INTERFACE SDL2 EGL glapi drm_nouveau nx)

add_library(nx_modplug INTERFACE)
target_include_directories(nx_modplug INTERFACE ${PORTLIBS}/include)
target_link_directories(nx_modplug INTERFACE ${PORTLIBS}/lib)
target_link_libraries(nx_modplug INTERFACE modplug stdc++ m)

# --- Phase 1: toolchain smoke test ------------------------------------------------------------------
# Proves SDL2 video/audio/input, libmodplug and the packed romfs on hardware or in an emulator.
add_executable(hello platform/switch/hello.cpp)
target_link_libraries(hello PRIVATE nx_modplug nx_sdl2)

nx_generate_nacp(hello.nacp NAME "Bejeweled Deluxe" AUTHOR "PopCap Games" VERSION "${PROJECT_VERSION}")

set(ROMFS_DIR ${CMAKE_SOURCE_DIR}/romfs)
if(NOT IS_DIRECTORY ${ROMFS_DIR})
    message(FATAL_ERROR "romfs/ is missing: run tools/pack_data.ps1 to copy the game data in")
endif()

nx_create_nro(hello
    OUTPUT ${CMAKE_BINARY_DIR}/hello.nro
    ICON ${CMAKE_SOURCE_DIR}/res/icon.jpg
    NACP ${CMAKE_BINARY_DIR}/hello.nacp
    ROMFS ${ROMFS_DIR}
)
