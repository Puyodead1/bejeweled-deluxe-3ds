// SexyApp: PopCap's internal layer over the public SexyAppBase (registration, update checks, ads, beta
// support, build info). Reconstructed from Bejeweled.exe 1.87; most of the logic is the 1.862 code that
// used to live in SexyAppBase, moved into this subclass.

#include "SexyApp.h"
#include "InternetManager.h"
#include "BetaSupport.h"
#include "SEHCatcher.h"
#include "Buffer.h"
#include "BigInt.h"
#include "Debug.h"
#include "Common.h"
#include <direct.h>
#include <fstream>
#include <time.h>

using namespace Sexy;

SexyApp* Sexy::gSexyApp = NULL;

// The build tool patches the number and date into this marker after linking.
// The last 8 characters are the seed CheckSignature hashes in front of signed files (properties\partner.xml),
// so they must stay the original's: Steam's partner.xml.sig only verifies with 28959109.
char* gBuildInfo = "00000000PACPOPPOPCAPPACPOPPOPCAPBUILDINFOMARKERPACPOPPOPCAPPACPOPPOPCAPXXXXXXXXX28959109";

// Set from Init(); consumed by the crash report uploader.
static std::string gSEHSubmitKey;

// FUNCTION: GAME187 0x497c40
static void SetSEHSubmitKey(const std::string& theKey)
{
	gSEHSubmitKey = theKey;
}

// FUNCTION: GAME187 0x488aa0
SexyApp::SexyApp()
{
	gSexyApp = this;

	mTimesPlayed = 0;
	mTimesExecuted = 0;
	mTimedOut = false;
	mIsRegistered = false;
	mBuildUnlocked = false;

	mDownloadId = 0;
	mRegSource = "ingame";
	mSkipAd = false;
	mDontUpdate = false;
	mLastVerCheckQueryTime = 0;

	mDemoPrefix = "popcap";
	mDemoFileName = mDemoPrefix + ".dmo";
	mCompanyName = "PopCap";
	mFullCompanyName = "PopCap Games";

	mInternetManager = new InternetManager();
	mBetaSupport = NULL;
	mBetaValidate = false;

	char aBuildNumStr[9] = {0};
	strncpy(aBuildNumStr, gBuildInfo, 8);
	mBuildNum = atoi(aBuildNumStr);
	if (mBuildNum != 0)
		mBuildDate = gBuildInfo + 8;
}

// FUNCTION: GAME187 0x488d00
SexyApp::~SexyApp()
{
	delete mBetaSupport;
	delete mInternetManager;
}

// Read the per-product record from popcreg.dat, falling back to the old HKLM registry location.
// FUNCTION: GAME187 0x4892f0
static void ReadRegInfo(SexyApp* theApp, std::string* theRegName, std::string* theRegCode, int* theTimesPlayed, int* theTimesExecuted)
{
	char aPath[MAX_PATH + 4];
	if (CheckForVista())
	{
		strcpy(aPath, GetPathFrom("..", GetAppDataFolder()).c_str());
	}
	else
		GetWindowsDirectoryA(aPath, MAX_PATH);

	if (aPath[strlen(aPath) - 1] != '\\')
		strcat(aPath, "\\");
	strcat(aPath, "popcreg.dat");

	FILE* aFP = fopen(aPath, "rb");
	if (aFP != NULL)
	{
		char aProdName[0x10000];
		char aRegName[0x10000];
		char aRegCode[0x10000];
		ushort aLen;
		while (fread(&aLen, 2, 1, aFP) == 1)
		{
			short aTimesPlayed;
			short aTimesExecuted;

			aProdName[aLen] = 0;
			fread(aProdName, 1, aLen, aFP);
			if (fread(&aLen, 2, 1, aFP) != 1)
				break;
			aRegName[aLen] = 0;
			fread(aRegName, 1, aLen, aFP);
			if (fread(&aLen, 2, 1, aFP) != 1)
				break;
			aRegCode[aLen] = 0;
			fread(aRegCode, 1, aLen, aFP);
			if ((fread(&aTimesPlayed, 2, 1, aFP) != 1) || (fread(&aTimesExecuted, 2, 1, aFP) != 1))
				break;

			if (theApp->mProdName == aProdName)
			{
				*theRegName = aRegName;
				*theRegCode = aRegCode;
				*theTimesPlayed = aTimesPlayed;
				*theTimesExecuted = aTimesExecuted;
				fclose(aFP);
				return;
			}
		}

		fclose(aFP);
	}

	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + theApp->mRegKey);
	HKEY aKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, aKeyName.c_str(), 0, KEY_WRITE, &aKey) == ERROR_SUCCESS)
	{
		DWORD aType;
		DWORD aLen;
		BYTE aRegName[256];
		BYTE aRegCode[256];
		DWORD aValue;

		aType = REG_SZ;
		aLen = 256;
		if (RegQueryValueExA(aKey, "RegName", NULL, &aType, aRegName, &aLen) == ERROR_SUCCESS)
		{
			aRegName[aLen] = 0;
			*theRegName = (char*) aRegName;
		}

		aType = REG_SZ;
		aLen = 256;
		if (RegQueryValueExA(aKey, "RegCode", NULL, &aType, aRegCode, &aLen) == ERROR_SUCCESS)
		{
			aRegCode[aLen] = 0;
			*theRegCode = (char*) aRegCode;
		}

		aType = REG_DWORD;
		aLen = 4;
		if (RegQueryValueExA(aKey, "TimesPlayed", NULL, &aType, (BYTE*) &aValue, &aLen) == ERROR_SUCCESS)
			*theTimesPlayed = aValue;

		aType = REG_DWORD;
		aLen = 4;
		if (RegQueryValueExA(aKey, "TimesExecuted", NULL, &aType, (BYTE*) &aValue, &aLen) == ERROR_SUCCESS)
			*theTimesExecuted = aValue;

		RegCloseKey(aKey);
	}
}

// Writing the shared registration record is compiled out in this build.
// FUNCTION: GAME187 0x4892b0
static void WriteRegInfo(std::string theRegName, std::string theRegCode)
{
}

// FUNCTION: GAME187 0x4898c0
void SexyApp::ReadFromRegistry()
{
	SexyAppBase::ReadFromRegistry();

	// Values the installer writes under HKLM\SOFTWARE\<RegKey>.
	char aStr[1024];
	ulong aLen;
	ulong aType;
	int anInt;

	aLen = sizeof(aStr) - 1;
	if (RegistryReadKey("ReferId", &aType, (uchar*) aStr, &aLen, HKEY_LOCAL_MACHINE) && (aType == REG_SZ))
	{
		aStr[aLen] = 0;
		mReferId = aStr;
	}
	mReferId = SexyStringToString(GetString("ReferId", StringToSexyString(mReferId)));

	aLen = sizeof(aStr) - 1;
	if (RegistryReadKey("RegisterLink", &aType, (uchar*) aStr, &aLen, HKEY_LOCAL_MACHINE) && (aType == REG_SZ))
	{
		aStr[aLen] = 0;
		mRegisterLink = aStr;
	}
	else
		mRegisterLink = "http://www.popcap.com/register.php?theGame=" + mProdName + "&referid=" + mReferId;

	aLen = sizeof(aStr) - 1;
	if (RegistryReadKey("Variation", &aType, (uchar*) aStr, &aLen, HKEY_LOCAL_MACHINE) && (aType == REG_SZ))
	{
		aStr[aLen] = 0;
		mVariation = aStr;
	}

	aLen = sizeof(aStr) - 1;
	if (RegistryReadKey("PartnerName", &aType, (uchar*) aStr, &aLen, HKEY_LOCAL_MACHINE) && (aType == REG_SZ))
	{
		aStr[aLen] = 0;
		mFullCompanyName = aStr;
	}

	aLen = sizeof(int);
	if (RegistryReadKey("DontUpdate", &aType, (uchar*) &anInt, &aLen, HKEY_LOCAL_MACHINE) && (aType == REG_DWORD))
		mDontUpdate = anInt != 0;

	aLen = sizeof(int);
	if (RegistryReadKey("DownloadId", &aType, (uchar*) &anInt, &aLen, HKEY_LOCAL_MACHINE) && (aType == REG_DWORD))
		mDownloadId = anInt;

	if (RegistryReadInteger("LastVerCheckQueryTime", &anInt))
		mLastVerCheckQueryTime = anInt;
	else if ((aLen = sizeof(int), RegistryReadKey("LastVerCheckQueryTime", &aType, (uchar*) &anInt, &aLen, HKEY_LOCAL_MACHINE)) && (aType == REG_DWORD))
		mLastVerCheckQueryTime = anInt;
	else
	{
		time_t aTimeNow;
		time(&aTimeNow);
		mLastVerCheckQueryTime = aTimeNow;
	}

	std::string aRegName;
	std::string aRegCode;
	int aTimesPlayed = 0;
	int aTimesExecuted = 0;
	ReadRegInfo(this, &aRegName, &aRegCode, &aTimesPlayed, &aTimesExecuted);
	mTimesPlayed = aTimesPlayed;
	mTimesExecuted = aTimesExecuted;
	mRegUserName = aRegName;
	mRegCode = aRegCode;

	mIsRegistered |= Validate(mRegUserName, mRegCode);

	mRegisterLink = SexyStringToString(GetString("RegisterLink", StringToSexyString(mRegisterLink)));
	mDontUpdate = GetBoolean("DontUpdate", mDontUpdate);
	mFullCompanyName = SexyStringToString(GetString("PartnerName", StringToSexyString(mFullCompanyName)));
}

// FUNCTION: GAME187 0x48a450
void SexyApp::WriteToRegistry()
{
	SexyAppBase::WriteToRegistry();

	RegistryWriteInteger("LastVerCheckQueryTime", mLastVerCheckQueryTime);

	if ((mRegUserName.length() == 0) &&
		(mUserName.length() > 0) &&
		(mRegCode.length() > 0))
		mRegUserName = mUserName;

	WriteRegInfo(mRegUserName, mRegCode);
}

// FUNCTION: GAME187 0x48a570
bool SexyApp::OpenHTMLTemplate(const std::string& theTemplateFile, const DefinesMap& theDefinesMap)
{
	std::fstream anInStream(theTemplateFile.c_str(), std::ios::in);

	if (!anInStream.is_open())
		return false;

	WIN32_FIND_DATAA aFindData;

	HANDLE aFindHandle = FindFirstFileA("temp\\tpl*.html", &aFindData);
	if (aFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::string aFilePath = std::string("temp\\") + aFindData.cFileName;
			DeleteFileA(aFilePath.c_str());
		}
		while (FindNextFileA(aFindHandle, &aFindData));

		FindClose(aFindHandle);
	}

	mkdir("temp");

	std::string anOutFilename = StrFormat("temp\\tpl%04d.html", rand() % 10000);

	std::fstream anOutStream(anOutFilename.c_str(), std::ios::out);
	if (!anOutStream.is_open())
		return false;

	char aStr[4096];
	while (!anInStream.eof())
	{
		anInStream.getline(aStr, 4096);

		std::string aNewString = Evaluate(aStr, theDefinesMap);

		anOutStream << aNewString.c_str() << std::endl;
	}

	return OpenURL(GetFullPath(anOutFilename), false);
}

// FUNCTION: GAME187 0x48aab0
bool SexyApp::OpenRegisterPage(DefinesMap theStatsMap)
{
	DefinesMap aDefinesMap;

	aDefinesMap.insert(DefinesMap::value_type("Src", mRegSource));
	aDefinesMap.insert(DefinesMap::value_type("ProdName", mProdName));
	aDefinesMap.insert(DefinesMap::value_type("Version", mProductVersion));
	aDefinesMap.insert(DefinesMap::value_type("Variation", mVariation));
	aDefinesMap.insert(DefinesMap::value_type("ReferId", mReferId));
	aDefinesMap.insert(DefinesMap::value_type("DownloadId", StrFormat("%d", mDownloadId)));
	aDefinesMap.insert(DefinesMap::value_type("TimesPlayed", StrFormat("%d", mTimesPlayed)));
	aDefinesMap.insert(DefinesMap::value_type("TimesExecuted", StrFormat("%d", mTimesExecuted)));
	aDefinesMap.insert(DefinesMap::value_type("TimedOut", mTimedOut ? "Y" : "N"));

	std::string aStatsString;
	DefinesMap::iterator anItr = theStatsMap.begin();
	while (anItr != theStatsMap.end())
	{
		std::string aKeyString = anItr->first;
		std::string aValueString = anItr->second;

		aStatsString += StrFormat("%04X", aKeyString.length()) + aKeyString + "S" + StrFormat("%04X", aValueString.length()) + aValueString;

		++anItr;
	}

	aDefinesMap.insert(DefinesMap::value_type("Stats", aStatsString));

	if (FileExists("register.tpl"))
		return OpenHTMLTemplate("register.tpl", aDefinesMap);
	else
		return OpenURL(mRegisterLink, false);
}

// FUNCTION: GAME187 0x48b8a0
bool SexyApp::ShouldCheckForUpdate()
{
	if (mDontUpdate)
		return false;

	time_t aTimeNow;
	time(&aTimeNow);

	// Check at most once a week, unless the last run didn't shut down cleanly.
	return (mLastVerCheckQueryTime == 0) ||
		(!mLastShutdownWasGraceful) ||
		((aTimeNow - mLastVerCheckQueryTime) > 7*24*60*60);
}

// FUNCTION: GAME187 0x48b910
void SexyApp::UpdateCheckQueried()
{
	time_t aTimeNow;
	time(&aTimeNow);
	mLastVerCheckQueryTime = aTimeNow;
}

// FUNCTION: GAME187 0x48b940
void SexyApp::URLOpenSucceeded(const std::string& theURL)
{
	SexyAppBase::URLOpenSucceeded(theURL);

	if (mShutdownOnURLOpen)
		mSkipAd = true;
}

// FUNCTION: GAME187 0x48b970
bool SexyApp::OpenRegisterPage()
{
	DefinesMap aDefinesMap;
	return OpenRegisterPage(aDefinesMap);
}

// GLOBAL: accent folding used by Validate
static char gAccentTable[][2] =
{
	{'\x80', 'C'}, {'\x81', 'u'}, {'\x82', 'e'}, {'\x83', 'a'}, {'\x84', 'a'}, {'\x85', 'a'}, {'\x86', 'a'}, {'\x87', 'c'},
	{'\x88', 'e'}, {'\x89', 'e'}, {'\x8a', 'e'}, {'\x8b', 'i'}, {'\x8c', 'i'}, {'\x8d', 'i'}, {'\x8e', 'A'}, {'\x8f', 'A'},
	{'\x90', 'E'}, {'\x91', 'a'}, {'\x92', 'A'}, {'\x93', 'o'}, {'\x94', 'o'}, {'\x95', 'o'}, {'\x96', 'u'}, {'\x97', 'u'},
	{'\x98', 'y'}, {'\x99', 'O'}, {'\x9a', 'U'}, {'\xa0', 'a'}, {'\xa1', 'i'}, {'\xa2', 'o'}, {'\xa3', 'u'}, {'\xa4', 'n'},
	{'\xa5', 'N'}, {'\xc0', 'A'}, {'\xc1', 'A'}, {'\xc2', 'A'}, {'\xc3', 'A'}, {'\xc4', 'A'}, {'\xc5', 'A'}, {'\xc6', 'A'},
	{'\xc7', 'C'}, {'\xc8', 'E'}, {'\xc9', 'E'}, {'\xca', 'E'}, {'\xcb', 'E'}, {'\xcc', 'I'}, {'\xcd', 'I'}, {'\xce', 'I'},
	{'\xcf', 'I'}, {'\xd0', 'D'}, {'\xd1', 'N'}, {'\xd2', 'O'}, {'\xd3', 'O'}, {'\xd4', 'O'}, {'\xd5', 'O'}, {'\xd6', 'O'},
	{'\xd8', 'O'}, {'\xd9', 'U'}, {'\xda', 'U'}, {'\xdb', 'U'}, {'\xdc', 'U'}, {'\xdd', 'Y'}, {'\xde', 'b'}, {'\xdf', 'B'},
	{'\xe0', 'a'}, {'\xe1', 'a'}, {'\xe2', 'a'}, {'\xe3', 'a'}, {'\xe4', 'a'}, {'\xe5', 'a'}, {'\xe6', 'a'}, {'\xe7', 'c'},
	{'\xe8', 'e'}, {'\xe9', 'e'}, {'\xea', 'e'}, {'\xeb', 'e'}, {'\xec', 'i'}, {'\xed', 'i'}, {'\xee', 'i'}, {'\xef', 'i'},
	{'\xf0', 'o'}, {'\xf1', 'n'}, {'\xf2', 'o'}, {'\xf3', 'o'}, {'\xf4', 'o'}, {'\xf5', 'o'}, {'\xf6', 'o'}, {'\xf8', 'o'},
	{'\xf9', 'u'}, {'\xfa', 'u'}, {'\xfb', 'u'}, {'\xfc', 'u'}, {'\xfd', 'y'},
	{0, 0}
};

// FUNCTION: GAME187 0x488ec0
bool SexyApp::Validate(const std::string& theUserName, const std::string& theRegCode)
{
	BigInt aModulus("42BF94023BBA6D040C8B81D9");
	BigInt anExponent("11");

	std::string aUserName;
	bool hadSpace = false;
	for (unsigned int i = 0; i < theUserName.length(); i++)
	{
		if (theUserName[i] == ' ')
		{
			if (aUserName.length() > 0)
				hadSpace = true;
		}
		else
		{
			if (hadSpace)
			{
				aUserName += " ";
				hadSpace = false;
			}

			char aChar = theUserName[i];
			for (int j = 0; gAccentTable[j][0] != 0; j++)
			{
				if (gAccentTable[j][0] == aChar)
					aChar = gAccentTable[j][1];
			}

			aUserName += (char) toupper(aChar);
		}
	}

	std::string aProdName;
	aProdName = mProdName;
	for (unsigned int i = 0; i < aProdName.length(); i++)
		aProdName[i] = toupper(aProdName[i]);

	aUserName += "\n";
	aUserName += aProdName;

	BigInt aHash = HashData(aUserName, 94);
	BigInt aSig = KeyToInt(theRegCode);
	BigInt aResult = aSig.ModPow(anExponent, aModulus);
	return aResult == aHash;
}

// FUNCTION: GAME187 0x48ba10
bool SexyApp::CheckSignature(const Buffer& theBuffer, const std::string& theFileName)
{
	if (mSkipSignatureChecks)
		return true;

	FILE* aFP = fopen((theFileName + ".sig").c_str(), "rb");
	if (aFP == NULL)
		return false;

	char aSigStr[25];
	fread(aSigStr, 1, 24, aFP);
	aSigStr[24] = 0;
	fclose(aFP);

	char* aDataPtr = new char[theBuffer.GetDataLen() + 4];
	char aSeedStr[9] = {0};
	strncpy(aSeedStr, gBuildInfo + 80, 8);
	*((int*) aDataPtr) = atoi(aSeedStr);

	int aDataLen = 4;
	theBuffer.SeekFront();
	while (!theBuffer.AtEnd())
	{
		uchar aChar = theBuffer.ReadByte();
		fread(&aChar, 1, 1, aFP);
		if (!isspace(aChar))
			aDataPtr[aDataLen++] = aChar;
	}

	BigInt aModulus("D99BC76AB7B2578738E606F7");
	BigInt anExponent("11");
	BigInt aHash = HashData(aDataPtr, aDataLen, 94);
	delete aDataPtr;
	BigInt aSig(aSigStr);
	BigInt aResult = aSig.ModPow(anExponent, aModulus);
	return aResult == aHash;
}

// FUNCTION: GAME187 0x48bdb0
void SexyApp::PreTerminate()
{
	if (!mSkipAd)
	{
		if (((!mIsRegistered) || (mInternetManager->HasAd(AdInfo::AD_POPUP, true))) && (Rand() % 2 == 0))
			mInternetManager->ShowAd();
	}
}

// FUNCTION: GAME187 0x48be00
void SexyApp::OpenUpdateURL()
{
	OpenURL(mInternetManager->GetUpdateURL(), true);
	Shutdown();
}

// FUNCTION: GAME187 0x48bea0
void SexyApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-version")
	{
		std::string aVersionString =
			"Product: " + mProdName + "\r\n" +
			"Version: " + mProductVersion + "\r\n" +
			"Build Num: " + StringToSexyString(StrFormat("%d", mBuildNum)) + "\r\n" +
			"Build Date: " + mBuildDate;

		MessageBoxA(NULL, aVersionString.c_str(), "Version Info", MB_OK | MB_ICONINFORMATION);
		DoExit(0);
	}

	SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}

// FUNCTION: GAME187 0x48c260
std::string SexyApp::GetGameSEHInfo()
{
	char aTimesPlayedStr[16];
	sprintf(aTimesPlayedStr, "%d", mTimesPlayed);

	std::string anInfoString = SexyAppBase::GetGameSEHInfo() +
		"Times Played: " + std::string(aTimesPlayedStr) + "\r\n" +
		"Build Num: " + StrFormat("%d", mBuildNum) + "\r\n" +
		"Build Date: " + mBuildDate + "\r\n";

	if (mReferId.length() != 0)
		anInfoString += "ReferId: " + mReferId + "\r\n";

	return anInfoString;
}

// FUNCTION: GAME187 0x48c6f0
void SexyApp::GetSEHWebParams(DefinesMap* theDefinesMap)
{
	theDefinesMap->insert(DefinesMap::value_type("username", mUserName));
	theDefinesMap->insert(DefinesMap::value_type("buildnum", StrFormat("%d", mBuildNum)));
	theDefinesMap->insert(DefinesMap::value_type("builddate", mBuildDate));
	theDefinesMap->insert(DefinesMap::value_type("referid", mReferId));
}

// FUNCTION: GAME187 0x48cab0
void SexyApp::PreDisplayHook()
{
	if ((mBetaValidate) && (!mBetaSupport->Validate()))
	{
		Shutdown();
		DoExit(0);
	}
}

// FUNCTION: GAME187 0x48caf0
void SexyApp::InitPropertiesHook()
{
	// Load properties if we need to
	bool checkSig = !IsScreenSaver();
	LoadProperties("properties\\partner.xml", false, checkSig);

	if (GetBoolean("NoReg", false))
	{
		mIsRegistered = true;
		mBuildUnlocked = true;
	}

	mProdName = SexyStringToString(GetString("ProdName", StringToSexyString(mProdName)));
	mIsWindowed = GetBoolean("DefaultWindowed", mIsWindowed);

	SexyString aNewTitle = GetString("Title", _S(""));
	if (aNewTitle.length() > 0)
		mTitle = aNewTitle + _S(" ") + StringToSexyString(mProductVersion);

	mInternetManager->Init();
	mBetaSupport = new BetaSupport(this);
}

// FUNCTION: GAME187 0x48cfb0
void SexyApp::Init()
{
	SEHCatcher::mCrashMessage = SexyStringToWString(GetString("UNEXPECTED_ERROR",
		_S("An unexpected error has occured!  Pressing 'Send Report' will send us helpful debugging information that may help us resolve this issue in the future.\r\n\r\nYou can also contact us directly at feedback@popcap.com.")));
	SEHCatcher::mSubmitMessage = SexyStringToWString(GetString("PLEASE_HELP",
		_S("Please help us out by providing as much information as you can about this crash. Is this the first time it happened? Have you used other PopCap Deluxe games successfully before? Have you upgraded your drivers or any software recently that may be interfering with this program?")));
	SEHCatcher::mSubmitErrorMessage = SexyStringToWString(GetString("FAILED_CONNECT_POPCAP",
		_S("Failed to connect to PopCap servers.  Please check your Internet connection.\nIf you are on a dial-up connection, you may have to manually connect to your ISP.")));
	SEHCatcher::mSubmitHost = "www.popcap.com";

	OutputDebug(_S("Product: %s\r\n"), mProdName.c_str());
	OutputDebug(_S("BuildNum: %d\r\n"), mBuildNum);
	OutputDebug(_S("BuildDate: %s\r\n"), mBuildDate.c_str());

	SetSEHSubmitKey("LU5OkT7!L06fiD9nMbCE9ZoJuXxgx4zcgVDd2!a1tb2uHfhRrLG5wnNySQswXqAt2h");

	SexyAppBase::Init();

	mTimesExecuted++;
	if (IsScreenSaver())
		mSkipAd = true;
}

// FUNCTION: GAME187 0x48d450
void SexyApp::UpdateFrames()
{
	SexyAppBase::UpdateFrames();
	mInternetManager->Update();
}
