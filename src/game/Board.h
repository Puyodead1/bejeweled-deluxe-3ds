#ifndef __BOARD_H__
#define __BOARD_H__

#include <string>
#include <list>
#include "win32compat.h"
#include "SexyAppFramework/Widget.h"
#include "decomp.h"

namespace Sexy
{

class WinDMApp;
class Image;
class Font;
class MemoryImage;
class DDImage;
class Graphics;
class SoundInstance;

// A gem on the board. Not polymorphic; size 0x78 or more.
class Gem
{
public:
	Gem();

	int mColor;                      // 0x00
	undefined m_unk0x04[0x08 - 0x04];
	double mX;                       // 0x08
	double mY;                       // 0x10
	int mCol;                        // 0x18
	int mRow;                        // 0x1c
	int m_unk0x20;                   // 0x20
	undefined m_unk0x24[0x28 - 0x24];
	double m_unk0x28;                // 0x28
	int mSwapOffsetX;                // 0x30  offset from the swap centre
	int mSwapOffsetY;                // 0x34
	bool m_unk0x38;                  // 0x38
	undefined m_unk0x39[0x40 - 0x39];
	double m_unk0x40;                // 0x40
	int m_unk0x48;                   // 0x48
	bool m_unk0x4c;                  // 0x4c
	undefined m_unk0x4d[0x50 - 0x4d];
	int m_unk0x50;                   // 0x50
	int m_unk0x54;                   // 0x54
	int m_unk0x58;                   // 0x58
	int m_unk0x5c;                   // 0x5c
	double mAngle;                   // 0x60  from the board centre, for the game over explosion
	double m_unk0x68;                // 0x68
	double mDist;                    // 0x70  from the board centre
};

// A floating score shown where a match was made. Size 0x14.
struct ScorePopup
{
	int mX;                          // 0x00
	int mY;                          // 0x04
	int mTimer;                      // 0x08
	int mColor;                      // 0x0c
	int mPoints;                     // 0x10
};

typedef std::list<ScorePopup> ScorePopupList;

// RTTI: .?AVBoard@Sexy@@ - the game board widget, size 0x2e8.
// VTABLE: GAME 0x4c646c
// VTABLE: GAME187 0x5065cc
class Board : public Widget
{
public:
	Board(WinDMApp* theApp, bool timeTrial);
	virtual ~Board();

	virtual void Draw(Graphics* g);
	virtual void Update();
	virtual void KeyChar(char theChar);
	virtual void MouseLeave();
	virtual void MouseMove(int x, int y);
	virtual void MouseDown(int x, int y, int theClickCount);
	virtual void MouseUp(int x, int y, int theClickCount);
	virtual void MouseDrag(int x, int y);

	int GetColX(int theCol);
	int GetRowY(int theRow);
	void EndGame(int theDelay);
	void FUN_004010cf();
	bool CanPlay();
	void Pause(bool userPause);
	void Unpause(bool userPause);
	Gem* GetGemAt(int x, int y);
	bool HasMatch();
	int GetPan(int x);
	bool FindMove(int* theMove, int theSkip, bool horizontal, bool vertical);
	void TrySwap(Gem* theGem1, Gem* theGem2);
	void GetBoardSize(int* theWidth, int* theHeight);
	void GetBoardCenter(int* theX, int* theY);
	void SetMessage(const std::string& theText, int theDuration);
	void UpdateMessageFx();
	void StartExplode();
	void UpdateCollapse();
	void UpdateGameOver();
	void ShowHint();
	void MakeBackground();
	void FillBoard();
	void UpdateLevelTransition();
	void UpdateDropIn();
	void Hint();

	bool HasSavedGame();
	bool LoadGame();
	bool SaveGame();
	bool DeleteSavedGame();
	void UpdateLevelUp();
	bool ProcessMatches();
	void UpdateFalling();
	void UpdateSwap();

	WinDMApp* mApp;                  // 0x58
	int m_unk0x5c;                   // 0x5c  level
	int m_unk0x60;                   // 0x60
	Gem* mBoard[8][8];               // 0x64  [row][col]
	int mState;                      // 0x164  1 = idle, 2 = swapping, 5/6 = game over, 7 = exploding
	bool mTimeTrial;                 // 0x168
	undefined m_unk0x169[0x16c - 0x169];
	SoundInstance* m_unk0x16c;       // 0x16c
	bool m_unk0x170;                 // 0x170
	undefined m_unk0x171[0x174 - 0x171];
	Gem* mSelectedGem;               // 0x174
	int m_unk0x178;                  // 0x178
	bool mHintActive;                // 0x17c  tutorial hint showing
	undefined m_unk0x17d[0x180 - 0x17d];
	int mHintBounce;                 // 0x180
	int mHintBounceDir;              // 0x184
	int mHintCol1;                   // 0x188
	int mHintRow;                    // 0x18c
	int mHintCol2;                   // 0x190
	Gem* mSwapGem1;                  // 0x194
	Gem* mSwapGem2;                  // 0x198
	int mSwapPct;                    // 0x19c
	int mSwapCenterX;                // 0x1a0
	int mSwapCenterY;                // 0x1a4
	int m_unk0x1a8;                  // 0x1a8
	DWORD mStartTick;                // 0x1ac  update count when the clock was last started
	int mElapsedTime;                // 0x1b0  seconds before mStartTick
	int mGameTime;                   // 0x1b4  seconds played
	int m_unk0x1b8;                  // 0x1b8  reset when a hint is shown
	int mScore;                      // 0x1bc
	double m_unk0x1c0;               // 0x1c0  hint penalty factor
	int m_unk0x1c8;                  // 0x1c8
	int mGemsCleared;                // 0x1cc
	int m_unk0x1d0;                  // 0x1d0
	int m_unk0x1d4;                  // 0x1d4  incremented after each collapse
	int mLastLandSoundTick;          // 0x1d8
	int m_unk0x1dc;                  // 0x1dc  game over delay
	int mLongestCascade;             // 0x1e0
	int mBestMove;                   // 0x1e4
	int m_unk0x1e8;                  // 0x1e8
	int mCascadeScore;               // 0x1ec  points scored in the current cascade
	int m_unk0x1f0;                  // 0x1f0  -1 when unused
	undefined m_unk0x1f4[0x1f8 - 0x1f4];
	double m_unk0x1f8;               // 0x1f8
	double mFlash;                   // 0x200
	int mFlashTime;                  // 0x208
	int mFlashDuration;              // 0x20c
	bool m_unk0x210;                 // 0x210
	undefined m_unk0x211[0x214 - 0x211];
	int mPauseCount;                 // 0x214
	int mUserPauseCount;             // 0x218
	ScorePopupList mScorePopups;     // 0x21c
	Font* mPopupFont;                // 0x228
	Font* mScoreFont;                // 0x22c
	bool m_unk0x230;                 // 0x230
	undefined m_unk0x231[0x234 - 0x231];
	int m_unk0x234;                  // 0x234
	int m_unk0x238;                  // 0x238
	int m_unk0x23c;                  // 0x23c
	int m_unk0x240;                  // 0x240
	undefined m_unk0x244[0x248 - 0x244];
	double m_unk0x248;               // 0x248
	double m_unk0x250;               // 0x250
	double m_unk0x258;               // 0x258
	int m_unk0x260;                  // 0x260  time bar image
	DWORD mLastTickSound;            // 0x264
	int mMouseX;                     // 0x268
	int mMouseY;                     // 0x26c
	int mDownX;                      // 0x270
	int mDownY;                      // 0x274
	bool mMouseDown;                 // 0x278
	undefined m_unk0x279[0x27c - 0x279];
	int mClickDelay;                 // 0x27c  clicks ignored while > 0 (after unpausing)
	DDImage* m_unk0x280;             // 0x280
	DDImage* m_unk0x284;             // 0x284
	bool m_unk0x288;                 // 0x288  background built
	undefined m_unk0x289[0x28c - 0x289];
	MemoryImage* mMessageImage;      // 0x28c
	MemoryImage* mMessageFxImage;    // 0x290
	bool mShowMessage;               // 0x294
	undefined m_unk0x295[0x298 - 0x295];
	std::string mMessage;            // 0x298
	undefined m_unk0x2b4[0x2b8 - 0x2b4];
	double mMessagePhase;            // 0x2b8
	int mMessageTime;                // 0x2c0
	int mMessageDuration;            // 0x2c4
	int m_unk0x2c8;                  // 0x2c8  level transition wipe position
	int m_unk0x2cc;                  // 0x2cc
	int m_unk0x2d0;                  // 0x2d0
	int m_unk0x2d4;                  // 0x2d4
	double m_unk0x2d8;               // 0x2d8
	bool m_unk0x2e0;                 // 0x2e0
	undefined m_unk0x2e1[0x2e8 - 0x2e1];
};

}

#endif // __BOARD_H__