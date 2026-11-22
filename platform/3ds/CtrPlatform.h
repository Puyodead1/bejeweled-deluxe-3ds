#ifndef __CTRPLATFORM_H__
#define __CTRPLATFORM_H__

// Platform services that need libctru directly. Kept out of the framework headers because <3ds.h> defines short
// type names (u8, u32, Result, Thread, ...) that collide with game code.

#include <stdint.h>
#include <string>

namespace CtrPlatform
{
	// What the system told the game since the last call, as APPLET_* bits. libctru calls our APT hook from inside
	// aptMainLoop (PumpSystem): the HOME menu and sleep suspend us there and resume us before it returns.
	enum
	{
		APPLET_FOCUS_CHANGED	= 1,	// IsInFocus() changed: HOME menu, sleep, a library applet
		APPLET_RESUMED			= 2,	// running again after the system suspended us
	};
	int			TakeAppletEvents();
	bool		IsInFocus();
	// For scripted tests: report a focus change as if the system had sent one (losing focus also runs the suspend
	// handler), or a close request.
	void		SimulateFocus(bool inFocus);
	void		SimulateClose();

	// Called from inside PumpSystem, on the main thread, just before the HOME menu, sleep or a library applet (the
	// keyboard, the error screen) suspends the app. It may be the last code that runs: the console can be switched off
	// from the HOME menu, or its battery run out while asleep, so this is where to save.
	void		SetSuspendHandler(void (*theProc)(void*), void* theArg);

	// Runs the system's event handling (aptMainLoop). Returns false once the system wants the app to close (HOME
	// menu "Close", or power off); the game should then shut down normally, without drawing (the GPU may no longer
	// be ours).
	bool		PumpSystem();
	bool		IsClosing();	// PumpSystem has returned false

	bool		IsNew3DS();

	// Buttons and touch, read once per frame (hidScanInput). Button bits are libctru's KEY_* bit numbers, i.e. bit
	// (1 << PAD_x) in SexyAppBase's PAD enum. Touch is in bottom-screen pixels (0..319, 0..239).
	struct Input
	{
		uint32_t	mHeld;
		uint32_t	mPressed;		// went down since the last read
		uint32_t	mReleased;		// went up since the last read
		bool		mTouching;
		int			mTouchX;
		int			mTouchY;
	};
	void		ReadInput(Input& theInput);

	// Microseconds from an arbitrary start, for timing.
	uint64_t	MicroTime();

	// Mounts RomFS (the game data), opens the log (sdmc:/3ds/Bejeweled/bejeweled.log, or 3dslink when the .3dsx was
	// sent with 3dslink) and finishes interrupted save writes. Call first thing in main().
	void		Init();
	// Waits for the worker threads and unmounts what Init mounted.
	void		Exit();

	// Starts a background thread. On the New 3DS it gets the extra core the game may use; on the Old 3DS it shares
	// the main thread's core below the main thread's priority, so it runs while the main thread waits (for vblank, or
	// in Sleep). Returns false if no thread could be started. Exit() waits for it.
	bool		StartWorkerThread(void (*theProc)(void*), void* theArg, size_t theStackSize);

	// Shows the system software keyboard (blocking) with theInitial filled in, at most theMaxChars long. Returns false if
	// the player cancelled. The result keeps only printable ASCII, the characters the game's fonts have.
	bool		ShowKeyboard(const std::string& theHeader, const std::string& theInitial, int theMaxChars, std::string& theResult);

	// Shows the system error applet with a title line and details; returns when the user dismisses it.
	void		ShowError(const std::string& theTitle, const std::string& theText);

	// Memory in use and free, for the log.
	std::string	MemoryString();

	// Saves are written whole to TempFileName(path) and then moved over the real file by CommitFile, so a crash or a
	// pulled SD card mid-write leaves the old file rather than a truncated one. The SD card's FAT can't rename over a
	// file, so there is a moment with only the .tmp; RecoverFiles (at startup) finishes such a move, and deletes a
	// .tmp whose real file still exists (a write that didn't finish).
	std::string	TempFileName(const std::string& thePath);
	bool		CommitFile(const std::string& thePath);
	void		RecoverFiles(const std::string& theDir);
}

#endif
