// SexyApp: PopCap's internal layer over the public SexyAppBase (registration, update checks, ads, beta
// support, build info). Reconstructed from Bejeweled.exe 1.87.
//
// Switch port: this is always the full game, so it starts registered and unlocked. Update checks, ads, the
// beta validator, crash report submission and the shared popcreg.dat record are gone; the play counters live
// in the app's own settings store with everything else SexyAppBase saves.

#include "SexyApp.h"
#include "Buffer.h"
#include "BigInt.h"
#include "Debug.h"
#include "Common.h"
#include <time.h>

using namespace Sexy;

SexyApp* Sexy::gSexyApp = NULL;

// The build tool patches the number and date into this marker after linking.
// The last 8 characters are the seed CheckSignature hashes in front of signed files (properties\partner.xml),
// so they must stay the original's: Steam's partner.xml.sig only verifies with 28959109.
const char* gBuildInfo = "00000000PACPOPPOPCAPPACPOPPOPCAPBUILDINFOMARKERPACPOPPOPCAPPACPOPPOPCAPXXXXXXXXX28959109";

// FUNCTION: GAME187 0x488aa0
SexyApp::SexyApp()
{
	gSexyApp = this;

	mTimesPlayed = 0;
	mTimesExecuted = 0;
	mTimedOut = false;
	mIsRegistered = true;
	mBuildUnlocked = true;

	mDownloadId = 0;
	mRegSource = "ingame";
	mSkipAd = true;
	mDontUpdate = true;
	mLastVerCheckQueryTime = 0;

	mDemoPrefix = "popcap";
	mDemoFileName = mDemoPrefix + ".dmo";
	mCompanyName = "PopCap";
	mFullCompanyName = "PopCap Games";

	char aBuildNumStr[9] = {0};
	strncpy(aBuildNumStr, gBuildInfo, 8);
	mBuildNum = atoi(aBuildNumStr);
	if (mBuildNum != 0)
		mBuildDate = gBuildInfo + 8;
}

// FUNCTION: GAME187 0x488d00
SexyApp::~SexyApp()
{
}

// FUNCTION: GAME187 0x4898c0
void SexyApp::ReadFromRegistry()
{
	SexyAppBase::ReadFromRegistry();

	int anInt;
	if (RegistryReadInteger("TimesPlayed", &anInt))
		mTimesPlayed = anInt;
	if (RegistryReadInteger("TimesExecuted", &anInt))
		mTimesExecuted = anInt;

	std::string aStr;
	if (RegistryReadString("RegName", &aStr))
		mRegUserName = aStr;
	if (RegistryReadString("RegCode", &aStr))
		mRegCode = aStr;

	mReferId = SexyStringToString(GetString("ReferId", StringToSexyString(mReferId)));
	mFullCompanyName = SexyStringToString(GetString("PartnerName", StringToSexyString(mFullCompanyName)));
}

// FUNCTION: GAME187 0x48a450
void SexyApp::WriteToRegistry()
{
	SexyAppBase::WriteToRegistry();

	if ((mRegUserName.length() == 0) &&
		(mUserName.length() > 0) &&
		(mRegCode.length() > 0))
		mRegUserName = mUserName;

	RegistryWriteInteger("TimesPlayed", mTimesPlayed);
	RegistryWriteInteger("TimesExecuted", mTimesExecuted);
	if (mRegUserName.length() > 0)
		RegistryWriteString("RegName", mRegUserName);
	if (mRegCode.length() > 0)
		RegistryWriteString("RegCode", mRegCode);
}

// FUNCTION: GAME187 0x48a570
bool SexyApp::OpenHTMLTemplate(const std::string& theTemplateFile, const DefinesMap& theDefinesMap)
{
	return false;
}

// FUNCTION: GAME187 0x48aab0
bool SexyApp::OpenRegisterPage(DefinesMap theStatsMap)
{
	return false;
}

// FUNCTION: GAME187 0x48b8a0
bool SexyApp::ShouldCheckForUpdate()
{
	return false;
}

// FUNCTION: GAME187 0x48b910
void SexyApp::UpdateCheckQueried()
{
	time_t aTimeNow;
	time(&aTimeNow);
	mLastVerCheckQueryTime = (DWORD)aTimeNow;
}

// FUNCTION: GAME187 0x48b940
void SexyApp::URLOpenSucceeded(const std::string& theURL)
{
	SexyAppBase::URLOpenSucceeded(theURL);
}

// FUNCTION: GAME187 0x48b970
bool SexyApp::OpenRegisterPage()
{
	DefinesMap aDefinesMap;
	return OpenRegisterPage(aDefinesMap);
}

// GLOBAL: accent folding used by Validate
static const unsigned char gAccentTable[][2] =
{
	{0x80, 'C'}, {0x81, 'u'}, {0x82, 'e'}, {0x83, 'a'}, {0x84, 'a'}, {0x85, 'a'}, {0x86, 'a'}, {0x87, 'c'},
	{0x88, 'e'}, {0x89, 'e'}, {0x8a, 'e'}, {0x8b, 'i'}, {0x8c, 'i'}, {0x8d, 'i'}, {0x8e, 'A'}, {0x8f, 'A'},
	{0x90, 'E'}, {0x91, 'a'}, {0x92, 'A'}, {0x93, 'o'}, {0x94, 'o'}, {0x95, 'o'}, {0x96, 'u'}, {0x97, 'u'},
	{0x98, 'y'}, {0x99, 'O'}, {0x9a, 'U'}, {0xa0, 'a'}, {0xa1, 'i'}, {0xa2, 'o'}, {0xa3, 'u'}, {0xa4, 'n'},
	{0xa5, 'N'}, {0xc0, 'A'}, {0xc1, 'A'}, {0xc2, 'A'}, {0xc3, 'A'}, {0xc4, 'A'}, {0xc5, 'A'}, {0xc6, 'A'},
	{0xc7, 'C'}, {0xc8, 'E'}, {0xc9, 'E'}, {0xca, 'E'}, {0xcb, 'E'}, {0xcc, 'I'}, {0xcd, 'I'}, {0xce, 'I'},
	{0xcf, 'I'}, {0xd0, 'D'}, {0xd1, 'N'}, {0xd2, 'O'}, {0xd3, 'O'}, {0xd4, 'O'}, {0xd5, 'O'}, {0xd6, 'O'},
	{0xd8, 'O'}, {0xd9, 'U'}, {0xda, 'U'}, {0xdb, 'U'}, {0xdc, 'U'}, {0xdd, 'Y'}, {0xde, 'b'}, {0xdf, 'B'},
	{0xe0, 'a'}, {0xe1, 'a'}, {0xe2, 'a'}, {0xe3, 'a'}, {0xe4, 'a'}, {0xe5, 'a'}, {0xe6, 'a'}, {0xe7, 'c'},
	{0xe8, 'e'}, {0xe9, 'e'}, {0xea, 'e'}, {0xeb, 'e'}, {0xec, 'i'}, {0xed, 'i'}, {0xee, 'i'}, {0xef, 'i'},
	{0xf0, 'o'}, {0xf1, 'n'}, {0xf2, 'o'}, {0xf3, 'o'}, {0xf4, 'o'}, {0xf5, 'o'}, {0xf6, 'o'}, {0xf8, 'o'},
	{0xf9, 'u'}, {0xfa, 'u'}, {0xfb, 'u'}, {0xfc, 'u'}, {0xfd, 'y'},
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

			unsigned char aChar = theUserName[i];
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
	// The romfs is built from the user's own install; nothing here needs a partner signature.
	return true;
}

// FUNCTION: GAME187 0x48bdb0
void SexyApp::PreTerminate()
{
}

// FUNCTION: GAME187 0x48be00
void SexyApp::OpenUpdateURL()
{
}

// FUNCTION: GAME187 0x48bea0
void SexyApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}

// FUNCTION: GAME187 0x48c260
std::string SexyApp::GetGameSEHInfo()
{
	std::string anInfoString = SexyAppBase::GetGameSEHInfo() +
		"Times Played: " + StrFormat("%d", mTimesPlayed) + "\r\n" +
		"Build Num: " + StrFormat("%d", mBuildNum) + "\r\n" +
		"Build Date: " + mBuildDate + "\r\n";

	if (mReferId.length() != 0)
		anInfoString += "ReferId: " + mReferId + "\r\n";

	return anInfoString;
}

// FUNCTION: GAME187 0x48c6f0
void SexyApp::GetSEHWebParams(DefinesMap* theDefinesMap)
{
}

// FUNCTION: GAME187 0x48cab0
void SexyApp::PreDisplayHook()
{
}

// FUNCTION: GAME187 0x48caf0
void SexyApp::InitPropertiesHook()
{
	LoadProperties("properties/partner.xml", false, false);

	mIsRegistered = true;
	mBuildUnlocked = true;

	mProdName = SexyStringToString(GetString("ProdName", StringToSexyString(mProdName)));

	SexyString aNewTitle = GetString("Title", _S(""));
	if (aNewTitle.length() > 0)
		mTitle = aNewTitle + _S(" ") + StringToSexyString(mProductVersion);
}

// FUNCTION: GAME187 0x48cfb0
void SexyApp::Init()
{
	OutputDebug(_S("Product: %s\r\n"), mProdName.c_str());
	OutputDebug(_S("BuildNum: %d\r\n"), mBuildNum);
	OutputDebug(_S("BuildDate: %s\r\n"), mBuildDate.c_str());

	SexyAppBase::Init();

	mTimesExecuted++;
}

// FUNCTION: GAME187 0x48d450
void SexyApp::UpdateFrames()
{
	SexyAppBase::UpdateFrames();
}
