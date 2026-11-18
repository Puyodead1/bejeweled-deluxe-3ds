#ifndef __HIGHSCOREWIDGET_H__
#define __HIGHSCOREWIDGET_H__

#include <string>
#include "SexyAppFramework/Widget.h"
#include "SexyAppFramework/ButtonListener.h"
#include "decomp.h"

namespace Sexy
{

class WinDMApp;
class HyperlinkWidget;
class MemoryImage;
class DDImage;

// RTTI: .?AVHighScoreWidget@Sexy@@ - the game over / high score screen, size 0x20c.
// It slides down over a snapshot of the board, then asks for the player's name.
// VTABLE: GAME 0x4c6acc Sexy::Widget
// VTABLE: GAME 0x4c6ab4 Sexy::ButtonListener
// VTABLE: GAME187 0x506cb8 Sexy::ButtonListener
// VTABLE: GAME187 0x506cdc Sexy::Widget
class HighScoreWidget : public Widget, public ButtonListener
{
public:
	HighScoreWidget(WinDMApp* theApp);
	virtual ~HighScoreWidget();

	virtual void AddedToManager(WidgetManager* theWidgetManager);
	virtual void RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void Draw(Graphics* g);
	virtual void Update();
	virtual void ShowScoreMessage(); // vtable+0xb0
	virtual void SubmitScore(const std::string& theName); // vtable+0xb4
	virtual void ButtonDepress(int theId);

	int GetDay();
	int GetRank(int theMode, int theScore);
	void Load();
	void Save();
	std::string GetOrdinal(int theNum);
	void DoShowScoreMessage();

	// 0x58: ButtonListener vtable
	WinDMApp* mApp;                     // 0x05c
	int mSlideY;                        // 0x060  starts at -height, slides to 0
	HyperlinkWidget* mPlayAgainLink;    // 0x064
	DDImage* mBoardImage;               // 0x068  snapshot of the board taken on Resize
	std::string mTip;                   // 0x06c
	std::string mTodayNames[2];         // 0x088  [0] = normal, [1] = time trial
	int mTodayScores[2];                // 0x0c0
	std::string mNames[2][5];           // 0x0c8  all-time table
	int mScores[2][5];                  // 0x1e0
	int mColorCycle;                    // 0x208
};

}

#endif // __HIGHSCOREWIDGET_H__