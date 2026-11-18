#include "InternetManager.h"
#include "SexyAppBase.h"
#include "Buffer.h"
#include "Common.h"
#include <set>
#include <algorithm>
#include <direct.h>
#include <stdio.h>
#include <ctype.h>
#include <objbase.h>
#include <exdisp.h>

using namespace Sexy;

// FUNCTION: GAME187 0x464830
InternetManager::InternetManager()
{
	mCheckDone = false;
	mUpToDate = true;
	mGotReply = false;
	mMaxMoreAds = 4;
}

// 1.87: the ad list is loaded from SexyApp::InitPropertiesHook, not the constructor.
// FUNCTION: GAME187 0x464a30
void InternetManager::Init()
{
	LoadAdList();
}

// FUNCTION: GAME187 0x464930
InternetManager::~InternetManager()
{
	SaveAdList();
}

// FUNCTION: GAME187 0x466430
void InternetManager::CheckAdFiles(AdInfo& theAd)
{
	theAd.mHasFiles = true;

	for (std::list<std::string>::iterator anItr = theAd.mFiles.begin(); anItr != theAd.mFiles.end(); ++anItr)
	{
		if (!gSexyAppBase->FileExists(*anItr))
			theAd.mHasFiles = false;
	}

	if (theAd.mFiles.size() == 0)
		theAd.mHasFiles = false;
}

// FUNCTION: GAME187 0x466490
void InternetManager::CheckAllAdFiles()
{
	for (AdInfoList::iterator anItr = mAdList.begin(); anItr != mAdList.end(); ++anItr)
		CheckAdFiles(*anItr);
}

// FUNCTION: GAME187 0x464aa0
AdInfo* InternetManager::FindAd(AdInfoList& theList, AdInfo& theAd)
{
	if (theAd.mFiles.size() == 0)
		return NULL;

	for (AdInfoList::iterator anItr = theList.begin(); anItr != theList.end(); ++anItr)
	{
		AdInfo* anAd = &*anItr;
		if ((anAd->mFiles.size() != 0) && (anAd->mFiles.front() == theAd.mFiles.front()))
			return anAd;
	}

	return NULL;
}

// FUNCTION: GAME187 0x465ba0
void InternetManager::AdToString(AdInfo& theAd, std::string& theString)
{
	if (theAd.mType == AdInfo::AD_POPUP)
		theString = StrFormat("popup %d %d |", theAd.mWidth, theAd.mHeight);
	else if (theAd.mType == AdInfo::AD_MORE)
		theString = StrFormat("more %s %s |", theAd.mName.c_str(), theAd.mLink.c_str());

	for (std::list<std::string>::iterator anItr = theAd.mFiles.begin(); anItr != theAd.mFiles.end(); ++anItr)
	{
		theString += " ";
		theString += *anItr;
	}

	theString += " | ";
	theString += theAd.mEnabled ? "1 " : "0 ";
	theString += theAd.mSelected ? "1 " : "0 ";
}

// FUNCTION: GAME187 0x464da0
void InternetManager::ParseAdLine(const std::string& theLine, bool isNew)
{
	if (theLine.length() == 0)
		return;

	std::string aLine = theLine;
	AdInfo anAd;
	int anIdx = 0;
	int aSection = 0;
	std::string aToken;

	int aStart = aLine.find_first_not_of(' ');
	int anEnd = aLine.find(' ', aStart + 1);
	if ((anEnd == -1) || (aStart == -1))
		return;

	aToken = StringToLower(aLine.substr(aStart, anEnd - aStart));
	if ((aToken == "popup") || (isdigit(aToken[0])))
		anAd.mType = AdInfo::AD_POPUP;
	else if (aToken == "more")
		anAd.mType = AdInfo::AD_MORE;
	else
		return;

	if (!isdigit(aToken[0]))
		aLine = aLine.substr(anEnd + 1);

	while (aLine.length() > 0)
	{
		int aPos = aLine.find(' ');
		if (aPos == -1)
			aPos = aLine.length();

		aToken = aLine.substr(0, aPos);
		if (aToken.compare("|") == 0)
		{
			anIdx = 0;
			aSection++;
		}
		else
		{
			switch (aSection)
			{
			case 0:
				if (anAd.mType == AdInfo::AD_POPUP)
				{
					switch (anIdx)
					{
					case 0:
						anAd.mWidth = atoi(aToken.c_str());
						break;
					case 1:
						anAd.mHeight = atoi(aToken.c_str());
						break;
					}
				}
				else if (anAd.mType == AdInfo::AD_MORE)
				{
					switch (anIdx)
					{
					case 0:
						anAd.mName = aToken;
						break;
					case 1:
						anAd.mLink = aToken;
						break;
					}
				}
				break;
			case 1:
			{
				anAd.mFiles.push_back(aToken);
				std::list<std::string>::iterator aFileItr = std::find(mFileList.begin(), mFileList.end(), aToken);
				if (aFileItr == mFileList.end())
					mFileList.push_back(aToken);
				if (anAd.mFiles.size() == 1)
				{
					anAd.mEnabled = true;
					AdInfo* anOldAd = FindAd(mOldAdList, anAd);
					if (anOldAd != NULL)
					{
						anAd.mEnabled = anOldAd->mEnabled;
						anAd.mSelected = anOldAd->mSelected;
						anAd.mSeen = anOldAd->mSeen;
					}
					else
						anAd.mSeen = isNew;
				}
				break;
			}
			case 2:
				switch (anIdx)
				{
				case 0:
					anAd.mEnabled = atoi(aToken.c_str()) != 0;
					break;
				case 1:
					anAd.mSelected = atoi(aToken.c_str()) != 0;
					break;
				}
				break;
			}

			anIdx++;
		}

		if (aPos + 1 >= (int) aLine.length())
			break;
		aLine = aLine.substr(aPos + 1);
	}

	mAdList.push_back(anAd);
}

// FUNCTION: GAME187 0x4654c0
void InternetManager::SelectAds()
{
	AdInfoList aMoreList;
	std::set<std::string, StringLessNoCase> aLinkSet;

	AdInfoList::iterator anItr;
	for (anItr = mOldAdList.begin(); anItr != mOldAdList.end(); ++anItr)
	{
		AdInfo* anAd = &*anItr;
		if ((anAd->mType == AdInfo::AD_MORE) && (FindAd(mAdList, *anAd) == NULL))
		{
			CheckAdFiles(*anAd);
			if (anAd->mHasFiles)
				aMoreList.push_back(*anAd);
		}
	}

	int aCount = 0;
	anItr = mAdList.begin();
	while (anItr != mAdList.end())
	{
		AdInfo& anAd = *anItr;
		if (anAd.mType == AdInfo::AD_MORE)
		{
			CheckAdFiles(anAd);
			if (anAd.mHasFiles)
			{
				if (anAd.mSelected)
					aMoreList.push_back(anAd);
				else if (!anAd.mSeen)
				{
					aLinkSet.insert(anAd.mLink);
					aCount++;
				}
			}
		}

		if (anAd.mSelected)
			mAdList.erase(anItr++);
		else
			++anItr;
	}

	anItr = aMoreList.begin();
	while ((anItr != aMoreList.end()) && (aCount < mMaxMoreAds))
	{
		AdInfo* anAd = &*anItr;
		if (aLinkSet.find(anAd->mLink) == aLinkSet.end())
		{
			aLinkSet.insert(anAd->mLink);
			anAd->mSelected = true;
			mAdList.push_back(*anAd);
			aCount++;
			aMoreList.erase(anItr++);
		}
		else
			++anItr;
	}

	for (anItr = aMoreList.begin(); (anItr != aMoreList.end()) && (aCount < mMaxMoreAds); ++anItr)
	{
		AdInfo* anAd = &*anItr;
		anAd->mSelected = true;
		mAdList.push_back(*anAd);
		aCount++;
	}
}

// FUNCTION: GAME187 0x465a60
bool InternetManager::LoadAdList()
{
	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile("adlist.txt", &aBuffer))
		return false;

	while (!aBuffer.AtEnd())
	{
		std::string aLine = aBuffer.ReadLine();
		ParseAdLine(aLine, false);
	}

	SelectAds();
	return true;
}

// FUNCTION: GAME187 0x465d70
bool InternetManager::SaveAdList()
{
	if (mAdList.size() == 0)
		return true;

	Buffer aBuffer;
	SelectAds();

	for (AdInfoList::iterator anItr = mAdList.begin(); anItr != mAdList.end(); ++anItr)
	{
		AdInfo& anAd = *anItr;
		std::string aLine;
		AdToString(anAd, aLine);
		aBuffer.WriteLine(aLine);
	}

	return gSexyAppBase->WriteBufferToFile("adlist.txt", &aBuffer);
}

// FUNCTION: GAME187 0x464a40
void InternetManager::CheckForUpdates(const std::string& theURL)
{
	mCheckDone = false;
	mTransfer.Get(theURL);
}

// FUNCTION: GAME187 0x464b20
bool InternetManager::CheckReply()
{
	if (mCheckDone)
		return true;

	if (mTransfer.GetResultCode() != HTTPTransfer::RESULT_DONE)
		return false;

	mGotReply = true;
	mOldAdList = mAdList;
	mAdList.clear();
	mFileList.clear();

	int aLineNum = 0;
	std::string aContent = mTransfer.GetContent();
	while (aContent.length() > 0)
	{
		int aPos = aContent.find('\n');
		if (aPos == -1)
			aPos = aContent.length();

		std::string aLine = aContent.substr(0, aPos);
		while ((aLine.length() > 0) && ((aLine[aLine.length() - 1] == '\r') || (aLine[aLine.length() - 1] == ' ')))
			aLine.resize(aLine.length() - 1);

		if (aLine.length() > 0)
		{
			switch (aLineNum)
			{
			case 0:
				mUpToDate = atoi(aLine.c_str()) != 0;
				break;
			case 1:
				mUpdateURL = aLine;
				break;
			default:
				ParseAdLine(aLine, true);
				break;
			}

			aLineNum++;
		}

		if (aPos + 1 >= (int) aContent.length())
			break;
		aContent = aContent.substr(aPos + 1);
	}

	SelectAds();
	return true;
}

std::string InternetManager::GetUpdateURL()
{
	if (!CheckReply())
		return "";

	return mUpdateURL;
}

// FUNCTION: GAME187 0x464a60
int InternetManager::GetCheckStatus()
{
	switch (mTransfer.GetResultCode())
	{
	case HTTPTransfer::RESULT_DONE:
		return 2;
	case HTTPTransfer::RESULT_NOT_STARTED:
		return 0;
	case HTTPTransfer::RESULT_NOT_COMPLETED:
		return 1;
	default:
		return 3;
	}
}

// FUNCTION: GAME187 0x466280
bool InternetManager::IsUpToDate()
{
	if (!CheckReply())
		return true;

	return mUpToDate;
}

// FUNCTION: GAME187 0x466300
void InternetManager::RemoveExistingFiles()
{
	CheckReply();

	std::list<std::string>::iterator anItr = mFileList.begin();
	while (anItr != mFileList.end())
	{
		std::string aFileName = *anItr;
		if (gSexyAppBase->FileExists(aFileName))
			anItr = mFileList.erase(anItr);
		else
			++anItr;
	}
}

// FUNCTION: GAME187 0x465ef0
void InternetManager::Update()
{
	if (!mGotReply)
		return;

	for (int i = 0; i < 4; i++)
	{
		HTTPTransfer* aTransfer = &mFileTransfers[i];

		int aResult = aTransfer->GetResultCode();
		if (aResult == HTTPTransfer::RESULT_DONE)
		{
			std::string aFileName = aTransfer->mSpecifiedRelURL;

			int aSlashPos = aFileName.find('/');
			while (aSlashPos != -1)
			{
				int aNextPos = aSlashPos + 1;
				{
					std::string aDir = aFileName.substr(0, aSlashPos);
					_mkdir(aDir.c_str());
				}
				aSlashPos = aFileName.find('/', aNextPos);
			}

			OutputDebugString(StrFormat("AdFile: %s\r\n", aFileName.c_str()).c_str());

			FILE* aFP = fopen(aFileName.c_str(), "wb");
			if (aFP != NULL)
			{
				std::string aContent = aTransfer->GetContent();
				fwrite(aContent.c_str(), 1, aContent.length(), aFP);
				fclose(aFP);
			}

			aTransfer->Reset();
		}
		else if (aResult == HTTPTransfer::RESULT_NOT_COMPLETED)
			continue;
		else if (aResult != HTTPTransfer::RESULT_NOT_STARTED)
		{
			OutputDebugString(("Error on AdFile: " + aTransfer->mSpecifiedRelURL + "\r\n").c_str());
			aTransfer->Reset();
		}

		if (mFileList.size() > 0)
		{
			std::string aFile = mFileList.front();
			mFileList.pop_front();
			aTransfer->Get(mTransfer.mURL, aFile);
		}
	}
}

// FUNCTION: GAME187 0x4664e0
bool InternetManager::HasAd(int theType, bool includeSeen)
{
	CheckAllAdFiles();

	for (AdInfoList::iterator anItr = mAdList.begin(); anItr != mAdList.end(); ++anItr)
	{
		if ((anItr->mType == theType) && (anItr->mHasFiles) && (anItr->mEnabled) && ((includeSeen) || (!anItr->mSeen)))
			return true;
	}

	return false;
}

// FUNCTION: GAME187 0x466540
void InternetManager::ShowAd()
{
	int aCount = 0;
	AdInfo* anAd = NULL;

	CheckAllAdFiles();

	AdInfoList::iterator anItr;
	for (anItr = mAdList.begin(); anItr != mAdList.end(); ++anItr)
	{
		AdInfo* anInfo = &*anItr;
		if ((anInfo->mHasFiles) && (anInfo->mType == AdInfo::AD_POPUP))
		{
			if ((anInfo->mEnabled) && (anAd == NULL))
				anAd = anInfo;
			aCount++;
		}
	}

	if (anAd == NULL)
	{
		if (aCount <= 0)
			return;

		int aNum = Rand() % aCount;
		for (anItr = mAdList.begin(); anItr != mAdList.end(); ++anItr)
		{
			anAd = &*anItr;
			if ((anAd->mHasFiles) && (anAd->mType == AdInfo::AD_POPUP) && (aNum-- == 0))
				break;
		}

		if (anAd == NULL)
			return;
	}

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		IWebBrowser2* aBrowser = NULL;
		if (SUCCEEDED(CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_SERVER, IID_IWebBrowser2, (void**) &aBrowser)))
		{
			RECT aRect = {0, 0, anAd->mWidth, anAd->mHeight};
			AdjustWindowRect(&aRect, WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

			aBrowser->put_AddressBar(VARIANT_FALSE);
			aBrowser->put_MenuBar(VARIANT_FALSE);
			aBrowser->put_StatusBar(VARIANT_FALSE);
			aBrowser->put_ToolBar(VARIANT_FALSE);
			aBrowser->put_Resizable(VARIANT_FALSE);
			aBrowser->put_Width(aRect.right - aRect.left);
			aBrowser->put_Height(aRect.bottom - aRect.top);

			char aURL[4096];
			strcpy(aURL, "file://");
			_getcwd(aURL + strlen(aURL), 4096);
			strcat(aURL, "\\");
			strcat(aURL, anAd->mFiles.front().c_str());

			VARIANT anEmpty = {0};
			WCHAR aWURL[4096];
			mbstowcs(aWURL, aURL, 4096);
			BSTR aBStr = SysAllocString(aWURL);
			aBrowser->Navigate(aBStr, &anEmpty, &anEmpty, &anEmpty, &anEmpty);
			SysFreeString(aBStr);

			aBrowser->put_Visible(VARIANT_TRUE);
			aBrowser->Release();
		}
	}

	anAd->mEnabled = false;
}
