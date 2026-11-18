#ifndef __SWITCHPLATFORM_H__
#define __SWITCHPLATFORM_H__

// Platform services that need libnx directly. Kept out of the framework headers because <switch.h> defines short
// type names (u8, u32, Result, ...) that collide with game code.

#include <stdint.h>
#include <string>

namespace SwitchPlatform
{
	// What the system told the game since the last call, as APPLET_* bits. SDL's Switch driver reports none of these
	// (only the HOME-menu close, as SDL_QUIT, and docking when the window is resizable); libnx calls our hook from
	// appletMainLoop, which SDL_PollEvent runs on the main thread.
	enum
	{
		APPLET_FOCUS_CHANGED	= 1,	// IsInFocus() changed: HOME menu, sleep, an overlay or a library applet
		APPLET_RESUMED			= 2,	// running again after the system suspended us (HOME menu / sleep)
	};
	int			TakeAppletEvents();
	bool		IsInFocus();
	bool		IsDocked();
	// For emulators without a HOME menu: report a focus change as if the system had sent one.
	void		SimulateFocus(bool inFocus);

	// Microseconds from an arbitrary start, for timing.
	uint64_t	MicroTime();

	// Mounts RomFS (the game data) and, in debug builds, sends stdout/stderr to nxlink. Call first thing in main().
	void		Init();
	// Shuts SDL down and unmounts what Init mounted.
	void		Exit();

	// Starts a background thread on a different CPU core from the caller's. Horizon doesn't time-slice threads of
	// equal priority on one core, so a busy worker started with pthread_create (which uses the caller's core) keeps the
	// main thread from running at all. Returns false if no thread could be started. Exit() waits for it.
	bool		StartWorkerThread(void (*theProc)(void*), void* theArg, size_t theStackSize);

	// Shows the system software keyboard (blocking) with theInitial filled in, at most theMaxChars long. Returns false if
	// the player cancelled. The result keeps only printable ASCII, the characters the game's fonts have.
	bool		ShowKeyboard(const std::string& theHeader, const std::string& theInitial, int theMaxChars, std::string& theResult);

	// Shows the system error applet with a title line and details; returns when the user dismisses it.
	void		ShowError(const std::string& theTitle, const std::string& theText);

	// Saves are written whole to TempFileName(path) and then moved over the real file by CommitFile, so a crash or a
	// pulled SD card mid-write leaves the old file rather than a truncated one. The SD card's FAT can't rename over a
	// file, so there is a moment with only the .tmp; RecoverFiles (at startup) finishes such a move, and deletes a
	// .tmp whose real file still exists (a write that didn't finish).
	std::string	TempFileName(const std::string& thePath);
	bool		CommitFile(const std::string& thePath);
	void		RecoverFiles(const std::string& theDir);
}

#endif
