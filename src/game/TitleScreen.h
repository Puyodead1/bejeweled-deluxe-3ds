#ifndef __TITLESCREEN_H__
#define __TITLESCREEN_H__

#include "SexyAppFramework/Widget.h"
#include "SexyAppFramework/ButtonListener.h"
#include "decomp.h"

namespace Sexy
{

class WinDMApp;
class HyperlinkWidget;

// RTTI: .?AVTitleScreen@Sexy@@ - the loading/title screen with the trial nag, size 0x88.
// VTABLE: GAME 0x4c72b4 Sexy::Widget
// VTABLE: GAME 0x4c72a0 Sexy::ButtonListener
// VTABLE: GAME187 0x507650 Sexy::ButtonListener
// VTABLE: GAME187 0x507674 Sexy::Widget
class TitleScreen : public Widget, public ButtonListener
{
public:
	TitleScreen(WinDMApp* theApp);
	virtual ~TitleScreen();

	virtual void AddedToManager(WidgetManager* theWidgetManager);
	virtual void RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void Draw(Graphics* g);
	virtual void Update();
	virtual void ButtonDepress(int theId);

	// 0x58: ButtonListener vtable
	WinDMApp* mApp;                     // 0x5c
	double mCurBarWidth;                // 0x60  loading bar fill, 0..1
	double mTimerProgress;              // 0x68  forced delay for unregistered players, 0..1
	bool mLoadingComplete;              // 0x70
	int m_unk0x74;                      // 0x74
	bool mRegisterClicked;              // 0x78
	bool mDemoExpired;                  // 0x79
	int mTotalTimerSec;                 // 0x7c  property "TotalTimerSec"
	HyperlinkWidget* mRegisterLink;     // 0x80
	HyperlinkWidget* mStartLink;        // 0x84
};

}

#endif // __TITLESCREEN_H__