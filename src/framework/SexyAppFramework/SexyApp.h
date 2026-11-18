#ifndef __SEXYAPP_H__
#define __SEXYAPP_H__

// The PopCap-internal layer between the public SexyAppBase and the game (1.87). Not part of the public
// framework release; reconstructed from Bejeweled.exe 1.87 (vtable 0x51e52c, ctor 0x488aa0).
// In 1.862 this code still lived inside SexyAppBase itself.

#include "SexyAppBase.h"

namespace Sexy
{

// VTABLE: GAME187 0x51e52c Sexy::ButtonListener
// VTABLE: GAME187 0x51e6cc Sexy::DialogListener
class SexyApp : public SexyAppBase
{
public:
	std::string				mReferId;				// 0x640
	std::string				mVariation;				// 0x65c
	DWORD					mDownloadId;			// 0x678
	std::string				mRegSource;				// 0x67c
	DWORD					mLastVerCheckQueryTime;	// 0x698
	bool					mSkipAd;				// 0x69c
	bool					mDontUpdate;			// 0x69d

	int						mBuildNum;				// 0x6a0
	std::string				mBuildDate;				// 0x6a4

	std::string				mUserName;				// 0x6c0
	std::string				mRegUserName;			// 0x6dc
	std::string				mRegCode;				// 0x6f8
	bool					mIsRegistered;			// 0x714
	bool					mBuildUnlocked;			// 0x715

	int						mTimesPlayed;			// 0x718
	int						mTimesExecuted;			// 0x71c
	bool					mTimedOut;				// 0x720

public:
	SexyApp();
	virtual ~SexyApp();

	// SexyAppBase overrides
	virtual void			UpdateFrames();
	virtual void			PreTerminate();
	virtual void			WriteToRegistry();
	virtual void			ReadFromRegistry();
	virtual void			PreDisplayHook();
	virtual void			URLOpenSucceeded(const std::string& theURL);
	virtual std::string		GetGameSEHInfo();
	virtual void			GetSEHWebParams(DefinesMap* theDefinesMap);
	virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue);
	virtual void			Init();
	virtual bool			CheckSignature(const Buffer& theBuffer, const std::string& theFileName);
	virtual void			InitPropertiesHook();

	// New virtuals (slots 97-102)
	virtual bool			ShouldCheckForUpdate();
	virtual void			UpdateCheckQueried();
	// MSVC emits overloads in reverse declaration order: () is slot 99, (DefinesMap) slot 100.
	virtual bool			OpenRegisterPage(DefinesMap theStatsMap);
	virtual bool			OpenRegisterPage();
	virtual bool			OpenHTMLTemplate(const std::string& theTemplateFile, const DefinesMap& theDefinesMap);
	virtual void			OpenUpdateURL();

	bool					Validate(const std::string& theUserName, const std::string& theRegCode);

};

extern SexyApp* gSexyApp;

}

#endif // __SEXYAPP_H__
