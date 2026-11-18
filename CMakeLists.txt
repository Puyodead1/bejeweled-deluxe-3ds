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

# Image and Ogg codecs, also from dkp-pacman (switch-libpng, switch-libjpeg-turbo, switch-zlib, switch-libvorbisidec,
# switch-libogg). They replace the 1999-era copies the Windows game shipped with.
add_library(nx_codecs INTERFACE)
target_include_directories(nx_codecs INTERFACE ${PORTLIBS}/include)
target_link_directories(nx_codecs INTERFACE ${PORTLIBS}/lib)
target_link_libraries(nx_codecs INTERFACE png jpeg vorbisidec ogg z)

# Old MSVC code: char is signed there, and a few pre-standard constructs only get warnings with -fpermissive.
set(SEXY_CXX_FLAGS -fsigned-char -fpermissive -fno-strict-aliasing -Wno-write-strings -Wno-narrowing
    -Wno-unused-result -Wno-deprecated-declarations -Wno-multichar)

# --- SexyAppFramework + ImageLib + PakLib ------------------------------------------------------------
set(SEXY_DIR ${CMAKE_SOURCE_DIR}/src/framework/SexyAppFramework)
file(GLOB SEXY_SOURCES CONFIGURE_DEPENDS ${SEXY_DIR}/*.cpp)
# SWTri_*.cpp are #included by SWTri.cpp; they aren't translation units.
list(FILTER SEXY_SOURCES EXCLUDE REGEX "/SWTri_[^/]*\\.cpp$")
file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/platform/switch/*.cpp)
list(FILTER PLATFORM_SOURCES EXCLUDE REGEX "/hello\\.cpp$")

add_library(sexyframework STATIC
    ${SEXY_SOURCES}
    ${PLATFORM_SOURCES}
    src/framework/ImageLib/ImageLib.cpp
    src/framework/PakLib/PakInterface.cpp
)
target_include_directories(sexyframework PUBLIC
    ${CMAKE_SOURCE_DIR}/platform/switch
    ${CMAKE_SOURCE_DIR}/src/framework
    ${SEXY_DIR}
)
target_compile_options(sexyframework PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${SEXY_CXX_FLAGS}>)
target_link_libraries(sexyframework PUBLIC nx_codecs nx_modplug nx_sdl2)

# --- The game ----------------------------------------------------------------------------------------
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/game/*.cpp)
add_executable(bejeweled ${GAME_SOURCES})
target_include_directories(bejeweled PRIVATE ${CMAKE_SOURCE_DIR}/src/game)
target_link_libraries(bejeweled PRIVATE sexyframework)

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

nx_generate_nacp(bejeweled.nacp NAME "Bejeweled Deluxe" AUTHOR "PopCap Games" VERSION "${PROJECT_VERSION}")
nx_create_nro(bejeweled
    OUTPUT ${CMAKE_BINARY_DIR}/bejeweled.nro
    ICON ${CMAKE_SOURCE_DIR}/res/icon.jpg
    NACP ${CMAKE_BINARY_DIR}/bejeweled.nacp
    ROMFS ${ROMFS_DIR}
)
