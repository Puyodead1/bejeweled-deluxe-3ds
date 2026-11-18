#ifndef __HIGHSCOREDIALOG_H__
#define __HIGHSCOREDIALOG_H__

#include "SexyAppFramework/Dialog.h"
#include "decomp.h"

namespace Sexy
{

class WinDMApp;
class HighScoreWidget;

// RTTI: .?AVHighScoreDialog@Sexy@@ - the high score table dialog, size 0x118.

// VTABLE: GAME 0x4c66dc Sexy::Widget
// VTABLE: GAME 0x4c66c4 Sexy::ButtonListener
// VTABLE: GAME187 0x50684c Sexy::ButtonListener
// VTABLE: GAME187 0x50686c Sexy::Widget
class HighScoreDialog : public Dialog
{
public:
	HighScoreDialog(WinDMApp* theApp);
	virtual ~HighScoreDialog();

	virtual void Draw(Graphics* g);
	virtual int GetPreferredHeight(int theWidth);

	HighScoreWidget* mHighScoreWidget; // 0x110  only used for its score tables
	WinDMApp* mApp;                    // 0x114
};

}

#endif // __HIGHSCOREDIALOG_H__