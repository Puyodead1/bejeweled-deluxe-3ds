#ifndef __INTERNETMANAGER_H__
#define __INTERNETMANAGER_H__

#include <string>
#include <list>
#include "HTTPTransfer.h"

namespace Sexy
{

// One line of adlist.txt / the update check reply. Size 0x54.
struct AdInfo
{
	enum
	{
		AD_POPUP = 1,
		AD_MORE = 2
	};

	int mType;                         // 0x00
	int mWidth;                        // 0x04
	int mHeight;                       // 0x08
	std::list<std::string> mFiles;     // 0x0c
	bool mHasFiles;                    // 0x18
	bool mEnabled;                     // 0x19
	bool mSelected;                    // 0x1a
	bool mSeen;                        // 0x1b
	std::string mName;                 // 0x1c
	std::string mLink;                 // 0x38

	AdInfo() : mType(0), mSelected(false), mSeen(false)
	{
		mHasFiles = false;
		mEnabled = false;
	}
};

typedef std::list<AdInfo> AdInfoList;

// RTTI: .?AVInternetManager@Sexy@@ - owned by SexyApp at +0xd0. Size 0x54c.
// VTABLE: GAME187 0x510bb4
class InternetManager
{
public:
	int mMaxMoreAds;                   // 0x004
	HTTPTransfer mTransfer;            // 0x008
	HTTPTransfer mFileTransfers[4];    // 0x100
	bool mCheckDone;                   // 0x4f0
	bool mUpToDate;             // 0x4f1
	bool mGotReply;                    // 0x4f2
	std::string mUpdateURL;            // 0x4f4
	AdInfoList mAdList;                // 0x510
	AdInfoList mOldAdList;             // 0x51c
	std::list<std::string> mFileList;  // 0x528

protected:
	void CheckAdFiles(AdInfo& theAd);
	void CheckAllAdFiles();
	AdInfo* FindAd(AdInfoList& theList, AdInfo& theAd);
	void AdToString(AdInfo& theAd, std::string& theString);
	virtual void ParseAdLine(const std::string& theLine, bool isNew);
	void SelectAds();
	bool LoadAdList();
	bool SaveAdList();
	bool CheckReply();

public:
	InternetManager();
	virtual ~InternetManager();

	void Init();

	void CheckForUpdates(const std::string& theURL);
	int GetCheckStatus(); // 0x454420
	bool IsUpToDate(); // 0x45b900
	std::string GetUpdateURL();
	void RemoveExistingFiles(); // 0x45b990
	void Update();

	bool HasAd(int theType, bool includeSeen);
	void ShowAd();
};

}

#endif // __INTERNETMANAGER_H__