#include "SwitchPlatform.h"

#include <switch.h>
#include <SDL.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

static bool gRomfsMounted = false;
static int gNxlinkSocket = -1;

void SwitchPlatform::Init()
{
#ifndef NDEBUG
	// Logs go to `nxlink -s` when the .nro was sent with it; harmless otherwise.
	if (R_SUCCEEDED(socketInitializeDefault()))
	{
		gNxlinkSocket = nxlinkStdio();
		if (gNxlinkSocket < 0)
			socketExit();
	}
#endif

	if (gNxlinkSocket < 0)
	{
		// No nxlink host listening: keep the log next to the saves instead.
		mkdir("sdmc:/switch", 0777);
		mkdir("sdmc:/switch/Bejeweled", 0777);
		if (freopen("sdmc:/switch/Bejeweled/bejeweled.log", "w", stderr) != NULL)
			setvbuf(stderr, NULL, _IOLBF, 1024);
	}

	Result aResult = romfsInit();
	gRomfsMounted = R_SUCCEEDED(aResult);
	if (!gRomfsMounted)
		fprintf(stderr, "romfsInit failed: 0x%x\n", (unsigned)aResult);
}

void SwitchPlatform::Exit()
{
	SDL_Quit();

	if (gRomfsMounted)
		romfsExit();
	gRomfsMounted = false;

	if (gNxlinkSocket >= 0)
	{
		close(gNxlinkSocket);
		socketExit();
		gNxlinkSocket = -1;
	}
}

void SwitchPlatform::ShowError(const std::string& theTitle, const std::string& theText)
{
	fprintf(stderr, "[%s] %s\n", theTitle.c_str(), theText.c_str());

	ErrorApplicationConfig aConfig;
	if (R_SUCCEEDED(errorApplicationCreate(&aConfig, theTitle.c_str(), theText.c_str())))
		errorApplicationShow(&aConfig);
}
