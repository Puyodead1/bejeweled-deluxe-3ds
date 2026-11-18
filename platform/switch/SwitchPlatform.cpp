#include "SwitchPlatform.h"

#include <switch.h>
#include <arpa/inet.h>
#include <SDL.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>

static bool gRomfsMounted = false;
static int gNxlinkSocket = -1;

static AppletHookCookie gAppletHookCookie;
static int gAppletEvents = 0;
static bool gInFocus = true;

// Runs on the main thread, from appletMainLoop inside SDL_PollEvent.
static void AppletHook(AppletHookType theHook, void* theParam)
{
	switch (theHook)
	{
	case AppletHookType_OnFocusState:
	{
		bool inFocus = appletGetFocusState() == AppletFocusState_InFocus;
		fprintf(stderr, "applet: focus state %d\n", (int)appletGetFocusState());
		if (inFocus != gInFocus)
		{
			gInFocus = inFocus;
			gAppletEvents |= SwitchPlatform::APPLET_FOCUS_CHANGED;
		}
		break;
	}
	case AppletHookType_OnResume:
		fprintf(stderr, "applet: resumed\n");
		gAppletEvents |= SwitchPlatform::APPLET_RESUMED;
		break;
	case AppletHookType_OnOperationMode:
		fprintf(stderr, "applet: %s\n", SwitchPlatform::IsDocked() ? "docked" : "handheld");
		break;
	default:
		break;
	}
}

int SwitchPlatform::TakeAppletEvents()
{
	int anEvents = gAppletEvents;
	gAppletEvents = 0;
	return anEvents;
}

bool SwitchPlatform::IsInFocus()
{
	return gInFocus;
}

bool SwitchPlatform::IsDocked()
{
	return appletGetOperationMode() == AppletOperationMode_Console;
}

void SwitchPlatform::SimulateFocus(bool inFocus)
{
	if (inFocus != gInFocus)
	{
		gInFocus = inFocus;
		gAppletEvents |= APPLET_FOCUS_CHANGED;
		if (inFocus)
			gAppletEvents |= APPLET_RESUMED;
	}
}

uint64_t SwitchPlatform::MicroTime()
{
	return armTicksToNs(armGetSystemTick()) / 1000;
}

void SwitchPlatform::Init()
{
	// Logs go to `nxlink -s` when the .nro was sent with nxlink (the loader leaves the sender's address), in any
	// build, so a release build on the console can be watched too. Started any other way, no sockets are set up.
	if (__nxlink_host.s_addr != 0 && R_SUCCEEDED(socketInitializeDefault()))
	{
		gNxlinkSocket = nxlinkStdio();
		if (gNxlinkSocket < 0)
			socketExit();
	}

	if (gNxlinkSocket < 0)
	{
		// No nxlink host listening: keep the log next to the saves instead.
		mkdir("sdmc:/switch", 0777);
		mkdir("sdmc:/switch/Bejeweled", 0777);
		if (freopen("sdmc:/switch/Bejeweled/bejeweled.log", "w", stderr) != NULL)
			setvbuf(stderr, NULL, _IOLBF, 1024);
	}

	// Finish or drop any save write a crash or power cut interrupted (see CommitFile).
	RecoverFiles("sdmc:/switch/Bejeweled");

	// Closing the game from the HOME menu: hold the exit (up to 15 s) so SDL turns it into SDL_QUIT and the game
	// shuts down normally, saving the board and settings, instead of being killed.
	appletLockExit();

	// Focus and suspend/resume (see TakeAppletEvents). libnx's default focus handling stays: the system suspends us
	// while the HOME menu is up or the console sleeps, and we still hear about every focus change.
	gInFocus = appletGetFocusState() == AppletFocusState_InFocus;
	appletHook(&gAppletHookCookie, AppletHook, NULL);

	Result aResult = romfsInit();
	gRomfsMounted = R_SUCCEEDED(aResult);
	if (!gRomfsMounted)
		fprintf(stderr, "romfsInit failed: 0x%x\n", (unsigned)aResult);
}

static Thread gWorkerThreads[4];
static int gNumWorkerThreads = 0;

bool SwitchPlatform::StartWorkerThread(void (*theProc)(void*), void* theArg, size_t theStackSize)
{
	if (gNumWorkerThreads >= (int)(sizeof(gWorkerThreads) / sizeof(gWorkerThreads[0])))
		return false;

	s32 aPriority = 0x2C;
	svcGetThreadPriority(&aPriority, CUR_THREAD_HANDLE);

	// Any other core this process may use, else the default one.
	u64 aCoreMask = 0;
	if (R_FAILED(svcGetInfo(&aCoreMask, InfoType_CoreMask, CUR_PROCESS_HANDLE, 0)))
		aCoreMask = 0;
	int aCurCore = (int)svcGetCurrentProcessorNumber();
	int aCore = -2;
	for (int i = 0; i < 4; i++)
	{
		if (i != aCurCore && (aCoreMask & (1ULL << i)) != 0)
		{
			aCore = i;
			break;
		}
	}

	Thread* aThread = &gWorkerThreads[gNumWorkerThreads];
	Result aResult = threadCreate(aThread, theProc, theArg, NULL, (theStackSize + 0xFFF) & ~(size_t)0xFFF, aPriority, aCore);
	if (R_FAILED(aResult) && aCore != -2)
		aResult = threadCreate(aThread, theProc, theArg, NULL, (theStackSize + 0xFFF) & ~(size_t)0xFFF, aPriority, aCore = -2);
	if (R_FAILED(aResult))
	{
		fprintf(stderr, "threadCreate failed: 0x%x\n", (unsigned)aResult);
		return false;
	}
	if (R_FAILED(threadStart(aThread)))
	{
		threadClose(aThread);
		return false;
	}

	fprintf(stderr, "worker thread on core %d (main on %d, mask 0x%llx)\n", aCore, aCurCore, (unsigned long long)aCoreMask);
	gNumWorkerThreads++;
	return true;
}

void SwitchPlatform::Exit()
{
	for (int i = 0; i < gNumWorkerThreads; i++)
	{
		threadWaitForExit(&gWorkerThreads[i]);
		threadClose(&gWorkerThreads[i]);
	}
	gNumWorkerThreads = 0;

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

	appletUnhook(&gAppletHookCookie);
	appletUnlockExit();
}

std::string SwitchPlatform::TempFileName(const std::string& thePath)
{
	return thePath + ".tmp";
}

bool SwitchPlatform::CommitFile(const std::string& thePath)
{
	std::string aTemp = TempFileName(thePath);
	remove(thePath.c_str());
	if (rename(aTemp.c_str(), thePath.c_str()) != 0)
	{
		fprintf(stderr, "can't move %s into place\n", aTemp.c_str());
		return false;
	}
	return true;
}

static bool IsDir(const std::string& thePath)
{
	struct stat aStat;
	return stat(thePath.c_str(), &aStat) == 0 && S_ISDIR(aStat.st_mode);
}

static bool Exists(const std::string& thePath)
{
	struct stat aStat;
	return stat(thePath.c_str(), &aStat) == 0;
}

void SwitchPlatform::RecoverFiles(const std::string& theDir)
{
	DIR* aDir = opendir(theDir.c_str());
	if (aDir == NULL)
		return;

	std::vector<std::string> aNames;
	while (struct dirent* anEntry = readdir(aDir))
		aNames.push_back(anEntry->d_name);
	closedir(aDir);

	std::string aBase = theDir;
	if (!aBase.empty() && aBase[aBase.size() - 1] != '/')
		aBase += '/';
	for (size_t i = 0; i < aNames.size(); i++)
	{
		const std::string& aName = aNames[i];
		if (aName == "." || aName == "..")
			continue;
		std::string aPath = aBase + aName;
		if (IsDir(aPath))
		{
			RecoverFiles(aPath);
			continue;
		}
		if (aName.size() <= 4 || aName.compare(aName.size() - 4, 4, ".tmp") != 0)
			continue;

		std::string aReal = aPath.substr(0, aPath.size() - 4);
		if (Exists(aReal))
		{
			fprintf(stderr, "saves: dropping unfinished %s\n", aPath.c_str());
			remove(aPath.c_str());
		}
		else
		{
			fprintf(stderr, "saves: recovering %s\n", aReal.c_str());
			rename(aPath.c_str(), aReal.c_str());
		}
	}
}

bool SwitchPlatform::ShowKeyboard(const std::string& theHeader, const std::string& theInitial, int theMaxChars, std::string& theResult)
{
	SwkbdConfig aConfig;
	if (R_FAILED(swkbdCreate(&aConfig, 0)))
		return false;

	swkbdConfigMakePresetDefault(&aConfig);
	swkbdConfigSetHeaderText(&aConfig, theHeader.c_str());
	swkbdConfigSetGuideText(&aConfig, theHeader.c_str());
	swkbdConfigSetInitialText(&aConfig, theInitial.c_str());
	swkbdConfigSetStringLenMax(&aConfig, theMaxChars);

	char aText[256] = {0};
	Result aResult = swkbdShow(&aConfig, aText, sizeof(aText));
	swkbdClose(&aConfig);
	fprintf(stderr, "swkbd: %s '%s'\n", R_SUCCEEDED(aResult) ? "ok" : "cancelled", aText);
	if (R_FAILED(aResult))
		return false;	// cancelled (or the applet couldn't start)

	theResult.clear();
	for (const char* p = aText; *p != 0; p++)
	{
		if (*p >= 0x20 && *p <= 0x7E)
			theResult += *p;
	}
	return true;
}

void SwitchPlatform::ShowError(const std::string& theTitle, const std::string& theText)
{
	fprintf(stderr, "[%s] %s\n", theTitle.c_str(), theText.c_str());

	ErrorApplicationConfig aConfig;
	if (R_SUCCEEDED(errorApplicationCreate(&aConfig, theTitle.c_str(), theText.c_str())))
		errorApplicationShow(&aConfig);
}
