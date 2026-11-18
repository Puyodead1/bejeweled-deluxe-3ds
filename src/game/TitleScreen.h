#ifndef __TITLESCREEN_H__
#define __TITLESCREEN_H__

#include "SexyAppFramework/Widget.h"
#include "SexyAppFramework/ButtonListener.h"
#include "decomp.h"

namespace Sexy
{

class WinDMApp;
class HyperlinkWidget;

// RTTI: .?AVTitleScreen@Sexy@@ - the loading/title screen, size 0x88. Switch port: without the trial's forced delay,
// play count, nag line and register link (the port is the full game).
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
	virtual void Draw(Graphics* g);
	virtual void Update();
	virtual void ButtonDepress(int theId);

	// Loading has finished and the play link is up.
	bool CanStart() { return mLoadingComplete; }

	WinDMApp* mApp;
	double mCurBarWidth;                // loading bar fill, 0..1
	bool mLoadingComplete;
	HyperlinkWidget* mStartLink;
};

}

#endif // __TITLESCREEN_H__
