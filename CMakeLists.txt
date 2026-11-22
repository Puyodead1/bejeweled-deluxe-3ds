cmake_minimum_required(VERSION 3.20)

# Bejeweled Deluxe 1.87 for Nintendo 3DS (homebrew .3dsx).
# Configure with the devkitPro toolchain from the devkitPro msys2 shell; build.ps1 does that for you:
#   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake
project(Bejeweled VERSION 1.87.0 LANGUAGES C CXX ASM)

if(NOT NINTENDO_3DS)
    message(FATAL_ERROR "Configure with -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake (see build.ps1)")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(dkp-embedded-binary)

set(PORTLIBS ${DEVKITPRO}/portlibs/3ds)

# citro3d (installed with the 3DS toolchain). libctru and libm come from the toolchain's standard libraries.
add_library(ctr_citro3d INTERFACE)
target_link_libraries(ctr_citro3d INTERFACE citro3d)

# libmodplug for the .s3m music (3ds-libmodplug).
add_library(ctr_modplug INTERFACE)
target_include_directories(ctr_modplug INTERFACE ${PORTLIBS}/include)
target_link_directories(ctr_modplug INTERFACE ${PORTLIBS}/lib)
target_link_libraries(ctr_modplug INTERFACE modplug stdc++ m)

# Image and Ogg codecs (3ds-libpng, 3ds-libjpeg-turbo, 3ds-zlib, 3ds-libvorbisidec, 3ds-libogg).
add_library(ctr_codecs INTERFACE)
target_include_directories(ctr_codecs INTERFACE ${PORTLIBS}/include)
target_link_directories(ctr_codecs INTERFACE ${PORTLIBS}/lib)
target_link_libraries(ctr_codecs INTERFACE png jpeg vorbisidec ogg z m)

# The sprite shader: pixel positions -> clip space, texture coordinate and vertex colour passed through.
# (ctr_add_shader_library only takes its sources as plain arguments, not after a SOURCES keyword.)
ctr_add_shader_library(sprite_shader ${CMAKE_SOURCE_DIR}/platform/3ds/shaders/sprite.v.pica)
dkp_add_embedded_binary_library(sprite_shader_bin sprite_shader)

set(ROMFS_DIR ${CMAKE_SOURCE_DIR}/romfs)
if(NOT IS_DIRECTORY ${ROMFS_DIR})
    message(FATAL_ERROR "romfs/ is missing: run tools/pack_data.ps1 to copy the game data in")
endif()

# --- Phase 1 smoke test --------------------------------------------------------------------------------
# Checks the toolchain and every system service the port needs: citro3d (render to a 640x480 texture, present it to
# both screens), RomFS, libjpeg/libpng, NDSP with libmodplug and Tremor, touch and buttons, SD card writes.
add_executable(smoke platform/3ds/smoke/smoke.cpp)
target_compile_options(smoke PRIVATE -Wall -Wno-unused-function)
target_link_libraries(smoke PRIVATE sprite_shader_bin ctr_citro3d ctr_codecs ctr_modplug)

ctr_generate_smdh(smoke.smdh
    NAME "Bejeweled smoke test"
    DESCRIPTION "3DS port bring-up check"
    AUTHOR "PopCap Games"
    ICON ${CMAKE_SOURCE_DIR}/res/icon48.png
)
ctr_create_3dsx(smoke
    OUTPUT ${CMAKE_BINARY_DIR}/smoke.3dsx
    SMDH ${CMAKE_BINARY_DIR}/smoke.smdh
    ROMFS ${ROMFS_DIR}
)

# --- SexyAppFramework + ImageLib + PakLib ------------------------------------------------------------
# The decompiled framework is old MSVC code: char is signed there, and it leans on permissive conversions.
set(SEXY_CXX_FLAGS -fsigned-char -fpermissive -fno-strict-aliasing -Wno-write-strings -Wno-narrowing
    -Wno-unused-result -Wno-deprecated-declarations -Wno-multichar)

set(SEXY_DIR ${CMAKE_SOURCE_DIR}/src/framework/SexyAppFramework)
file(GLOB SEXY_SOURCES CONFIGURE_DEPENDS ${SEXY_DIR}/*.cpp)
# SWTri_*.cpp are #included by SWTri.cpp; they aren't translation units.
list(FILTER SEXY_SOURCES EXCLUDE REGEX "/SWTri_[^/]*\\.cpp$")
file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/platform/3ds/*.cpp)

add_library(sexyframework STATIC
    ${SEXY_SOURCES}
    src/framework/ImageLib/ImageLib.cpp
    src/framework/PakLib/PakInterface.cpp
    ${PLATFORM_SOURCES}
)
target_include_directories(sexyframework PUBLIC
    ${CMAKE_SOURCE_DIR}/platform/3ds
    ${CMAKE_SOURCE_DIR}/src/framework
    ${SEXY_DIR}
)
target_compile_options(sexyframework PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${SEXY_CXX_FLAGS}>)
target_link_libraries(sexyframework PUBLIC ctr_codecs ctr_modplug ctr_citro3d sprite_shader_bin)

# --- The game ----------------------------------------------------------------------------------------
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/game/*.cpp)
add_executable(bejeweled ${GAME_SOURCES})
target_include_directories(bejeweled PRIVATE ${CMAKE_SOURCE_DIR}/src/game)
target_link_libraries(bejeweled PRIVATE sexyframework)

ctr_generate_smdh(bejeweled.smdh
    NAME "Bejeweled Deluxe"
    DESCRIPTION "Bejeweled Deluxe 1.87"
    AUTHOR "PopCap Games"
    ICON ${CMAKE_SOURCE_DIR}/res/icon48.png
)
ctr_create_3dsx(bejeweled
    OUTPUT ${CMAKE_BINARY_DIR}/bejeweled.3dsx
    SMDH ${CMAKE_BINARY_DIR}/bejeweled.smdh
    ROMFS ${ROMFS_DIR}
)
