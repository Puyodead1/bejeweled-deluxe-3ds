#ifndef __BETASUPPORT_H__
#define __BETASUPPORT_H__

#include "decomp.h"
#include "HTTPTransfer.h"
#include "Buffer.h"
#include <windows.h>
#include <string>
#include <map>

namespace Sexy
{

class SexyApp;

typedef std::map<std::string, Buffer> BetaBufferMap;

// VTABLE: GAME187 0x510bc0
// SIZE 0x1ac
class BetaSupport
{
public:
	SexyApp* mApp;                       // 0x004
	HFONT mNormalFont;                   // 0x008
	HFONT mSmallFont;                    // 0x00c
	HFONT mBoldFont;                     // 0x010
	int mTickCount;                      // 0x014
	HWND mHWnd;                          // 0x018
	HWND mStatusHWnd;                    // 0x01c
	HWND mUserNameHWnd;                  // 0x020
	HWND mPasswordHWnd;                  // 0x024
	HTTPTransfer mTransfer;              // 0x028
	bool mCancelled;                     // 0x124
	bool mDone;                          // 0x125
	std::string mCommentURL;             // 0x12c
	std::string m_unk0x148;              // 0x148
	std::string mUserName;               // 0x164
	std::string mPassword;               // 0x180
	bool mValidated;                     // 0x19c
	BetaBufferMap mBufferMap;            // 0x1a0

protected:
	void CenterWindow(HWND theHWnd); // 0x446ca0
	void MessageLoop(); // 0x446d20
	void LoadFromRegistry(); // 0x44eb20
	void SaveToRegistry(); // 0x4478e0
	bool Login(); // 0x44ec30
	bool CheckValidation(); // 0x455e00

public:
	BetaSupport(SexyApp* theApp); // 0x45adc0
	virtual ~BetaSupport(); // 0x45a5e0

	bool Validate(); // 0x456af0
};

}

#endif // __BETASUPPORT_H__
