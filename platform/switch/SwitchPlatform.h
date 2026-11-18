#ifndef __SWITCHPLATFORM_H__
#define __SWITCHPLATFORM_H__

// Platform services that need libnx directly. Kept out of the framework headers because <switch.h> defines short
// type names (u8, u32, Result, ...) that collide with game code.

#include <string>

namespace SwitchPlatform
{
	// Mounts RomFS (the game data) and, in debug builds, sends stdout/stderr to nxlink. Call first thing in main().
	void		Init();
	// Shuts SDL down and unmounts what Init mounted.
	void		Exit();

	// Shows the system error applet with a title line and details; returns when the user dismisses it.
	void		ShowError(const std::string& theTitle, const std::string& theText);
}

#endif
