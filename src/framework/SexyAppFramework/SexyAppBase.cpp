//#define SEXY_TRACING_ENABLED
//#define SEXY_PERF_ENABLED

#include <fstream>
#include <SDL.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>

#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Widget.h"
#include "Debug.h"
#include "KeyCodes.h"
#include "DDInterface.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "Dialog.h"
#include "../ImageLib/ImageLib.h"
#include "Rect.h"
#include "PropertiesParser.h"
#include "PerfTimer.h"
#include "MTRand.h"
#include <fstream>
#include <time.h>
#include <math.h>
#include "SysFont.h"
#include "ResourceManager.h"
#include "AutoCrit.h"
#include "Debug.h"
#include "../PakLib/PakInterface.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include "NullSoundManager.h"
#include "SwitchRegistry.h"
#include "SwitchPlatform.h"
#include <string>


using namespace Sexy;

const int DEMO_FILE_ID = 0x42BEEF78;
const int DEMO_VERSION = 2;

SexyAppBase* Sexy::gSexyAppBase = NULL;

static bool gScreenSaverActive = false;


//HotSpot: 11 4
//Size: 32 32
unsigned char gFingerCursorData[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc3,
	0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff,
	0xc0, 0x07, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xfc, 0x40, 0x01, 0xff, 0xfc, 0x00, 0x01, 0xff,
	0xfc, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x01, 0xff, 0xff, 0x00, 0x01, 0xff, 0xff, 0x00, 0x01,
	0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80, 0x03, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xc0,
	0x03, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18,
	0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
	0x18, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x1b, 0x60, 0x00, 0x00, 0x1b, 0x68, 0x00,
	0x00, 0x1b, 0x6c, 0x00, 0x01, 0x9f, 0xec, 0x00, 0x01, 0xdf, 0xfc, 0x00, 0x00, 0xdf, 0xfc,
	0x00, 0x00, 0x5f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f,
	0xf8, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00,
	0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00
};

//HotSpot: 15 10
//Size: 32 32
unsigned char gDraggingCursorData[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xf0, 0x07, 0xff, 0xff, 0xe0,
	0x01, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff,
	0xe0, 0x00, 0xff, 0xfe, 0x60, 0x00, 0xff, 0xfc, 0x20, 0x00, 0xff, 0xfc, 0x00, 0x00, 0xff,
	0xfe, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x80, 0x00,
	0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xf0,
	0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x01, 0xb0, 0x00, 0x00, 0x0d, 0xb0, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00,
	0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00,
	0x01, 0x8d, 0xb6, 0x00, 0x01, 0xcf, 0xfe, 0x00, 0x00, 0xef, 0xfe, 0x00, 0x00, 0xff, 0xfe,
	0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x1f,
	0xfc, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x07, 0xf8, 0x00, 0x00, 0x03, 0xf8, 0x00, 0x00,
	0x03, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00
};
static DDImage* gFPSImage = NULL;

//////////////////////////////////////////////////////////////////////////

// FUNCTION: GAME187 0x43ea60

//////////////////////////////////////////////////////////////////////////

// FUNCTION: GAME187 0x43eaa0
SexyAppBase::SexyAppBase()
{
	gSexyAppBase = this;

	ImageLib::InitJPEG2000();

	mMutex = NULL;
	mNotifyGameMessage = 0;

#ifdef _DEBUG
	mOnlyAllowOneCopyToRun = false;
#else
	mOnlyAllowOneCopyToRun = true;
#endif

	mProductVersion = GetProductVersion("");

	mNoDefer = false;
	mFullScreenPageFlip = true; // should we page flip in fullscreen?
	mTimeLoaded = GetTickCount();
	mSEHOccured = false;
	mProdName = "Product";
	mTitle = _S("SexyApp");
	mShutdown = false;
	mExitToTop = false;
	mWidth = 640;
	mHeight = 480;
	mFullscreenBits = 16;
	mIsWindowed = true;
	mIsPhysWindowed = true;
	mFullScreenWindow = false;
	mPreferredX = -1;
	mPreferredY = -1;
	mIsScreenSaver = false;
	mAllowMonitorPowersave = true;
	mHWnd = NULL;
	mDDInterface = NULL;
	mMusicInterface = NULL;
	mInvisHWnd = NULL;
	mFrameTime = 10;
	mNonDrawCount = 0;
	mDrawCount = 0;
	mSleepCount = 0;
	mUpdateCount = 0;
	mUpdateAppState = 0;
	mUpdateAppDepth = 0;
	mPendingUpdatesAcc = 0.0;
	mUpdateFTimeAcc = 0.0;
	mHasPendingDraw = true;
	mIsDrawing = false;
	mLastDrawWasEmpty = false;
	mLastTimeCheck = 0;
	mUpdateMultiplier = 1;
	mPaused = false;
	mFastForwardToUpdateNum = 0;
	mFastForwardToMarker = false;
	mFastForwardStep = false;
	mSoundManager = NULL;
	mCursorNum = CURSOR_POINTER;
	mMouseIn = false;
	mPointerX = -1;
	mPointerY = -1;
	mPointerIn = false;
	mPointerVisible = false;
	mPadPointerX = 0;
	mPadPointerY = 0;
	mPadButtonDown = false;
	mPadLastTick = 0;
	mAutoInputIndex = 0;
	mAutoInputNext = 0;
	mAutoStickX = 0;
	mAutoStickY = 0;
	mAutoStickUntil = 0;
	for (int aPad = 0; aPad < (int)(sizeof(mGameControllers) / sizeof(mGameControllers[0])); aPad++)
		mGameControllers[aPad] = NULL;
	mRunning = false;
	mActive = true;
	mProcessInTimer = false;
	mMinimized = false;
	mPhysMinimized = false;
	mIsDisabled = false;
	mLoaded = false;
	mYieldMainThread = false;
	mLoadingFailed = false;
	mLoadingThreadStarted = false;
	mAutoStartLoadingThread = true;
	mLoadingThreadCompleted = false;
	mCursorThreadRunning = false;
	mNumLoadingThreadTasks = 0;
	mCompletedLoadingThreadTasks = 0;
	mLastDrawTick = timeGetTime();
	mNextDrawTick = timeGetTime();
	mSysCursor = true;
	mForceFullscreen = false;
	mForceWindowed = false;
	mHasFocus = true;
	mCustomCursorsEnabled = false;
	mCustomCursorDirty = false;
	mOverrideCursor = NULL;
	mIsOpeningURL = false;
	mInitialized = false;
	mLastShutdownWasGraceful = true;
	mReadFromRegistry = false;
	mCmdLineParsed = false;
	mSkipSignatureChecks = false;
	mCtrlDown = false;
	mAltDown = false;
	mStepMode = 0;
	mCleanupSharedImages = false;
	mStandardWordWrap = true;
	mbAllowExtendedChars = true;
	mEnableMaximizeButton = false;

	mMusicVolume = 0.85;
	mSfxVolume = 0.85;
	mDemoMusicVolume = mDemoSfxVolume = 0.0;
	mMuteCount = 0;
	mAutoMuteCount = 0;
	mDemoMute = false;
	mMuteOnLostFocus = true;
	mCurHandleNum = 0;
	mFPSTime = 0;
	mFPSStartTick = GetTickCount();
	mFPSFlipCount = 0;
	mFPSCount = 0;
	mFPSDirtyCount = 0;
	mShowFPS = false;
	mShowFPSMode = FPS_ShowFPS;
	mDrawTime = 0;
	mScreenBltTime = 0;
	mAlphaDisabled = false;
	mDebugKeysEnabled = false;
	mOldWndProc = 0;
	mNoSoundNeeded = false;
	mWantFMod = false;

	mSyncRefreshRate = 100;
	mVSyncUpdates = false;
	mVSyncBroken = false;
	mVSyncBrokenCount = 0;
	mVSyncBrokenTestStartTick = 0;
	mVSyncBrokenTestUpdates = 0;
	mWaitForVSync = false;
	mSoftVSyncWait = true;
	mUserChanged3DSetting = false;
	mAutoEnable3D = false;
	mTest3D = false;
	mMinVidMemory3D = 6;
	mRecommendedVidMemory3D = 14;
	mRelaxUpdateBacklogCount = 0;
	mWidescreenAware = false;
	mEnableWindowAspect = false;
	mWindowAspect.Set(4, 3);
	mIsWideWindow = false;

	int i;

	for (i = 0; i < NUM_CURSORS; i++)
		mCursorImages[i] = NULL;

	for (i = 0; i < 256; i++)
		mAdd8BitMaxTable[i] = i;

	for (i = 256; i < 512; i++)
		mAdd8BitMaxTable[i] = 255;

	// Set default strings.  Init could read in overrides from partner.xml
	SetString("DIALOG_BUTTON_OK",		L"OK");
	SetString("DIALOG_BUTTON_CANCEL",	L"CANCEL");

	SetString("UPDATE_CHECK_TITLE",		L"Update Check");
	SetString("UPDATE_CHECK_BODY",		L"Checking if there are any updates available for this product ...");

	SetString("UP_TO_DATE_TITLE",		L"Up to Date");
	SetString("UP_TO_DATE_BODY",		L"There are no updates available for this product at this time.");
	SetString("NEW_VERSION_TITLE",		L"New Version");
	SetString("NEW_VERSION_BODY",		L"There is an update available for this product.  Would you like to visit the web site to download it?");


	mDemoPrefix = "sexyapp";
	mDemoFileName = mDemoPrefix + ".dmo";
	mPlayingDemoBuffer = false;
	mManualShutdown = false;
	mRecordingDemoBuffer = false;
	mLastDemoMouseX = 0;
	mLastDemoMouseY = 0;
	mLastDemoUpdateCnt = 0;
	mDemoNeedsCommand = true;
	mDemoLoadingComplete = false;
	mDemoLength = 0;
	mDemoCmdNum = 0;
	mDemoCmdOrder = -1; // Means we haven't processed any demo commands yet
	mDemoCmdBitPos = 0;

	mWidgetManager = new WidgetManager(this);
	mResourceManager = new ResourceManager(this);

	mPrimaryThreadId = 0;

	mTabletPC = false;
	mInvisHWnd = NULL;
	mHandCursor = NULL;
	mDraggingCursor = NULL;

	//std::wifstream stringsFile(_wfopen(L".\\properties\\fstrings", L"rb"));
	//
	//if(!stringsFile)
	//{
	//	MessageBox(NULL, "file missing: 'install-folder\\properties\\fstrings' Please re-install", "FATAL ERROR", MB_OK);
	//	DoExit(1);
	//}
	//std::getline(stringsFile, mString_HardwareAccelSwitchedOn);
	//std::getline(stringsFile, mString_HardwareAccelConfirm);
	//std::getline(stringsFile, mString_HardwareAccelNotWorking);
	//std::getline(stringsFile, mString_SetColorDepth);
	//std::getline(stringsFile, mString_FailedInitDirectDrawColon);
	//std::getline(stringsFile, mString_UnableOpenProperties);
	//std::getline(stringsFile, mString_SigCheckFailed);
	//std::getline(stringsFile, mString_InvalidCommandLineParam);
	//std::getline(stringsFile, mString_RequiresDirectX);
	//std::getline(stringsFile, mString_YouNeedDirectX);
	//std::getline(stringsFile, mString_FailedInitDirectDraw);
	//std::getline(stringsFile, mString_FatalError);
	//std::getline(stringsFile, mString_UnexpectedErrorOccured);
	//std::getline(stringsFile, mString_PleaseHelpBy);
	//std::getline(stringsFile, mString_FailedConnectPopcap);
	//stringsFile.close();
}

// FUNCTION: GAME187 0x43fa60
SexyAppBase::~SexyAppBase()
{
	Shutdown();

	DialogMap::iterator aDialogItr = mDialogMap.begin();
	while (aDialogItr != mDialogMap.end())
	{
		mWidgetManager->RemoveWidget(aDialogItr->second);
		delete aDialogItr->second;
		++aDialogItr;
	}
	mDialogMap.clear();
	mDialogList.clear();

	delete mWidgetManager;
	delete mResourceManager;
	delete gFPSImage;
	gFPSImage = NULL;

	SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
	while (aSharedImageItr != mSharedImageMap.end())
	{
		SharedImage* aSharedImage = &aSharedImageItr->second;
		DBG_ASSERTE(aSharedImage->mRefCount == 0);
		delete aSharedImage->mImage;
		mSharedImageMap.erase(aSharedImageItr++);
	}

	delete mDDInterface;
	delete mMusicInterface;
	delete mSoundManager;

	if (mHWnd != NULL)
	{
		SDL_DestroyWindow((SDL_Window*)mHWnd);
		mHWnd = NULL;
	}

	WaitForLoadingThread();

	gSexyAppBase = NULL;

	WriteDemoBuffer();
}

// FUNCTION: GAME187 0x440590

// FUNCTION: GAME187 0x440730
void SexyAppBase::ClearUpdateBacklog(bool relaxForASecond)
{
	mLastTimeCheck = timeGetTime();
	mUpdateFTimeAcc = 0.0;

	if (relaxForASecond)
		mRelaxUpdateBacklogCount = 1000;
}

// FUNCTION: GAME187 0x440750
bool SexyAppBase::IsScreenSaver()
{
	return mIsScreenSaver;
}

// FUNCTION: GAME187 0x440760
bool SexyAppBase::AppCanRestore()
{
	return !mIsDisabled;
}

bool SexyAppBase::ReadDemoBuffer(std::string &theError)
{
	FILE* aFP = fopen(mDemoFileName.c_str(), "rb");

	if (aFP == NULL)
	{
		theError = "Demo file not found: " + mDemoFileName;
		return false;
	}

	struct AutoFile { FILE *f; AutoFile(FILE *file) : f(file) {} ~AutoFile() { fclose(f); } };
	AutoFile aCloseFile(aFP);

	uint32_t aFileID;
	fread(&aFileID, 4, 1, aFP);

	DBG_ASSERTE(aFileID == DEMO_FILE_ID);
	if (aFileID != DEMO_FILE_ID)
	{
		theError = "Invalid demo file.";
		return false;
	}


	uint32_t aVersion;
	fread(&aVersion, 4, 1, aFP);

	fread(&mRandSeed, 4, 1, aFP);
	SRand(mRandSeed);

	ushort aStrLen = 4;
	fread(&aStrLen, 2, 1, aFP);
	if (aStrLen > 255)
		aStrLen = 255;
	char aStr[256];
	fread(aStr, 1, aStrLen, aFP);
	aStr[aStrLen] = '\0';

	DBG_ASSERTE(mProductVersion == aStr);
	if (mProductVersion != aStr)
	{
		theError = "This demo file appears to be for '" + std::string(aStr) + "'";
		return false;
	}

	int aFilePos = ftell(aFP);
	fseek(aFP, 0, SEEK_END);
	int aBytesLeft = ftell(aFP) - aFilePos;
	fseek(aFP, aFilePos, SEEK_SET);

	uchar* aBuffer;
	// read marker list
	if (aVersion >= 2)
	{
		int aSize;
		fread(&aSize, 4, 1, aFP);
		aBytesLeft -= 4;

		if (aSize >= aBytesLeft)
		{
			theError = "Invalid demo file.";
			return false;
		}

		Buffer aMarkerBuffer;

		aBuffer = new uchar[aSize];
		fread(aBuffer, 1, aSize, aFP);
		aMarkerBuffer.WriteBytes(aBuffer, aSize);
		aMarkerBuffer.SeekFront();

		int aNumItems = aMarkerBuffer.ReadLong();
		int i;
		for (i=0; i<aNumItems && !aMarkerBuffer.AtEnd(); i++)
		{
			mDemoMarkerList.push_back(DemoMarker());
			DemoMarker &aMarker = mDemoMarkerList.back();
			aMarker.first = aMarkerBuffer.ReadString();
			aMarker.second = aMarkerBuffer.ReadLong();
		}

		if (i!=aNumItems)
		{
			theError = "Invalid demo file.";
			return false;
		}

		aBytesLeft -= aSize;

		delete [] aBuffer;
	}

	// Read demo commands
	fread(&mDemoLength, 4, 1, aFP);
	aBytesLeft -= 4;

	if (aBytesLeft <= 0)
	{
		theError = "Invalid demo file.";
		return false;
	}


	aBuffer = new uchar[aBytesLeft];
	fread(aBuffer, 1, aBytesLeft, aFP);

	mDemoBuffer.WriteBytes(aBuffer, aBytesLeft);
	mDemoBuffer.SeekFront();

	delete [] aBuffer;
	return true;
}

// FUNCTION: GAME187 0x440c90
void SexyAppBase::WriteDemoBuffer()
{
	if (mRecordingDemoBuffer)
	{
		FILE* aFP = fopen(mDemoFileName.c_str(), "w+b");

		if (aFP != NULL)
		{
			uint32_t aFileID = DEMO_FILE_ID;
			fwrite(&aFileID, 4, 1, aFP);

			uint32_t aVersion = DEMO_VERSION;
			fwrite(&aVersion, 4, 1, aFP);

			fwrite(&mRandSeed, 4, 1, aFP);

			ushort aStrLen = mProductVersion.length();
			fwrite(&aStrLen, 2, 1, aFP);
			fwrite(mProductVersion.c_str(), 1, mProductVersion.length(), aFP);

			Buffer aMarkerBuffer;
			aMarkerBuffer.WriteLong(mDemoMarkerList.size());
			for (DemoMarkerList::iterator aMarkerItr = mDemoMarkerList.begin(); aMarkerItr != mDemoMarkerList.end(); ++aMarkerItr)
			{
				aMarkerBuffer.WriteString(aMarkerItr->first);
				aMarkerBuffer.WriteLong(aMarkerItr->second);
			}
			int aMarkerBufferSize = aMarkerBuffer.GetDataLen();
			fwrite(&aMarkerBufferSize, 4, 1, aFP);
			fwrite(aMarkerBuffer.GetDataPtr(), aMarkerBufferSize, 1, aFP);

			uint32_t aDemoLength = mUpdateCount;
			fwrite(&aDemoLength, 4, 1, aFP);

			fwrite(mDemoBuffer.GetDataPtr(), 1, mDemoBuffer.GetDataLen(), aFP);
			fclose(aFP);
		}
	}
}

// FUNCTION: GAME187 0x440eb0
void SexyAppBase::DemoSyncBuffer(Buffer* theBuffer)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_SYNC);

		uint32_t aLen = mDemoBuffer.ReadLong();

		theBuffer->Clear();
		for (int i = 0; i < (int) aLen; i++)
			theBuffer->WriteByte(mDemoBuffer.ReadByte());
	}
	else if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_SYNC, 5);
		mDemoBuffer.WriteLong(theBuffer->GetDataLen());
		mDemoBuffer.WriteBytes((uchar*) theBuffer->GetDataPtr(), theBuffer->GetDataLen());
	}
}

// FUNCTION: GAME187 0x440f60
void SexyAppBase::DemoSyncString(std::string* theString)
{
	Buffer aBuffer;
	aBuffer.WriteString(*theString);
	DemoSyncBuffer(&aBuffer);
	*theString = aBuffer.ReadString();
}

void SexyAppBase::DemoSyncInt(int* theInt)
{
	Buffer aBuffer;
	aBuffer.WriteLong(*theInt);
	DemoSyncBuffer(&aBuffer);
	*theInt = aBuffer.ReadLong();
}

// FUNCTION: GAME187 0x441000
void SexyAppBase::DemoSyncBool(bool* theBool)
{
	Buffer aBuffer;
	aBuffer.WriteBoolean(*theBool);
	DemoSyncBuffer(&aBuffer);
	*theBool = aBuffer.ReadBoolean();
}

void SexyAppBase::DemoAssertStringEqual(const std::string& theString)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_ASSERT_STRING_EQUAL);

		std::string aString = mDemoBuffer.ReadString();
		DBG_ASSERTE(aString == theString);
	}
	else if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_ASSERT_STRING_EQUAL, 5);
		mDemoBuffer.WriteString(theString);
	}
}

void SexyAppBase::DemoAddMarker(const std::string& theString)
{
	if (mPlayingDemoBuffer)
	{
		mFastForwardToMarker = false;
	}
	else if (mRecordingDemoBuffer)
	{
		mDemoMarkerList.push_back(DemoMarker(theString,mUpdateCount));
	}
}

void SexyAppBase::DemoRegisterHandle(HANDLE theHandle)
{
	if ((mRecordingDemoBuffer) || (mPlayingDemoBuffer))
	{
		// Insert the handle into a map with an auto-incrementing number so
		//  we can match up the auto-incrementing numbers with the handle
		//  later on, as handles may not be the same between executions
		std::pair<HandleToIntMap::iterator, bool> aPair = mHandleToIntMap.insert(HandleToIntMap::value_type(theHandle, mCurHandleNum));
		DBG_ASSERT(aPair.second);
		mCurHandleNum++;
	}
}

void SexyAppBase::DemoWaitForHandle(HANDLE theHandle)
{
	WaitForSingleObject(theHandle, INFINITE);

	if ((mRecordingDemoBuffer) || (mPlayingDemoBuffer))
	{
		// Remove the handle from our waiting map
		HandleToIntMap::iterator anItr = mHandleToIntMap.find(theHandle);
		DBG_ASSERT(anItr != mHandleToIntMap.end());
		mHandleToIntMap.erase(anItr);
	}
}

bool SexyAppBase::DemoCheckHandle(HANDLE theHandle)
{
	if (mPlayingDemoBuffer)
	{
		// We only need to try to get the result if we think we are waiting for one
		if (gSexyAppBase->PrepareDemoCommand(false))
		{
			if ((!gSexyAppBase->mDemoIsShortCmd) && (gSexyAppBase->mDemoCmdNum == DEMO_HANDLE_COMPLETE))
			{
				// Find auto-incrementing handle num from handle
				HandleToIntMap::iterator anItr = mHandleToIntMap.find(theHandle);
				DBG_ASSERT(anItr != mHandleToIntMap.end());

				int anOldBufferPos = gSexyAppBase->mDemoBuffer.mReadBitPos;

				// Since we don't require a demo result entry to be here, we must verify
				//  that this is referring to us
				int aDemoHandleNum = gSexyAppBase->mDemoBuffer.ReadLong();

				if (aDemoHandleNum == anItr->second)
				{
					// Alright, this was the handle we were waiting for!
					gSexyAppBase->mDemoNeedsCommand = true;

					// Actually wait for our local buddy to complete
					WaitForSingleObject(theHandle, INFINITE);
					mHandleToIntMap.erase(anItr);

					return true;
				}
				else
				{
					// Not us, go back
					gSexyAppBase->mDemoBuffer.mReadBitPos = anOldBufferPos;
				}
			}
		}

		return false;
	}
	else
	{
		if (WaitForSingleObject(theHandle, 0) == WAIT_OBJECT_0)
		{
			if (mRecordingDemoBuffer)
			{
				// Find auto-incrementing handle num from handle
				HandleToIntMap::iterator anItr = mHandleToIntMap.find(theHandle);
				DBG_ASSERT(anItr != mHandleToIntMap.end());

				gSexyAppBase->WriteDemoTimingBlock();
				gSexyAppBase->mDemoBuffer.WriteNumBits(0, 1);
				gSexyAppBase->mDemoBuffer.WriteNumBits(DEMO_HANDLE_COMPLETE, 5);
				gSexyAppBase->mDemoBuffer.WriteLong(anItr->second);

				mHandleToIntMap.erase(anItr);
			}

			return true;
		}

		return false;
	}
}

void SexyAppBase::DemoAssertIntEqual(int theInt)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_ASSERT_INT_EQUAL);

		int anInt = mDemoBuffer.ReadLong();
		DBG_ASSERTE(anInt == theInt);
	}
	else if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_ASSERT_INT_EQUAL, 5);
		mDemoBuffer.WriteLong(theInt);
	}
}

// FUNCTION: GAME187 0x441080
Dialog* SexyAppBase::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog(NULL, NULL, theDialogId, isModal, theDialogHeader,	theDialogLines, theDialogFooter, theButtonMode);
	return aDialog;
}

// FUNCTION: GAME187 0x441110
Dialog* SexyAppBase::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	KillDialog(theDialogId);

	Dialog* aDialog = NewDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);

	AddDialog(theDialogId, aDialog);

	return aDialog;
}


// FUNCTION: GAME187 0x441160
Dialog*	SexyAppBase::GetDialog(int theDialogId)
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())
		return anItr->second;

	return NULL;
}

// FUNCTION: GAME187 0x4411d0
bool SexyAppBase::KillDialog(int theDialogId, bool removeWidget, bool deleteWidget)
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())
	{
		Dialog* aDialog = anItr->second;

		// set the result to something else so DoMainLoop knows that the dialog is gone
		// in case nobody else sets mResult
		if (aDialog->mResult == -1)
			aDialog->mResult = 0;

		DialogList::iterator aListItr = std::find(mDialogList.begin(),mDialogList.end(),aDialog);
		if (aListItr != mDialogList.end())
			mDialogList.erase(aListItr);

		mDialogMap.erase(anItr);

		if (removeWidget || deleteWidget)
		mWidgetManager->RemoveWidget(aDialog);

		if (aDialog->IsModal())
		{
			ModalClose();
			mWidgetManager->RemoveBaseModal(aDialog);
		}

		if (deleteWidget)
		SafeDeleteWidget(aDialog);

		return true;
	}

	return false;
}

// FUNCTION: GAME187 0x441340
bool SexyAppBase::KillDialog(int theDialogId)
{
	return KillDialog(theDialogId,true,true);
}

// FUNCTION: GAME187 0x441360
bool SexyAppBase::KillDialog(Dialog* theDialog)
{
	return KillDialog(theDialog->mId);
}

// FUNCTION: GAME187 0x441380
int SexyAppBase::GetDialogCount()
{
	return mDialogMap.size();
}

// FUNCTION: GAME187 0x441390
void SexyAppBase::AddDialog(int theDialogId, Dialog* theDialog)
{
	KillDialog(theDialogId);

	if (theDialog->mWidth == 0)
	{
		// Set the dialog position ourselves
		int aWidth = mWidth/2;
		theDialog->Resize((mWidth - aWidth)/2, mHeight / 5, aWidth, theDialog->GetPreferredHeight(aWidth));
	}

	mDialogMap.insert(DialogMap::value_type(theDialogId, theDialog));
	mDialogList.push_back(theDialog);

	mWidgetManager->AddWidget(theDialog);
	if (theDialog->IsModal())
	{
		mWidgetManager->AddBaseModal(theDialog);
		ModalOpen();
	}
}

// FUNCTION: GAME187 0x4414a0
void SexyAppBase::AddDialog(Dialog* theDialog)
{
	AddDialog(theDialog->mId, theDialog);
}

void SexyAppBase::ModalOpen()
{
}

void SexyAppBase::ModalClose()
{
}

// FUNCTION: GAME187 0x4414c0
void SexyAppBase::DialogButtonPress(int theDialogId, int theButtonId)
{
	if (theButtonId == Dialog::ID_YES)
		ButtonPress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonPress(3000 + theDialogId);
}

// FUNCTION: GAME187 0x441510
void SexyAppBase::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theButtonId == Dialog::ID_YES)
		ButtonDepress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonDepress(3000 + theDialogId);
}

void SexyAppBase::GotFocus()
{
}

void SexyAppBase::LostFocus()
{
}

// FUNCTION: GAME187 0x441560
void SexyAppBase::URLOpenFailed(const std::string& theURL)
{
	mIsOpeningURL = false;
}

// FUNCTION: GAME187 0x441570
void SexyAppBase::URLOpenSucceeded(const std::string& theURL)
{
	mIsOpeningURL = false;

	if (mShutdownOnURLOpen)
		Shutdown();
}

// FUNCTION: GAME187 0x441590
bool SexyAppBase::OpenURL(const std::string& theURL, bool shutdownOnOpen)
{
	// No browser to hand the URL to.
	URLOpenFailed(theURL);
	return false;
}

// FUNCTION: GAME187 0x441630
std::string SexyAppBase::GetProductVersion(const std::string& thePath)
{
	// The Windows build read this from the exe's version resource.
	return "1.87";
}

// FUNCTION: GAME187 0x4417b0
void SexyAppBase::WaitForLoadingThread()
{
	while ((mLoadingThreadStarted) && (!mLoadingThreadCompleted))
		Sleep(20);
}

// FUNCTION: GAME187 0x4417e0
void SexyAppBase::SetCursorImage(int theCursorNum, Image* theImage)
{
	if ((theCursorNum >= 0) && (theCursorNum < NUM_CURSORS))
	{
		mCursorImages[theCursorNum] = theImage;
		EnforceCursor();
	}
}

// FUNCTION: GAME187 0x441800
void SexyAppBase::TakeScreenshot()
{
	if (mDDInterface == NULL || mDDInterface->GetScreenImage() == NULL)
		return;

	// Get free image name
	std::string anImageDir = GetAppDataFolder() + "_screenshots";
	MkDir(anImageDir);
	anImageDir += "/";

	int aMaxId = 0;
	std::string anImagePrefix = "image";
	DIR* aDir = opendir(anImageDir.c_str());
	if (aDir != NULL)
	{
		struct dirent* anEntry;
		while ((anEntry = readdir(aDir)) != NULL)
		{
			int aNum = 0;
			if (sscanf(anEntry->d_name, (anImagePrefix + "%d.png").c_str(), &aNum) == 1)
			{
				if (aNum > aMaxId)
					aMaxId = aNum;
			}
		}
		closedir(aDir);
	}
	std::string anImageName = anImageDir + anImagePrefix + StrFormat("%d.png", aMaxId + 1);

	DDImage* aScreen = mDDInterface->GetScreenImage();
	ImageLib::Image aSaveImage;
	aSaveImage.mBits = aScreen->GetBits();
	aSaveImage.mWidth = aScreen->mWidth;
	aSaveImage.mHeight = aScreen->mHeight;
	ImageLib::WritePNGImage(anImageName, &aSaveImage);
	aSaveImage.mBits = NULL;

	ClearUpdateBacklog();
}

// FUNCTION: GAME187 0x441cb0
void SexyAppBase::DumpProgramInfo()
{
	Deltree(GetAppDataFolder() + "_dump");

	for (;;)
	{
		if (mkdir((GetAppDataFolder() + "_dump").c_str(), 0777) == 0 || errno == EEXIST)
			break;
		Sleep(100);
	}

	std::fstream aDumpStream((GetAppDataFolder() + "_dump/imagelist.html").c_str(), std::ios::out);

	time_t aTime;
	time(&aTime);
	tm* aTM = localtime(&aTime);

	aDumpStream << "<HTML><BODY BGCOLOR=EEEEFF><CENTER><FONT SIZE=+2><B>" << asctime(aTM) << "</B></FONT><BR>" << std::endl;

	int anImgNum = 0;

	int aThumbWidth = 64;
	int aThumbHeight = 64;

	ImageLib::Image anImageLibImage;
	anImageLibImage.mWidth = aThumbWidth;
	anImageLibImage.mHeight = aThumbHeight;
	anImageLibImage.mBits = new uint32_t[aThumbWidth*aThumbHeight];

	typedef std::multimap<int, MemoryImage*, std::greater<int> > SortedImageMap;

	int aTotalMemory = 0;

	SortedImageMap aSortedImageMap;
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;

		int aNumPixels = aMemoryImage->mWidth*aMemoryImage->mHeight;

		DDImage* aDDImage = dynamic_cast<DDImage*>(aMemoryImage);

		int aBitsMemory = 0;
		int aSurfaceMemory = 0;
		int aPalletizedMemory = 0;
		int aNativeAlphaMemory = 0;
		int aRLAlphaMemory = 0;
		int aRLAdditiveMemory = 0;
		int aTextureMemory = 0;

		int aMemorySize = 0;
		if (aMemoryImage->mBits != NULL)
			aBitsMemory = aNumPixels * 4;
		if ((aDDImage != NULL) && (aDDImage->mSurface != NULL))
			aSurfaceMemory = aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != NULL)
			aPalletizedMemory = aNumPixels + 256*4;
		if (aMemoryImage->mNativeAlphaData != NULL)
		{
			if (aMemoryImage->mColorTable != NULL)
				aNativeAlphaMemory = 256*4;
			else
				aNativeAlphaMemory = aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != NULL)
			aRLAlphaMemory = aNumPixels;
		if (aMemoryImage->mRLAdditiveData != NULL)
			aRLAdditiveMemory = aNumPixels;

		aMemorySize = aBitsMemory + aSurfaceMemory + aPalletizedMemory + aNativeAlphaMemory + aRLAlphaMemory + aRLAdditiveMemory + aTextureMemory;
		aTotalMemory += aMemorySize;

		aSortedImageMap.insert(SortedImageMap::value_type(aMemorySize, aMemoryImage));

		++anItr;
	}

	aDumpStream << "Total Image Allocation: " << CommaSeperate(aTotalMemory).c_str() << " bytes<BR>";
	aDumpStream << "<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=4>";

	int aTotalMemorySize = 0;
	int aTotalBitsMemory = 0;
	int aTotalSurfaceMemory = 0;
	int aTotalPalletizedMemory = 0;
	int aTotalNativeAlphaMemory = 0;
	int aTotalRLAlphaMemory = 0;
	int aTotalRLAdditiveMemory = 0;
	int aTotalTextureMemory = 0;

	SortedImageMap::iterator aSortedItr = aSortedImageMap.begin();
	while (aSortedItr != aSortedImageMap.end())
	{
		MemoryImage* aMemoryImage = aSortedItr->second;

		char anImageName[256];
		sprintf(anImageName, "img%04d.png", anImgNum);

		char aThumbName[256];
		sprintf(aThumbName, "thumb%04d.jpg", anImgNum);

		aDumpStream << "<TR>" << std::endl;

		aDumpStream << "<TD><A HREF=" << anImageName << "><IMG SRC=" << aThumbName << " WIDTH=" << aThumbWidth << " HEIGHT=" << aThumbHeight << "></A></TD>" << std::endl;

		int aNumPixels = aMemoryImage->mWidth*aMemoryImage->mHeight;

		DDImage* aDDImage = dynamic_cast<DDImage*>(aMemoryImage);

		int aMemorySize = aSortedItr->first;

		int aBitsMemory = 0;
		int aSurfaceMemory = 0;
		int aPalletizedMemory = 0;
		int aNativeAlphaMemory = 0;
		int aRLAlphaMemory = 0;
		int aRLAdditiveMemory = 0;
		int aTextureMemory = 0;
		std::string aTextureFormatName;

		if (aMemoryImage->mBits != NULL)
			aBitsMemory = aNumPixels * 4;
		if ((aDDImage != NULL) && (aDDImage->mSurface != NULL))
			aSurfaceMemory = aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != NULL)
			aPalletizedMemory = aNumPixels + 256*4;
		if (aMemoryImage->mNativeAlphaData != NULL)
		{
			if (aMemoryImage->mColorTable != NULL)
				aNativeAlphaMemory = 256*4;
			else
				aNativeAlphaMemory = aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != NULL)
			aRLAlphaMemory = aNumPixels;
		if (aMemoryImage->mRLAdditiveData != NULL)
			aRLAdditiveMemory = aNumPixels;

		aTotalMemorySize		+= aMemorySize;
		aTotalBitsMemory		+= aBitsMemory;
		aTotalTextureMemory		+= aTextureMemory;
		aTotalSurfaceMemory		+= aSurfaceMemory;
		aTotalPalletizedMemory	+= aPalletizedMemory;
		aTotalNativeAlphaMemory	+= aNativeAlphaMemory;
		aTotalRLAlphaMemory		+= aRLAlphaMemory;
		aTotalRLAdditiveMemory	+= aRLAdditiveMemory;



		char aStr[256];
		sprintf(aStr, "%d x %d<BR>%s bytes", aMemoryImage->mWidth, aMemoryImage->mHeight, CommaSeperate(aMemorySize).c_str());
		aDumpStream << "<TD ALIGN=RIGHT>" << aStr << "</TD>" << std::endl;

		aDumpStream << "<TD>" << SexyStringToString(((aBitsMemory != 0) ? _S("mBits<BR>") + CommaSeperate(aBitsMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aPalletizedMemory != 0) ? _S("Palletized<BR>") + CommaSeperate(aPalletizedMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aSurfaceMemory != 0) ? _S("DDSurface<BR>") + CommaSeperate(aSurfaceMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mD3DData!=NULL) ? _S("Texture<BR>") + StringToSexyString(aTextureFormatName) + _S("<BR>") + CommaSeperate(aTextureMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;

		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mIsVolatile) ? _S("Volatile") : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mForcedMode) ? _S("Forced") : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mHasAlpha) ? _S("HasAlpha") : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mHasTrans) ? _S("HasTrans") : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aNativeAlphaMemory != 0) ? _S("NativeAlpha<BR>") + CommaSeperate(aNativeAlphaMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aRLAlphaMemory != 0) ? _S("RLAlpha<BR>") + CommaSeperate(aRLAlphaMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aRLAdditiveMemory != 0) ? _S("RLAdditive<BR>") + CommaSeperate(aRLAdditiveMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << (aMemoryImage->mFilePath.empty()? "&nbsp;":aMemoryImage->mFilePath) << "</TD>" << std::endl;

		aDumpStream << "</TR>" << std::endl;

		// Write thumb

		MemoryImage aCopiedImage(*aMemoryImage);

		uint32_t* aBits = aCopiedImage.GetBits();

		uint32_t* aThumbBitsPtr = anImageLibImage.mBits;

		for (int aThumbY = 0; aThumbY < aThumbHeight; aThumbY++)
			for (int aThumbX = 0; aThumbX < aThumbWidth; aThumbX++)
			{
				int aSrcX = (int) (aCopiedImage.mWidth  * (aThumbX + 0.5)) / aThumbWidth;
				int aSrcY = (int) (aCopiedImage.mHeight * (aThumbY + 0.5)) / aThumbHeight;

				*(aThumbBitsPtr++) = aBits[aSrcX + (aSrcY*aCopiedImage.mWidth)];
			}

		ImageLib::WriteJPEGImage((GetAppDataFolder() + std::string("_dump\\") + aThumbName).c_str(), &anImageLibImage);

		// Write high resolution image

		ImageLib::Image anFullImage;
		anFullImage.mBits = aCopiedImage.GetBits();
		anFullImage.mWidth = aCopiedImage.GetWidth();
		anFullImage.mHeight = aCopiedImage.GetHeight();

		ImageLib::WritePNGImage((GetAppDataFolder() + std::string("_dump\\") + anImageName).c_str(), &anFullImage);

		anFullImage.mBits = NULL;

		anImgNum++;

		aSortedItr++;
	}

	aDumpStream << "<TD>Totals</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalMemorySize)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalBitsMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalPalletizedMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalSurfaceMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalTextureMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalNativeAlphaMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalRLAlphaMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalRLAdditiveMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;

	aDumpStream << "</TABLE></CENTER></BODY></HTML>" << std::endl;
}

// FUNCTION: GAME187 0x444520
double SexyAppBase::GetLoadingThreadProgress()
{
	if (mLoaded)
		return 1.0;
	if (!mLoadingThreadStarted)
		return 0.0;
	if (mNumLoadingThreadTasks == 0)
		return 0.0;
	return min(mCompletedLoadingThreadTasks / (double) mNumLoadingThreadTasks, 1.0);
}

// FUNCTION: GAME187 0x444570
bool SexyAppBase::RegistryWrite(const std::string& theValueName, uint32_t theType, const uchar* theValue, uint32_t theLength)
{
	if (mRegKey.length() == 0)
		return false;

	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_REGISTRY_WRITE);

		return mDemoBuffer.ReadNumBits(1, false) != 0;
	}

	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mRegKey);
	std::string aValueName;

	int aSlashPos = (int) theValueName.rfind('\\');
	if (aSlashPos != -1)
	{
		aKeyName += "\\" + theValueName.substr(0, aSlashPos);
		aValueName = theValueName.substr(aSlashPos + 1);
	}
	else
	{
		aValueName = theValueName;
	}

	if (!SwitchRegistry::Write(aKeyName, aValueName, theType, theValue, theLength))
	{
		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_REGISTRY_WRITE, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure
		}

		return false;
	}

	if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_REGISTRY_WRITE, 5);
		mDemoBuffer.WriteNumBits(1, 1); // success
	}

	return true;
}

// FUNCTION: GAME187 0x444930
bool SexyAppBase::RegistryWriteString(const std::string& theValueName, const std::string& theString)
{
	return RegistryWrite(theValueName, REG_SZ, (uchar*) theString.c_str(), theString.length());
}

// FUNCTION: GAME187 0x444970
bool SexyAppBase::RegistryWriteInteger(const std::string& theValueName, int theValue)
{
	return RegistryWrite(theValueName, REG_DWORD, (uchar*) &theValue, sizeof(int));
}

// FUNCTION: GAME187 0x444990
bool SexyAppBase::RegistryWriteBoolean(const std::string& theValueName, bool theValue)
{
	int aValue = theValue ? 1 : 0;
	return RegistryWrite(theValueName, REG_DWORD, (uchar*) &aValue, sizeof(int));
}

bool SexyAppBase::RegistryWriteData(const std::string& theValueName, const uchar* theValue, uint32_t theLength)
{
	return RegistryWrite(theValueName, REG_BINARY, (uchar*) theValue, theLength);
}

// FUNCTION: GAME187 0x4449b0
void SexyAppBase::WriteToRegistry()
{
	RegistryWriteInteger("MusicVolume", (int) (mMusicVolume * 100));
	RegistryWriteInteger("SfxVolume", (int) (mSfxVolume * 100));
	RegistryWriteInteger("Muted", (mMuteCount - mAutoMuteCount > 0) ? 1 : 0);
	RegistryWriteInteger("ScreenMode", mIsWindowed ? 0 : 1);
	RegistryWriteInteger("PreferredX", mPreferredX);
	RegistryWriteInteger("PreferredY", mPreferredY);
	RegistryWriteInteger("CustomCursors", mCustomCursorsEnabled ? 1 : 0);
	RegistryWriteInteger("InProgress", 0);
	RegistryWriteBoolean("WaitForVSync", mWaitForVSync);
}

bool SexyAppBase::RegistryEraseKey(const SexyString& _theKeyName)
{
	std::string theKeyName = SexyStringToStringFast(_theKeyName);
	if (mRegKey.length() == 0)
		return false;

	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_REGISTRY_ERASE);

		return mDemoBuffer.ReadNumBits(1, false) != 0;
	}

	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mRegKey) + "\\" + theKeyName;

	if (!SwitchRegistry::DeleteKey(aKeyName))
	{
		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_REGISTRY_ERASE, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure
		}

		return false;
	}

	if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_REGISTRY_ERASE, 5);
		mDemoBuffer.WriteNumBits(1, 1); // success
	}

	return true;
}

// FUNCTION: GAME187 0x444d00
void SexyAppBase::RegistryEraseValue(const SexyString& _theValueName)
{
	std::string theValueName = SexyStringToStringFast(_theValueName);
	if (mRegKey.length() == 0)
		return;

	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mRegKey);
	std::string aValueName;

	int aSlashPos = (int) theValueName.rfind('\\');
	if (aSlashPos != -1)
	{
		aKeyName += "\\" + theValueName.substr(0, aSlashPos);
		aValueName = theValueName.substr(aSlashPos + 1);
	}
	else
	{
		aValueName = theValueName;
	}

	SwitchRegistry::DeleteValue(aKeyName, aValueName);
}

bool SexyAppBase::RegistryGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys)
{
	theSubKeys->clear();

	if (mRegKey.length() == 0)
		return false;

	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_REGISTRY_GETSUBKEYS);

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		if (!success)
			return false;

		int aNumKeys = mDemoBuffer.ReadLong();

		for (int i = 0; i < aNumKeys; i++)
			theSubKeys->push_back(mDemoBuffer.ReadString());

		return true;
	}
	else
	{
		std::string aKeyName = RemoveTrailingSlash(RemoveTrailingSlash("SOFTWARE\\" + mRegKey) + "\\" + theKeyName);
		if (SwitchRegistry::GetSubKeys(aKeyName, theSubKeys))
		{

			if (mRecordingDemoBuffer)
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_REGISTRY_GETSUBKEYS, 5);
				mDemoBuffer.WriteNumBits(1, 1); // success
				mDemoBuffer.WriteLong(theSubKeys->size());

				for (int i = 0; i < (int) theSubKeys->size(); i++)
					mDemoBuffer.WriteString((*theSubKeys)[i]);
			}

			return true;
		}
		else
		{
			if (mRecordingDemoBuffer)
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_REGISTRY_GETSUBKEYS, 5);
				mDemoBuffer.WriteNumBits(0, 1); // failure
			}

			return false;
		}
	}
}

// FUNCTION: GAME187 0x444fc0
bool SexyAppBase::RegistryRead(const std::string& theValueName, uint32_t* theType, uchar* theValue, uint32_t* theLength)
{
	return RegistryReadKey(theValueName, theType, theValue, theLength, HKEY_CURRENT_USER);
}

// FUNCTION: GAME187 0x444fe0
bool SexyAppBase::RegistryReadKey(const std::string& theValueName, uint32_t* theType, uchar* theValue, uint32_t* theLength, HKEY theKey)
{
	if (mRegKey.length() == 0)
		return false;

	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return false;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_REGISTRY_READ);

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		if (!success)
			return false;

		*theType = mDemoBuffer.ReadLong();

		uint32_t aLen = mDemoBuffer.ReadLong();
		*theLength = aLen;

		if (*theLength >= aLen)
		{
			mDemoBuffer.ReadBytes(theValue, aLen);
			return true;
		}
		else
		{
			for (int i = 0; i < (int) aLen; i++)
				mDemoBuffer.ReadByte();
			return false;
		}
	}
	else
	{
		std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mRegKey);
		std::string aValueName;

		int aSlashPos = (int) theValueName.rfind('\\');
		if (aSlashPos != -1)
		{
			aKeyName += "\\" + theValueName.substr(0, aSlashPos);
			aValueName = theValueName.substr(aSlashPos + 1);
		}
		else
		{
			aValueName = theValueName;
		}

		{
			if (SwitchRegistry::Read(aKeyName, aValueName, theType, theValue, theLength))
			{
				if (mRecordingDemoBuffer)
				{
					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_REGISTRY_READ, 5);
					mDemoBuffer.WriteNumBits(1, 1); // success
					mDemoBuffer.WriteLong(*theType);
					mDemoBuffer.WriteLong(*theLength);
					mDemoBuffer.WriteBytes(theValue, *theLength);
				}

				return true;
			}
		}

		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_REGISTRY_READ, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure
		}

		return false;
	}
}

// FUNCTION: GAME187 0x4453c0
bool SexyAppBase::RegistryReadString(const std::string& theKey, std::string* theString)
{
	char aStr[1024];

	uint32_t aType;
	uint32_t aLen = sizeof(aStr) - 1;
	if (!RegistryRead(theKey, &aType, (uchar*) aStr, &aLen))
		return false;

	if (aType != REG_SZ)
		return false;

	aStr[aLen] = 0;

	*theString = aStr;
	return true;
}

// FUNCTION: GAME187 0x445470
bool SexyAppBase::RegistryReadInteger(const std::string& theKey, int* theValue)
{
	uint32_t aType;
	uint32_t aLong;
	uint32_t aLen = 4;
	if (!RegistryRead(theKey, &aType, (uchar*) &aLong, &aLen))
		return false;

	if (aType != REG_DWORD)
		return false;

	*theValue = aLong;
	return true;
}

// FUNCTION: GAME187 0x4454c0
bool SexyAppBase::RegistryReadBoolean(const std::string& theKey, bool* theValue)
{
	int aValue;
	if (!RegistryReadInteger(theKey, &aValue))
		return false;

	*theValue = aValue != 0;
	return true;
}

bool SexyAppBase::RegistryReadData(const std::string& theKey, uchar* theValue, uint32_t* theLength)
{
	uint32_t aType;
	uint32_t aLen = *theLength;
	if (!RegistryRead(theKey, &aType, (uchar*) theValue, theLength))
		return false;

	if (aType != REG_BINARY)
		return false;

	return true;
}

// FUNCTION: GAME187 0x4454f0
void SexyAppBase::ReadFromRegistry()
{
	mReadFromRegistry = true;
	mRegKey = SexyStringToString(GetString("RegistryKey", StringToSexyString(mRegKey)));

	if (mRegKey.length() == 0)
		return;

	int anInt;
	if (RegistryReadInteger("MusicVolume", &anInt))
		mMusicVolume = anInt / 100.0;

	if (RegistryReadInteger("SfxVolume", &anInt))
		mSfxVolume = anInt / 100.0;

	if (RegistryReadInteger("Muted", &anInt))
		mMuteCount = anInt;

	if (RegistryReadInteger("ScreenMode", &anInt))
		mIsWindowed = anInt == 0;

	RegistryReadInteger("PreferredX", &mPreferredX);
	RegistryReadInteger("PreferredY", &mPreferredY);

	if (RegistryReadInteger("CustomCursors", &anInt))
		EnableCustomCursors(anInt != 0);

	RegistryReadBoolean("WaitForVSync", &mWaitForVSync);

	if (RegistryReadInteger("InProgress", &anInt))
		mLastShutdownWasGraceful = anInt == 0;

	if (!IsScreenSaver())
		RegistryWriteInteger("InProgress", 1);
}

// FUNCTION: GAME187 0x445a60
bool SexyAppBase::WriteBytesToFile(const std::string& theFileName, const void *theData, uint32_t theDataLen)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_FILE_WRITE);

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		if (!success)
			return false;

		return true;
	}

	MkDir(GetFileDir(theFileName));
	FILE* aFP = fopen(theFileName.c_str(), "w+b");

	if (aFP == NULL)
	{
		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_FILE_WRITE, 5);
			mDemoBuffer.WriteNumBits(0, 1); // failure
		}

		return false;
	}

	fwrite(theData, 1, theDataLen, aFP);
	fclose(aFP);

	if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_FILE_WRITE, 5);
		mDemoBuffer.WriteNumBits(1, 1); // success
	}

	return true;
}

// FUNCTION: GAME187 0x445bf0
bool SexyAppBase::WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer)
{
	return WriteBytesToFile(theFileName,theBuffer->GetDataPtr(),theBuffer->GetDataLen());
}


// FUNCTION: GAME187 0x445c20
bool SexyAppBase::ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo)
{
	if ((mPlayingDemoBuffer) && (!dontWriteToDemo))
	{
		if (mManualShutdown)
			return false;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_FILE_READ);

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		if (!success)
			return false;

		uint32_t aLen = mDemoBuffer.ReadLong();

		theBuffer->Clear();
		for (int i = 0; i < (int) aLen; i++)
			theBuffer->WriteByte(mDemoBuffer.ReadByte());

		return true;
	}
	else
	{
		PFILE* aFP = p_fopen(theFileName.c_str(), "rb");

		if (aFP == NULL)
		{
			if ((mRecordingDemoBuffer) && (!dontWriteToDemo))
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_FILE_READ, 5);
				mDemoBuffer.WriteNumBits(0, 1); // failure
			}

			return false;
		}

		p_fseek(aFP, 0, SEEK_END);
		int aFileSize = p_ftell(aFP);
		p_fseek(aFP, 0, SEEK_SET);

		uchar* aData = new uchar[aFileSize];

		p_fread(aData, 1, aFileSize, aFP);
		p_fclose(aFP);

		theBuffer->Clear();
		theBuffer->SetData(aData, aFileSize);

		if ((mRecordingDemoBuffer) && (!dontWriteToDemo))
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_FILE_READ, 5);
			mDemoBuffer.WriteNumBits(1, 1); // success
			mDemoBuffer.WriteLong(aFileSize);
			mDemoBuffer.WriteBytes(aData, aFileSize);
		}

		delete [] aData;

		return true;
	}
}

// FUNCTION: GAME187 0x445dc0
bool SexyAppBase::FileExists(const std::string& theFileName)
{
	if (mPlayingDemoBuffer)
	{
		if (mManualShutdown)
			return true;

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_FILE_EXISTS);

		bool success = mDemoBuffer.ReadNumBits(1, false) != 0;
		return success;
	}
	else
	{
		PFILE* aFP = p_fopen(theFileName.c_str(), "rb");

		if (mRecordingDemoBuffer)
		{
			WriteDemoTimingBlock();
			mDemoBuffer.WriteNumBits(0, 1);
			mDemoBuffer.WriteNumBits(DEMO_FILE_EXISTS, 5);
			mDemoBuffer.WriteNumBits((aFP != NULL) ? 1 : 0, 1);
		}

		if (aFP == NULL)
			return false;

		p_fclose(aFP);
		return true;
	}
}

// FUNCTION: GAME187 0x445e70
bool SexyAppBase::EraseFile(const std::string& theFileName)
{
	if (mPlayingDemoBuffer)
		return true;

	return unlink(theFileName.c_str()) == 0;
}

// FUNCTION: GAME187 0x445eb0
void SexyAppBase::SEHOccured()
{
	SetMusicVolume(0);
	mSEHOccured = true;
	EnforceCursor();
}

// FUNCTION: GAME187 0x445ef0
std::string SexyAppBase::GetGameSEHInfo()
{
	int aSecLoaded = (GetTickCount() - mTimeLoaded) / 1000;

	char aTimeStr[16];
	sprintf(aTimeStr, "%02d:%02d:%02d", (aSecLoaded/60/60), (aSecLoaded/60)%60, aSecLoaded%60);

	char aThreadIdStr[16];
	sprintf(aThreadIdStr, "%X", mPrimaryThreadId);

	std::string anInfoString =
		"Product: " + mProdName + "\r\n" +
		"Version: " + mProductVersion + "\r\n";

	anInfoString +=
		"Time Loaded: " + std::string(aTimeStr) + "\r\n"
		"Fullscreen: " + (mIsWindowed ? std::string("No") : std::string("Yes")) + "\r\n"
		"Primary ThreadId: " + aThreadIdStr + "\r\n";

	return anInfoString;
}

void SexyAppBase::GetSEHWebParams(DefinesMap* theDefinesMap)
{
}

void SexyAppBase::ShutdownHook()
{
}

// FUNCTION: GAME187 0x446420
void SexyAppBase::Shutdown()
{
	if ((mPrimaryThreadId != 0) && (GetCurrentThreadId() != mPrimaryThreadId))
	{
		mLoadingFailed = true;
	}
	else if (!mShutdown)
	{
		mExitToTop = true;
		mShutdown = true;
		ShutdownHook();

		if (mPlayingDemoBuffer)
		{
			//if the music/sfx volume is 0, then it means that in playback
			//someone pressed the "S" key to mute sounds (or that the
			//sound volume was set to 0 in the first place). Out of politeness,
			//return the system sound volume to what it last was in the game.
			SetMusicVolume(mDemoMusicVolume);
			SetSfxVolume(mDemoSfxVolume);
		}

		// Blah
		while (mCursorThreadRunning)
		{
			Sleep(10);
		}

		if (mMusicInterface != NULL)
			mMusicInterface->StopAllMusic();

		RestoreScreenResolution();

		if (mReadFromRegistry)
			WriteToRegistry();

		ImageLib::CloseJPEG2000();
	}
}

// FUNCTION: GAME187 0x446530
void SexyAppBase::RestoreScreenResolution()
{
	mFullScreenWindow = false;
}

// FUNCTION: GAME187 0x446570
void SexyAppBase::DoExit(int theCode)
{
	RestoreScreenResolution();
	exit(theCode);
}

// FUNCTION: GAME187 0x446580
void SexyAppBase::UpdateFrames()
{
	mUpdateCount++;

	if (!mMinimized)
	{
		if (mWidgetManager->UpdateFrame())
			++mFPSDirtyCount;
	}

	mMusicInterface->Update();
	CleanSharedImages();
}

// FUNCTION: GAME187 0x4465c0
void SexyAppBase::DoUpdateFramesF(float theFrac)
{
	if ((mVSyncUpdates) && (!mMinimized))
		mWidgetManager->UpdateFrameF(theFrac);
}

// FUNCTION: GAME187 0x4465f0
bool SexyAppBase::DoUpdateFrames()
{
	SEXY_AUTO_PERF("SexyAppBase::DoUpdateFrames");

	if (gScreenSaverActive)
		return false;

	if (mPlayingDemoBuffer)
	{
		if ((mLoadingThreadCompleted) && (!mLoaded) && (mDemoLoadingComplete))
		{
			mLoaded = true;
			mYieldMainThread = false;
			LoadingThreadCompleted();
		}

		// Hrrm not sure why we check (mUpdateCount != mLastDemoUpdateCnt) here
		if ((mLoaded == mDemoLoadingComplete) && (mUpdateCount != mLastDemoUpdateCnt))
		{
			UpdateFrames();
			return true;
		}

		return false;
	}
	else
	{
		if ((mLoadingThreadCompleted) && (!mLoaded))
		{
			mLoaded = true;
			mYieldMainThread = false;
			LoadingThreadCompleted();

			if (mRecordingDemoBuffer)
			{
				WriteDemoTimingBlock();
				mDemoBuffer.WriteNumBits(0, 1);
				mDemoBuffer.WriteNumBits(DEMO_LOADING_COMPLETE, 5);
			}
		}

		UpdateFrames();
		return true;
	}
}

bool gIsFailing = false;

// FUNCTION: GAME187 0x4466e0
void SexyAppBase::Redraw(Rect* theClipRect)
{
	// Do mIsDrawing check because we could enter here at a bad time if any windows messages
	//  are processed during WidgetManager->Draw
	if ((mIsDrawing) || (mShutdown))
		return;

	if (!mDDInterface->Redraw(theClipRect))
	{
		gIsFailing = true;

		// Lost the renderer: build it again and reupload everything.
		mWidgetManager->mImage = NULL;
		int aResult = InitDDInterface();
		if (aResult != DDInterface::RESULT_OK)
			return;

		ReInitImages();

		mWidgetManager->mImage = mDDInterface->GetScreenImage();
		mWidgetManager->MarkAllDirty();

		mLastTime = timeGetTime();
	}
	else
		gIsFailing = false;

	mFPSFlipCount++;
}

///////////////////////////// FPS Stuff
static PerfTimer gFPSTimer;
static int gFrameCount;
static int gFPSDisplay;
static bool gForceDisplay = false;
// FUNCTION: GAME187 0x446880
static void CalculateFPS()
{
	gFrameCount++;

	static SysFont aFont(gSexyAppBase,"Tahoma",8);
	if (gFPSImage==NULL)
	{
		gFPSImage = new DDImage(gSexyAppBase->mDDInterface);
		gFPSImage->Create(50,aFont.GetHeight()+4);
		gFPSImage->SetImageMode(false,false);
		gFPSImage->SetVolatile(true);
		gFPSImage->mPurgeBits = false;
		gFPSImage->mWantDDSurface = true;
		gFPSImage->PurgeBits();
	}

	if (gFPSTimer.GetDuration() >= 1000 || gForceDisplay)
	{
		gFPSTimer.Stop();
		if (!gForceDisplay)
			gFPSDisplay = (int)(gFrameCount*1000/gFPSTimer.GetDuration() + 0.5f);
		else
		{
			gForceDisplay = false;
			gFPSDisplay = 0;
		}

		gFPSTimer.Start();
		gFrameCount = 0;

		Graphics aDrawG(gFPSImage);
		aDrawG.SetFont(&aFont);
		SexyString aFPS = StrFormat(_S("FPS: %d"), gFPSDisplay);
		aDrawG.SetColor(0x000000);
		aDrawG.FillRect(0,0,gFPSImage->GetWidth(),gFPSImage->GetHeight());
		aDrawG.SetColor(0xFFFFFF);
		aDrawG.DrawString(aFPS,2,aFont.GetAscent());
		//gFPSImage->mKeepBits = false;
		//gFPSImage->GenerateDDSurface();
		gFPSImage->mBitsChangedCount++;
	}
}

///////////////////////////// FPS Stuff to draw mouse coords
// FUNCTION: GAME187 0x446b80
static void FPSDrawCoords(int theX, int theY)
{
	static SysFont aFont(gSexyAppBase,"Tahoma",8);
	if (gFPSImage==NULL)
	{
		gFPSImage = new DDImage(gSexyAppBase->mDDInterface);
		gFPSImage->Create(50,aFont.GetHeight()+4);
		gFPSImage->SetImageMode(false,false);
		gFPSImage->SetVolatile(true);
		gFPSImage->mPurgeBits = false;
		gFPSImage->mWantDDSurface = true;
		gFPSImage->PurgeBits();
	}

	Graphics aDrawG(gFPSImage);
	aDrawG.SetFont(&aFont);
	SexyString aFPS = StrFormat(_S("%d,%d"),theX,theY);
	aDrawG.SetColor(0x000000);
	aDrawG.FillRect(0,0,gFPSImage->GetWidth(),gFPSImage->GetHeight());
	aDrawG.SetColor(0xFFFFFF);
	aDrawG.DrawString(aFPS,2,aFont.GetAscent());
	gFPSImage->mBitsChangedCount++;
}

///////////////////////////// Demo TimeLeft Stuff
static DDImage* gDemoTimeLeftImage = NULL;
// FUNCTION: GAME187 0x446e10
static void CalculateDemoTimeLeft()
{
	static SysFont aFont(gSexyAppBase,"Tahoma",8);
	static DWORD aLastTick = 0;

	if (gDemoTimeLeftImage==NULL)
	{
		gDemoTimeLeftImage = new DDImage(gSexyAppBase->mDDInterface);
		gDemoTimeLeftImage->Create(50,aFont.GetHeight()+4);
		gDemoTimeLeftImage->SetImageMode(false,false);
		gDemoTimeLeftImage->SetVolatile(true);
		gDemoTimeLeftImage->mPurgeBits = false;
		gDemoTimeLeftImage->mWantDDSurface = true;
		gDemoTimeLeftImage->PurgeBits();
	}

	DWORD aTick = GetTickCount();
	if (aTick - aLastTick < 1000/gSexyAppBase->mUpdateMultiplier)
		return;

	aLastTick = aTick;

	int aNumUpdatesLeft = gSexyAppBase->mDemoLength - gSexyAppBase->mUpdateCount;
	Graphics aDrawG(gDemoTimeLeftImage);
	aDrawG.SetFont(&aFont);

	int aTotalSeconds = aNumUpdatesLeft*gSexyAppBase->mFrameTime/1000;
	int aSeconds = aTotalSeconds%60;
	int aMinutes = (aTotalSeconds/60)%60;
	int anHours = (aTotalSeconds/3600);

	SexyString aFPS = StrFormat(_S("%02d:%02d:%02d"), anHours,aMinutes,aSeconds);
	aDrawG.SetColor(0x000000);
	aDrawG.FillRect(0,0,gDemoTimeLeftImage->GetWidth(),gDemoTimeLeftImage->GetHeight());
	aDrawG.SetColor(0xFFFFFF);
	aDrawG.DrawString(aFPS,2,aFont.GetAscent());
	gDemoTimeLeftImage->mBitsChangedCount++;
}

// FUNCTION: GAME187 0x447140
static void UpdateScreenSaverInfo(DWORD theTick)
{
	// The console handles screen dimming itself.
}

// FUNCTION: GAME187 0x447280
bool SexyAppBase::DrawDirtyStuff()
{
	SEXY_AUTO_PERF("SexyAppBase::DrawDirtyStuff");
	MTAutoDisallowRand aDisallowRand;

	if (gIsFailing) // just try to reinit
	{
		Redraw(NULL);
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;
		return false;
	}

	if (mShowFPS)
	{
		switch(mShowFPSMode)
		{
			case FPS_ShowFPS: CalculateFPS(); break;
			case FPS_ShowCoords:
				if (mWidgetManager!=NULL)
					FPSDrawCoords(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
				break;
		}

		if (mPlayingDemoBuffer)
			CalculateDemoTimeLeft();
	}

	DWORD aStartTime = timeGetTime();

	// Update user input and screen saver info
	static DWORD aPeriodicTick = 0;
	if (aStartTime-aPeriodicTick > 1000)
	{
		aPeriodicTick = aStartTime;
		UpdateScreenSaverInfo(aStartTime);
	}

	if (gScreenSaverActive)
	{
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;
		return false;
	}

	mIsDrawing = true;
	bool drewScreen = mWidgetManager->DrawScreen();
	mIsDrawing = false;

	if ((drewScreen || (aStartTime - mLastDrawTick >= 1000) || (mCustomCursorDirty)) &&
		((int) (aStartTime - mNextDrawTick) >= 0))
	{
		mLastDrawWasEmpty = false;

		mDrawCount++;

		DWORD aMidTime = timeGetTime();

		mFPSCount++;
		mFPSTime += aMidTime - aStartTime;

		mDrawTime += aMidTime - aStartTime;

		if (mShowFPS)
		{
			Graphics g(mDDInterface->GetScreenImage());
			g.DrawImage(gFPSImage,mWidth-gFPSImage->GetWidth()-10,mHeight-gFPSImage->GetHeight()-10);

			if (mPlayingDemoBuffer)
				g.DrawImage(gDemoTimeLeftImage,mWidth-gDemoTimeLeftImage->GetWidth()-10,mHeight-gFPSImage->GetHeight()-gDemoTimeLeftImage->GetHeight()-15);
		}

		if (mWaitForVSync && mIsPhysWindowed && mSoftVSyncWait)
		{
			DWORD aTick = timeGetTime();
			if (aTick-mLastDrawTick < mDDInterface->mMillisecondsPerFrame)
				Sleep(mDDInterface->mMillisecondsPerFrame - (aTick-mLastDrawTick));
		}

		DWORD aPreScreenBltTime = timeGetTime();
		mLastDrawTick = aPreScreenBltTime;

		Redraw(NULL);

		// This is our one UpdateFTimeAcc if we are vsynched
		UpdateFTimeAcc();

		DWORD aEndTime = timeGetTime();

		mScreenBltTime = aEndTime - aPreScreenBltTime;

#ifdef _DEBUG
		/*if (mFPSTime >= 5000) // Show FPS about every 5 seconds
		{
			uint32_t aTickNow = GetTickCount();

			OutputDebugString(StrFormat(_S("Theoretical FPS: %d\r\n"), (int) (mFPSCount * 1000 / mFPSTime)).c_str());
			OutputDebugString(StrFormat(_S("Actual      FPS: %d\r\n"), (mFPSFlipCount * 1000) / max((aTickNow - mFPSStartTick), 1)).c_str());
			OutputDebugString(StrFormat(_S("Dirty Rate     : %d\r\n"), (mFPSDirtyCount * 1000) / max((aTickNow - mFPSStartTick), 1)).c_str());

			mFPSTime = 0;
			mFPSCount = 0;
			mFPSFlipCount = 0;
			mFPSStartTick = aTickNow;
			mFPSDirtyCount = 0;
		}*/
#endif

		if ((mLoadingThreadStarted) && (!mLoadingThreadCompleted))
		{
			int aTotalTime = aEndTime - aStartTime;

			mNextDrawTick += 35 + max(aTotalTime, 15);

			if ((int) (aEndTime - mNextDrawTick) >= 0)
				mNextDrawTick = aEndTime;

			/*char aStr[256];
			sprintf(aStr, "Next Draw Time: %d\r\n", mNextDrawTick);
			OutputDebugString(aStr);*/
		}
		else
			mNextDrawTick = aEndTime;

		mHasPendingDraw = false;
		mCustomCursorDirty = false;

		return true;
	}
	else
	{
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;
		return false;
	}
}

// FUNCTION: GAME187 0x447590
void SexyAppBase::LogScreenSaverError(const std::string &theError)
{
	fprintf(stderr, "%s %u\n", theError.c_str(), (unsigned)GetTickCount());
}

// FUNCTION: GAME187 0x447630
void SexyAppBase::BeginPopup()
{
}

// FUNCTION: GAME187 0x447660
void SexyAppBase::EndPopup()
{
	if (!mIsPhysWindowed)
		mNoDefer = false;

	ClearUpdateBacklog();
	ClearKeysDown();

	if (mWidgetManager->mDownButtons)
	{
		mWidgetManager->DoMouseUps();
		ReleaseCapture();
	}
}

// FUNCTION: GAME187 0x4476a0
int SexyAppBase::MsgBox(const std::string& theText, const std::string& theTitle, int theFlags)
{
	// No modal message boxes: log it and give the cautious answer.
	fprintf(stderr, "[%s] %s\n", theTitle.c_str(), theText.c_str());

	BeginPopup();
	EndPopup();

	switch (theFlags & 0xF)
	{
	case MB_OKCANCEL:		return IDCANCEL;
	case MB_YESNO:
	case MB_YESNOCANCEL:	return IDNO;
	default:				return IDOK;
	}
}

// FUNCTION: GAME187 0x447720
int SexyAppBase::MsgBox(const std::wstring& theText, const std::wstring& theTitle, int theFlags)
{
	return MsgBox(WStringToString(theText), WStringToString(theTitle), theFlags);
}

// FUNCTION: GAME187 0x447810
void SexyAppBase::Popup(const std::string& theString)
{
	BeginPopup();
	if (!mShutdown)
		SwitchPlatform::ShowError(SexyStringToString(GetString("FATAL_ERROR", _S("FATAL ERROR"))), theString);
	EndPopup();
}

// FUNCTION: GAME187 0x4479d0
void SexyAppBase::Popup(const std::wstring& theString)
{
	Popup(WStringToString(theString));
}

// FUNCTION: GAME187 0x447be0
void SexyAppBase::SafeDeleteWidget(Widget* theWidget)
{
	WidgetSafeDeleteInfo aWidgetSafeDeleteInfo;
	aWidgetSafeDeleteInfo.mUpdateAppDepth = mUpdateAppDepth;
	aWidgetSafeDeleteInfo.mWidget = theWidget;
	mSafeDeleteList.push_back(aWidgetSafeDeleteInfo);
}



// FUNCTION: GAME187 0x447c40

// FUNCTION: GAME187 0x447fa0

// FUNCTION: GAME187 0x447fb0

// FUNCTION: GAME187 0x4480e0

// FUNCTION: GAME187 0x4483e0

// FUNCTION: GAME187 0x4485c0

static DWORD gPowerSaveTick = 0;
// FUNCTION: GAME187 0x448660

// FUNCTION: GAME187 0x448ac0
// Win32 virtual key code for an SDL key, or 0 when the game has no use for it.
static int SDLKeyToKeyCode(SDL_Keycode theKey)
{
	if (theKey >= SDLK_a && theKey <= SDLK_z)
		return 'A' + (theKey - SDLK_a);
	if (theKey >= SDLK_0 && theKey <= SDLK_9)
		return '0' + (theKey - SDLK_0);
	if (theKey >= SDLK_F1 && theKey <= SDLK_F12)
		return KEYCODE_F1 + (theKey - SDLK_F1);

	switch (theKey)
	{
	case SDLK_BACKSPACE:	return KEYCODE_BACK;
	case SDLK_TAB:			return KEYCODE_TAB;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:		return KEYCODE_RETURN;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:		return KEYCODE_SHIFT;
	case SDLK_LCTRL:
	case SDLK_RCTRL:		return KEYCODE_CONTROL;
	case SDLK_LALT:
	case SDLK_RALT:			return KEYCODE_MENU;
	case SDLK_PAUSE:		return 0x13;
	case SDLK_ESCAPE:		return KEYCODE_ESCAPE;
	case SDLK_SPACE:		return KEYCODE_SPACE;
	case SDLK_PAGEUP:		return 0x21;
	case SDLK_PAGEDOWN:		return 0x22;
	case SDLK_END:			return 0x23;
	case SDLK_HOME:			return 0x24;
	case SDLK_LEFT:			return 0x25;
	case SDLK_UP:			return 0x26;
	case SDLK_RIGHT:		return 0x27;
	case SDLK_DOWN:			return 0x28;
	case SDLK_INSERT:		return 0x2D;
	case SDLK_DELETE:		return 0x2E;
	default:				return 0;
	}
}

// Decode one UTF-8 sequence; advances theStr.
static unsigned int DecodeUTF8(const char*& theStr)
{
	unsigned char c = (unsigned char)*theStr++;
	if (c < 0x80)
		return c;

	int aCount = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : 1;
	unsigned int aChar = c & (0x3F >> aCount);
	while (aCount-- > 0 && (*theStr & 0xC0) == 0x80)
		aChar = (aChar << 6) | (*theStr++ & 0x3F);
	return aChar;
}

// Queues a message the way WindowProc did: nothing after WM_CLOSE, at most one pending WM_TIMER.
void SexyAppBase::PushMessage(UINT theMessage, WPARAM wParam, LPARAM lParam)
{
	if (mDeferredMessages.size() > 0)
	{
		MSG* aLast = &mDeferredMessages.back();
		if (aLast->message == WM_CLOSE)
			return;
		if ((theMessage == WM_TIMER) && (aLast->message == WM_TIMER))
			return;
	}

	MSG aMsg;
	ZeroMemory(&aMsg, sizeof(aMsg));
	aMsg.hwnd = mHWnd;
	aMsg.message = theMessage;
	aMsg.wParam = wParam;
	aMsg.lParam = lParam;
	aMsg.time = GetTickCount();
	mDeferredMessages.push_back(aMsg);
}

void SexyAppBase::ProcessSDLEvents()
{
	if (!mAutoInput.empty() && !mShutdown)
		ProcessAutoInput();

	SDL_Event anEvent;
	while (SDL_PollEvent(&anEvent))
	{
		if (mShutdown)
			continue;

		switch (anEvent.type)
		{
		case SDL_QUIT:
			PushMessage(WM_CLOSE, 0, 0);
			CloseRequestAsync();
			break;

		case SDL_WINDOWEVENT:
			if (anEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED || anEvent.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				bool isActive = anEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED;
				if (!mPlayingDemoBuffer)
					mActive = isActive;
				PushMessage(WM_ACTIVATEAPP, isActive ? 1 : 0, 0);
			}
			else if (anEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || anEvent.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				// Docking changes the output size: refit the picture and the touch mapping.
				mDDInterface->UpdatePresentationRect();
				mWidgetManager->Resize(mScreenBounds, mDDInterface->mPresentationRect);
				PushMessage(WM_DISPLAYCHANGE, 0, 0);
			}
			break;

		// Touch arrives here too: SDL turns the first finger into mouse events (which == SDL_TOUCH_MOUSEID).
		case SDL_MOUSEMOTION:
			// A finger only moves while it's down; a hovering mouse always counts.
			if (anEvent.motion.which != SDL_TOUCH_MOUSEID || anEvent.motion.state != 0)
				MovePointer(anEvent.motion.x, anEvent.motion.y, false);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			bool isDown = anEvent.type == SDL_MOUSEBUTTONDOWN;
			bool isTouch = anEvent.button.which == SDL_TOUCH_MOUSEID;
			LPARAM aPos = MAKELONG(anEvent.button.x, anEvent.button.y);
			UINT aMessage;
			if (anEvent.button.button == SDL_BUTTON_RIGHT)
				aMessage = isDown ? ((anEvent.button.clicks == 2) ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN) : WM_RBUTTONUP;
			else if (anEvent.button.button == SDL_BUTTON_MIDDLE)
				aMessage = isDown ? WM_MBUTTONDOWN : WM_MBUTTONUP;
			else
				aMessage = isDown ? ((anEvent.button.clicks == 2) ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN) : WM_LBUTTONUP;

			// A touch lands without any hover first; move the pointer there so the widget under it is "over".
			if (isDown)
				MovePointer(anEvent.button.x, anEvent.button.y, false);
			PushMessage(aMessage, 0, aPos);

			// A lifted finger isn't over anything any more, so hover highlights go away (the next WM_TIMER sees it).
			if (!isDown && isTouch)
				mPointerIn = false;
			break;
		}

		case SDL_JOYDEVICEADDED:
			OpenControllers();
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			HandleControllerButton(anEvent.jbutton.button, anEvent.type == SDL_JOYBUTTONDOWN);
			break;

		case SDL_MOUSEWHEEL:
			PushMessage(WM_MOUSEWHEEL, MAKELONG(0, anEvent.wheel.y * 120), 0);
			break;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			int aKeyCode = SDLKeyToKeyCode(anEvent.key.keysym.sym);
			if (aKeyCode == 0)
				break;

			bool keyDown = anEvent.type == SDL_KEYDOWN;
			if (aKeyCode == VK_CONTROL)
				mCtrlDown = keyDown;
			if (aKeyCode == VK_MENU)
				mAltDown = keyDown;

			if ((keyDown) && (DebugKeyDownAsync(aKeyCode, mCtrlDown, mAltDown)))
				break;

			if (keyDown)
			{
				PushMessage(mAltDown ? WM_SYSKEYDOWN : WM_KEYDOWN, aKeyCode, 0);

				// Windows also sends WM_CHAR for these; text input only reports printable characters.
				if (aKeyCode == KEYCODE_BACK || aKeyCode == KEYCODE_TAB || aKeyCode == KEYCODE_RETURN || aKeyCode == KEYCODE_ESCAPE)
					PushMessage(WM_CHAR, aKeyCode, 0);
			}
			else
				PushMessage(mAltDown ? WM_SYSKEYUP : WM_KEYUP, aKeyCode, 0);
			break;
		}

		case SDL_TEXTINPUT:
		{
			const char* aText = anEvent.text.text;
			while (*aText != 0)
			{
				unsigned int aChar = DecodeUTF8(aText);
				if (aChar >= 0x20 && (sizeof(SexyChar) > 1 || aChar < 0x100))
					PushMessage(WM_CHAR, aChar, 0);
			}
			break;
		}
		}
	}

	DWORD aTimeNow = GetTickCount();
	if (!mShutdown)
		UpdateControllerPointer(aTimeNow - mPadLastTick);
	mPadLastTick = aTimeNow;

	// Stand-in for the 10 ms SetTimer the window used to get.
	static DWORD aLastTimerPush = 0;
	if (aTimeNow - aLastTimerPush >= (DWORD)mFrameTime)
	{
		aLastTimerPush = aTimeNow;
		PushMessage(WM_TIMER, 100, 0);
	}
}

// Moves the pointer (window pixels) and tells the widgets. fromController shows the game's cursor; touch hides it.
void SexyAppBase::MovePointer(int theX, int theY, bool fromController)
{
	mPointerX = theX;
	mPointerY = theY;
	mPointerIn = true;

	// The software cursor is drawn in game pixels.
	int aGameX = theX, aGameY = theY;
	mDDInterface->RemapMouse(aGameX, aGameY);
	mDDInterface->SetCursorPos(aGameX, aGameY);
	if (mPointerVisible != fromController)
	{
		mPointerVisible = fromController;
		EnforceCursor();
	}
	PushMessage(WM_MOUSEMOVE, 0, MAKELONG(theX, theY));
}

// Joystick button numbers on SDL's Switch driver (Nintendo labels).
enum
{
	PAD_A = 0, PAD_B = 1, PAD_X = 2, PAD_Y = 3, PAD_LSTICK = 4, PAD_RSTICK = 5, PAD_L = 6, PAD_R = 7, PAD_ZL = 8,
	PAD_ZR = 9, PAD_PLUS = 10, PAD_MINUS = 11, PAD_LEFT = 12, PAD_UP = 13, PAD_RIGHT = 14, PAD_DOWN = 15
};

void SexyAppBase::OpenControllers()
{
	int aCount = std::min(SDL_NumJoysticks(), (int)(sizeof(mGameControllers) / sizeof(mGameControllers[0])));
	for (int i = 0; i < aCount; i++)
	{
		if (mGameControllers[i] == NULL)
			mGameControllers[i] = SDL_JoystickOpen(i);
	}
}

// A key press the way Windows delivers one: down, the character, up.
void SexyAppBase::PushKeyPress(int theKeyCode, int theChar)
{
	PushMessage(WM_KEYDOWN, theKeyCode, 0);
	if (theChar != 0)
		PushMessage(WM_CHAR, theChar, 0);
	PushMessage(WM_KEYUP, theKeyCode, 0);
}

void SexyAppBase::HandleControllerButton(int theButton, bool isDown)
{
	switch (theButton)
	{
	case PAD_A:
	case PAD_ZR:
	case PAD_R:
	{
		// Click where the controller pointer is; bring it out first if touch had hidden it.
		if (isDown == mPadButtonDown)
			break;
		if (!mPointerVisible)
		{
			mPadPointerX = mPointerX;
			mPadPointerY = mPointerY;
		}
		int aX = (int)mPadPointerX, aY = (int)mPadPointerY;
		if (isDown)
			MovePointer(aX, aY, true);
		PushMessage(isDown ? WM_LBUTTONDOWN : WM_LBUTTONUP, 0, MAKELONG(aX, aY));
		mPadButtonDown = isDown;
		break;
	}

	case PAD_B:
		if (isDown)
			PushKeyPress(KEYCODE_ESCAPE, KEYCODE_ESCAPE);
		break;

	case PAD_PLUS:
		// Any character pauses the board.
		if (isDown)
			PushKeyPress(KEYCODE_SPACE, ' ');
		break;
	}
}

// Left stick and d-pad steer the pointer, faster the further the stick is pushed.
void SexyAppBase::UpdateControllerPointer(DWORD theElapsed)
{
	if (theElapsed > 100)
		theElapsed = 100;

	const int aDeadZone = 6000;
	double aDX = 0, aDY = 0;
	for (int i = 0; i < (int)(sizeof(mGameControllers) / sizeof(mGameControllers[0])); i++)
	{
		SDL_Joystick* aJoy = (SDL_Joystick*)mGameControllers[i];
		if (aJoy == NULL)
			continue;

		for (int anAxis = 0; anAxis < 2; anAxis++)
		{
			int aValue = SDL_JoystickGetAxis(aJoy, anAxis);
			if (abs(aValue) <= aDeadZone)
				continue;
			double aNorm = (abs(aValue) - aDeadZone) / (32767.0 - aDeadZone);
			if (aNorm > 1.0)
				aNorm = 1.0;
			double aSpeed = 0.15 + 0.85 * aNorm * aNorm;
			(anAxis == 0 ? aDX : aDY) += (aValue < 0) ? -aSpeed : aSpeed;
		}

		if (SDL_JoystickGetButton(aJoy, PAD_LEFT))	aDX -= 0.4;
		if (SDL_JoystickGetButton(aJoy, PAD_RIGHT))	aDX += 0.4;
		if (SDL_JoystickGetButton(aJoy, PAD_UP))	aDY -= 0.4;
		if (SDL_JoystickGetButton(aJoy, PAD_DOWN))	aDY += 0.4;
	}

	if ((int)(mAutoStickUntil - GetTickCount()) > 0)
	{
		aDX += mAutoStickX;
		aDY += mAutoStickY;
	}

	if (aDX == 0 && aDY == 0)
		return;

	if (!mPointerVisible)
	{
		// Pick up from wherever the pointer was (or the middle of the screen the first time).
		if (mPointerX < 0)
		{
			Rect& aRect = mDDInterface->mPresentationRect;
			mPointerX = aRect.mX + aRect.mWidth / 2;
			mPointerY = aRect.mY + aRect.mHeight / 2;
		}
		mPadPointerX = mPointerX;
		mPadPointerY = mPointerY;
	}

	// Full deflection crosses the picture in about a second and a half.
	Rect& aRect = mDDInterface->mPresentationRect;
	double aPixelsPerMs = aRect.mWidth / 1500.0;
	mPadPointerX += aDX * aPixelsPerMs * theElapsed;
	mPadPointerY += aDY * aPixelsPerMs * theElapsed;
	mPadPointerX = std::max((double)aRect.mX, std::min(mPadPointerX, (double)(aRect.mX + aRect.mWidth - 1)));
	mPadPointerY = std::max((double)aRect.mY, std::min(mPadPointerY, (double)(aRect.mY + aRect.mHeight - 1)));

	int aX = (int)mPadPointerX, aY = (int)mPadPointerY;
	if (aX != mPointerX || aY != mPointerY || !mPointerVisible || !mPointerIn)
		MovePointer(aX, aY, true);
}

// Scripted input for testing, read from <app data>/autoinput.txt when that file exists. One command per line, each
// after a delay in milliseconds from the previous one; positions are in game pixels (640x480):
//   <ms> tap <x> <y>            touch down and up
//   <ms> down|move|up <x> <y>   touch down, drag, lift
//   <ms> pad <button> down|up   joystick button number (0 = A, 1 = B, 10 = +, ...)
//   <ms> stick <dx> <dy> <ms>   hold the left stick at dx,dy (-1..1) for a while
//   <ms> hint | hintdrag        make the game's hinted move by two taps or by a drag (GetAutoInputMove)
//   <ms> shot                   screenshot to <app data>/_screenshots/
//   <ms> quit
// Lines starting with # are ignored.
void SexyAppBase::LoadAutoInput()
{
	FILE* aFP = fopen((GetAppDataFolder() + "autoinput.txt").c_str(), "r");
	if (aFP == NULL)
		return;

	char aLine[256];
	while (fgets(aLine, sizeof(aLine), aFP) != NULL)
	{
		if (aLine[0] != '#' && aLine[0] != '\r' && aLine[0] != '\n')
			mAutoInput.push_back(aLine);
	}
	fclose(aFP);

	mAutoInputIndex = 0;
	mAutoInputNext = GetTickCount();
	fprintf(stderr, "autoinput: %d commands\n", (int)mAutoInput.size());
}

void SexyAppBase::ProcessAutoInput()
{
	DWORD aNow = GetTickCount();
	while (mAutoInputIndex < mAutoInput.size())
	{
		const char* aLine = mAutoInput[mAutoInputIndex].c_str();
		int aDelay = 0;
		char aCmd[32] = "";
		int aRead = 0;
		if (sscanf(aLine, "%d %31s %n", &aDelay, aCmd, &aRead) < 2)
		{
			mAutoInputIndex++;
			continue;
		}
		if ((int)(aNow - mAutoInputNext) < aDelay)
			break;
		mAutoInputNext += aDelay;
		mAutoInputIndex++;

		const char* anArgs = aLine + aRead;
		Rect& aRect = mDDInterface->mPresentationRect;
		double a1 = 0, a2 = 0, a3 = 0;
		char aWord[16] = "";
		int aNumArgs = sscanf(anArgs, "%lf %lf %lf", &a1, &a2, &a3);
		int aWinX = aRect.mX + (int)(a1 * aRect.mWidth / mWidth);
		int aWinY = aRect.mY + (int)(a2 * aRect.mHeight / mHeight);
		fprintf(stderr, "autoinput @%u: %s", (unsigned)aNow, aLine);

		std::string aCommand = aCmd;
		if (aCommand == "tap" || aCommand == "down" || aCommand == "move" || aCommand == "up")
		{
			if (aNumArgs < 2)
				continue;

			SDL_Event anEvent;
			SDL_zero(anEvent);
			if (aCommand == "move")
			{
				anEvent.type = SDL_MOUSEMOTION;
				anEvent.motion.which = SDL_TOUCH_MOUSEID;
				anEvent.motion.state = SDL_BUTTON_LMASK;
				anEvent.motion.x = aWinX;
				anEvent.motion.y = aWinY;
				SDL_PushEvent(&anEvent);
				continue;
			}

			anEvent.button.which = SDL_TOUCH_MOUSEID;
			anEvent.button.button = SDL_BUTTON_LEFT;
			anEvent.button.clicks = 1;
			anEvent.button.x = aWinX;
			anEvent.button.y = aWinY;
			if (aCommand != "up")
			{
				anEvent.type = SDL_MOUSEBUTTONDOWN;
				anEvent.button.state = SDL_PRESSED;
				SDL_PushEvent(&anEvent);
			}
			if (aCommand != "down")
			{
				anEvent.type = SDL_MOUSEBUTTONUP;
				anEvent.button.state = SDL_RELEASED;
				SDL_PushEvent(&anEvent);
			}
		}
		else if (aCommand == "pad")
		{
			int aButton = 0;
			if (sscanf(anArgs, "%d %15s", &aButton, aWord) == 2)
				HandleControllerButton(aButton, strcmp(aWord, "down") == 0);
		}
		else if (aCommand == "stick")
		{
			mAutoStickX = a1;
			mAutoStickY = a2;
			mAutoStickUntil = aNow + (DWORD)a3;
		}
		else if (aCommand == "hint" || aCommand == "hintdrag")
		{
			// Make the move the game would hint, by tapping both gems or by dragging one onto the other.
			int aX1, aY1, aX2, aY2;
			if (!GetAutoInputMove(aX1, aY1, aX2, aY2))
			{
				fprintf(stderr, "autoinput: no move\n");
				continue;
			}

			char aBuf[4][64];
			std::vector<std::string> aSteps;
			if (aCommand == "hint")
			{
				sprintf(aBuf[0], "0 tap %d %d\n", aX1, aY1);
				sprintf(aBuf[1], "250 tap %d %d\n", aX2, aY2);
				aSteps.assign(aBuf, aBuf + 2);
			}
			else
			{
				sprintf(aBuf[0], "0 down %d %d\n", aX1, aY1);
				sprintf(aBuf[1], "60 move %d %d\n", (aX1 + aX2) / 2, (aY1 + aY2) / 2);
				sprintf(aBuf[2], "60 move %d %d\n", aX2, aY2);
				sprintf(aBuf[3], "60 up %d %d\n", aX2, aY2);
				aSteps.assign(aBuf, aBuf + 4);
			}
			mAutoInput.insert(mAutoInput.begin() + mAutoInputIndex, aSteps.begin(), aSteps.end());
		}
		else if (aCommand == "shot")
			TakeScreenshot();
		else if (aCommand == "quit")
		{
			PushMessage(WM_CLOSE, 0, 0);
			CloseRequestAsync();
		}
	}
}

// FUNCTION: GAME187 0x449430
void SexyAppBase::HandleNotifyGameMessage(int theType, int theParam)
{
}

// FUNCTION: GAME187 0x449490
void SexyAppBase::RehupFocus()
{
	bool wantHasFocus = mActive && !mMinimized;

	if (wantHasFocus != mHasFocus)
	{
		mHasFocus = wantHasFocus;

		if (mHasFocus)
		{
			if (mMuteOnLostFocus)
				Unmute(true);

			mWidgetManager->GotFocus();
			GotFocus();
		}
		else
		{
			if (mMuteOnLostFocus)
				Mute(true);

			mWidgetManager->LostFocus();
			LostFocus();

			ReleaseCapture();
			mWidgetManager->DoMouseUps();
		}
	}
}

// FUNCTION: GAME187 0x449530
void SexyAppBase::ClearKeysDown()
{
	if (mWidgetManager != NULL) // fix stuck alt-key problem
	{
		for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++)
			mWidgetManager->mKeyDown[aKeyNum] = false;
	}
	mCtrlDown = false;
	mAltDown = false;
}

// FUNCTION: GAME187 0x449570
void SexyAppBase::WriteDemoTimingBlock()
{
	// Demo writing functions can only be called from the main thread and after SexyAppBase::Init
	DBG_ASSERTE(GetCurrentThreadId() == mPrimaryThreadId);

	while (mUpdateCount - mLastDemoUpdateCnt > 15)
	{
		mDemoBuffer.WriteNumBits(15, 4);
		mLastDemoUpdateCnt += 15;

		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_IDLE, 5);
		mDemoCmdOrder++;
	}

	mDemoBuffer.WriteNumBits(mUpdateCount - mLastDemoUpdateCnt, 4);
	mLastDemoUpdateCnt = mUpdateCount;
	mDemoCmdOrder++;
}

int aNumBigMoveMessages = 0;
int aNumSmallMoveMessages = 0;
int aNumTimerMessages = 0;

// FUNCTION: GAME187 0x449600
bool SexyAppBase::PrepareDemoCommand(bool required)
{
	if (mDemoNeedsCommand)
	{
		mDemoCmdBitPos = mDemoBuffer.mReadBitPos;

		mLastDemoUpdateCnt += mDemoBuffer.ReadNumBits(4, false);

		mDemoIsShortCmd = mDemoBuffer.ReadNumBits(1, false) == 1;

		if (mDemoIsShortCmd)
			mDemoCmdNum = mDemoBuffer.ReadNumBits(1, false);
		else
			mDemoCmdNum = mDemoBuffer.ReadNumBits(5, false);

		mDemoNeedsCommand = false;

		mDemoCmdOrder++;
	}

	DBG_ASSERTE((mUpdateCount == mLastDemoUpdateCnt) || (!required));

	return mUpdateCount == mLastDemoUpdateCnt;
}

// FUNCTION: GAME187 0x449680
void SexyAppBase::ProcessDemo()
{
	if (mPlayingDemoBuffer)
	{
		// At end of demo buffer?  How dare you!
		DBG_ASSERTE(!mDemoBuffer.AtEnd());

		while ((!mShutdown) && (mUpdateCount == mLastDemoUpdateCnt) && (!mDemoBuffer.AtEnd()))
		{
			if (PrepareDemoCommand(false))
			{
				mDemoNeedsCommand = true;

				if (mDemoIsShortCmd)
				{
					switch (mDemoCmdNum)
					{
					case 0:
						{
							int aDeltaX = mDemoBuffer.ReadNumBits(6, true);
							int aDeltaY = mDemoBuffer.ReadNumBits(6, true);
							mLastDemoMouseX += aDeltaX;
							mLastDemoMouseY += aDeltaY;

							mWidgetManager->MouseMove(mLastDemoMouseX, mLastDemoMouseY);
						}
						break;
					case 1:
						{
							bool down = mDemoBuffer.ReadNumBits(1, false) != 0;
							int aBtnCount = mDemoBuffer.ReadNumBits(3, true);

							if (down)
								mWidgetManager->MouseDown(mLastDemoMouseX, mLastDemoMouseY, aBtnCount);
							else
								mWidgetManager->MouseUp(mLastDemoMouseX, mLastDemoMouseY, aBtnCount);
						}
						break;
					}
				}
				else
				{
					switch (mDemoCmdNum)
					{
					case DEMO_MOUSE_POSITION:
						{
							mLastDemoMouseX = mDemoBuffer.ReadNumBits(12, false);
							mLastDemoMouseY = mDemoBuffer.ReadNumBits(12, false);

							mWidgetManager->MouseMove(mLastDemoMouseX, mLastDemoMouseY);
						}
						break;
					case DEMO_ACTIVATE_APP:
						{
							mActive = mDemoBuffer.ReadNumBits(1, false) != 0;

							RehupFocus();

							if ((mActive) && (!mIsWindowed))
								mWidgetManager->MarkAllDirty();

							if ((mIsOpeningURL) && (!mActive))
								URLOpenSucceeded(mOpeningURL);
						}
						break;
					case DEMO_SIZE:
						{
							bool isMinimized = mDemoBuffer.ReadBoolean();

							if ((!mShutdown) && (isMinimized != mMinimized))
							{
								mMinimized = isMinimized;

								// We don't want any sounds (or music) playing while its minimized
								if (mMinimized)
									Mute(true);
								else
								{
									Unmute(true);
									mWidgetManager->MarkAllDirty();
								}
							}

							RehupFocus();
						}
						break;
					case DEMO_MOUSE_WHEEL:
						{
							int aScroll = mDemoBuffer.ReadNumBits(8, true);
							mWidgetManager->MouseWheel(aScroll);
						}
						break;
					case DEMO_KEY_DOWN:
						{
							KeyCode aKeyCode = (KeyCode) mDemoBuffer.ReadNumBits(8, false);
							mWidgetManager->KeyDown(aKeyCode);
						}
						break;
					case DEMO_KEY_UP:
						{
							KeyCode aKeyCode = (KeyCode) mDemoBuffer.ReadNumBits(8, false);
							mWidgetManager->KeyUp(aKeyCode);
						}
						break;
					case DEMO_KEY_CHAR:
						{
							int sizeMult = (int)mDemoBuffer.ReadNumBits(1, false) + 1; // will be 1 for single, 2 for double
							SexyChar aChar = (SexyChar) mDemoBuffer.ReadNumBits(8*sizeMult, false);
							mWidgetManager->KeyChar(aChar);
						}
						break;
					case DEMO_CLOSE:
						Shutdown();
						break;
					case DEMO_MOUSE_ENTER:
						mMouseIn = true;
						EnforceCursor();
						break;
					case DEMO_MOUSE_EXIT:
						mWidgetManager->MouseExit(mLastDemoMouseX, mLastDemoMouseY);
						mMouseIn = false;
						EnforceCursor();
						break;
					case DEMO_LOADING_COMPLETE:
						mDemoLoadingComplete = true;
						break;
					case DEMO_VIDEO_DATA:
						mIsWindowed = mDemoBuffer.ReadBoolean();
						mSyncRefreshRate = mDemoBuffer.ReadByte();
						break;
					case DEMO_IDLE:
						break;
					default:
						DBG_ASSERTE("Invalid Demo Command" == 0);
						break;
					}
				}
			}
		}
	}
}

// FUNCTION: GAME187 0x4499f0
void SexyAppBase::ShowMemoryUsage()
{
	std::string aStr;
	aStr += StrFormat("Num Images: %d\r\n", (int)mMemoryImageSet.size());
	aStr += StrFormat("Num Sounds: %d\r\n", mSoundManager->GetNumSounds());

	MsgBox(aStr, "Video Stats", MB_OK);
	mLastTime = timeGetTime();
}

// FUNCTION: GAME187 0x44a390
bool SexyAppBase::IsAltKeyUsed(WPARAM wParam)
{
	int aChar = tolower(wParam);
	switch (aChar)
	{
		case 13: // alt-enter
		case 'r':
			return true;
		default:
			return false;
	}
}

// FUNCTION: GAME187 0x44a3c0
bool SexyAppBase::DebugKeyDown(int theKey)
{
	if ((theKey == 'R') && (mWidgetManager->mKeyDown[KEYCODE_MENU]))
	{
#if 0 // not in the shipped 1.87: Alt+R mod-value reparse
		if (ReparseModValues())
		else
		{
			for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++) // prevent alt from getting stuck
				mWidgetManager->mKeyDown[aKeyNum] = false;
		}
#endif
	}
	else if (theKey == VK_F3)
	{
		if(mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			mShowFPS = true;
			if (++mShowFPSMode >= Num_FPS_Types)
				mShowFPSMode = 0;
		}
		else
			mShowFPS = !mShowFPS;

		mWidgetManager->MarkAllDirty();

		if (mShowFPS)
		{
			gFPSTimer.Start();
			gFrameCount = 0;
			gFPSDisplay = 0;
			gForceDisplay = true;
		}
	}
	else if (theKey == VK_F8)
	{
		if(mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			Set3DAcclerated(!Is3DAccelerated());

			char aBuf[512];
			sprintf(aBuf,"3D-Mode: %s",Is3DAccelerated()?"ON":"OFF");
			MsgBox(aBuf,"Mode Switch",MB_OK);
			mLastTime = timeGetTime();
		}
		else
			ShowMemoryUsage();

		return true;
	}
	else if (theKey == VK_F10)
	{
#if 0 // not in the shipped 1.87: Ctrl+F10 slow motion, Shift+F10 / F10 step mode
		if (mWidgetManager->mKeyDown[KEYCODE_CONTROL])
		{
			if (mUpdateMultiplier==0.25)
				mUpdateMultiplier = 1.0;
			else
				mUpdateMultiplier = 0.25;
		}
		else if(mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			mStepMode = 0;
			ClearUpdateBacklog();
		}
		else
			mStepMode = 1;
#endif

		return true;
	}
	else if (theKey == VK_F11)
	{
		if (mWidgetManager->mKeyDown[KEYCODE_SHIFT])
			DumpProgramInfo();
		else
			TakeScreenshot();

		return true;
	}
	else if (theKey == VK_F2)
	{
		bool isPerfOn = !SexyPerf::IsPerfOn();
		if (isPerfOn)
		{
//			MsgBox("Perf Monitoring: ON", "Perf Monitoring", MB_OK);
			ClearUpdateBacklog();
			SexyPerf::BeginPerf();
		}
		else
		{
			SexyPerf::EndPerf();
			MsgBox(SexyPerf::GetResults().c_str(), "Perf Results", MB_OK);
			ClearUpdateBacklog();
		}
	}
	else
		return false;

	return false;
}

// FUNCTION: GAME187 0x44a670
bool SexyAppBase::DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown)
{
	return false;
}

void SexyAppBase::CloseRequestAsync()
{
}

// Why did I defer messages?  Oh, incase a dialog comes up such as a crash
//  it won't keep crashing and stuff
// FUNCTION: GAME187 0x44a680
bool SexyAppBase::ProcessDeferredMessages(bool singleMessage)
{
	while (mDeferredMessages.size() > 0)
	{
		MSG aMsg = mDeferredMessages.front();
		mDeferredMessages.pop_front();

		UINT uMsg = aMsg.message;
		LPARAM lParam = aMsg.lParam;
		WPARAM wParam = aMsg.wParam;
		HWND hWnd = aMsg.hwnd;

		if ((mRecordingDemoBuffer) && (!mShutdown))
		{
			switch (uMsg)
			{
//  TODO: switch to killfocus/setfocus?
//			case WM_SETFOCUS:
//			case WM_KILLFOCUS:
//				if (hWnd == mHWnd)
//				{
//					WriteDemoTimingBlock();
//					mDemoBuffer.WriteNumBits(0, 1);
//					mDemoBuffer.WriteNumBits(DEMO_ACTIVATE_APP, 5);
//					mDemoBuffer.WriteNumBits(uMsg==WM_SETFOCUS ? 1 : 0, 1);
//				}
//				break;

			case WM_ACTIVATEAPP:
				if (hWnd == mHWnd)
				{
					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_ACTIVATE_APP, 5);
					mDemoBuffer.WriteNumBits((wParam != 0) ? 1 : 0, 1);
				}
				break;

			case WM_SIZE:
				{
					bool isMinimized = wParam == SIZE_MINIMIZED;

					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_SIZE, 5);
					mDemoBuffer.WriteBoolean(isMinimized);
				}
				break;
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDBLCLK:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_MOUSEMOVE:
				{
					int aCurX = (short) LOWORD(lParam);
					int aCurY = (short) HIWORD(lParam);

					int aDiffX = aCurX - mLastDemoMouseX;
					int aDiffY = aCurY - mLastDemoMouseY;

					if ((abs(aCurX - mLastDemoMouseX) < 32) && (abs(aCurY - mLastDemoMouseY) < 32))
					{
						if ((aDiffX != 0) || (aDiffY != 0))
						{
							WriteDemoTimingBlock();
							mDemoBuffer.WriteNumBits(1, 1);
							mDemoBuffer.WriteNumBits(0, 1);
							mDemoBuffer.WriteNumBits(aDiffX, 6);
							mDemoBuffer.WriteNumBits(aDiffY, 6);
						}
					}
					else
					{
						WriteDemoTimingBlock();
						mDemoBuffer.WriteNumBits(0, 1);
						mDemoBuffer.WriteNumBits(DEMO_MOUSE_POSITION, 5);
						mDemoBuffer.WriteNumBits(aCurX, 12);
						mDemoBuffer.WriteNumBits(aCurY, 12);
					}

					bool down = true;
					int aBtnNum = 0;
					switch (uMsg)
					{
						case WM_LBUTTONDOWN:
							aBtnNum = 1;
							break;
						case WM_RBUTTONDOWN:
							aBtnNum = -1;
							break;
						case WM_MBUTTONDOWN:
							aBtnNum = 3;
							break;
						case WM_LBUTTONDBLCLK:
							aBtnNum = 2;
							break;
						case WM_RBUTTONDBLCLK:
							aBtnNum = -2;
							break;
						case WM_LBUTTONUP:
							aBtnNum = 1;
							down = false;
							break;
						case WM_RBUTTONUP:
							aBtnNum = -1;
							down = false;
							break;
						case WM_MBUTTONUP:
							aBtnNum = 3;
							down = false;
							break;
					}

					if (aBtnNum != 0)
					{
						WriteDemoTimingBlock();
						mDemoBuffer.WriteNumBits(1, 1);
						mDemoBuffer.WriteNumBits(1, 1);
						mDemoBuffer.WriteNumBits(down ? 1 : 0, 1);
						mDemoBuffer.WriteNumBits(aBtnNum, 3);
					}

					mLastDemoMouseX = aCurX;
					mLastDemoMouseY = aCurY;
				}
				break;
			case WM_MOUSEWHEEL:
				{
					int aZDelta = ((short)HIWORD(wParam)) / 120;

					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_MOUSE_WHEEL, 5);
					mDemoBuffer.WriteNumBits(aZDelta, 8);
				}
				break;
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				{
					KeyCode aKeyCode = (KeyCode) wParam;

					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_KEY_DOWN, 5);
					mDemoBuffer.WriteNumBits(aKeyCode, 8);
				}
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				{
					KeyCode aKeyCode = (KeyCode) wParam;

					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_KEY_UP, 5);
					mDemoBuffer.WriteNumBits((int) aKeyCode, 8);
				}
				break;
			case WM_CHAR:
				{
					SexyChar aChar = (SexyChar) wParam;

					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_KEY_CHAR, 5);
					mDemoBuffer.WriteNumBits(sizeof(SexyChar) == 2, 1);
					mDemoBuffer.WriteNumBits(aChar, sizeof(SexyChar)*8);
				}
				break;
			case WM_CLOSE:
				if ((hWnd == mHWnd) || (hWnd == mInvisHWnd))
				{
					WriteDemoTimingBlock();
					mDemoBuffer.WriteNumBits(0, 1);
					mDemoBuffer.WriteNumBits(DEMO_CLOSE, 5);
				}
				break;
			}

			int aBufferSize = mDemoBuffer.GetDataLen();
		}

		if (!mPlayingDemoBuffer)
		{
			switch (uMsg)
			{
//  TODO: switch to killfocus/setfocus?
//			case WM_KILLFOCUS:
//			case WM_SETFOCUS:
			case WM_ACTIVATEAPP:
				if ((hWnd == mHWnd) && (!gInAssert) && (!mSEHOccured) && (!mShutdown))
				{
//					mActive = uMsg==WM_SETFOCUS;

					RehupFocus();

					if ((mActive) && (!mIsWindowed))
						mWidgetManager->MarkAllDirty();

					if ((mIsOpeningURL) && (!mActive))
						URLOpenSucceeded(mOpeningURL);
				}
				break;
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDBLCLK:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_MOUSEMOVE:
				if ((!gInAssert) && (!mSEHOccured))
				{
					int x = (short) LOWORD(lParam);
					int y = (short) HIWORD(lParam);
					mWidgetManager->RemapMouse(x, y);

					mLastUserInputTick = mLastTimerTime;

					mWidgetManager->MouseMove(x, y);

					if (!mMouseIn)
					{
						if (mRecordingDemoBuffer)
						{
							WriteDemoTimingBlock();
							mDemoBuffer.WriteNumBits(0, 1);
							mDemoBuffer.WriteNumBits(DEMO_MOUSE_ENTER, 5);
						}

						mMouseIn = true;
						EnforceCursor();
					}

					switch (uMsg)
					{
					case WM_LBUTTONDOWN:
						SetCapture(hWnd);
						mWidgetManager->MouseDown(x, y, 1);
						break;
					case WM_RBUTTONDOWN:
						SetCapture(hWnd);
						mWidgetManager->MouseDown(x, y, -1);
						break;
					case WM_MBUTTONDOWN:
						SetCapture(hWnd);
						mWidgetManager->MouseDown(x, y, 3);
						break;
					case WM_LBUTTONDBLCLK:
						SetCapture(hWnd);
						mWidgetManager->MouseDown(x, y, 2);
						break;
					case WM_RBUTTONDBLCLK:
						SetCapture(hWnd);
						mWidgetManager->MouseDown(x, y, -2);
						break;
					case WM_LBUTTONUP:
						if ((mWidgetManager->mDownButtons & ~1) == 0)
							ReleaseCapture();
						mWidgetManager->MouseUp(x, y, 1);
						break;
					case WM_RBUTTONUP:
						if ((mWidgetManager->mDownButtons & ~2) == 0)
							ReleaseCapture();
						mWidgetManager->MouseUp(x, y, -1);
						break;
					case WM_MBUTTONUP:
						if ((mWidgetManager->mDownButtons & ~4) == 0)
							ReleaseCapture();
						mWidgetManager->MouseUp(x, y, 3);
						break;
					}
				}
				break;
			case WM_MOUSEWHEEL:
				{
					char aZDelta = ((short)HIWORD(wParam)) / 120;
					mWidgetManager->MouseWheel(aZDelta);
				}
				break;
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				mLastUserInputTick = mLastTimerTime;

				if (wParam==VK_RETURN && uMsg==WM_SYSKEYDOWN && !mForceFullscreen && !mForceWindowed)
				{
					SwitchScreenMode(!mIsWindowed);
					ClearKeysDown();
					break;
				}
				else if ((wParam == 'D') && (mWidgetManager != NULL) && (mWidgetManager->mKeyDown[KEYCODE_CONTROL]) && (mWidgetManager->mKeyDown[KEYCODE_MENU]))
				{
					mDebugKeysEnabled = !mDebugKeysEnabled;
				}

				if (mDebugKeysEnabled)
				{
					if (DebugKeyDown(wParam))
						break;
				}

				mWidgetManager->KeyDown((KeyCode) wParam);
				break;

			case WM_KEYUP:
			case WM_SYSKEYUP:
				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->KeyUp((KeyCode) wParam);
				break;
			case WM_CHAR:
				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->KeyChar((SexyChar) wParam);
				break;
			case WM_SIZE:
				{
					bool isMinimized = wParam == SIZE_MINIMIZED;

					if ((hWnd == mHWnd) && (!mShutdown) && (isMinimized != mMinimized))
					{
						mMinimized = isMinimized;

						// We don't want any sounds (or music) playing while its minimized
						if (mMinimized)
						{
							if (mMuteOnLostFocus)
								Mute(true);
						}
						else
						{
							if (mMuteOnLostFocus)
								Unmute(true);

							mWidgetManager->MarkAllDirty();
						}
					}

					RehupFocus();
					if (wParam==SIZE_MAXIMIZED)
						SwitchScreenMode(false);
				}
				break;
			case WM_TIMER:
				if ((!gInAssert) && (!mSEHOccured) && (mRunning))
				{
					DWORD aTimeNow = GetTickCount();
					if (aTimeNow - mLastTimerTime > 500)
						mLastBigDelayTime = aTimeNow;

					mLastTimerTime = aTimeNow;

					if ((mIsOpeningURL) &&
						(aTimeNow - mLastBigDelayTime > 5000))
					{
						if ((aTimeNow - mOpeningURLTime > 8000) && (!mActive))
						{
							//TODO: Have some demo message thing
							URLOpenSucceeded(mOpeningURL);
						}
						else if ((aTimeNow - mOpeningURLTime > 12000) && (mActive))
						{
							URLOpenFailed(mOpeningURL);
						}
					}

					// The pointer is "in" while a finger is down, or while a mouse or the controller pointer is out.
					int aPointX = mPointerX;
					int aPointY = mPointerY;
					bool isMouseIn = mPointerIn;

					if (mMouseIn != isMouseIn)
					{
						if ((mRecordingDemoBuffer) && (!mShutdown))
						{
							WriteDemoTimingBlock();
							mDemoBuffer.WriteNumBits(0, 1);

							if (isMouseIn)
								mDemoBuffer.WriteNumBits(DEMO_MOUSE_ENTER, 5);
							else
								mDemoBuffer.WriteNumBits(DEMO_MOUSE_EXIT, 5);
						}

						if (!isMouseIn)
						{
							int x = aPointX;
							int y = aPointY;
							mWidgetManager->RemapMouse(x, y);
							mWidgetManager->MouseExit(x, y);
						}

						mMouseIn = isMouseIn;
						EnforceCursor();
					}
				}
				break;

			case WM_SYSCOLORCHANGE:
			case WM_DISPLAYCHANGE:
				mWidgetManager->SysColorChangedAll();
				mWidgetManager->MarkAllDirty();
				break;
			}
		}

		switch (uMsg)
		{
		case WM_CLOSE:
			if ((hWnd == mHWnd) || (hWnd == mInvisHWnd))
			{
				// This should short-circuit all demo calls, otherwise we will get
				//  all sorts of weird asserts because we are changing
				//  program flow
				mManualShutdown = true;

				Shutdown();
			}
			break;
		}

		if (singleMessage)
			break;
	}

	return (mDeferredMessages.size() > 0);
}

void SexyAppBase::Done3dTesting()
{
}

// return file name that you want to upload
// FUNCTION: GAME187 0x44b280
std::string	SexyAppBase::NotifyCrashHook()
{
	return "";
}

// FUNCTION: GAME187 0x44b2b0
void SexyAppBase::MakeWindow()
{
	if (mHWnd == NULL)
	{
		SDL_Window* aWindow = SDL_CreateWindow(SexyStringToStringFast(mTitle).c_str(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_FULLSCREEN);
		if (aWindow == NULL)
		{
			Popup(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
			DoExit(1);
		}
		mHWnd = (HWND)aWindow;
	}

	// Always one fullscreen window; mIsWindowed is only remembered for the options dialog.
	mIsPhysWindowed = false;

	if (mDDInterface == NULL)
		mDDInterface = new DDInterface(this);

	int aResult = InitDDInterface();
	if (aResult != DDInterface::RESULT_OK)
	{
		Popup(GetString("FAILED_INIT_DIRECTDRAW", _S("Failed to initialize DirectDraw: ")) + StringToSexyString(DDInterface::ResultToString(aResult) + " " + mDDInterface->mErrorString));
		DoExit(1);
	}

	bool isActive = mActive;
	mActive = true;

	mPhysMinimized = false;
	if (mMinimized)
	{
		if (mMuteOnLostFocus)
			Unmute(true);

		mMinimized = false;
		isActive = mActive; // set this here so we don't call RehupFocus again.
		RehupFocus();
	}

	if (isActive != mActive)
		RehupFocus();

	ReInitImages();

	mWidgetManager->mImage = mDDInterface->GetScreenImage();
	mWidgetManager->MarkAllDirty();
}

// FUNCTION: GAME187 0x44ba60
void SexyAppBase::DeleteNativeImageData()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		aMemoryImage->DeleteNativeData();
		++anItr;
	}
}

// FUNCTION: GAME187 0x44bae0
void SexyAppBase::DeleteExtraImageData()
{
	AutoCrit anAutoCrit(mDDInterface->mCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		aMemoryImage->DeleteExtraBuffers();
		++anItr;
	}
}

// FUNCTION: GAME187 0x44bbb0
void SexyAppBase::ReInitImages()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		aMemoryImage->ReInit();
		++anItr;
	}
}


void SexyAppBase::LoadingThreadProc()
{
}

void SexyAppBase::LoadingThreadCompleted()
{
}

// FUNCTION: GAME187 0x44bc30
void SexyAppBase::LoadingThreadProcStub(void *theArg)
{
	SexyAppBase* aSexyApp = (SexyAppBase*) theArg;

	aSexyApp->LoadingThreadProc();

	char aStr[256];
	sprintf(aStr, "Resource Loading Time: %d\r\n", (GetTickCount() - aSexyApp->mTimeLoaded));
	OutputDebugStringA(aStr);

	aSexyApp->mLoadingThreadCompleted = true;
}

// FUNCTION: GAME187 0x44bca0
void SexyAppBase::StartLoadingThread()
{
	if (!mLoadingThreadStarted)
	{
		mYieldMainThread = true;
		mLoadingThreadStarted = true;

		// On another core, or the title screen wouldn't draw until loading is done. Image decoding (libpng/libjpeg)
		// wants more stack than a default thread gets.
		if (!SwitchPlatform::StartWorkerThread(LoadingThreadProcStub, this, 1024 * 1024))
			LoadingThreadProcStub(this);
	}
}
// FUNCTION: GAME187 0x44bce0
void SexyAppBase::CursorThreadProc()
{
	// The custom cursor is drawn with each frame; there is no hardware cursor to chase.
	mCursorThreadRunning = false;
}

// FUNCTION: GAME187 0x44be00
void SexyAppBase::CursorThreadProcStub(void *theArg)
{
	SexyAppBase* aSexyApp = (SexyAppBase*) theArg;
	aSexyApp->CursorThreadProc();
}

// FUNCTION: GAME187 0x44be20
void SexyAppBase::StartCursorThread()
{
}

// FUNCTION: GAME187 0x44be50
void SexyAppBase::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	// There is only the one fullscreen mode; keep the choice so the options dialog shows it.
	mIsWindowed = wantWindowed;
	Set3DAcclerated(is3d);

	if (force)
	{
		MakeWindow();
		mLastTime = timeGetTime();
	}
}

// FUNCTION: GAME187 0x44bf10
void SexyAppBase::SwitchScreenMode(bool wantWindowed)
{
	SwitchScreenMode(wantWindowed, Is3DAccelerated());
}

// FUNCTION: GAME187 0x44bf40
void SexyAppBase::SwitchScreenMode()
{
	SwitchScreenMode(mIsWindowed, Is3DAccelerated(), true);
}

// FUNCTION: GAME187 0x44bf70
void SexyAppBase::SetAlphaDisabled(bool isDisabled)
{
	if (mAlphaDisabled != isDisabled)
	{
		mAlphaDisabled = isDisabled;
		mDDInterface->SetVideoOnlyDraw(mAlphaDisabled);
		mWidgetManager->mImage = mDDInterface->GetScreenImage();
		mWidgetManager->MarkAllDirty();
	}
}

// FUNCTION: GAME187 0x44bfc0
void SexyAppBase::EnforceCursor()
{
	if (mDDInterface == NULL)
		return;

	// Only the game's own cursor images are ever drawn, and only while a pointer is over the screen.
	Image* aCursorImage = NULL;
	if ((!mSEHOccured) && (mMouseIn) && (mCursorImages[mCursorNum] != NULL) &&
		((mPlayingDemoBuffer) || (mPointerVisible) || (mCursorNum == CURSOR_CUSTOM)))
		aCursorImage = mCursorImages[mCursorNum];

	if (mDDInterface->SetCursorImage(aCursorImage))
		mCustomCursorDirty = true;

	mSysCursor = aCursorImage == NULL;
}

// FUNCTION: GAME187 0x44c1d0
void SexyAppBase::ProcessSafeDeleteList()
{
	MTAutoDisallowRand aDisallowRand;

	WidgetSafeDeleteList::iterator anItr = mSafeDeleteList.begin();
	while (anItr != mSafeDeleteList.end())
	{
		WidgetSafeDeleteInfo* aWidgetSafeDeleteInfo = &(*anItr);
		if (mUpdateAppDepth <= aWidgetSafeDeleteInfo->mUpdateAppDepth)
		{
			delete aWidgetSafeDeleteInfo->mWidget;
			anItr = mSafeDeleteList.erase(anItr);
		}
		else
			++anItr;
	}
}

// FUNCTION: GAME187 0x44c2d0
void SexyAppBase::UpdateFTimeAcc()
{
	DWORD aCurTime = timeGetTime();

	if (mLastTimeCheck != 0)
	{
		int aDeltaTime = aCurTime - mLastTimeCheck;

		mUpdateFTimeAcc = min(mUpdateFTimeAcc + aDeltaTime, 200.0);

		if (mRelaxUpdateBacklogCount > 0)
			mRelaxUpdateBacklogCount = max(mRelaxUpdateBacklogCount - aDeltaTime, 0);
	}

	mLastTimeCheck = aCurTime;
}

//int aNumCalls = 0;
//DWORD aLastCheck = 0;

// FUNCTION: GAME187 0x44c340
bool SexyAppBase::Process(bool allowSleep)
{
	/*DWORD aTimeNow = GetTickCount();
	if (aTimeNow - aLastCheck >= 10000)
	{
		OutputDebugString(StrFormat(_S("FUpdates: %d\n"), aNumCalls).c_str());
		aLastCheck = aTimeNow;
		aNumCalls = 0;
	}*/

	if (mLoadingFailed)
		Shutdown();

	bool isVSynched = (!mPlayingDemoBuffer) && (mVSyncUpdates) && (!mLastDrawWasEmpty) && (!mVSyncBroken) &&
		((!mIsPhysWindowed) || (mIsPhysWindowed && mWaitForVSync && !mSoftVSyncWait));
	double aFrameFTime;
	double anUpdatesPerUpdateF;

	if (mVSyncUpdates)
	{
		aFrameFTime = (1000.0 / mSyncRefreshRate) / mUpdateMultiplier;
		anUpdatesPerUpdateF = (float) (1000.0 / (mFrameTime * mSyncRefreshRate));
	}
	else
	{
		aFrameFTime = mFrameTime / mUpdateMultiplier;
		anUpdatesPerUpdateF = 1.0;
	}

	// Do we need to fast forward?
	if (mPlayingDemoBuffer)
	{
		if (mUpdateCount < mFastForwardToUpdateNum || mFastForwardToMarker)
		{
			if (!mDemoMute && !mFastForwardStep)
			{
				mDemoMute = true;
				Mute(true);
			}

			static DWORD aTick = GetTickCount();
			while (mUpdateCount < mFastForwardToUpdateNum || mFastForwardToMarker)
			{
				ClearUpdateBacklog();
				int aLastUpdateCount = mUpdateCount;

				// Actual updating code below
				//////////////////////////////////////////////////////////////////////////

				bool hadRealUpdate = DoUpdateFrames();

				if (hadRealUpdate)
				{
					mPendingUpdatesAcc += anUpdatesPerUpdateF;
					mPendingUpdatesAcc -= 1.0;
					ProcessSafeDeleteList();

					// Process any extra updates
					while (mPendingUpdatesAcc >= 1.0)
					{
						// These should just be IDLE commands we have to clear out
						ProcessDemo();

						bool hasRealUpdate = DoUpdateFrames();
						DBG_ASSERTE(hasRealUpdate);

						if (!hasRealUpdate)
							break;

						ProcessSafeDeleteList();
						mPendingUpdatesAcc -= 1.0;
					}

					DoUpdateFramesF((float) anUpdatesPerUpdateF);
					ProcessSafeDeleteList();
				}

				//////////////////////////////////////////////////////////////////////////

				// If the update count doesn't change, its because we are
				//  playing back a demo and need to read more
				if (aLastUpdateCount == mUpdateCount)
					return true;

				DWORD aNewTick = GetTickCount();
				if (aNewTick - aTick >= 1000 || mFastForwardStep) // let the app draw some
				{
					mFastForwardStep = false;
					aTick = GetTickCount();
					DrawDirtyStuff();
					return true;
				}
			}
		}

		if (mDemoMute)
		{
			mDemoMute = false;
			mSoundManager->StopAllSounds();
			Unmute(true);
		}
	}

	// Make sure we're not paused
	if ((!mPaused) && (mUpdateMultiplier > 0))
	{
		uint32_t aStartTime = timeGetTime();

		uint32_t aCurTime = aStartTime;
		int aCumSleepTime = 0;

		// When we are VSynching, only calculate this FTimeAcc right after drawing

		if (!isVSynched)
			UpdateFTimeAcc();

		// mNonDrawCount is used to make sure we draw the screen at least
		// 10 times per second, even if it means we have to slow down
		// the updates to make it draw 10 times per second in "game time"

		bool didUpdate = false;

		if (mUpdateAppState == UPDATESTATE_PROCESS_1)
		{
			if ((++mNonDrawCount < (int) ceil(10*mUpdateMultiplier)) || (!mLoaded))
			{
				bool doUpdate = false;

				if (isVSynched)
				{
					// Synch'ed to vertical refresh, so update as soon as possible after draw
					doUpdate = (!mHasPendingDraw) || (mUpdateFTimeAcc >= (int) (aFrameFTime * 0.75));
				}
				else if (mUpdateFTimeAcc >= aFrameFTime)
				{
					doUpdate = true;
				}

				if (doUpdate)
				{
					// Do VSyncBroken test.  This test fails if we're in fullscreen and
					// "don't vsync" has been forced in Advanced settings up Display Properties
					if ((!mPlayingDemoBuffer) && (mUpdateMultiplier == 1.0))
					{
						mVSyncBrokenTestUpdates++;
						if (mVSyncBrokenTestUpdates >= (DWORD) ((1000+mFrameTime-1)/mFrameTime))
						{
							// It has to be running 33% fast to be "broken" (25% = 1/0.800)
							if (aStartTime - mVSyncBrokenTestStartTick <= 800)
							{
								// The test has to fail 3 times in a row before we decide that
								//  vsync is broken overall
								mVSyncBrokenCount++;
								if (mVSyncBrokenCount >= 3)
									mVSyncBroken = true;
							}
							else
								mVSyncBrokenCount = 0;

							mVSyncBrokenTestStartTick = aStartTime;
							mVSyncBrokenTestUpdates = 0;
						}
					}

					bool hadRealUpdate = DoUpdateFrames();
					if (hadRealUpdate)
						mUpdateAppState = UPDATESTATE_PROCESS_2;

					mHasPendingDraw = true;
					didUpdate = true;
				}
			}
		}
		else if (mUpdateAppState == UPDATESTATE_PROCESS_2)
		{
			mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mPendingUpdatesAcc += anUpdatesPerUpdateF;
			mPendingUpdatesAcc -= 1.0;
			ProcessSafeDeleteList();

			// Process any extra updates
			while (mPendingUpdatesAcc >= 1.0)
			{
				// These should just be IDLE commands we have to clear out
				ProcessDemo();

				++mNonDrawCount;
				bool hasRealUpdate = DoUpdateFrames();
				DBG_ASSERTE(hasRealUpdate);

				if (!hasRealUpdate)
					break;

				ProcessSafeDeleteList();
				mPendingUpdatesAcc -= 1.0;
			}

			//aNumCalls++;
			DoUpdateFramesF((float) anUpdatesPerUpdateF);
			ProcessSafeDeleteList();

			// Don't let mUpdateFTimeAcc dip below 0
			//  Subtract an extra 0.2ms, because sometimes refresh rates have some
			//  fractional component that gets truncated, and it's better to take off
			//  too much to keep our timing tending toward occuring right after
			//  redraws
			if (isVSynched)
				mUpdateFTimeAcc = max(mUpdateFTimeAcc - aFrameFTime - 0.2f, 0.0);
			else
				mUpdateFTimeAcc -= aFrameFTime;

			if (mRelaxUpdateBacklogCount > 0)
				mUpdateFTimeAcc = 0;

			didUpdate = true;
		}

		if (!didUpdate)
		{
			mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mNonDrawCount = 0;

			if (mHasPendingDraw)
			{
				DrawDirtyStuff();
			}
			else
			{
				// Let us take into account the time it took to draw dirty stuff
				int aTimeToNextFrame = (int) (aFrameFTime - mUpdateFTimeAcc);
				if (aTimeToNextFrame > 0)
				{
					if (!allowSleep)
						return false;

					// Wait till next processing cycle
					++mSleepCount;
					Sleep(aTimeToNextFrame);

					aCumSleepTime += aTimeToNextFrame;
				}
			}
		}

		if (mYieldMainThread)
		{
			// This is to make sure that the title screen doesn't take up any more than
			// 1/3 of the processor time

			uint32_t anEndTime = timeGetTime();
			int anElapsedTime = (anEndTime - aStartTime) - aCumSleepTime;
			int aLoadingYieldSleepTime = min(250, (anElapsedTime * 2) - aCumSleepTime);

			if (aLoadingYieldSleepTime >= 0)
			{
				if (!allowSleep)
					return false;

				Sleep(aLoadingYieldSleepTime);
			}
		}
	}

	ProcessSafeDeleteList();
	return true;
}

/*void SexyAppBase::DoMainLoop()
{
	Dialog* aDialog = NULL;
	if (theModalDialogId != -1)
	{
		aDialog = GetDialog(theModalDialogId);
		DBG_ASSERTE(aDialog != NULL);
		if (aDialog == NULL)
			return;
	}

	while (!mShutdown)
	{
		MSG msg;
		while ((PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) && (!mShutdown))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		ProcessDemo();
		ProcessDeferredMessages();

		if ((aDialog != NULL) && (aDialog->mResult != -1))
			return;

		if (!mShutdown)
		{
			//++aCount;
			Process();
		}
	}
}*/

// FUNCTION: GAME187 0x44c8c0
void SexyAppBase::DoMainLoop()
{
	while (!mShutdown)
	{
		if (mExitToTop)
			mExitToTop = false;
		UpdateApp();
	}
}

// FUNCTION: GAME187 0x44c900
bool SexyAppBase::UpdateAppStep(bool* updated)
{
	if (updated != NULL)
		*updated = false;

	if (mExitToTop)
		return false;

	if (mUpdateAppState == UPDATESTATE_PROCESS_DONE)
		mUpdateAppState = UPDATESTATE_MESSAGES;

	mUpdateAppDepth++;

	// We update in two stages to avoid doing a Process if our loop termination
	//  condition has already been met by processing windows messages
	if (mUpdateAppState == UPDATESTATE_MESSAGES)
	{
		ProcessSDLEvents();

		ProcessDemo();
		if (!ProcessDeferredMessages(true))
		{
			mUpdateAppState = UPDATESTATE_PROCESS_1;
		}
	}
	else
	{
		// Process changes state by itself
		if (mStepMode)
		{
			if (mStepMode==2)
			{
				Sleep(mFrameTime);
				mUpdateAppState = UPDATESTATE_PROCESS_DONE; // skip actual update until next step
			}
			else
			{
				mStepMode = 2;
				DoUpdateFrames();
				DoUpdateFramesF(1.0f);
				DrawDirtyStuff();
			}
		}
		else
		{
			int anOldUpdateCnt = mUpdateCount;
			Process();
			if (updated != NULL)
				*updated = mUpdateCount != anOldUpdateCnt;
		}
	}

	mUpdateAppDepth--;

	return true;
}

// FUNCTION: GAME187 0x44ca80
bool SexyAppBase::UpdateApp()
{
	bool updated;
	for (;;)
	{
		if (!UpdateAppStep(&updated))
			return false;
		if (updated)
			return true;
	}
}

// FUNCTION: GAME187 0x44cad0
int SexyAppBase::InitDDInterface()
{
	PreDDInterfaceInitHook();
	DeleteNativeImageData();
	int aResult = mDDInterface->Init(mHWnd, mIsPhysWindowed);
	DemoSyncRefreshRate();
	if ( DDInterface::RESULT_OK == aResult )
	{
		mScreenBounds.mX = ( mWidth - mDDInterface->mWidth ) / 2;
		mScreenBounds.mY = ( mHeight - mDDInterface->mHeight ) / 2;
		mScreenBounds.mWidth = mDDInterface->mWidth;
		mScreenBounds.mHeight = mDDInterface->mHeight;
		mWidgetManager->Resize(mScreenBounds, mDDInterface->mPresentationRect);
		PostDDInterfaceInitHook();
	}
	return aResult;
}

void SexyAppBase::PreTerminate()
{
}

// FUNCTION: GAME187 0x44cb80
void SexyAppBase::Start()
{
	if (mShutdown)
		return;

	StartCursorThread();

	if (mAutoStartLoadingThread)
		StartLoadingThread();


	int aCount = 0;
	int aSleepCount = 0;

	DWORD aStartTime = timeGetTime();

	mRunning = true;
	mLastTime = aStartTime;
	mLastUserInputTick = aStartTime;
	mLastTimerTime = aStartTime;

	DoMainLoop();
	ProcessSafeDeleteList();

	mRunning = false;

	WaitForLoadingThread();

	char aString[256];
	sprintf(aString, "Seconds       = %g\r\n", (timeGetTime() - aStartTime) / 1000.0);
	OutputDebugStringA(aString);
	//sprintf(aString, "Count         = %d\r\n", aCount);
	//OutputDebugString(aString);
	sprintf(aString, "Sleep Count   = %d\r\n", mSleepCount);
	OutputDebugStringA(aString);
	sprintf(aString, "Update Count  = %d\r\n", mUpdateCount);
	OutputDebugStringA(aString);
	sprintf(aString, "Draw Count    = %d\r\n", mDrawCount);
	OutputDebugStringA(aString);
	sprintf(aString, "Draw Time     = %d\r\n", mDrawTime);
	OutputDebugStringA(aString);
	sprintf(aString, "Screen Blt    = %d\r\n", mScreenBltTime);
	OutputDebugStringA(aString);
	if (mDrawTime+mScreenBltTime > 0)
	{
		sprintf(aString, "Avg FPS       = %d\r\n", (mDrawCount*1000)/(mDrawTime+mScreenBltTime));
		OutputDebugStringA(aString);
	}

	PreTerminate();

	WriteToRegistry();
}

// FUNCTION: GAME187 0x492f30
bool SexyAppBase::CheckSignature(const Buffer& theBuffer, const std::string& theFileName)
{
	// Add your own signature checking code here
	return false;
}

// FUNCTION: GAME187 0x44cd70
bool SexyAppBase::LoadProperties(const std::string& theFileName, bool required, bool checkSig)
{
	Buffer aBuffer;
	if (!ReadBufferFromFile(theFileName, &aBuffer))
	{
		if (!required)
			return true;
		else
		{
			Popup(GetString("UNABLE_OPEN_PROPERTIES", _S("Unable to open properties file ")) + StringToSexyString(theFileName));
			return false;
		}
	}
	if (checkSig)
	{
		if (!CheckSignature(aBuffer, theFileName))
		{
			Popup(GetString("PROPERTIES_SIG_FAILED", _S("Signature check failed on ")) + StringToSexyString(theFileName + "'"));
			return false;
		}
	}

	PropertiesParser aPropertiesParser(this);

	// Load required language-file properties
		if (!aPropertiesParser.ParsePropertiesBuffer(aBuffer))
		{
			Popup(aPropertiesParser.GetErrorText());
			return false;
		}
		else
			return true;
}

// FUNCTION: GAME187 0x44d1c0
bool SexyAppBase::LoadProperties()
{
	// Load required language-file properties
	return LoadProperties("properties/default.xml", true, false);
}

void SexyAppBase::LoadResourceManifest()
{
	if (!mResourceManager->ParseResourcesFile("properties/resources.xml"))
		ShowResourceError(true);
}

void SexyAppBase::ShowResourceError(bool doExit)
{
	Popup(mResourceManager->GetErrorText());
	if (doExit)
		DoExit(0);
}

bool SexyAppBase::GetBoolean(const std::string& theId)
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);
	DBG_ASSERTE(anItr != mBoolProperties.end());

	if (anItr != mBoolProperties.end())
		return anItr->second;
	else
		return false;
}

// FUNCTION: GAME187 0x44d250
bool SexyAppBase::GetBoolean(const std::string& theId, bool theDefault)
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);

	if (anItr != mBoolProperties.end())
		return anItr->second;
	else
		return theDefault;
}

int SexyAppBase::GetInteger(const std::string& theId)
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);
	DBG_ASSERTE(anItr != mIntProperties.end());

	if (anItr != mIntProperties.end())
		return anItr->second;
	else
		return false;
}

// FUNCTION: GAME187 0x44d2c0
int SexyAppBase::GetInteger(const std::string& theId, int theDefault)
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);

	if (anItr != mIntProperties.end())
		return anItr->second;
	else
		return theDefault;
}

double SexyAppBase::GetDouble(const std::string& theId)
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);
	DBG_ASSERTE(anItr != mDoubleProperties.end());

	if (anItr != mDoubleProperties.end())
		return anItr->second;
	else
		return false;
}

double SexyAppBase::GetDouble(const std::string& theId, double theDefault)
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);

	if (anItr != mDoubleProperties.end())
		return anItr->second;
	else
		return theDefault;
}

// FUNCTION: GAME187 0x44d330
SexyString SexyAppBase::GetString(const std::string& theId)
{
	StringWStringMap::iterator anItr = mStringProperties.find(theId);
	DBG_ASSERTE(anItr != mStringProperties.end());

	if (anItr != mStringProperties.end())
		return WStringToSexyString(anItr->second);
	else
		return _S("");
}

// FUNCTION: GAME187 0x44d3d0
SexyString SexyAppBase::GetString(const std::string& theId, const SexyString& theDefault)
{
	StringWStringMap::iterator anItr = mStringProperties.find(theId);

	if (anItr != mStringProperties.end())
		return WStringToSexyString(anItr->second);
	else
		return theDefault;
}

StringVector SexyAppBase::GetStringVector(const std::string& theId)
{
	StringStringVectorMap::iterator anItr = mStringVectorProperties.find(theId);
	DBG_ASSERTE(anItr != mStringVectorProperties.end());

	if (anItr != mStringVectorProperties.end())
		return anItr->second;
	else
		return StringVector();
}

// FUNCTION: GAME187 0x44d470
void SexyAppBase::SetString(const std::string& theId, const std::wstring& theValue)
{
	std::pair<StringWStringMap::iterator, bool> aPair = mStringProperties.insert(StringWStringMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}


// FUNCTION: GAME187 0x44d5b0
void SexyAppBase::SetBoolean(const std::string& theId, bool theValue)
{
	std::pair<StringBoolMap::iterator, bool> aPair = mBoolProperties.insert(StringBoolMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

// FUNCTION: GAME187 0x44d670
void SexyAppBase::SetInteger(const std::string& theId, int theValue)
{
	std::pair<StringIntMap::iterator, bool> aPair = mIntProperties.insert(StringIntMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

// FUNCTION: GAME187 0x44d760
void SexyAppBase::SetDouble(const std::string& theId, double theValue)
{
	std::pair<StringDoubleMap::iterator, bool> aPair = mDoubleProperties.insert(StringDoubleMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

// FUNCTION: GAME187 0x44d820
void SexyAppBase::DoParseCmdLine()
{
	// hbmenu passes no arguments worth parsing.
	mCmdLineParsed = true;
}

// FUNCTION: GAME187 0x44d8d0
void SexyAppBase::ParseCmdLine(const std::string& theCmdLine)
{
	// Command line example:  -play -demofile="game demo.dmo"
	// Results in HandleCmdLineParam("-play", ""); HandleCmdLineParam("-demofile", "game demo.dmo");
	std::string aCurParamName;
	std::string aCurParamValue;

	int aSpacePos = 0;
	bool inQuote = false;
	bool onValue = false;

	for (int i = 0; i < (int) theCmdLine.length(); i++)
	{
		char c = theCmdLine[i];
		bool atEnd = false;

		if (c == '"')
		{
			inQuote = !inQuote;

			if (!inQuote)
				atEnd = true;
		}
		else if ((c == ' ') && (!inQuote))
			atEnd = true;
		else if (c == '=')
			onValue = true;
		else if (onValue)
			aCurParamValue += c;
		else
			aCurParamName += c;

		if (i == theCmdLine.length() - 1)
			atEnd = true;

		if (atEnd && !aCurParamName.empty())
		{
			HandleCmdLineParam(aCurParamName, aCurParamValue);
			aCurParamName = "";
			aCurParamValue = "";
			onValue = false;
		}
	}
}

// FUNCTION: GAME187 0x44da80
static int GetMaxDemoFileNum(const std::string& theDemoPrefix, int theMaxToKeep, bool doErase)
{
	std::string aDir = GetFileDir(theDemoPrefix);
	std::string aPrefix = GetFileName(theDemoPrefix);
	if (aDir.empty())
		aDir = ".";

	typedef std::set<int> IntSet;
	IntSet aSet;

	DIR* aDirHandle = opendir(aDir.c_str());
	if (aDirHandle == NULL)
		return 0;
	struct dirent* anEntry;
	while ((anEntry = readdir(aDirHandle)) != NULL)
	{
		int aNum = 0;
		if (sscanf(anEntry->d_name, (aPrefix + "%d.dmo").c_str(), &aNum) == 1)
			aSet.insert(aNum);
	}
	closedir(aDirHandle);

	IntSet::iterator anItr = aSet.begin();
	if ((int)aSet.size()>theMaxToKeep-1 && doErase)
		unlink(StrFormat((theDemoPrefix + "%d.dmo").c_str(),*anItr).c_str());

	if (aSet.empty())
		return 0;

	anItr = aSet.end();
	--anItr;
	return (*anItr);
}

// FUNCTION: GAME187 0x44dd80
void SexyAppBase::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-play")
	{
		mPlayingDemoBuffer = true;
		mRecordingDemoBuffer = false;
	}
	else if (theParamName == "-recnum")
	{
		int aNum = atoi(theParamValue.c_str());
		if (aNum<=0)
			aNum=5;

		int aDemoFileNum = GetMaxDemoFileNum(mDemoPrefix, aNum, true) + 1;
		mDemoFileName = SexyStringToString(StrFormat(StringToSexyString(mDemoPrefix + "%d.dmo").c_str(),aDemoFileNum));
		if (mDemoFileName.length() < 2 || (mDemoFileName[1] != ':' && mDemoFileName[2] != '\\'))
		{
			mDemoFileName = GetAppDataFolder() + mDemoFileName;
		}
		mRecordingDemoBuffer = true;
		mPlayingDemoBuffer = false;
	}
	else if (theParamName == "-playnum")
	{
		int aNum = atoi(theParamValue.c_str())-1;
		if (aNum<0)
			aNum=0;

		int aDemoFileNum = GetMaxDemoFileNum(mDemoPrefix, aNum, false)-aNum;
		mDemoFileName = SexyStringToString(StrFormat(StringToSexyString(mDemoPrefix + "%d.dmo").c_str(),aDemoFileNum));
		mRecordingDemoBuffer = false;
		mPlayingDemoBuffer = true;
	}
	else if (theParamName == "-record")
	{
		mRecordingDemoBuffer = true;
		mPlayingDemoBuffer = false;
	}
	else if (theParamName == "-demofile")
	{
		mDemoFileName = theParamValue;
		if (mDemoFileName.length() < 2 || (mDemoFileName[1] != ':' && mDemoFileName[2] != '\\'))
		{
			mDemoFileName = GetAppDataFolder() + mDemoFileName;
		}
	}
	else if (theParamName == "-crash")
	{
		// Try to access NULL
		char* a = 0;
		*a = '!';
	}
	else if (theParamName == "-screensaver")
	{
		mIsScreenSaver = true;
	}
	else if (theParamName == "-changedir")
	{
		mChangeDirTo = theParamValue;
	}
	else
	{
		Popup(GetString("INVALID_COMMANDLINE_PARAM", _S("Invalid command line parameter: ")) + StringToSexyString(theParamName));
		DoExit(0);
	}
}

void SexyAppBase::PreDisplayHook()
{
}

void SexyAppBase::PreDDInterfaceInitHook()
{
}

void SexyAppBase::PostDDInterfaceInitHook()
{
}

// FUNCTION: GAME187 0x44e3d0
bool SexyAppBase::ChangeDirHook(const char *theIntendedPath)
{
	return false;
}

// FUNCTION: GAME187 0x44e3e0
MusicInterface* SexyAppBase::CreateMusicInterface(HWND theWindow)
{
	return new MusicInterface;
}

void SexyAppBase::InitPropertiesHook()
{
}

// FUNCTION: GAME187 0x4bfb70
void SexyAppBase::InitHook()
{
}

// FUNCTION: GAME187 0x44e4d0
void SexyAppBase::Init()
{
	mPrimaryThreadId = GetCurrentThreadId();

	if (mShutdown)
		return;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) != 0)
	{
		Popup(std::string("SDL_Init failed: ") + SDL_GetError());
		DoExit(1);
	}

	InitPropertiesHook();
	ReadFromRegistry();

	// Saves, screenshots and demos go on the SD card next to the .nro.
	SetAppDataFolder("sdmc:/switch/Bejeweled/");
	MkDir(GetAppDataFolder());
	if (mDemoFileName.find(':') == std::string::npos)
		mDemoFileName = GetAppDataFolder() + mDemoFileName;

	if (!mCmdLineParsed)
		DoParseCmdLine();

	if (IsScreenSaver())
		mOnlyAllowOneCopyToRun = false;

	mRandSeed = GetTickCount();
	SRand(mRandSeed);

	// Set up demo recording stuff
	if (mPlayingDemoBuffer)
	{
		std::string anError;
		if (!ReadDemoBuffer(anError))
		{
			mPlayingDemoBuffer = false;
			Popup(anError);
			DoExit(0);
		}
	}

	srand(GetTickCount());

	// Let app do something before showing window, or switching to fullscreen mode
	// PreDisplayHook must call mWidgetManager->Resize if it changes mWidth or mHeight.
	PreDisplayHook();

	mWidgetManager->Resize(Rect(0, 0, mWidth, mHeight), Rect(0, 0, mWidth, mHeight));

	MakeWindow();

	if (mPlayingDemoBuffer)
	{
		// Get video data

		PrepareDemoCommand(true);
		mDemoNeedsCommand = true;

		DBG_ASSERTE(!mDemoIsShortCmd);
		DBG_ASSERTE(mDemoCmdNum == DEMO_VIDEO_DATA);

		mIsWindowed = mDemoBuffer.ReadBoolean();
		mSyncRefreshRate = mDemoBuffer.ReadByte();
	}

	if (mSoundManager == NULL)
		mSoundManager = new NullSoundManager();

	SetSfxVolume(mSfxVolume);

	mMusicInterface = CreateMusicInterface(mInvisHWnd);

	SetMusicVolume(mMusicVolume);

	if (IsScreenSaver())
	{
		SetCursor(CURSOR_NONE);
	}

	InitHook();

	LoadAutoInput();

	mInitialized = true;
}

// FUNCTION: GAME187 0x44f000
void SexyAppBase::HandleGameAlreadyRunning()
{
}

// FUNCTION: GAME187 0x44f030
// No system clipboard; copy/paste works within the game.
static std::string gClipboard;

void SexyAppBase::CopyToClipboard(const std::string& theString)
{
	if (mPlayingDemoBuffer)
		return;

	gClipboard = theString;
}

// FUNCTION: GAME187 0x44f140
std::string	SexyAppBase::GetClipboard()
{
	std::string aString;
	if (!mPlayingDemoBuffer)
		aString = gClipboard;

	DemoSyncString(&aString);

	return aString;
}

// FUNCTION: GAME187 0x44f200
void SexyAppBase::SetCursor(int theCursorNum)
{
	mCursorNum = theCursorNum;
	EnforceCursor();
}

int SexyAppBase::GetCursor()
{
	return mCursorNum;
}

// FUNCTION: GAME187 0x44f210
void SexyAppBase::EnableCustomCursors(bool enabled)
{
	mCustomCursorsEnabled = enabled;
	EnforceCursor();
}

// FUNCTION: GAME187 0x44f230
Sexy::DDImage* SexyAppBase::GetImage(const std::string& theFileName, bool commitBits)
{
	ImageLib::Image* aLoadedImage = ImageLib::GetImage(theFileName, true);

	if (aLoadedImage == NULL)
		return NULL;

	DDImage* anImage = new DDImage(mDDInterface);
	anImage->mFilePath = theFileName;
	anImage->SetBits(aLoadedImage->GetBits(), aLoadedImage->GetWidth(), aLoadedImage->GetHeight(), commitBits);
	anImage->mFilePath = theFileName;
	delete aLoadedImage;

	return anImage;
}

// FUNCTION: GAME187 0x44f310
Sexy::DDImage* SexyAppBase::CreateCrossfadeImage(Sexy::Image* theImage1, const Rect& theRect1, Sexy::Image* theImage2, const Rect& theRect2, double theFadeFactor)
{
	MemoryImage* aMemoryImage1 = dynamic_cast<MemoryImage*>(theImage1);
	MemoryImage* aMemoryImage2 = dynamic_cast<MemoryImage*>(theImage2);

	if ((aMemoryImage1 == NULL) || (aMemoryImage2 == NULL))
		return NULL;

	if ((theRect1.mX < 0) || (theRect1.mY < 0) ||
		(theRect1.mX + theRect1.mWidth > theImage1->GetWidth()) ||
		(theRect1.mY + theRect1.mHeight > theImage1->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect1 out of bounds");
		return NULL;
	}

	if ((theRect2.mX < 0) || (theRect2.mY < 0) ||
		(theRect2.mX + theRect2.mWidth > theImage2->GetWidth()) ||
		(theRect2.mY + theRect2.mHeight > theImage2->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect2 out of bounds");
		return NULL;
	}

	int aWidth = theRect1.mWidth;
	int aHeight = theRect1.mHeight;

	DDImage* anImage = new DDImage(mDDInterface);
	anImage->Create(aWidth, aHeight);

	uint32_t* aDestBits = anImage->GetBits();
	uint32_t* aSrcBits1 = aMemoryImage1->GetBits();
	uint32_t* aSrcBits2 = aMemoryImage2->GetBits();

	int aSrc1Width = aMemoryImage1->GetWidth();
	int aSrc2Width = aMemoryImage2->GetWidth();
	uint32_t aMult = (int) (theFadeFactor*256);
	uint32_t aOMM = (256 - aMult);

	for (int y = 0; y < aHeight; y++)
	{
		uint32_t* s1 = &aSrcBits1[(y+theRect1.mY)*aSrc1Width+theRect1.mX];
		uint32_t* s2 = &aSrcBits2[(y+theRect2.mY)*aSrc2Width+theRect2.mX];
		uint32_t* d = &aDestBits[y*aWidth];

		for (int x = 0; x < aWidth; x++)
		{
			uint32_t p1 = *s1++;
			uint32_t p2 = *s2++;

			//p1 = 0;
			//p2 = 0xFFFFFFFF;

			*d++ =
				((((p1 & 0x000000FF)*aOMM + (p2 & 0x000000FF)*aMult)>>8) & 0x000000FF) |
				((((p1 & 0x0000FF00)*aOMM + (p2 & 0x0000FF00)*aMult)>>8) & 0x0000FF00) |
				((((p1 & 0x00FF0000)*aOMM + (p2 & 0x00FF0000)*aMult)>>8) & 0x00FF0000) |
				((((p1 >> 24)*aOMM + (p2 >> 24)*aMult)<<16) & 0xFF000000);
		}
	}

	anImage->BitsChanged();

	return anImage;
}

void SexyAppBase::ColorizeImage(Image* theImage, const Color& theColor)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	if (aSrcMemoryImage == NULL)
		return;

	uint32_t* aBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == NULL)
	{
		aBits = aSrcMemoryImage->GetBits();
		aNumColors = theImage->GetWidth()*theImage->GetHeight();
	}
	else
	{
		aBits = aSrcMemoryImage->mColorTable;
		aNumColors = 256;
	}

	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) &&
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			uint32_t aColor = aBits[i];

			aBits[i] =
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			uint32_t aColor = aBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}

	aSrcMemoryImage->BitsChanged();
}

// FUNCTION: GAME187 0x44f640
DDImage* SexyAppBase::CreateColorizedImage(Image* theImage, const Color& theColor)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	if (aSrcMemoryImage == NULL)
		return NULL;

	DDImage* anImage = new DDImage(mDDInterface);

	anImage->Create(theImage->GetWidth(), theImage->GetHeight());

	uint32_t* aSrcBits;
	uint32_t* aDestBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == NULL)
	{
		aSrcBits = aSrcMemoryImage->GetBits();
		aDestBits = anImage->GetBits();
		aNumColors = theImage->GetWidth()*theImage->GetHeight();
	}
	else
	{
		aSrcBits = aSrcMemoryImage->mColorTable;
		aDestBits = anImage->mColorTable = new uint32_t[256];
		aNumColors = 256;

		anImage->mColorIndices = new uchar[anImage->mWidth*theImage->mHeight];
		memcpy(anImage->mColorIndices, aSrcMemoryImage->mColorIndices, anImage->mWidth*theImage->mHeight);
	}

	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) &&
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			uint32_t aColor = aSrcBits[i];

			aDestBits[i] =
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			uint32_t aColor = aSrcBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aDestBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}

	anImage->BitsChanged();

	return anImage;
}

// FUNCTION: GAME187 0x44f8f0
DDImage* SexyAppBase::CopyImage(Image* theImage, const Rect& theRect)
{
	DDImage* anImage = new DDImage(mDDInterface);

	anImage->Create(theRect.mWidth, theRect.mHeight);

	Graphics g(anImage);
	g.DrawImage(theImage, -theRect.mX, -theRect.mY);

	anImage->CopyAttributes(theImage);

	return anImage;
}

// FUNCTION: GAME187 0x44f9d0
DDImage* SexyAppBase::CopyImage(Image* theImage)
{
	return CopyImage(theImage, Rect(0, 0, theImage->GetWidth(), theImage->GetHeight()));
}

void SexyAppBase::MirrorImage(Image* theImage)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	uint32_t* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int y = 0; y < aSrcMemoryImage->mHeight; y++)
	{
		uint32_t* aLeftBits = aSrcBits + (y * aPhysSrcWidth);
		uint32_t* aRightBits = aLeftBits + (aPhysSrcWidth - 1);

		for (int x = 0; x < (aPhysSrcWidth >> 1); x++)
		{
			uint32_t aSwap = *aLeftBits;

			*(aLeftBits++) = *aRightBits;
			*(aRightBits--) = aSwap;
		}
	}

	aSrcMemoryImage->BitsChanged();
}

void SexyAppBase::FlipImage(Image* theImage)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	uint32_t* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcHeight = aSrcMemoryImage->mHeight;
	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int x = 0; x < aPhysSrcWidth; x++)
	{
		uint32_t* aTopBits    = aSrcBits + x;
		uint32_t* aBottomBits = aTopBits + (aPhysSrcWidth * (aPhysSrcHeight - 1));

		for (int y = 0; y < (aPhysSrcHeight >> 1); y++)
		{
			uint32_t aSwap = *aTopBits;

			*aTopBits = *aBottomBits;
			aTopBits += aPhysSrcWidth;
			*aBottomBits = aSwap;
			aBottomBits -= aPhysSrcWidth;
		}
	}

	aSrcMemoryImage->BitsChanged();
}

void SexyAppBase::RotateImageHue(Sexy::MemoryImage *theImage, int theDelta)
{
	while (theDelta < 0)
		theDelta += 256;

	int aSize = theImage->mWidth * theImage->mHeight;
	DWORD *aPtr = theImage->GetBits();
	for (int i=0; i<aSize; i++)
	{
		DWORD aPixel = *aPtr;
		int alpha = aPixel&0xff000000;
		int r = (aPixel>>16)&0xff;
		int g = (aPixel>>8) &0xff;
		int b = aPixel&0xff;

		int maxval = max(r, max(g, b));
		int minval = min(r, min(g, b));
		int h = 0;
		int s = 0;
		int l = (minval+maxval)/2;
		int delta = maxval - minval;

		if (delta != 0)
		{
			s = (delta * 256) / ((l <= 128) ? (minval + maxval) : (512 - maxval - minval));

			if (r == maxval)
				h = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
			else if (g == maxval)
				h = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
			else
				h = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));

			h /= 6;
		}

		h += theDelta;
		if (h >= 256)
			h -= 256;

		double v= (l < 128) ? (l * (255+s))/255 :
				(l+s-l*s/255);

		int y = (int) (2*l-v);

		int aColorDiv = (6 * h) / 256;
		int x = (int)(y+(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
		if (x > 255)
			x = 255;

		int z = (int) (v-(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
		if (z < 0)
			z = 0;

		switch (aColorDiv)
		{
			case 0: r = (int) v; g = x; b = y; break;
			case 1: r = z; g= (int) v; b = y; break;
			case 2: r = y; g= (int) v; b = x; break;
			case 3: r = y; g = z; b = (int) v; break;
			case 4: r = x; g = y; b = (int) v; break;
			case 5: r = (int) v; g = y; b = z; break;
			default: r = (int) v; g = x; b = y; break;
		}

		*aPtr++ = alpha | (r<<16) | (g << 8) | (b);

	}

	theImage->BitsChanged();
}

// FUNCTION: GAME187 0x44fa10
uint32_t SexyAppBase::HSLToRGB(int h, int s, int l)
{
	int r;
	int g;
	int b;

	double v= (l < 128) ? (l * (255+s))/255 :
			(l+s-l*s/255);

	int y = (int) (2*l-v);

	int aColorDiv = (6 * h) / 256;
	int x = (int)(y+(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
	if (x > 255)
		x = 255;

	int z = (int) (v-(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
	if (z < 0)
		z = 0;

	switch (aColorDiv)
	{
		case 0: r = (int) v; g = x; b = y; break;
		case 1: r = z; g= (int) v; b = y; break;
		case 2: r = y; g= (int) v; b = x; break;
		case 3: r = y; g = z; b = (int) v; break;
		case 4: r = x; g = y; b = (int) v; break;
		case 5: r = (int) v; g = y; b = z; break;
		default: r = (int) v; g = x; b = y; break;
	}

	return 0xFF000000 | (r << 16) | (g << 8) | (b);
}

uint32_t SexyAppBase::RGBToHSL(int r, int g, int b)
{
	int maxval = max(r, max(g, b));
	int minval = min(r, min(g, b));
	int hue = 0;
	int saturation = 0;
	int luminosity = (minval+maxval)/2;
	int delta = maxval - minval;

	if (delta != 0)
	{
		saturation = (delta * 256) / ((luminosity <= 128) ? (minval + maxval) : (512 - maxval - minval));

		if (r == maxval)
			hue = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
		else if (g == maxval)
			hue = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
		else
			hue = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));

		hue /= 6;
	}

	return 0xFF000000 | (hue) | (saturation << 8) | (luminosity << 16);
}

void SexyAppBase::HSLToRGB(const uint32_t* theSource, uint32_t* theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		uint32_t src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (HSLToRGB((src & 0xFF), (src >> 8) & 0xFF, (src >> 16) & 0xFF) & 0x00FFFFFF);
	}
}

void SexyAppBase::RGBToHSL(const uint32_t* theSource, uint32_t* theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		uint32_t src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (RGBToHSL(((src >> 16) & 0xFF), (src >> 8) & 0xFF, (src & 0xFF)) & 0x00FFFFFF);
	}
}

// FUNCTION: GAME187 0x44fb90
void SexyAppBase::PrecacheAdditive(MemoryImage* theImage)
{
	theImage->GetRLAdditiveData(mDDInterface);
}

// FUNCTION: GAME187 0x44fbb0
void SexyAppBase::PrecacheAlpha(MemoryImage* theImage)
{
	theImage->GetRLAlphaData();
}

void SexyAppBase::PrecacheNative(MemoryImage* theImage)
{
	theImage->GetNativeAlphaData(mDDInterface);
}


// FUNCTION: GAME187 0x44fbc0
void SexyAppBase::PlaySample(int theSoundNum)
{
	if (!mSoundManager)
		return;

	SoundInstance* aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != NULL)
	{
		aSoundInstance->Play(false, true);
	}
}


// FUNCTION: GAME187 0x44fbf0
void SexyAppBase::PlaySample(int theSoundNum, int thePan)
{
	if (!mSoundManager)
		return;

	SoundInstance* aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != NULL)
	{
		aSoundInstance->SetPan(thePan);
		aSoundInstance->Play(false, true);
	}
}

// FUNCTION: GAME187 0x44fc40
bool SexyAppBase::IsMuted()
{
	return mMuteCount > 0;
}

// FUNCTION: GAME187 0x44fc50
void SexyAppBase::Mute(bool autoMute)
{
	mMuteCount++;
	if (autoMute)
		mAutoMuteCount++;

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}

// FUNCTION: GAME187 0x44fca0
void SexyAppBase::Unmute(bool autoMute)
{
	if (mMuteCount > 0)
	{
		mMuteCount--;
		if (autoMute)
			mAutoMuteCount--;
	}

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}


// FUNCTION: GAME187 0x44fd00
double SexyAppBase::GetMusicVolume()
{
	return mMusicVolume;
}

// FUNCTION: GAME187 0x44fd10
void SexyAppBase::SetMusicVolume(double theVolume)
{
	mMusicVolume = theVolume;

	if (mMusicInterface != NULL)
		mMusicInterface->SetVolume((mMuteCount > 0) ? 0.0 : mMusicVolume);
}

// FUNCTION: GAME187 0x44fd50
double SexyAppBase::GetSfxVolume()
{
	return mSfxVolume;
}

// FUNCTION: GAME187 0x44fd60
void SexyAppBase::SetSfxVolume(double theVolume)
{
	mSfxVolume = theVolume;

	if (mSoundManager != NULL)
		mSoundManager->SetVolume((mMuteCount > 0) ? 0.0 : mSfxVolume);
}

// FUNCTION: GAME187 0x44fda0
double SexyAppBase::GetMasterVolume()
{
	return mSoundManager->GetMasterVolume();
}

// FUNCTION: GAME187 0x44fdb0
void SexyAppBase::SetMasterVolume(double theMasterVolume)
{
	mSfxVolume = theMasterVolume;
	mSoundManager->SetMasterVolume(mSfxVolume);
}

// FUNCTION: GAME187 0x44fdd0
void SexyAppBase::AddMemoryImage(MemoryImage* theMemoryImage)
{
	AutoCrit anAutoCrit(mDDInterface->mCritSect);
	mMemoryImageSet.insert(theMemoryImage);
}

// FUNCTION: GAME187 0x44fe50
void SexyAppBase::RemoveMemoryImage(MemoryImage* theMemoryImage)
{
	AutoCrit anAutoCrit(mDDInterface->mCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.find(theMemoryImage);
	if (anItr != mMemoryImageSet.end())
		mMemoryImageSet.erase(anItr);

	Remove3DData(theMemoryImage);
}

// FUNCTION: GAME187 0x44ff10
void SexyAppBase::Remove3DData(MemoryImage* theMemoryImage)
{
	if (mDDInterface)
		mDDInterface->Remove3DData(theMemoryImage);
}


// FUNCTION: GAME187 0x44ff30
bool SexyAppBase::Is3DAccelerated()
{
	return mDDInterface->mIs3D;
}

// FUNCTION: GAME187 0x44ff40
bool SexyAppBase::Is3DAccelerationSupported()
{
	return false;
}

// FUNCTION: GAME187 0x44ff60
bool SexyAppBase::Is3DAccelerationRecommended()
{
	return false;
}

// FUNCTION: GAME187 0x44ff90
void SexyAppBase::DemoSyncRefreshRate()
{
	mSyncRefreshRate = mDDInterface->mRefreshRate;

	if (mRecordingDemoBuffer)
	{
		WriteDemoTimingBlock();
		mDemoBuffer.WriteNumBits(0, 1);
		mDemoBuffer.WriteNumBits(DEMO_VIDEO_DATA, 5);
		mDemoBuffer.WriteBoolean(mIsWindowed);
		uchar aByte = (uchar) mSyncRefreshRate;
		mDemoBuffer.WriteByte(aByte);
	}
}

// FUNCTION: GAME187 0x44fff0
void SexyAppBase::Set3DAcclerated(bool is3D, bool reinit)
{
	if (mDDInterface->mIs3D == is3D)
		return;

	mUserChanged3DSetting = true;
	mDDInterface->mIs3D = is3D;

	if (reinit)
	{
		int aResult = InitDDInterface();

		if (is3D && aResult != DDInterface::RESULT_OK)
		{
			Set3DAcclerated(false, reinit);
			return;
		}
		else if (aResult != DDInterface::RESULT_OK)
		{
			Popup(GetString("FAILED_INIT_DIRECTDRAW", _S("Failed to initialize DirectDraw: ")) + StringToSexyString(DDInterface::ResultToString(aResult) + " " + mDDInterface->mErrorString));
			DoExit(1);
		}

		ReInitImages();

		mWidgetManager->mImage = mDDInterface->GetScreenImage();
		mWidgetManager->MarkAllDirty();
	}
}

// FUNCTION: GAME187 0x450210
SharedImageRef SexyAppBase::GetSharedImage(const std::string& theFileName, const std::string& theVariant, bool* isNew)
{
	std::string anUpperFileName = StringToUpper(theFileName);
	std::string anUpperVariant = StringToUpper(theVariant);

	std::pair<SharedImageMap::iterator, bool> aResultPair;
	SharedImageRef aSharedImageRef;

	{
		AutoCrit anAutoCrit(mDDInterface->mCritSect);
		aResultPair = mSharedImageMap.insert(SharedImageMap::value_type(SharedImageMap::key_type(anUpperFileName, anUpperVariant), SharedImage()));
		aSharedImageRef = &aResultPair.first->second;
	}

	if (isNew != NULL)
		*isNew = aResultPair.second;

	if (aResultPair.second)
	{
		// Pass in a '!' as the first char of the file name to create a new image
		if ((theFileName.length() > 0) && (theFileName[0] == '!'))
			aSharedImageRef.mSharedImage->mImage = new DDImage(mDDInterface);
		else
			aSharedImageRef.mSharedImage->mImage = GetImage(theFileName,false);
	}

	return aSharedImageRef;
}

// FUNCTION: GAME187 0x450590
void SexyAppBase::CleanSharedImages()
{
	AutoCrit anAutoCrit(mDDInterface->mCritSect);

	if (mCleanupSharedImages)
	{
		// Delete shared images with reference counts of 0
		// This doesn't occur in ~SharedImageRef because sometimes we can not only access the image
		//  through the SharedImageRef returned by GetSharedImage, but also by calling GetSharedImage
		//  again with the same params -- so we can have instances where we do the 'final' deref on
		//  an image but immediately re-request it via GetSharedImage
		SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
		while (aSharedImageItr != mSharedImageMap.end())
		{
			SharedImage* aSharedImage = &aSharedImageItr->second;
			if (aSharedImage->mRefCount == 0)
			{
				delete aSharedImage->mImage;
				mSharedImageMap.erase(aSharedImageItr++);
			}
			else
				++aSharedImageItr;
		}

		mCleanupSharedImages = false;
	}
}
