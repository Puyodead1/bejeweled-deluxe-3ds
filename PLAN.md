# Switch port plan

Target: Bejeweled Deluxe 1.87 as a homebrew `.nro`, game data packed in RomFS. This repo only builds for the Switch,
so Windows/DirectX code can be deleted or rewritten freely; no match/decomp constraints apply.

## Architecture

SDL2 platform layer on top of the framework's existing software renderer:

```
src/game  ->  SexyAppBase / SexyApp  ->  platform/switch
                                          |- DDInterface   screen = 640x480 MemoryImage, uploaded each frame to an SDL
                                          |                streaming texture and scaled (letterboxed) to 1280x720/1920x1080
                                          |- DDImage       MemoryImage-only (no DirectDraw surface)
                                          |- SoundManager  software mixer on SDL audio: Ogg (Tremor) + WAV, volume/pan/pitch
                                          |- MusicInterface libmodplug for the three .s3m songs, mixed into the same stream
                                          |- registry      key/value file in sdmc:/switch/Bejeweled/
                                          |- threads/locks SDL_Thread / SDL_mutex
                                          '- removed       HTTP/update check, BetaSupport, SEHCatcher, Flash, BASS/FMOD, D3D
```

The class names `DDInterface`/`DDImage` stay, so game code needs few edits.

Input: touchscreen = mouse (tap to select, drag to swap); stick/D-pad moves a cursor; A = click, B = back,
+ = pause/options. Trial/registration UI is removed: the port runs as the full game.

## Libraries

| Library | Source | Use |
| --- | --- | --- |
| SDL2 2.28.5 | `switch-sdl2` (+ `switch-mesa`, `switch-libdrm_nouveau`) | video, audio, input |
| libmodplug 0.8.9 | `switch-libmodplug` | S3M music |
| zlib 1.1.3, libpng 1.0.5, libjpeg 6b, Tremor | `third_party/` | ImageLib and Ogg decoding, PopCap's versions |
| FreeType | `switch-freetype`, only if needed | `SysFont` fallback (dialogs/buttons when no font is set) |

`switch-sdl2` also pulled in `switch-zlib` 1.3.1. The vendored zlib 1.1.3 has the same symbol names, so link only
one: either rename/drop the vendored copy and use 1.3.1 (libpng 1.0.5 only uses the stable zlib API), or keep
1.1.3 and make sure nothing else links `-lz`.

## Known porting issues (from the survey)

- `typedef unsigned long ulong` is 64-bit on aarch64 (LP64); it's used ~790 times, mostly as 32-bit ARGB pixels →
  `uint32_t`. `DWORD` (~450 uses) needs a 32-bit typedef.
- `DECOMP_SIZE_ASSERT` checks 32-bit layouts → make it a no-op. `m_unk0x...` padding assumes 32-bit offsets; only
  matters where structs are read from files.
- Inline asm in `D3DInterface.cpp` (deleted with D3D), `ModVal.cpp`, `PerfTimer.cpp`, `SWTri_Pixel8888.cpp` → C.
- MSVC-isms gcc rejects: `= NULL` pure specifiers, `__int64`, `stricmp`/`_snprintf`, `<direct.h>`/`<io.h>`, missing
  `typename`, `std::string::iterator` used as `char*`.
- Paths use `\` and inconsistent case → normalise to lowercase `/` in the file layer (RomFS is stored lowercase).
- `SexyAppBase.cpp` (182 KB) mixes the Win32 window/message loop with app logic: the biggest single job.

## Phases

1. **Toolchain + skeleton** — done. New repo, `switch-sdl2`/`switch-libmodplug` installed, CMake via devkitPro msys2
   (`build.ps1`), RomFS packing (`tools/pack_data.ps1`), icon from the 1.87 exe, smoke test `platform/switch/hello.cpp`.
   In Eden 0.2.1: RomFS (89 files), SDL2 OpenGL ES renderer, 8 joysticks, libmodplug + SDL audio (real time, audible)
   all work. Rendering runs at a steady ~32 fps in Eden while audio keeps real time; check on hardware before
   chasing it.
2. **Compile the framework and game with gcc/aarch64.** Delete the Windows-only files, add `platform/switch/win32compat.h`
   for the remaining types, fix 64-bit and MSVC-isms, build third_party, stub what isn't ported yet. Goal: links.
   Done: `bejeweled.nro` links and boots in Eden without crashing (black screen so far). Along the way SexyAppBase
   moved onto SDL already: one fullscreen `SDL_Window`, SDL events turned into the same `WM_*` messages the deferred
   queue always handled (so demo recording still works), a file-backed registry (`SwitchRegistry`,
   `sdmc:/switch/Bejeweled/registry.dat`), pthreads for the loading thread, a silent `NullSoundManager`, the error
   applet for `Popup`. InternetManager/BetaSupport/update checks are gone. The log goes to
   `sdmc:/switch/Bejeweled/bejeweled.log` (or to `nxlink -s` in debug builds).
3. **Platform layer: video + input + main loop.** `DDInterface`/`DDImage` on SDL2, SexyAppBase's loop on SDL events,
   touch/controller → mouse/keys, file layer on RomFS. Goal: title screen, then a full game, in Eden.
4. **Audio.** SoundManager/SoundInstance mixer and the libmodplug MusicInterface with fades.
5. **Saves.** Registry and high scores (`userdata/hiscores.dat` seeded from RomFS) in `sdmc:/switch/Bejeweled/`.
6. **Hardware pass.** Docked/handheld resolution, home-menu suspend (pause audio), applet exit, performance (60 fps).
7. **Polish.** Remove trial/register/web UI, controller prompts where needed, cursor feel.
