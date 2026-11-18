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
}

void SwitchPlatform::ShowError(const std::string& theTitle, const std::string& theText)
{
	fprintf(stderr, "[%s] %s\n", theTitle.c_str(), theText.c_str());

	ErrorApplicationConfig aConfig;
	if (R_SUCCEEDED(errorApplicationCreate(&aConfig, theTitle.c_str(), theText.c_str())))
		errorApplicationShow(&aConfig);
}
