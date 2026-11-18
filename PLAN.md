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
   `sdmc:/switch/Bejeweled/registry.dat`), a worker thread for loading, a silent `NullSoundManager`, the error
   applet for `Popup`. InternetManager/BetaSupport/update checks are gone. The log goes to
   `sdmc:/switch/Bejeweled/bejeweled.log` (or to `nxlink -s` in debug builds).
3. **Platform layer: video + input + main loop.** `DDInterface`/`DDImage` on SDL2, SexyAppBase's loop on SDL events,
   touch/controller → mouse/keys, file layer on RomFS. Goal: title screen, then a full game, in Eden.
   Done: a game plays in Eden by touch and by controller. Touch is a mouse that only hovers while a finger is down;
   the left stick/d-pad steer a software cursor (shown only while the controller is in use), A/R/ZR click, B is
   Escape, + pauses. The loading thread runs on another core (`SwitchPlatform::StartWorkerThread`: Horizon doesn't
   time-slice equal-priority threads on one core, so the title screen couldn't draw), and the loading bar fills over
   ~0.6 s since RomFS loads in a fraction of a second. Fixed on the way: `DescParser` erasing from an empty string
   (libstdc++ memmoves gigabytes), legacy image fonts opening `data\…` without the RomFS path mapping (no score).
   Testing: `<app data>/autoinput.txt` scripts taps, drags, pad buttons, the stick, the game's own hint move and
   screenshots (see `SexyAppBase::LoadAutoInput`; `tools/run_eden.ps1 -AutoInput -Shots`).
4. **Audio.** SoundManager/SoundInstance mixer and the libmodplug MusicInterface with fades.
   Done: `SDLSoundManager` decodes the sounds (Ogg through Tremor, WAV) to 48 kHz stereo at load time and mixes them
   with the music in one SDL callback; volume and pan go through DirectSound's dB curves so levels match the PC.
   `SDLMusicInterface` plays the .mo3/.it songs with libmodplug and mirrors BassMusicInterface's fades. The autoinput
   `rec <ms>` command dumps the mixed output for checking (no clipping in a full game). Latency: a sound is mixed
   5–27 ms after `Play`, then waits behind one queued 1024-frame buffer (SDL's audren driver keeps two), so ≤ ~55 ms
   before the console's own output. 512 frames can't be used in Eden: it wakes the audio thread only every
   10–30 ms and then takes ~36k frames/s. `tools/run_eden.ps1 -Fresh` deletes the saved game so scripted runs don't
   open on the resume dialog.
5. **Saves.** Registry and high scores (`userdata/hiscores.dat` seeded from RomFS) in `sdmc:/switch/Bejeweled/`.
   Done: `WinDMApp::Init` seeds any missing `userdata` file from `romfs:/userdata` (the PC's MigrationManager copied
   the install's folder the same way). Saved game, high scores and registry are written to `<file>.tmp` and moved
   into place (`SwitchPlatform::CommitFile`); since FAT can't rename over a file, `SwitchPlatform::RecoverFiles` at
   startup finishes a move that was cut short or drops an unfinished .tmp. `WriteToRegistry` writes the registry
   file once instead of once per value (`SwitchRegistry::Batch`). `appletLockExit` holds a HOME-menu close long
   enough for SDL's SDL_QUIT to run the normal shutdown (saves the board, writes settings). Verified in Eden: seeding,
   quit → resume restores the same board and score, both recovery cases, and a timed game's high score with a typed
   name (new autoinput `text` command) written with PC-compatible checksums. Not testable in Eden: the HOME-menu
   close itself; check on hardware. The seeded scores are whatever the packed install's `hiscores.dat` holds.
6. **Hardware pass.** Docked/handheld resolution, home-menu suspend (pause audio), applet exit, performance (60 fps).
   Audio latency: on hardware audren runs 5 ms frames, so try 512 frames, or our own audrv backend with several
   small wave buffers and a fill target that grows on underrun (less delay than SDL's fixed two buffers).
7. **Polish.** Remove trial/register/web UI, controller prompts where needed, cursor feel.
