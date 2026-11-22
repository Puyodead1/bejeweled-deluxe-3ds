#include "Board.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WinDMApp.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Buffer.h"
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/Common.h"
#include "SexyAppFramework/DDImage.h"
#include "SexyAppFramework/Font.h"
#include "SexyAppFramework/ImageFont.h"
#include "SexyAppFramework/MemoryImage.h"
#include "SexyAppFramework/Rect.h"
#include "SexyAppFramework/SoundManager.h"
#include "SexyAppFramework/SoundInstance.h"

using namespace Sexy;

// FUNCTION: GAME 0x401010
// FUNCTION: GAME187 0x4c9aec
Gem::Gem()
{
	mX = 0.0;
	mColor = 0;
	mY = 0.0;
	mCol = 0;
	mRow = 0;
	mSwapOffsetX = 0;
	m_unk0x28 = 0.0;
	mSwapOffsetY = 0;
	m_unk0x50 = 0;
	m_unk0x20 = 0;
	m_unk0x40 = 0.0;
	m_unk0x4c = false;
	m_unk0x38 = false;
	m_unk0x48 = 0;
	m_unk0x54 = 0;
	m_unk0x58 = 0;
	m_unk0x5c = 0;
}

// FUNCTION: GAME 0x40104f
// FUNCTION: GAME187 0x4c9b25
int Board::GetColX(int theCol)
{
	return 51 * theCol + 203;
}

// FUNCTION: GAME 0x401061
// FUNCTION: GAME187 0x4c9b37
int Board::GetRowY(int theRow)
{
	return 51 * theRow + 16;
}

// FUNCTION: GAME 0x401071
// FUNCTION: GAME187 0x4c9b47
void Board::EndGame(int theDelay)
{
	mSelectedGem = NULL;
	mHintActive = false;
	mApp->KillDialog(9);
	mState = 5;
	mGameTime = mElapsedTime + (mUpdateCnt - mStartTick) / 100;
	m_unk0x1dc = theDelay;
	mApp->mTimesPlayed++;
}

// FUNCTION: GAME 0x4010cf
// FUNCTION: GAME187 0x4c9bab
void Board::FUN_004010cf()
{
	EndGame(50);
	m_unk0x210 = true;
}

// FUNCTION: GAME 0x4010e2
// FUNCTION: GAME187 0x4c9bbe
bool Board::CanPlay()
{
	return (!m_unk0x170) && (mState != 5) && (mState != 6);
}

// FUNCTION: GAME 0x4010fe
// FUNCTION: GAME187 0x4c9bda
void Board::Pause(bool userPause)
{
	if (mPauseCount == 0)
	{
		mElapsedTime += (mUpdateCnt - mStartTick) / 100;
		MarkDirty();
	}

	if (userPause)
	{
		if (mUserPauseCount == 0)
			MarkDirty();
		mUserPauseCount++;
	}

	mPauseCount++;
}

// FUNCTION: GAME 0x401157
// FUNCTION: GAME187 0x4c9c2d
void Board::Unpause(bool userPause)
{
	mClickDelay = 10;

	if ((userPause) && (mUserPauseCount > 0))
	{
		mUserPauseCount--;
		if (mUserPauseCount <= 0)
			MarkDirty();
	}

	if (mPauseCount > 0)
	{
		mPauseCount--;
		if (mPauseCount == 0)
			mStartTick = mUpdateCnt;
	}
}

// FUNCTION: GAME 0x4011ab
// FUNCTION: GAME187 0x4c9c7c
Gem* Board::GetGemAt(int x, int y)
{
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if ((aGem != NULL) && (x >= aGem->mX) && (x < aGem->mX + 49) && (y >= aGem->mY) && (y < aGem->mY + 49))
				return aGem;
		}
	}
	return NULL;
}

// FUNCTION: GAME 0x401227
// FUNCTION: GAME187 0x4c9cf5
bool Board::HasMatch()
{
	for (int aRow = 0; aRow < 8; aRow++)
	{
		int aCount = 0;
		int aColor = -1;
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if (aGem != NULL)
			{
				if (aGem->mColor == aColor)
				{
					if (++aCount == 3)
						return true;
				}
				else
				{
					aColor = aGem->mColor;
					aCount = 1;
				}
			}
			else
				aColor = -1;
		}
	}

	for (int aCol = 0; aCol < 8; aCol++)
	{
		int aCount = 0;
		int aColor = -1;
		for (int aRow = 0; aRow < 8; aRow++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if (aGem != NULL)
			{
				if (aGem->mColor == aColor)
				{
					if (++aCount == 3)
						return true;
				}
				else
				{
					aColor = aGem->mColor;
					aCount = 1;
				}
			}
			else
				aColor = -1;
		}
	}

	return false;
}

// FUNCTION: GAME 0x4012c3
// FUNCTION: GAME187 0x4c9d94
int Board::GetPan(int x)
{
	int aLeft = GetColX(0);
	int aWidth = GetColX(7) + 49 - aLeft;
	x = x - aWidth / 2 - aLeft;
	return (int)((x / (double)aWidth * 2) * 800);
}

// FUNCTION: GAME 0x401309
// FUNCTION: GAME187 0x4c9dda
bool Board::FindMove(int* theMove, int theSkip, bool horizontal, bool vertical)
{
	int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	int aCount = 0;

	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			if (mBoard[aRow][aCol] == NULL)
				continue;

			for (int aDir = 0; aDir < 4; aDir++)
			{
				int aNewCol = aCol + aDirs[aDir][0];
				int aNewRow = aRow + aDirs[aDir][1];
				if ((aNewCol >= 0) && (aNewCol < 8) && (aNewRow >= 0) && (aNewRow < 8))
				{
					Gem* aTemp = mBoard[aRow][aCol];
					mBoard[aRow][aCol] = mBoard[aNewRow][aNewCol];
					mBoard[aNewRow][aNewCol] = aTemp;

					int aLeft = aCol;
					int aRight = aCol;
					while ((aLeft > 0) && (mBoard[aRow][aLeft - 1] != NULL) && (mBoard[aRow][aCol]->mColor == mBoard[aRow][aLeft - 1]->mColor))
						aLeft--;
					while ((aRight < 7) && (mBoard[aRow][aLeft + 1] != NULL) && (mBoard[aRow][aCol]->mColor == mBoard[aRow][aRight + 1]->mColor))
						aRight++;
					int aTop = aRow;
					int aBottom = aRow;
					while ((aTop > 0) && (mBoard[aTop - 1][aCol] != NULL) && (mBoard[aRow][aCol]->mColor == mBoard[aTop - 1][aCol]->mColor))
						aTop--;
					while ((aBottom < 7) && (mBoard[aTop + 1][aCol] != NULL) && (mBoard[aRow][aCol]->mColor == mBoard[aBottom + 1][aCol]->mColor))
						aBottom++;

					aTemp = mBoard[aRow][aCol];
					mBoard[aRow][aCol] = mBoard[aNewRow][aNewCol];
					mBoard[aNewRow][aNewCol] = aTemp;

					if (((aRight - aLeft >= 2) && (horizontal)) || ((aBottom - aTop >= 2) && (vertical)))
					{
						if (aCount == theSkip)
						{
							if (theMove != NULL)
							{
								theMove[0] = aCol;
								theMove[1] = aRow;
								theMove[2] = aNewCol;
								theMove[3] = aNewRow;
							}
							return true;
						}
						aCount++;
					}
				}
			}
		}
	}

	return false;
}

// FUNCTION: GAME 0x401510
// FUNCTION: GAME187 0x4c9fd0
void Board::MouseMove(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
}

// FUNCTION: GAME 0x401529
// FUNCTION: GAME187 0x4c9fe9
void Board::MouseLeave()
{
	mMouseX = -1;
	mMouseY = -1;
}

// FUNCTION: GAME 0x401538
// FUNCTION: GAME187 0x4c9ff8
void Board::TrySwap(Gem* theGem1, Gem* theGem2)
{
	if (abs(theGem1->mCol - theGem2->mCol) + abs(theGem1->mRow - theGem2->mRow) == 1)
	{
		if ((mHintActive) && (theGem1->mCol == mHintCol1) && (theGem1->mRow == mHintRow) && (theGem2->mCol == mHintCol2) && (theGem2->mRow == mHintRow))
		{
			mApp->m_unk0x735 = false;
			mHintActive = false;
			mApp->KillDialog(9);
		}

		mSwapPct = 0;
		mSwapGem1 = theGem1;
		mSwapGem2 = theGem2;
		mSwapCenterX = (int)((theGem2->mX + theGem1->mX) * 0.5);
		mSwapCenterY = (int)((theGem1->mY + theGem2->mY) * 0.5);
		mSwapGem1->mSwapOffsetX = (int)(mSwapGem1->mX - mSwapCenterX);
		mSwapGem1->mSwapOffsetY = (int)(mSwapGem1->mY - mSwapCenterY);
		mSwapGem2->mSwapOffsetX = (int)(mSwapGem2->mX - mSwapCenterX);
		mSwapGem2->mSwapOffsetY = (int)(mSwapGem2->mY - mSwapCenterY);
		mState = 2;
	}
	else
	{
		mApp->PlaySample(7);
	}
}

// FUNCTION: GAME 0x40167a
// FUNCTION: GAME187 0x4ca13e
void Board::GetBoardSize(int* theWidth, int* theHeight)
{
	*theWidth = GetColX(7) + 49 - GetColX(0);
	*theHeight = GetRowY(7) + 49 - GetRowY(0);
}

// FUNCTION: GAME 0x4016b5
// FUNCTION: GAME187 0x4ca179
void Board::GetBoardCenter(int* theX, int* theY)
{
	int aLeft = GetColX(0);
	*theX = aLeft + (GetColX(7) + 49 - aLeft) / 2;
	int aTop = GetRowY(0);
	*theY = aTop + (GetRowY(7) + 49 - aTop) / 2;
}

// FUNCTION: GAME 0x401704
// FUNCTION: GAME187 0x4ca1c8
void Board::SetMessage(const std::string& theText, int theDuration)
{
	delete mMessageImage;
	delete mMessageFxImage;

	mMessageImage = new MemoryImage(mApp);
	ImageFont* aFont = mApp->m_unk0x694;
	mMessageImage->Create(mApp->m_unk0x694->StringWidth(theText), mApp->m_unk0x694->GetHeight());
	mMessageImage->SetImageMode(true, true);

	mMessageFxImage = new MemoryImage(mApp);
	mMessageFxImage->Create(mMessageImage->GetWidth(), mMessageImage->GetHeight() + 48);
	mMessageFxImage->SetImageMode(true, true);

	Graphics g(mMessageImage);
	g.SetColor(Color(255, 255, 255));
	g.SetFont(mApp->m_unk0x694);
	g.DrawString(theText, 0, mApp->m_unk0x694->GetAscent());

	mMessagePhase = 0.0;
	mMessageTime = theDuration;
	mMessageDuration = theDuration;
	mShowMessage = true;
}

// FUNCTION: GAME 0x401896
// FUNCTION: GAME187 0x4ca36e
void Board::UpdateMessageFx()
{
	uint32_t* aBits = mMessageImage->GetBits();
	uint32_t* aFxBits = mMessageFxImage->GetBits();
	int aWidth = mMessageImage->GetWidth();
	int aHeight = mMessageImage->GetHeight();
	int aFxHeight = mMessageFxImage->GetHeight();

	memset(aFxBits, 0, aWidth * aFxHeight * 4);

	int anAlpha = 255;
	if (mMessageTime - mMessageDuration < 75)
		anAlpha = 255 * (mMessageTime - mMessageDuration) / 75;
	else if (mMessageDuration < 75)
		anAlpha = 255 * mMessageDuration / 75;

	uint32_t* aColors = new uint32_t[mMessageFxImage->GetHeight()];
	int i;
	for (i = 0; i < aFxHeight; i++)
		aColors[i] = mApp->HSLToRGB((i * 4) % 256, 255, 148);

	for (i = 0; i < aWidth; i++)
	{
		int anOffset = (int)((sin(i * 0.024 + mMessagePhase) + 1.0) * 24.0);
		uint32_t* aSrc = aBits + i;
		uint32_t* aDest = aFxBits + anOffset * aWidth + i;
		for (int y = 0; y < aHeight; y++)
		{
			uint32_t aSrcColor = *aSrc;
			uint32_t aColorizer = aColors[y + anOffset];
			*aDest = ((((aSrcColor & 0xFF000000) >> 8) * anAlpha) & 0xFF000000) |
				(((((aSrcColor & 0x00FF0000) * ((aColorizer >> 16) & 0xFF)) & 0xFF000000) |
				(((aSrcColor & 0x0000FF00) * ((aColorizer >> 8) & 0xFF)) & 0x00FF0000) |
				(((aSrcColor & 0x000000FF) * (aColorizer & 0xFF)) & 0x0000FF00)) >> 8);
			aSrc += aWidth;
			aDest += aWidth;
		}
	}

	delete aColors;
	mMessageFxImage->BitsChanged();
}

// FUNCTION: GAME 0x401a9d
// FUNCTION: GAME187 0x4ca58d
void Board::StartExplode()
{
	if (m_unk0x16c != NULL)
		m_unk0x16c->Release();
	m_unk0x16c = mApp->mSoundManager->GetSoundInstance(19);
	if (m_unk0x16c != NULL)
		m_unk0x16c->Play(true, false);

	m_unk0x2e0 = false;
	m_unk0x2d8 = 0.0;
	m_unk0x2d0 = 0;
	mSelectedGem = NULL;

	int aCenterX;
	int aCenterY;
	GetBoardCenter(&aCenterX, &aCenterY);

	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if (aGem != NULL)
			{
				float aDX = (int)(aGem->mX + 24.0 - aCenterX);
				int aDY = (int)(aGem->mY + 24.0 - aCenterY);
				aGem->mAngle = atan2((float)aDY, aDX);
				aGem->mDist = sqrt(aDX * aDX + aDY * aDY);
				aGem->m_unk0x68 = 0.0;
			}
		}
	}

	mState = 7;
}

// FUNCTION: GAME 0x401bab
// FUNCTION: GAME187 0x4ca6b0
void Board::MouseDown(int x, int y, int theClickCount)
{
	if (mUserPauseCount != 0)
	{
		Unpause(true);
		return;
	}

	if (mClickDelay > 0)
		return;

	mMouseDown = true;
	mDownX = x;
	mDownY = y;

	if ((mState == 5) || (mState == 6) || (mState == 7) || (mState == 8))
		return;

	if (theClickCount > 0)
	{
		Gem* aGem = GetGemAt(x, y);
		if (mSelectedGem == NULL)
		{
			if (aGem != NULL)
			{
				m_unk0x178 = 0;
				mSelectedGem = aGem;
				MarkDirty();
				mApp->PlaySample(11, GetPan((int)(mSelectedGem->mX + 24.0)));
			}
			return;
		}

		if (mState != 1)
			return;

		int aDX = (int)(x - (mSelectedGem->mX + 24.0));
		int aDY = (int)(y - (mSelectedGem->mY + 24.0));
		int aCol = mSelectedGem->mCol;
		int aRow = mSelectedGem->mRow;
		int aMax = max(abs(aDX), abs(aDY));
		int aMin = min(abs(aDX), abs(aDY));
		if ((aMax >= 22) && (aMax <= 88.2) && (aMin <= 26))
		{
			if (abs(aDX) > abs(aDY))
			{
				if (aDX < 0)
					aCol--;
				else
					aCol++;
			}
			else
			{
				if (aDY < 0)
					aRow--;
				else
					aRow++;
			}

			if ((aCol >= 0) && (aCol < 8) && (aRow >= 0) && (aRow < 8))
				aGem = mBoard[aRow][aCol];
		}

		if ((aGem != NULL) && (aGem != mSelectedGem))
			TrySwap(mSelectedGem, aGem);
		mSelectedGem = NULL;
	}
	else
	{
		mSelectedGem = NULL;
	}

	MarkDirty();
}

// FUNCTION: GAME 0x401dba
// FUNCTION: GAME187 0x4ca8b7
void Board::MouseUp(int x, int y, int theClickCount)
{
	mMouseDown = false;
}

// FUNCTION: GAME 0x401dc4
// FUNCTION: GAME187 0x4ca8c1
void Board::MouseDrag(int x, int y)
{
	if ((mMouseDown) && (mSelectedGem != NULL) && (mState == 1))
	{
		int aDX = x - mDownX;
		int aDY = y - mDownY;
		if ((abs(aDX) >= 16) || (abs(aDY) >= 16))
		{
			if (abs(aDX) > abs(aDY))
			{
				if ((aDX > 0) && (mSelectedGem->mCol < 7))
					TrySwap(mSelectedGem, mBoard[mSelectedGem->mRow][mSelectedGem->mCol + 1]);
				else if ((aDX < 0) && (mSelectedGem->mCol > 0))
					TrySwap(mSelectedGem, mBoard[mSelectedGem->mRow][mSelectedGem->mCol - 1]);
			}
			else
			{
				if ((aDY > 0) && (mSelectedGem->mRow < 7))
					TrySwap(mSelectedGem, mBoard[mSelectedGem->mRow + 1][mSelectedGem->mCol]);
				else if ((aDY < 0) && (mSelectedGem->mRow > 0))
					TrySwap(mSelectedGem, mBoard[mSelectedGem->mRow - 1][mSelectedGem->mCol]);
			}

			mSelectedGem = NULL;
			MarkDirty();
		}
	}
}

// FUNCTION: GAME 0x401ed3
// FUNCTION: GAME187 0x4ca9d3
void Board::KeyChar(char theChar)
{
	if (mUserPauseCount > 0)
		Unpause(true);
	else
		Pause(true);
}

// FUNCTION: GAME 0x401eed
// FUNCTION: GAME187 0x4ca9ed
void Board::UpdateCollapse()
{
	bool aDone = true;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if ((aGem != NULL) && (aGem->m_unk0x4c))
			{
				if (mUpdateCnt % 3 == 0)
				{
					aGem->m_unk0x50++;
					if (aGem->m_unk0x50 >= 6)
					{
						if (aGem == mSelectedGem)
							mSelectedGem = NULL;
						delete aGem;
						mBoard[aRow][aCol] = NULL;
					}
					MarkDirty();
				}
				aDone = false;
			}
		}
	}

	if (aDone)
	{
		int aTopY = GetRowY(0);
		bool aChanged;
		do
		{
			aChanged = false;
			for (int aCol = 0; aCol < 8; aCol++)
			{
				bool aHole = false;
				int aMinY = aTopY;
				for (int aRow = 7; aRow >= 0; aRow--)
				{
					Gem* aGem = mBoard[aRow][aCol];
					if (aGem != NULL)
					{
						if (aMinY > aGem->mY)
							aMinY = (int)aGem->mY;
						if (aHole)
						{
							aGem->mRow++;
							aGem->m_unk0x28 = 1.0;
							mBoard[aRow + 1][aCol] = aGem;
						}
					}
					else
					{
						if (aHole)
							mBoard[aRow + 1][aCol] = NULL;
						aHole = true;
					}
				}

				if (aHole)
				{
					aChanged = true;
					Gem* aGem = new Gem();
					aGem->m_unk0x28 = 0.3;
					aGem->mRow = 0;
					aGem->mCol = aCol;
					aGem->mX = GetColX(aCol);
					aGem->mY = aMinY - 69;
					aGem->mColor = Rand() % 7;
					mBoard[0][aCol] = aGem;
				}
			}
		} while (aChanged);

		MarkDirty();
		mState = 0;
		m_unk0x1d4++;
	}
}

// FUNCTION: GAME 0x402085
// FUNCTION: GAME187 0x4cab82
void Board::UpdateGameOver()
{
	MarkDirty();

	if (m_unk0x1dc > 0)
	{
		if (--m_unk0x1dc == 0)
			mApp->PlaySample(9);

		if ((mUpdateCnt % 6 == 0) || (m_unk0x1dc == 0))
		{
			for (int aRow = 0; aRow < 8; aRow++)
			{
				for (int aCol = 0; aCol < 8; aCol++)
				{
					Gem* aGem = mBoard[aRow][aCol];
					if (aGem != NULL)
					{
						aGem->mX = GetColX(aGem->mCol) + Rand() % 9 - 4;
						aGem->mY = GetRowY(aGem->mRow) + Rand() % 9 - 4;
						if (m_unk0x1dc == 0)
						{
							aGem->m_unk0x28 = Rand() % 8 - 6;
							aGem->m_unk0x20 = Rand() % 7 - 3;
						}
					}
				}
			}
		}
	}
	else
	{
		bool aDone = true;
		for (int aRow = 0; aRow < 8; aRow++)
		{
			for (int aCol = 0; aCol < 8; aCol++)
			{
				Gem* aGem = mBoard[aRow][aCol];
				if (aGem != NULL)
				{
					aGem->mX += aGem->m_unk0x20;
					aGem->mY += aGem->m_unk0x28;
					aGem->m_unk0x28 += 0.15;
					if (mHeight + 49 > aGem->mY)
						aDone = false;
				}
			}
		}

		if (aDone)
		{
			if (m_unk0x210)
			{
				mApp->NewBoard();
			}
			else
			{
				mApp->PlaySample(16);
				mState = 6;
				mApp->ShowHighScores();
			}
		}
	}
}

// FUNCTION: GAME 0x40221e
// FUNCTION: GAME187 0x4cad2f
void Board::ShowHint()
{
	mScore = (int)(mScore - m_unk0x1c0 * 50.0);
	if (mScore < 0)
		mScore = 0;

	int aMove[4];
	if (FindMove(aMove, 0, true, true))
	{
		Gem*& aGem = mBoard[aMove[3]][aMove[2]];
		aGem->m_unk0x58 = 58;
		aGem->m_unk0x5c = 0;
		aGem->m_unk0x54 = 1;
	}

	m_unk0x1b8 = 0;
}
// GLOBAL: GAME 0x4c6390
static const int gBackgroundColors[4][6] = {
	{102, 82, 70, 102, 82, 70},
	{130, 72, 72, 130, 72, 72},
	{72, 130, 72, 72, 130, 72},
	{72, 72, 130, 72, 72, 130}
};

// FUNCTION: GAME 0x40231f
// FUNCTION: GAME187 0x4cae29
void Board::MakeBackground()
{
	Graphics g(m_unk0x284);
	int aColorSet = m_unk0x5c % 4;
	int aTileSet = m_unk0x5c % 5;
	m_unk0x288 = true;

	MemoryImage aTile1(mApp);
	aTile1.Create(51, 51);
	Graphics aTileG1(&aTile1);
	aTileG1.DrawImage(mApp->mImages[28], 0, 0, Rect(aTileSet * 102, 0, 51, 51));

	MemoryImage aTile2(mApp);
	aTile2.Create(102, 51);
	Graphics aTileG2(&aTile2);
	aTileG2.DrawImage(mApp->mImages[28], 0, 0, Rect(aTileSet * 102 + 51, 0, 51, 51));

	Image* aColoredTile1 = mApp->CreateColorizedImage(&aTile1, Color(gBackgroundColors[aColorSet][0], gBackgroundColors[aColorSet][1], gBackgroundColors[aColorSet][2]));
	Image* aColoredTile2 = mApp->CreateColorizedImage(&aTile2, Color(gBackgroundColors[aColorSet][3], gBackgroundColors[aColorSet][4], gBackgroundColors[aColorSet][5]));

	int aRow = -1;
	for (int y = -35; y <= 424; y += 51)
	{
		int aCol = -1;
		for (int x = 151; x <= 610; x += 51)
		{
			if ((aCol + aRow) % 2 == 0)
				g.DrawImage(aColoredTile1, x, y);
			else
				g.DrawImage(aColoredTile2, x, y);
			aCol++;
		}
		aRow++;
	}

	delete aColoredTile1;
	delete aColoredTile2;

	g.DrawImage(mApp->mImages[7], 0, 0);

	m_unk0x248 = 0.0;
	double aLevel = m_unk0x5c;
	if (mTimeTrial)
	{
		m_unk0x1c0 = aLevel * mApp->m_unk0x620 + mApp->m_unk0x618;
		m_unk0x240 = (int)((aLevel * mApp->m_unk0x610 + mApp->m_unk0x608) * m_unk0x1c0);
	}
	else
	{
		m_unk0x1c0 = aLevel * mApp->m_unk0x600 + mApp->m_unk0x5f8;
		m_unk0x240 = (int)((aLevel * mApp->m_unk0x5f0 + mApp->m_unk0x5ec) * m_unk0x1c0);
	}
	m_unk0x250 = (aLevel * mApp->m_unk0x630 + mApp->m_unk0x628) * m_unk0x1c0;
	m_unk0x258 = (aLevel * mApp->m_unk0x640 + mApp->m_unk0x638) * m_unk0x1c0;
}
// FUNCTION: GAME 0x40260a
// FUNCTION: GAME187 0x4cb182
void Board::FillBoard()
{
	mState = 0;
	m_unk0x1b8 = 0;

	int aRow;
	for (aRow = 7; aRow >= 0; aRow--)
	{
		for (int aCol = 0; aCol < 8; aCol++)
			mBoard[aRow][aCol] = NULL;
	}

	for (aRow = 7; aRow >= 0; aRow--)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = new Gem();
			aGem->mCol = aCol;
			aGem->mRow = aRow;
			aGem->mX = GetColX(aCol);
			int aY = 0;
			if (aRow < 7)
				aY = (int)mBoard[aRow + 1][aCol]->mY;
			aGem->mY = aY - Rand() % 240 - 65;
			mBoard[aRow][aCol] = aGem;
			do
			{
				aGem->mColor = Rand() % 7;
			} while (HasMatch());
		}
	}
}
// FUNCTION: GAME 0x40270d
// FUNCTION: GAME187 0x4cb284
void Board::UpdateLevelTransition()
{
	// 3DS port: the original copied this update's 3-row strip of the new background (m_unk0x284) into the shown one
	// (m_unk0x280) here. With the GPU renderer that re-uploads the whole 640x480 texture (1.3 MB) every frame, so Draw
	// shows the two split at the wipe instead, and the copy is made once when the wipe is done (below).

	m_unk0x2d8 -= 0.015;
	if (m_unk0x2d8 < 0.0)
	{
		m_unk0x2d8 = 0.0;
		if (m_unk0x16c != NULL)
		{
			m_unk0x16c->Release();
			m_unk0x16c = NULL;
		}
	}
	else if (m_unk0x16c != NULL)
	{
		m_unk0x16c->SetVolume(m_unk0x2d8);
	}

	if (mUpdateCnt % 4 == 0)
		m_unk0x2cc = Rand() % 128;

	MarkDirty();

	m_unk0x2c8 += 3;
	if (m_unk0x2c8 > mHeight)
	{
		Graphics g(m_unk0x280);
		g.DrawImage(m_unk0x284, 0, 0);

		m_unk0x60 = m_unk0x5c + 1;
		m_unk0x2d4 = 200;
		FillBoard();
		// (The trial's register nag rolled here; the Switch port is the full game.)
	}
}

// FUNCTION: GAME 0x402846
// FUNCTION: GAME187 0x4cb3bf
void Board::UpdateDropIn()
{
	bool aDone = true;
	MarkDirty();

	for (int aCol = 0; aCol < 8; aCol++)
	{
		bool aStart = Rand() % 15 == 0;
		for (int aRow = 7; aRow >= 0; aRow--)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if (aGem != NULL)
			{
				if (aGem->m_unk0x28 == 0.0)
				{
					if (aStart)
					{
						aGem->m_unk0x28 = 1.0;
						aStart = false;
					}
				}
				else
				{
					aGem->mY += aGem->m_unk0x28;
					aGem->m_unk0x28 += 0.18;
				}

				if (mHeight + 32 > aGem->mY)
					aDone = false;
			}
		}
	}

	if (aDone)
	{
		m_unk0x2d4 = 200;
		FillBoard();
	}
}
// FUNCTION: GAME 0x4028ff
// FUNCTION: GAME187 0x4cb475
void Board::Hint()
{
	if ((mState == 1) && (mPauseCount == 0))
		ShowHint();
}

// FUNCTION: GAME 0x402d1a
// FUNCTION: GAME187 0x4cb5bc
Board::Board(WinDMApp* theApp, bool timeTrial)
{
	m_unk0x1f8 = 0.0;
	mTimeTrial = timeTrial;
	mFlash = 0.0;
	m_unk0x170 = true;
	mApp = theApp;
	m_unk0x248 = 0.0;
	m_unk0x178 = 0;
	mSelectedGem = NULL;
	mLastLandSoundTick = 0;
	m_unk0x250 = 0.0;
	m_unk0x1dc = 0;
	m_unk0x210 = false;
	mHintActive = false;
	mGameTime = 0;
	mElapsedTime = 0;
	mGemsCleared = 0;
	mLongestCascade = 0;
	mBestMove = 0;
	mScore = 0;
	m_unk0x1c8 = 0;
	m_unk0x260 = 23;
	mLastTickSound = 0;
	mCascadeScore = 0;
	m_unk0x1e8 = 0;
	m_unk0x1f0 = 0;
	mFlashTime = 0;
	mFlashDuration = 0;
	mClickDelay = 0;
	mMouseDown = false;
	mStartTick = 0;
	mPauseCount = 0;
	mUserPauseCount = 0;
	m_unk0x230 = false;
	m_unk0x238 = 0;
	m_unk0x234 = 0;
	mSwapGem1 = NULL;
	mSwapGem2 = NULL;
	mPopupFont = theApp->m_unk0x684;
	mScoreFont = theApp->m_unk0x688;
	m_unk0x5c = 0;
	m_unk0x60 = 1;

	m_unk0x280 = new DDImage(mApp->mDDInterface);
	m_unk0x280->SetImageMode(false, false);
	m_unk0x280->Create(640, 480);

	m_unk0x284 = new DDImage(mApp->mDDInterface);
	m_unk0x284->SetImageMode(false, false);
	m_unk0x284->Create(640, 480);

	MakeBackground();
	m_unk0x23c = m_unk0x240;

	Graphics aG(m_unk0x280);
	aG.DrawImage(m_unk0x284, 0, 0);

	mHintCol1 = -1;
	m_unk0x2d8 = 0.0;
	mHintRow = -1;
	mHintCol2 = -1;
	m_unk0x288 = false;
	m_unk0x1a8 = 0;
	m_unk0x2d4 = 300;
	mMessageImage = NULL;
	mMessageFxImage = NULL;
	mShowMessage = false;
	m_unk0x2cc = 0;
	m_unk0x16c = NULL;
	m_unk0x1d0 = 0;
	m_unk0x1d4 = 0;
	mHintBounce = 0;
	mHintBounceDir = 1;

	do
	{
		FillBoard();

		int aMove[4];
		int aSkip = 0;
		while (FindMove(aMove, aSkip, true, false))
		{
			if ((aMove[2] == aMove[0] + 1) && (aMove[1] >= 4) && (aMove[1] < 7) && (aMove[0] >= 1) && (aMove[0] < 7))
			{
				mHintCol1 = aMove[0];
				mHintRow = aMove[1];
				mHintCol2 = aMove[2];
				break;
			}
			aSkip++;
		}
	} while (mHintCol1 == -1);
}
// FUNCTION: GAME 0x4031e0
// FUNCTION: GAME187 0x4cca37
bool Board::HasSavedGame()
{
	return mApp->FileExists(GetAppDataFolder() + "userdata/savegame.dat");
}

// FUNCTION: GAME 0x40322a
// FUNCTION: GAME187 0x4cca98
bool Board::LoadGame()
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(GetAppDataFolder() + "userdata/savegame.dat", &aBuffer))
		return false;

	int aVersion;
	aBuffer.ReadBytes((uchar*)&aVersion, 4);
	if (aVersion != 1)
		return false;

	bool aTimeTrial;
	aBuffer.ReadBytes((uchar*)&aTimeTrial, 1);
	mApp->SetGameMode(aTimeTrial, false);

	aBuffer.ReadBytes((uchar*)&m_unk0x5c, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x60, 4);

	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			delete mBoard[aRow][aCol];

			bool aHasGem;
			aBuffer.ReadBytes((uchar*)&aHasGem, 1);
			if (aHasGem)
			{
				Gem* aGem = new Gem();
				aBuffer.ReadBytes((uchar*)aGem, sizeof(Gem));
				aGem->mX = GetColX(aGem->mCol);
				aGem->mY = GetRowY(aGem->mRow);
				mBoard[aRow][aCol] = aGem;
			}
			else
			{
				mBoard[aRow][aCol] = NULL;
			}
		}
	}

	aBuffer.ReadBytes((uchar*)&mState, 4);
	if ((mState == 2) || (mState == 3))
		mState = 1;

	aBuffer.ReadBytes((uchar*)&mTimeTrial, 1);
	aBuffer.ReadBytes((uchar*)&m_unk0x170, 1);
	aBuffer.ReadBytes((uchar*)&m_unk0x1a8, 4);
	aBuffer.ReadBytes((uchar*)&mElapsedTime, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x1b8, 4);
	aBuffer.ReadBytes((uchar*)&mScore, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x1c0, 8);
	aBuffer.ReadBytes((uchar*)&m_unk0x1c8, 4);
	aBuffer.ReadBytes((uchar*)&mGemsCleared, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x1d0, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x1d4, 4);
	aBuffer.ReadBytes((uchar*)&mLongestCascade, 4);
	aBuffer.ReadBytes((uchar*)&mBestMove, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x230, 1);
	aBuffer.ReadBytes((uchar*)&m_unk0x234, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x238, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x23c, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x240, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x248, 8);
	aBuffer.ReadBytes((uchar*)&m_unk0x250, 8);
	aBuffer.ReadBytes((uchar*)&m_unk0x258, 8);
	aBuffer.ReadBytes((uchar*)&m_unk0x260, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x2c8, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x2d0, 4);
	aBuffer.ReadBytes((uchar*)&m_unk0x2d8, 8);
	aBuffer.ReadBytes((uchar*)&m_unk0x2e0, 1);

	mApp->m_unk0x735 = false;
	return true;
}

// FUNCTION: GAME 0x40356a
// FUNCTION: GAME187 0x4ccdfe
bool Board::SaveGame()
{
	Pause(false);
	Unpause(false);

	Buffer aBuffer;
	int aVersion = 1;
	aBuffer.WriteBytes((uchar*)&aVersion, 4);
	aBuffer.WriteBytes((uchar*)&mTimeTrial, 1);
	aBuffer.WriteBytes((uchar*)&m_unk0x5c, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x60, 4);

	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			bool aHasGem = aGem != NULL;
			aBuffer.WriteBytes((uchar*)&aHasGem, 1);
			if (aHasGem)
				aBuffer.WriteBytes((uchar*)aGem, sizeof(Gem));
		}
	}

	aBuffer.WriteBytes((uchar*)&mState, 4);
	if ((mState == 2) || (mState == 3))
		mState = 1;

	aBuffer.WriteBytes((uchar*)&mTimeTrial, 1);
	aBuffer.WriteBytes((uchar*)&m_unk0x170, 1);
	aBuffer.WriteBytes((uchar*)&m_unk0x1a8, 4);
	aBuffer.WriteBytes((uchar*)&mElapsedTime, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x1b8, 4);
	aBuffer.WriteBytes((uchar*)&mScore, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x1c0, 8);
	aBuffer.WriteBytes((uchar*)&m_unk0x1c8, 4);
	aBuffer.WriteBytes((uchar*)&mGemsCleared, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x1d0, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x1d4, 4);
	aBuffer.WriteBytes((uchar*)&mLongestCascade, 4);
	aBuffer.WriteBytes((uchar*)&mBestMove, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x230, 1);
	aBuffer.WriteBytes((uchar*)&m_unk0x234, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x238, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x23c, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x240, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x248, 8);
	aBuffer.WriteBytes((uchar*)&m_unk0x250, 8);
	aBuffer.WriteBytes((uchar*)&m_unk0x258, 8);
	aBuffer.WriteBytes((uchar*)&m_unk0x260, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x2c8, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x2d0, 4);
	aBuffer.WriteBytes((uchar*)&m_unk0x2d8, 8);
	aBuffer.WriteBytes((uchar*)&m_unk0x2e0, 1);

	MkDir(GetAppDataFolder() + "userdata");
	return mApp->WriteBufferToFile(GetAppDataFolder() + "userdata/savegame.dat", &aBuffer);
}

// FUNCTION: GAME 0x40383e
// FUNCTION: GAME187 0x4cd130
bool Board::DeleteSavedGame()
{
	return mApp->EraseFile(GetAppDataFolder() + "userdata/savegame.dat");
}

// FUNCTION: GAME 0x403888
// FUNCTION: GAME187 0x4cb931
void Board::UpdateLevelUp()
{
	m_unk0x2d8 += 0.02;
	if (m_unk0x2d8 > 1.0)
		m_unk0x2d8 = 1.0;
	if (m_unk0x16c != NULL)
		m_unk0x16c->SetVolume(m_unk0x2d8);

	int aWidth;
	int aHeight;
	int aCenterX;
	int aCenterY;
	GetBoardCenter(&aCenterX, &aCenterY);
	GetBoardSize(&aWidth, &aHeight);
	int aMaxDist = (int)((int)sqrt((double)(aHeight * aHeight + aWidth * aWidth)) / 2.2);

	double aMaxDistBefore = 0.0;
	double aMaxDistAfter = 0.0;
	bool aDone = true;

	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if (aGem != NULL)
			{
				aGem->mX = cos(aGem->mAngle) * aGem->mDist + aCenterX - 24.0;
				aGem->mY = sin(aGem->mAngle) * aGem->mDist + aCenterY - 24.0;

				if (!(aMaxDistBefore > aGem->mDist))
					aMaxDistBefore = aGem->mDist;

				aGem->mDist -= 1.0;
				if (aGem->mDist < 0.0)
					aGem->mDist = 0.0;
				else
					aDone = false;

				if (!(aMaxDistAfter > aGem->mDist))
					aMaxDistAfter = aGem->mDist;

				aGem->m_unk0x68 += (aMaxDist - aGem->mDist) / 110000.0;
				aGem->mAngle += aGem->m_unk0x68;

				if (mUpdateCnt % 8 == 0)
				{
						int aFrame = (int)(6.0 - aGem->mDist * 6.0 / 96.0 - 1.0);
					if (aFrame >= 0)
					{
						if (!aGem->m_unk0x4c)
						{
							aGem->m_unk0x50 = 0;
							aGem->m_unk0x4c = true;
						}
						else if (aFrame > aGem->m_unk0x50)
						{
							aGem->m_unk0x50++;
						}
					}
				}
			}
		}
	}

	if ((aMaxDistBefore >= 80.0) && (aMaxDistAfter < 80.0))
		mApp->PlaySample(15);

	if ((aMaxDistBefore >= 64.0) && (aMaxDistAfter < 64.0))
	{
		m_unk0x2e0 = true;
		SetMessage(StrFormat(mApp->GetString("LEVEL").c_str(), m_unk0x5c + 2), 400);
	}

	if (aDone)
	{
		m_unk0x5c++;
		MakeBackground();

		for (int aRow = 0; aRow < 8; aRow++)
		{
			for (int aCol = 0; aCol < 8; aCol++)
			{
				delete mBoard[aRow][aCol];
				mBoard[aRow][aCol] = NULL;
			}
		}

		m_unk0x23c = mScore + m_unk0x240;
		m_unk0x238 = mScore;
		mApp->PlaySample(17);
		m_unk0x2c8 = 0;
		mState = 8;
	}

	MarkDirty();
}

// FUNCTION: GAME 0x403bb3
// FUNCTION: GAME187 0x4cbc70
void Board::Draw(Graphics* g)
{
	int aCenterX;
	int aCenterY;
	GetBoardCenter(&aCenterX, &aCenterY);

	if (mState == 8)
	{
		// 3DS port: during the level transition the new background is shown above the wipe and the old one below it
		// (see UpdateLevelTransition).
		Graphics aNewG(*g);
		aNewG.ClipRect(0, 0, mWidth, m_unk0x2c8);
		aNewG.DrawImage(m_unk0x284, 0, 0);
		Graphics anOldG(*g);
		anOldG.ClipRect(0, m_unk0x2c8, mWidth, mHeight - m_unk0x2c8);
		anOldG.DrawImage(m_unk0x280, 0, 0);
	}
	else
		g->DrawImage(m_unk0x280, 0, 0);

	if (m_unk0x2d8 > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(m_unk0x2d8 * 255.0)));
		g->SetDrawMode(1);
		g->DrawImage(mApp->mImages[30], aCenterX - 128, aCenterY - 128, Rect(m_unk0x2d0 * 256, 0, 256, 256));
		g->SetDrawMode(0);
		g->SetColorizeImages(false);
	}

	if (mUserPauseCount == 0)
	{
		Graphics aG(*g);
		aG.ClipRect(194, 12, 428, 416);

		int aRow;
		int aCol;
		for (aRow = 0; aRow < 8; aRow++)
		{
			for (aCol = 0; aCol < 8; aCol++)
			{
				Gem* aGem = mBoard[aRow][aCol];
				if (aGem == NULL)
					continue;

				int aColor = aGem->mColor;
				int aSrcY = aColor * 104;
				int aFrame = aGem->m_unk0x54;
				int aSrcX;
				if (aFrame == 0)
					aSrcX = 2;
				else
					aSrcX = aFrame * 52 + 2;

				if ((aGem->m_unk0x38) && (aFrame == 0) && (!aGem->m_unk0x4c))
				{
					Image* anImage = mApp->CreateCrossfadeImage(mApp->mImages[8], Rect(aSrcX, aSrcY, 49, 49), mApp->mImages[8], Rect(aSrcX + aGem->m_unk0x48 * 52, aSrcY + 52, 49, 49), aGem->m_unk0x40);
					aG.DrawImage(anImage, (int)aGem->mX, (int)aGem->mY);
					delete anImage;
				}
				else if (aGem->m_unk0x4c)
				{
					aG.DrawImage(mApp->m_unk0x654[aColor][aGem->m_unk0x50], (int)aGem->mX, (int)aGem->mY);
				}
				else
				{
					aG.DrawImage(mApp->mImages[8], (int)aGem->mX, (int)aGem->mY, Rect(aSrcX, aSrcY, 49, 49));
				}
			}
		}

		for (aRow = 0; aRow < 8; aRow++)
		{
			for (aCol = 0; aCol < 8; aCol++)
			{
				Gem* aGem = mBoard[aRow][aCol];
				if ((aGem != NULL) && (aGem->m_unk0x58 > 0))
				{
					if (aGem->m_unk0x58 >= 16)
					{
						int aFrame = aGem->m_unk0x5c % 14;
						g->SetDrawMode(1);
						g->DrawImage(mApp->mImages[22], (int)(aGem->mX + 13.0), (int)(aGem->mY - 7.0), Rect(aFrame * 40, 0, 40, 40));
						g->SetDrawMode(0);
					}

					if (aGem->m_unk0x58 >= 8)
					{
						int aFrame = (aGem->m_unk0x5c - 8) % 14;
						if (aFrame >= 0)
						{
							g->SetDrawMode(1);
							g->DrawImage(mApp->mImages[22], (int)(aGem->mX - 10.0), (int)(aGem->mY + 3.0), Rect(aFrame * 40, 0, 40, 40));
							g->SetDrawMode(0);
						}
					}

					int aFrame = (aGem->m_unk0x5c - 16) % 14;
					if (aFrame >= 0)
					{
						g->SetDrawMode(1);
						g->DrawImage(mApp->mImages[22], (int)(aGem->mX + 5.0), (int)(aGem->mY + 19.0), Rect(aFrame * 40, 0, 40, 40));
						g->SetDrawMode(0);
					}
				}
			}
		}

		Gem* aSelGems[3] = {mSelectedGem, mSwapGem1, mSwapGem2};
		for (int i = 0; i < 3; i++)
		{
			if (aSelGems[i] != NULL)
				g->DrawImage(mApp->mImages[9], (int)(aSelGems[i]->mX - 2.0), (int)(aSelGems[i]->mY - 2.0), Rect((m_unk0x178 % 6) * 52, 0, 52, 52));
		}

		g->SetColor(Color::White);
		ScorePopupList::iterator anItr;
		for (anItr = mScorePopups.begin(); anItr != mScorePopups.end(); ++anItr)
		{
			g->SetFont(mApp->m_unk0x684);
			int anAlpha = 255 * anItr->mTimer / 18;
			if (anAlpha > 255)
				anAlpha = 255;
			g->SetColor(Color(255, 255, 255, anAlpha));

			char aBuf[12];
			sprintf(aBuf, "%d", anItr->mPoints);
			int aX = anItr->mX - mPopupFont->StringWidth(aBuf) / 2;
			int aY = anItr->mY + mPopupFont->GetAscent() / 2;
			g->DrawString(aBuf, aX, aY);
		}

		if (mHintActive)
		{
			g->DrawImage(mApp->mImages[34], GetColX(mHintCol1) + 3, GetRowY(mHintRow) + 38);
			if ((mSelectedGem != NULL) && (mSelectedGem->mCol == mHintCol1) && (mSelectedGem->mRow == mHintRow))
				g->DrawImage(mApp->mImages[35], GetColX(mHintCol2) - 5, GetRowY(mHintRow) + mHintBounce - 42);
			else
				g->DrawImage(mApp->mImages[35], GetColX(mHintCol1) - 5, GetRowY(mHintRow) + mHintBounce - 42);
		}

		if (mState == 8)
		{
			Graphics aWipeG(*g);
			aWipeG.ClipRect(0, m_unk0x2c8 - 8, mWidth, 16);
			aWipeG.SetDrawMode(1);
			for (int i = 0; i < (m_unk0x2cc + mWidth + 127) / 128; i++)
				aWipeG.DrawImage(mApp->mImages[32], i * 128 - m_unk0x2cc, m_unk0x2c8 - 8);
			aWipeG.SetDrawMode(0);
			aWipeG.DrawImage(mApp->mImages[7], 0, 0);
		}

		if (mShowMessage)
		{
			UpdateMessageFx();
			g->DrawImage(mMessageFxImage, aCenterX - mMessageFxImage->GetWidth() / 2, aCenterY + mMessageFxImage->GetHeight() / -2 - mHeight / 6);
		}

		if (m_unk0x2d8 > 0.0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int)(m_unk0x2d8 * 255.0)));
			Image* anImage = mApp->mImages[31];
			g->DrawImage(anImage, aCenterX - anImage->GetWidth() / 2, aCenterY - anImage->GetHeight() / 2);
			g->SetColorizeImages(false);
		}
	}
	else if ((mHasFocus) || (!mApp->mHasFocus))
	{
		g->SetFont(mApp->m_unk0x68c);
		g->SetColor(Color::White);

		int aX;
		int aY;
		GetBoardCenter(&aX, &aY);

		std::string aString = mApp->GetString("GAME_PAUSED");
		g->DrawString(aString, aX - g->GetFont()->StringWidth(aString) / 2, aY - g->GetFont()->GetHeight() / 2);
		aString = mApp->GetString("CLICK_HERE_TO_CONTINUE");
		g->DrawString(aString, aX - g->GetFont()->StringWidth(aString) / 2, aY + g->GetFont()->GetHeight());
	}

	char aStr[12];

	g->SetFont(mApp->m_unk0x688);
	sprintf(aStr, "%d", m_unk0x1c8);
	g->SetColor(Color(173, 231, 255));
	g->DrawString(aStr, 93 - mScoreFont->StringWidth(aStr) / 2, 106);

	g->DrawImage(mApp->m_unk0x6d0[m_unk0x260], 237, 444, Rect(0, 0, m_unk0x1a8, 25));

	sprintf(aStr, "%d", m_unk0x60);
	g->SetFont(mApp->m_unk0x68c);
	g->SetColor(Color::Black);
	g->DrawString(aStr, 606 - g->GetFont()->StringWidth(aStr) / 2, 465);

	if (m_unk0x1f8 > 0.0)
	{
		sprintf(aStr, "+%d", mCascadeScore);
		g->SetFont(mApp->m_unk0x68c);
		g->SetColor(Color(255, 255, 255, (int)(m_unk0x1f8 * 255.0)));
		g->DrawString(aStr, g->GetFont()->StringWidth(aStr) / -2 - g->GetFont()->CharWidth('+') / 3 + 91, 350);
	}

	if (mFlash > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mFlash * 255.0)));
		g->DrawImage(mApp->mImages[24], 64, 303);
		g->SetColorizeImages(false);
	}
}
// FUNCTION: GAME 0x40492d
// FUNCTION: GAME187 0x4cd1ba
Board::~Board()
{
	if (CanPlay())
		SaveGame();

	delete m_unk0x280;
	delete m_unk0x284;
	delete mMessageImage;
	delete mMessageFxImage;

	if (m_unk0x16c != NULL)
		m_unk0x16c->Release();

	mApp->KillDialog(9);

	if (CanPlay())
		mApp->mTimesPlayed++;
}

// FUNCTION: GAME 0x404a1a
// FUNCTION: GAME187 0x4cd287
bool Board::ProcessMatches()
{
	int aMatchCount = 0;
	int aTotalX = 0;
	int aLongest = 0;
	int aDir;
	int aLine;
	int aPos;
	int aStartRow;
	int aStartCol;
	int aColor;
	int aCount;
	int aCol;
	int aRow;

	for (aDir = 0; aDir < 2; aDir++)
	{
		for (aLine = 0; aLine < 8; aLine++)
		{
			aCount = 0;
			aStartRow = 0;
			aStartCol = 0;
			aColor = -1;

			for (aPos = 0; aPos < 8; aPos++)
			{
				if (aDir == 0)
				{
					aCol = aLine;
					aRow = aPos;
				}
				else
				{
					aCol = aPos;
					aRow = aLine;
				}

				Gem* aGem = mBoard[aRow][aCol];
				if ((aGem != NULL) && (aGem->mColor == aColor))
				{
					aCount++;
					if (aPos != 7)
						continue;
				}

				if (aCount >= 3)
				{
					aMatchCount++;

					int aX = 0;
					int aY = 0;
					for (int aMatchRow = aStartRow; aMatchRow <= aRow; aMatchRow++)
					{
						for (int aMatchCol = aStartCol; aMatchCol <= aCol; aMatchCol++)
						{
							Gem* aMatchGem = mBoard[aMatchRow][aMatchCol];
							if ((aMatchGem != NULL) && (aMatchGem->mColor == aColor))
							{
								aX = (int)(aX + aMatchGem->mX);
								aY = (int)(aY + aMatchGem->mY);
								aMatchGem->m_unk0x50 = 0;
								aMatchGem->m_unk0x4c = true;
							}
						}
					}

					int aCenterX = aX / aCount;
					int aCenterY = aY / aCount;
					aTotalX += aCenterX;

					int aPoints = 10;
					m_unk0x1d0++;
					switch (m_unk0x1d0)
					{
					case 1:
						break;
					case 2:
						aPoints = 20;
						break;
					case 3:
						aPoints = 30;
						break;
					case 4:
						aPoints = 40;
						break;
					case 5:
						aPoints = 60;
						break;
					case 6:
						aPoints = 80;
						break;
					case 7:
						aPoints = 110;
						break;
					case 8:
						aPoints = 160;
						break;
					default:
						aPoints = 210;
						break;
					}

					if (aCount > 3)
						aPoints += (aCount * 5 - 15) * 2;
					if (aCount > 5)
						aPoints += (aCount * 5 - 25) * 2;

					int aScore = (int)(aPoints * m_unk0x1c0);

					ScorePopup aPopup;
					aPopup.mX = aCenterX + 24;
					aPopup.mY = aCenterY + 16;
					aPopup.mTimer = 110;
					aPopup.mColor = -1;
					aPopup.mPoints = aScore;
					mScorePopups.push_back(aPopup);

					mGemsCleared += aCount;
					mScore += aScore;

					if (m_unk0x1d0 == 1)
					{
						mFlash = 0.0;
						mFlashTime = 0;
						mCascadeScore = 0;
						m_unk0x1e8 = 0;
					}

					int anExcellent = (int)(m_unk0x1c0 * 120.0);
					if ((mCascadeScore < anExcellent) && (mCascadeScore + aScore >= anExcellent))
					{
						SetMessage(mApp->GetString("EXCELLENT"), 200);
						mApp->PlaySample(20);
					}

					mCascadeScore += aScore;
					if (m_unk0x1f0 != -1)
					{
						double aTime = mCascadeScore / m_unk0x1c0 * 3.0;
						m_unk0x1f0 = (int)min(max(300.0, aTime), 1200.0);
					}

					if (aCount > aLongest)
						aLongest = aCount;
				}

				if (aGem != NULL)
					aColor = aGem->mColor;
				else
					aColor = -1;
				aStartRow = aRow;
				aStartCol = aCol;
				aCount = 1;
			}
		}
	}

	if (aMatchCount > 0)
	{
		double aPan = GetPan(aTotalX / aMatchCount + 24);
		m_unk0x1b8 = 0;
		if (aLongest == 3)
			mApp->PlaySample(0, (int)aPan);
		else
			mApp->PlaySample(1, (int)aPan);

		int aCascade = m_unk0x1d4 - 1;
		if (aCascade >= 5)
			aCascade = 4;
		mApp->PlaySample(aCascade + 2, (int)aPan);

		if (aMatchCount > 1)
			mApp->PlaySample(13, (int)aPan);
	}

	return aMatchCount > 0;
}

// FUNCTION: GAME 0x404e1e
// FUNCTION: GAME187 0x4cd687
void Board::UpdateFalling()
{
	int aLandCount = 0;
	int aTotalX = 0;
	bool aSettled = true;

	for (int aRow = 0; aRow < 8; aRow++)
	{
		double aTargetY = GetRowY(aRow);
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if (aGem->mY != aTargetY)
			{
				aSettled = false;
				aGem->mY += aGem->m_unk0x28;
				if (aGem->mY >= aTargetY)
				{
					aLandCount++;
					aTotalX = (int)(aTotalX + aGem->mX + 24.0);
					aGem->mY = aTargetY;
				}
				aGem->m_unk0x28 += 0.18;
			}
			else
			{
				aGem->m_unk0x28 = 0.0;
			}
		}
	}

	if ((aLandCount > 0) && (abs(mLastLandSoundTick - mUpdateCnt) >= 10))
	{
		mLastLandSoundTick = mUpdateCnt;
		mApp->PlaySample(8, GetPan(aTotalX / aLandCount));
	}

	if (aSettled)
	{
		if (ProcessMatches())
		{
			mState = 4;
		}
		else
		{
			if (!m_unk0x230)
			{
				if (m_unk0x1d4 > mLongestCascade)
					mLongestCascade = m_unk0x1d4;
				if (mCascadeScore > mBestMove)
					mBestMove = mCascadeScore;
			}

			if (!FindMove(NULL, 0, true, true))
			{
				if (mTimeTrial)
				{
					mState = 9;
				}
				else
				{
					SetMessage(mApp->GetString("NO_MOVES"), 400);
					EndGame(200);
				}
			}
			else
			{
				mState = 1;
			}
		}
	}

	MarkDirty();
}

// FUNCTION: GAME 0x404fea
// FUNCTION: GAME187 0x4cd84f
void Board::UpdateSwap()
{
	MarkDirty();

	mSwapPct += 5;
	if (mSwapPct == 180)
	{
		int aCol = mSwapGem1->mCol;
		int aRow = mSwapGem1->mRow;

		mSwapGem1->mCol = mSwapGem2->mCol;
		mSwapGem1->mRow = mSwapGem2->mRow;
		mSwapGem1->mX = GetColX(mSwapGem1->mCol);
		mSwapGem1->mY = GetRowY(mSwapGem1->mRow);
		mBoard[mSwapGem1->mRow][mSwapGem1->mCol] = mSwapGem1;

		mSwapGem2->mCol = aCol;
		mSwapGem2->mRow = aRow;
		mSwapGem2->mX = GetColX(mSwapGem2->mCol);
		mSwapGem2->mY = GetRowY(mSwapGem2->mRow);
		mBoard[mSwapGem2->mRow][mSwapGem2->mCol] = mSwapGem2;

		m_unk0x1d4 = 1;
		m_unk0x1d0 = 0;

		if (ProcessMatches())
		{
			mSwapGem1 = NULL;
			mSwapGem2 = NULL;
			mState = 4;

			if (mHintActive)
			{
				mHintActive = false;
				mApp->KillDialog(9);
				mApp->m_unk0x735 = false;
			}
			m_unk0x170 = false;
		}
		else if (mState == 2)
		{
			mSwapGem1->mSwapOffsetX = -mSwapGem1->mSwapOffsetX;
			mSwapGem1->mSwapOffsetY = -mSwapGem1->mSwapOffsetY;
			mSwapGem2->mSwapOffsetX = -mSwapGem1->mSwapOffsetX;
			mSwapGem2->mSwapOffsetY = -mSwapGem1->mSwapOffsetY;
			mSwapPct = 0;
			mApp->PlaySample(7);
			mState = 3;
		}
		else
		{
			mSwapGem1 = NULL;
			mSwapGem2 = NULL;
			mState = 1;
		}
	}
	else
	{
		double aCos = cos(mSwapPct * 3.14159 / 180.0);
		mSwapGem1->mX = (int)(mSwapGem1->mSwapOffsetX * aCos + mSwapCenterX);
		mSwapGem1->mY = (int)(mSwapGem1->mSwapOffsetY * aCos + mSwapCenterY);
		mSwapGem2->mX = (int)(mSwapGem2->mSwapOffsetX * aCos + mSwapCenterX);
		mSwapGem2->mY = (int)(mSwapGem2->mSwapOffsetY * aCos + mSwapCenterY);
	}
}

// FUNCTION: GAME 0x40524b
// FUNCTION: GAME187 0x4cdabc
void Board::Update()
{
	bool aRising;

	if ((mPauseCount > 0) || (mUserPauseCount > 0))
		return;

	if (mClickDelay > 0)
		mClickDelay--;

	Widget::Update();

	if (mShowMessage)
	{
		mMessagePhase += 0.07;
		MarkDirty();
		if (--mMessageDuration == 0)
			mShowMessage = false;
	}

	if (m_unk0x2d4 > 0)
		m_unk0x2d4--;

	if (mTimeTrial && (m_unk0x2d4 == 0) && ((mState == 1) || (mState == 2)))
	{
		m_unk0x248 += m_unk0x250 / 100.0;
		m_unk0x250 += m_unk0x258 / 100.0;
	}

	if (mFlashTime > 0)
	{
		mFlashTime--;
		int aPeriod = mFlashDuration / 3;
		int aPhase = abs(mFlashTime % aPeriod - aPeriod / 2);
		mFlash = 1.0 - aPhase / (aPeriod * 0.5);
		if (mFlashTime == 0)
			mFlash = 0.0;
		MarkDirty();
	}

	if ((m_unk0x2d8 > 0.0) && (mUpdateCnt % 2 == 0))
	{
		if (--m_unk0x2d0 < 0)
			m_unk0x2d0 = 8;
		MarkDirty();
	}

	if (mHintActive && (mUpdateCnt % 4 == 0))
	{
		mHintBounce += mHintBounceDir;
		if (abs(mHintBounce) >= 3)
			mHintBounceDir = -mHintBounceDir;
		MarkDirty();
	}

	if (m_unk0x1f0 > 0)
	{
		m_unk0x1f0--;
		if (m_unk0x1f0 < 50)
		{
			m_unk0x1f8 = m_unk0x1f0 / 50.0;
			MarkDirty();
		}
		else if (m_unk0x1f8 < 1.0)
		{
			m_unk0x1f8 += 0.05;
			if (m_unk0x1f8 > 1.0)
				m_unk0x1f8 = 1.0;
			MarkDirty();
		}
	}

	int aBar;
	if (mTimeTrial)
		aBar = (m_unk0x1c8 - (int)m_unk0x248 - m_unk0x238) * 340 / 2 / (m_unk0x23c - m_unk0x238) + 170;
	else
		aBar = (m_unk0x1c8 - (int)m_unk0x248 - m_unk0x238) * 340 / (m_unk0x23c - m_unk0x238);

	if (aBar > 340)
		aBar = 340;
	if (aBar < 0)
		aBar = 0;

	aRising = false;
	for (int i = 0; i < 2; i++)
	{
		if (aBar > m_unk0x1a8)
		{
			aRising = true;
			m_unk0x1a8++;
			MarkDirty();
		}
		else if (aBar < m_unk0x1a8)
		{
			m_unk0x1a8--;
			MarkDirty();
		}
	}

	int aBarImage;
	if (mTimeTrial)
	{
		double aPct = m_unk0x1a8 / 340.0;
		aBarImage = (m_unk0x1a8 * 24 - 1) / 340;
		if (!aRising && (mState != 5) && (mState != 6))
		{
			int aDelay = (int)(aPct * 1600.0);
			if (aDelay < 80)
				aDelay = 80;
			if ((aDelay <= 240) && (mUpdateCnt - mLastTickSound >= aDelay))
			{
				mApp->PlaySample(18);
				mLastTickSound = mUpdateCnt;
			}

			if ((aPct < 0.15) && ((mUpdateCnt / 8) % 2 == 0))
				aBarImage = 24;
		}
	}
	else
	{
		aBarImage = 23;
	}

	if ((mState == 7) && ((mUpdateCnt / 8) % 2 == 0))
		aBarImage = 24;

	if (aBarImage != m_unk0x260)
	{
		m_unk0x260 = aBarImage;
		MarkDirty();
	}

	if (mUpdateCnt == 150)
	{
		mApp->PlaySample(14);
		if (!mApp->m_unk0x735 || (mApp->mTimesPlayed >= 5))
			SetMessage(mApp->GetString("GO"), 250);

		if ((mUpdateCnt == 150) && (mApp->m_unk0x735) && (mApp->mTimesPlayed < 5))
		{
			mApp->ShowInstructions();
			mHintActive = true;
		}
	}

	if ((++m_unk0x1b8 > 1500) && (mState == 1))
	{
		m_unk0x1b8 = 0;
		mFlashDuration = 400;
		mFlashTime = 400;
	}

	if (mScore - m_unk0x1c8 >= (int)(m_unk0x1c0 * 150.0))
	{
		m_unk0x1c8 = m_unk0x1c8 - (int)(m_unk0x1c0 * -3.0);
		MarkDirty();
	}
	else if (mScore - m_unk0x1c8 >= (int)(m_unk0x1c0 * 100.0))
	{
		m_unk0x1c8 = m_unk0x1c8 - (int)(m_unk0x1c0 * -2.0);
		MarkDirty();
	}
	else if ((mScore - m_unk0x1c8 >= (int)(m_unk0x1c0 * 50.0)) || ((m_unk0x1c8 < mScore) && (mUpdateCnt % 2 == 0)))
	{
		m_unk0x1c8 += (int)m_unk0x1c0;
		if (m_unk0x1c8 > mScore)
			m_unk0x1c8 = mScore;
		MarkDirty();
	}

	if (mScore - m_unk0x1c8 <= (int)(m_unk0x1c0 * -150.0))
	{
		m_unk0x1c8 = m_unk0x1c8 - (int)(m_unk0x1c0 * 3.0);
		MarkDirty();
	}
	else if (mScore - m_unk0x1c8 <= (int)(m_unk0x1c0 * -100.0))
	{
		m_unk0x1c8 = m_unk0x1c8 - (int)(m_unk0x1c0 * 2.0);
		MarkDirty();
	}
	else if ((mScore - m_unk0x1c8 <= (int)(m_unk0x1c0 * -50.0)) || ((m_unk0x1c8 > mScore) && (mUpdateCnt % 2 == 0)))
	{
		m_unk0x1c8 -= (int)m_unk0x1c0;
		if (m_unk0x1c8 < mScore)
			m_unk0x1c8 = mScore;
		MarkDirty();
	}

	if ((mSelectedGem != NULL) && (mUpdateCnt % 8 == 0))
	{
		MarkDirty();
		m_unk0x178++;
	}

	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Gem* aGem = mBoard[aRow][aCol];
			if (aGem == NULL)
				continue;

			if ((mUpdateCnt % 5 == 0) && (aGem->m_unk0x58 > 0))
			{
				aGem->m_unk0x58--;
				aGem->m_unk0x5c++;
				MarkDirty();
			}

			if ((mUpdateCnt % 3 == 0) && ((aGem->m_unk0x54 != 0) || (aGem == mSelectedGem)))
			{
				aGem->m_unk0x54 = (aGem->m_unk0x54 + 1) % 15;
				MarkDirty();
				continue;
			}

			if (aGem->m_unk0x38)
			{
				aGem->m_unk0x40 -= 0.012;
				if (aGem->m_unk0x40 < 0.0)
				{
					aGem->m_unk0x38 = false;
					MarkDirty();
				}
			}

			if ((mState == 1) && (aGem != mSelectedGem))
			{
				double aMouseX = mMouseX;
				if (aMouseX >= aGem->mX)
				{
					double aMouseY = mMouseY;
					if ((aMouseY >= aGem->mY) && (aGem->mX + 49.0 > aMouseX) && (aMouseY < aGem->mY + 49.0))
					{
					aGem->m_unk0x40 += 0.045;
					if (aGem->m_unk0x40 > 1.0)
						aGem->m_unk0x40 = 1.0;
					if (!aGem->m_unk0x38)
					{
						aGem->m_unk0x48 = 0;
						aGem->m_unk0x38 = true;
					}
					}
				}
			}

			if ((mUpdateCnt % 3 == 0) && (aGem->m_unk0x38))
			{
				aGem->m_unk0x48 = (aGem->m_unk0x48 + 1) % 15;
				MarkDirty();
			}
		}
	}

	ScorePopupList::iterator anItr = mScorePopups.begin();
	while (anItr != mScorePopups.end())
	{
		if (mUpdateCnt % 3 == 0)
		{
			MarkDirty();
			int aY = anItr->mY;
			if (aY > mPopupFont->GetAscent() / 2 + 2)
				anItr->mY = aY - 1;
		}
		MarkDirty();

		if (--anItr->mTimer <= 0)
		{
			anItr = mScorePopups.erase(anItr);
			MarkDirty();
		}
		else
		{
			++anItr;
		}
	}

	if (mState == 1)
	{
		if (m_unk0x1c8 - (int)m_unk0x248 - m_unk0x238 >= m_unk0x23c - m_unk0x238)
			StartExplode();

		if ((mState == 1) && (m_unk0x1c8 - (int)m_unk0x248 - m_unk0x238 <= m_unk0x238 - m_unk0x23c))
		{
			SetMessage(mApp->GetString("TIME_UP"), 400);
			EndGame(200);
		}
	}

	switch (mState)
	{
	case 0:
		UpdateFalling();
		break;
	case 2:
	case 3:
		UpdateSwap();
		break;
	case 4:
		UpdateCollapse();
		break;
	case 5:
		UpdateGameOver();
		break;
	case 7:
		UpdateLevelUp();
		break;
	case 8:
		UpdateLevelTransition();
		break;
	case 9:
		UpdateDropIn();
		break;
	}
}
