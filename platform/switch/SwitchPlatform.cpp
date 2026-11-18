#include "SwitchPlatform.h"

#include <switch.h>
#include <SDL.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>

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

	// Finish or drop any save write a crash or power cut interrupted (see CommitFile).
	RecoverFiles("sdmc:/switch/Bejeweled");

	// Closing the game from the HOME menu: hold the exit (up to 15 s) so SDL turns it into SDL_QUIT and the game
	// shuts down normally, saving the board and settings, instead of being killed.
	appletLockExit();

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

static bool CopyOneFile(const std::string& theFrom, const std::string& theTo)
{
	FILE* anIn = fopen(theFrom.c_str(), "rb");
	if (anIn == NULL)
		return false;
	std::string aTemp = SwitchPlatform::TempFileName(theTo);
	FILE* anOut = fopen(aTemp.c_str(), "wb");
	if (anOut == NULL)
	{
		fclose(anIn);
		return false;
	}

	bool ok = true;
	char aBuf[4096];
	size_t aRead;
	while ((aRead = fread(aBuf, 1, sizeof(aBuf), anIn)) > 0)
	{
		if (fwrite(aBuf, 1, aRead, anOut) != aRead)
		{
			ok = false;
			break;
		}
	}
	if (ferror(anIn))
		ok = false;
	fclose(anIn);
	if (fclose(anOut) != 0)
		ok = false;
	if (!ok)
	{
		remove(aTemp.c_str());
		return false;
	}
	return SwitchPlatform::CommitFile(theTo);
}

void SwitchPlatform::SeedFiles(const std::string& theFromDir, const std::string& theToDir)
{
	DIR* aDir = opendir(theFromDir.c_str());
	if (aDir == NULL)
		return;

	std::vector<std::string> aNames;
	while (struct dirent* anEntry = readdir(aDir))
		aNames.push_back(anEntry->d_name);
	closedir(aDir);

	mkdir(theToDir.c_str(), 0777);
	for (size_t i = 0; i < aNames.size(); i++)
	{
		const std::string& aName = aNames[i];
		if (aName == "." || aName == "..")
			continue;
		std::string aFrom = theFromDir + "/" + aName;
		std::string aTo = theToDir + "/" + aName;
		if (IsDir(aFrom))
			SeedFiles(aFrom, aTo);
		else if (!Exists(aTo))
			fprintf(stderr, "saves: seeding %s%s\n", aTo.c_str(), CopyOneFile(aFrom, aTo) ? "" : " failed");
	}
}

void SwitchPlatform::ShowError(const std::string& theTitle, const std::string& theText)
{
	fprintf(stderr, "[%s] %s\n", theTitle.c_str(), theText.c_str());

	ErrorApplicationConfig aConfig;
	if (R_SUCCEEDED(errorApplicationCreate(&aConfig, theTitle.c_str(), theText.c_str())))
		errorApplicationShow(&aConfig);
}
