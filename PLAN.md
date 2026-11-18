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
                                          |- SoundManager  software mixer on audren (SwitchAudio): Ogg (Tremor) + WAV, volume/pan/pitch
                                          |- MusicInterface libmodplug for the three .s3m songs, mixed into the same stream
                                          |- registry      key/value file in sdmc:/switch/Bejeweled/
                                          |- threads/locks SDL_Thread / SDL_mutex
                                          '- removed       HTTP/update check, BetaSupport, SEHCatcher, Flash, BASS/FMOD, D3D
```

The class names `DDInterface`/`DDImage` stay, so game code needs few edits.

Input: touchscreen = mouse (tap to select, drag to swap); stick/D-pad moves a cursor; A = click, B = back,
Y = pause, - = options, X = hint, + = quit to hbmenu. Trial/registration UI is removed: the port runs as the full game.

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
5. **Saves.** Registry and high scores (`userdata/hiscores.dat`) in `sdmc:/switch/Bejeweled/`.
   Done: nothing is seeded. The Steam install has no `userdata` folder (the default table, Ace 1000 down to Wim 600,
   is built into `HighScoreWidget`; the PC's MigrationManager copies an install's folder only if there is one),
   so `pack_data.ps1` doesn't pack one. Saved game, high scores and registry are written to `<file>.tmp` and moved
   into place (`SwitchPlatform::CommitFile`); since FAT can't rename over a file, `SwitchPlatform::RecoverFiles` at
   startup finishes a move that was cut short or drops an unfinished .tmp. `WriteToRegistry` writes the registry
   file once instead of once per value (`SwitchRegistry::Batch`). `appletLockExit` holds a HOME-menu close long
   enough for SDL's SDL_QUIT to run the normal shutdown (saves the board, writes settings). Verified in Eden:
   quit → resume restores the same board and score, both recovery cases, and a timed game's high score with a typed
   name (new autoinput `text` command) written with PC-compatible checksums. Not testable in Eden: the HOME-menu
   close itself; check on hardware.
6. **Hardware pass.** Docked/handheld resolution, home-menu suspend (pause audio), applet exit, performance (60 fps).
   Done so far (in Eden; the console itself still to check):
   - Resolution: the window is created resizable at the current mode's size, which is what SDL's Switch driver needs
     to follow docking (1920x1080 docked, 1280x720 handheld, `SDL_WINDOWEVENT_SIZE_CHANGED` on a switch); the 640x480
     picture is pillarboxed at 4:3. Before, it always rendered 720p. Both modes checked with `run_eden.ps1 -Handheld`;
     switching while running can't be triggered from eden-cli.
   - Focus: SDL reports no focus changes on the Switch, so `SwitchPlatform` hooks the applet messages. Losing focus
     (HOME menu, sleep, overlays, library applets) takes the old WM_ACTIVATEAPP path: the board pauses and sound
     mutes (captured output is silent). libnx's default handling stays, so the system suspends the game behind the
     HOME menu; a resume clears the update backlog (SexyAppBase also caps catch-up at 200 ms). A timed game stays on
     "click here to continue" after focus returns, rather than running on at once as on the PC. Tested with the
     autoinput `focus <0|1>` command, which goes through the same path.
   - Audio: `SwitchAudio` drives the audio renderer directly, in place of SDL's driver (which kept one 21 ms buffer
     playing and one waiting): 5 ms buffers (one audren frame), 3 queued to start, one more each time the queue runs
     dry (not after a suspend-length gap). Eden settles at 4-5 (20-25 ms); a console waking every frame should stay at
     3. Eden won't play from a renderer with fewer voices than SDL's 24.
   - Frame timing: the log gets a `perf:` line every 10 s. In Eden: software draw 7.5-8.5 ms per frame, present
     17-21 ms, ~30 fps; the Phase 1 smoke test that drew nothing also ran at ~32 fps, so Eden's presentation is the
     limit there. On hardware: check that this reaches 60 fps and whether the software draw needs work.
   - First console run: no input at all. SDL's Switch driver sends touches only as finger events (no mouse events),
     and its joysticks are a fixed list with no device-added events, so nothing opened them. Fixed: SexyAppBase
     handles SDL_FINGER* itself (one finger, scaled to the window, same pointer path as the mouse) and opens the
     pads at startup. Autoinput taps now go in as finger events, so Eden tests the same path. Logs go to
     `nxlink -s` in any build when the NRO was sent with nxlink.
   - Quitting crashed ("the software was closed because an error occurred", 2168-0002) in two ways. First,
     `gWinDMApp` was a plain global whose destructor would have freed SDL objects after `SDL_Quit` (libnx's crt0 never
     runs static destructors, so this was latent; the app is now deleted in `main`). The real crash: `nxlinkStdio`
     puts stdout/stderr on the socket, `socketExit` removes the socket device, and `exit()` then closes those streams
     through a NULL device table. Fixed by closing fds 1 and 2 before `socketExit`; only nxlink-started runs were
     affected. Found with Atmosphère's crash report, which an nxlink-started game now prints at startup; reproduced in
     Eden by putting stdout on a UDP socket.
   - On the console: touch, A, the d-pad and the shortcuts work, Quit Game returns to hbmenu cleanly (+ takes the
     same exit path), and a high score name typed on the system keyboard is recorded. Frame
     timing: about 11 ms software draw + 5 ms present per drawn frame, 100 updates/s throughout (the game only
     redraws when something changes, so the fps figure varies with what's on screen).
   Still to check on hardware: HOME-menu close (saves) and suspend/resume, dock/undock while running, fps, audio
   queue length and feel.
7. **Polish.** Remove trial/register/web UI, controller prompts where needed, cursor feel.
   Done (checked in Eden):
   - Trial and web pieces gone: `RegisterDialog`, the register nags (`Board`, title screen, game-over message), the
     title screen's trial delay and register link, `OpenURL` and its "opening browser" dialogs, the update check.
     Options keeps the volume sliders, Help and High Scores; the Register button, the partner link ("Play More Great
     Games!"), Check for Updates, and the Fullscreen and Custom Cursors checkboxes are gone.
   - Controller shortcuts (`WinDMApp::ControllerButton`, ahead of the framework's defaults): A on the title
     starts; X is the hint orb; - opens Options (and closes it); Y pauses, and plays again on the game-over screen;
     + quits to the Homebrew Menu the way a HOME-menu close does (the board is saved);
     B answers the top dialog with No, or its only button (never on "continue your saved game?", where No deletes
     the save). A one-button dialog brings the pointer to its button.
   - Cursor: the d-pad moves the pointer a gem at a time on the board (`StepPointer`), first on the press, then
     every 110 ms after 300 ms held; off the board it glides as before. The stick is unchanged.
   - Wording: the PC strings that say click or "any key" are replaced in code after the properties load
     (`WinDMApp::SetSwitchStrings`), since the property files are copied from the install unchanged: the title's
     play link, "Tap or Press Y to Continue!", "PRESS Y TO PLAY AGAIN", three tips, the pause and controls parts of
     the instructions. The hint orb's hover picture has "CLICK HERE FOR A HINT" painted in; left as it is.
   - Name entry: the high score dialog opens the system keyboard (`SwitchPlatform::ShowKeyboard`) a moment
     after it appears and again on a tap on the name box; OK records the name (printable ASCII only, the fonts have
     nothing else), cancel leaves the dialog. eden-cli's keyboard answers "Eden" at once; the real one is for the
     console. Autoinput `endgame` ends a game to get there quickly.
