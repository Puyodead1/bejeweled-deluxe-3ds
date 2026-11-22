#include "CtrPlatform.h"

#include <3ds.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// The linear heap (memory the GPU, DSP and DMA can reach) holds the frame buffers, the render target, every drawn
// image's texture (C3DInterface) and the audio buffers. Everything else is the regular heap, where the game keeps its
// decoded images' bits.
extern "C" u32 __ctru_linear_heap_size;
u32 __ctru_linear_heap_size = 24 * 1024 * 1024;

// The main thread's stack. libctru's default is 32 KB, which the framework overflows at once (WStringToString alone
// puts 16 KB on the stack, and font and resource parsing nest deeply); the PC and Switch builds had a megabyte or more.
// Azahar doesn't fault on the overflow, it reads zeros and jumps to garbage, so this shows up as a crash at PC 0.
extern "C" u32 __stacksize__;
u32 __stacksize__ = 1024 * 1024;

static bool gRomfsMounted = false;
static int g3dslinkSocket = -1;
static u32* gSocBuffer = NULL;

static aptHookCookie gAptHookCookie;
static int gAppletEvents = 0;
static bool gInFocus = true;

// Runs on the main thread, inside aptMainLoop.
static void AptHook(APT_HookType theHook, void* theParam)
{
	switch (theHook)
	{
	case APTHOOK_ONSUSPEND:
	case APTHOOK_ONSLEEP:
		fprintf(stderr, "applet: %s\n", theHook == APTHOOK_ONSLEEP ? "sleep" : "suspended");
		if (gInFocus)
		{
			gInFocus = false;
			gAppletEvents |= CtrPlatform::APPLET_FOCUS_CHANGED;
		}
		break;
	case APTHOOK_ONRESTORE:
	case APTHOOK_ONWAKEUP:
		fprintf(stderr, "applet: %s\n", theHook == APTHOOK_ONWAKEUP ? "awake" : "restored");
		if (!gInFocus)
		{
			gInFocus = true;
			gAppletEvents |= CtrPlatform::APPLET_FOCUS_CHANGED;
		}
		gAppletEvents |= CtrPlatform::APPLET_RESUMED;
		break;
	case APTHOOK_ONEXIT:
		fprintf(stderr, "applet: exit\n");
		break;
	default:
		break;
	}
}

int CtrPlatform::TakeAppletEvents()
{
	int anEvents = gAppletEvents;
	gAppletEvents = 0;
	return anEvents;
}

bool CtrPlatform::IsInFocus()
{
	return gInFocus;
}

void CtrPlatform::SimulateFocus(bool inFocus)
{
	if (inFocus != gInFocus)
	{
		gInFocus = inFocus;
		gAppletEvents |= APPLET_FOCUS_CHANGED;
		if (inFocus)
			gAppletEvents |= APPLET_RESUMED;
	}
}

bool CtrPlatform::PumpSystem()
{
	return aptMainLoop();
}

bool CtrPlatform::IsNew3DS()
{
	bool isNew = false;
	APT_CheckNew3DS(&isNew);
	return isNew;
}

void CtrPlatform::ReadInput(Input& theInput)
{
	hidScanInput();
	theInput.mHeld = hidKeysHeld();
	theInput.mPressed = hidKeysDown();
	theInput.mReleased = hidKeysUp();
	theInput.mTouching = (theInput.mHeld & KEY_TOUCH) != 0;
	theInput.mTouchX = theInput.mTouchY = 0;
	if (theInput.mTouching)
	{
		touchPosition aTouch;
		hidTouchRead(&aTouch);
		theInput.mTouchX = aTouch.px;
		theInput.mTouchY = aTouch.py;
	}
}

uint64_t CtrPlatform::MicroTime()
{
	// The system tick runs at the ARM11's 268 MHz (SYSCLOCK_ARM11) on both models.
	return svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000000);
}

std::string CtrPlatform::MemoryString()
{
	struct mallinfo anInfo = mallinfo();
	char aBuf[128];
	snprintf(aBuf, sizeof(aBuf), "heap in use %u KB of %lu KB, linear free %lu KB", (unsigned)(anInfo.uordblks / 1024),
		envGetHeapSize() / 1024, linearSpaceFree() / 1024);
	return aBuf;
}

void CtrPlatform::Init()
{
	// The New 3DS's 804 MHz clock and L2 cache; nothing on the Old 3DS.
	osSetSpeedupEnable(true);

	// The screens' framebuffers (BGR8, double-buffered); citro3d draws into them, and the keyboard and error applets
	// need them set up.
	gfxInitDefault();

	// Logs go to `3dslink -s` when the .3dsx was sent with 3dslink (the loader leaves the sender's address);
	// started any other way, to a file next to the saves.
	if (__3dslink_host.s_addr != 0)
	{
		const u32 SOC_BUFFER_SIZE = 0x100000;
		gSocBuffer = (u32*)memalign(0x1000, SOC_BUFFER_SIZE);
		if (gSocBuffer != NULL && R_SUCCEEDED(socInit(gSocBuffer, SOC_BUFFER_SIZE)))
		{
			g3dslinkSocket = link3dsStdio();
			if (g3dslinkSocket < 0)
				socExit();
		}
		if (g3dslinkSocket < 0)
		{
			free(gSocBuffer);
			gSocBuffer = NULL;
		}
	}

	mkdir("sdmc:/3ds", 0777);
	mkdir("sdmc:/3ds/Bejeweled", 0777);
	if (g3dslinkSocket < 0)
	{
		if (freopen("sdmc:/3ds/Bejeweled/bejeweled.log", "w", stderr) != NULL)
			setvbuf(stderr, NULL, _IOLBF, 1024);
	}

	fprintf(stderr, "Bejeweled Deluxe 1.87, 3DS port (%s)\n", IsNew3DS() ? "New 3DS" : "Old 3DS");
	fprintf(stderr, "memory: heap %lu KB, linear heap %lu KB; %s\n", envGetHeapSize() / 1024, envGetLinearHeapSize() / 1024,
		MemoryString().c_str());

	// Finish or drop any save write a crash or power cut interrupted (see CommitFile).
	RecoverFiles("sdmc:/3ds/Bejeweled");

	gInFocus = true;
	aptHook(&gAptHookCookie, AptHook, NULL);

	Result aResult = romfsInit();
	gRomfsMounted = R_SUCCEEDED(aResult);
	if (!gRomfsMounted)
		fprintf(stderr, "romfsInit failed: 0x%08lX\n", aResult);
}

static Thread gWorkerThreads[4];
static int gNumWorkerThreads = 0;

bool CtrPlatform::StartWorkerThread(void (*theProc)(void*), void* theArg, size_t theStackSize)
{
	if (gNumWorkerThreads >= (int)(sizeof(gWorkerThreads) / sizeof(gWorkerThreads[0])))
		return false;

	s32 aPriority = 0x30;
	svcGetThreadPriority(&aPriority, CUR_THREAD_HANDLE);

	// The New 3DS lets an application use core 2. On the Old 3DS (or if that fails) the same core as the main thread,
	// at a lower priority (a larger number): threads of equal priority on one core don't take turns.
	Thread aThread = NULL;
	int aCore = -1;
	if (IsNew3DS())
	{
		aCore = 2;
		aThread = threadCreate(theProc, theArg, theStackSize, aPriority, aCore, false);
	}
	if (aThread == NULL)
	{
		aCore = 0;
		aThread = threadCreate(theProc, theArg, theStackSize, aPriority + 1 <= 0x3F ? aPriority + 1 : 0x3F, -2, false);
	}
	if (aThread == NULL)
	{
		fprintf(stderr, "threadCreate failed\n");
		return false;
	}

	fprintf(stderr, "worker thread on core %d\n", aCore);
	gWorkerThreads[gNumWorkerThreads++] = aThread;
	return true;
}

void CtrPlatform::Exit()
{
	fprintf(stderr, "exit: worker threads\n");
	for (int i = 0; i < gNumWorkerThreads; i++)
	{
		threadJoin(gWorkerThreads[i], U64_MAX);
		threadFree(gWorkerThreads[i]);
	}
	gNumWorkerThreads = 0;

	fprintf(stderr, "exit: romfs\n");
	if (gRomfsMounted)
		romfsExit();
	gRomfsMounted = false;

	aptUnhook(&gAptHookCookie);

	gfxExit();

	if (g3dslinkSocket >= 0)
	{
		fprintf(stderr, "exit: sockets, returning from main\n");
		fflush(stdout);
		svcSleepThread(200000000ULL);	// let 3dslink receive the last lines before the session goes

		// link3dsStdio put stdout and stderr on the socket. Close those descriptors while the socket device is still
		// there; exit() closing them after socExit reads a device that's gone.
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		close(g3dslinkSocket);
		socExit();
		g3dslinkSocket = -1;
		free(gSocBuffer);
		gSocBuffer = NULL;
	}
	else
		fflush(stderr);
}

std::string CtrPlatform::TempFileName(const std::string& thePath)
{
	return thePath + ".tmp";
}

bool CtrPlatform::CommitFile(const std::string& thePath)
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

void CtrPlatform::RecoverFiles(const std::string& theDir)
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

bool CtrPlatform::ShowKeyboard(const std::string& theHeader, const std::string& theInitial, int theMaxChars, std::string& theResult)
{
	SwkbdState aKeyboard;
	swkbdInit(&aKeyboard, SWKBD_TYPE_NORMAL, 2, theMaxChars);
	swkbdSetHintText(&aKeyboard, theHeader.c_str());
	swkbdSetInitialText(&aKeyboard, theInitial.c_str());
	swkbdSetValidation(&aKeyboard, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);

	char aText[64] = {0};
	SwkbdButton aButton = swkbdInputText(&aKeyboard, aText, sizeof(aText));
	fprintf(stderr, "swkbd: button %d '%s'\n", (int)aButton, aText);
	if (aButton != SWKBD_BUTTON_CONFIRM)
		return false;	// cancelled (or the applet couldn't start)

	theResult.clear();
	for (const char* p = aText; *p != 0; p++)
	{
		if (*p >= 0x20 && *p <= 0x7E)
			theResult += *p;
	}
	return true;
}

void CtrPlatform::ShowError(const std::string& theTitle, const std::string& theText)
{
	fprintf(stderr, "[%s] %s\n", theTitle.c_str(), theText.c_str());

	errorConf anError;
	errorInit(&anError, ERROR_TEXT_WORD_WRAP, CFG_LANGUAGE_EN);
	errorText(&anError, (theTitle + "\n\n" + theText).c_str());
	errorDisp(&anError);
}
