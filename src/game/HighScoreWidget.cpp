#include "HighScoreWidget.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "WinDMApp.h"
#include "Board.h"
#include "SexyAppFramework/HyperlinkWidget.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Font.h"
#include "SexyAppFramework/ImageFont.h"
#include "SexyAppFramework/Image.h"
#include "SexyAppFramework/DDImage.h"
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/Common.h"

using namespace Sexy;

// FUNCTION: GAME 0x406376
// FUNCTION: GAME187 0x4ced59
int HighScoreWidget::GetDay()
{
	time_t aTime;
	time(&aTime);
	tm* aLocalTime = localtime(&aTime);
	return aLocalTime->tm_yday + 366 * aLocalTime->tm_year;
}

// FUNCTION: GAME 0x40639e
// FUNCTION: GAME187 0x4ced82
int HighScoreWidget::GetRank(int theMode, int theScore)
{
	int aRank = 0;
	for (int i = 4; i >= 0; i--)
	{
		if (theScore > mScores[theMode][i])
			aRank = i + 1;
	}
	return aRank;
}

// FUNCTION: GAME 0x4063cb
// FUNCTION: GAME187 0x4cedaf
void HighScoreWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayAgainLink);
}

// FUNCTION: GAME 0x4063e9
// FUNCTION: GAME187 0x4cedd2
void HighScoreWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayAgainLink);
}

// FUNCTION: GAME 0x406407
// FUNCTION: GAME187 0x4cedf5
void HighScoreWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mSlideY = -theHeight;

	if (mBoardImage == NULL)
	{
		mBoardImage = new DDImage(mApp->mDDInterface);
		mBoardImage->Create(mWidth, mHeight);

		Graphics g(mBoardImage);
		mApp->mBoard->Draw(&g);
		mBoardImage->GenerateDDSurface();
	}
}

// FUNCTION: GAME 0x4064bb
// FUNCTION: GAME187 0x4ceeb7
void HighScoreWidget::ButtonDepress(int theId)
{
	mApp->NewBoard();
}

// FUNCTION: GAME 0x4066a6
// FUNCTION: GAME187 0x4ceec2
void HighScoreWidget::Load()
{
#if BEJEWELED_VERSION >= 1870
	FILE* aFP = fopen((GetAppDataFolder() + "userdata\\hiscores.dat").c_str(), "rb");
#else
	FILE* aFP = fopen("hiscores.dat", "rb");
#endif
	if (aFP != NULL)
	{
		for (int aMode = 0; aMode < 2; aMode++)
		{
			char aName[256];

			int aDay;
			fread(&aDay, 1, 4, aFP);
			unsigned short aLen;
			fread(&aLen, 1, 2, aFP);
			if (aLen > 255)
				aLen = 255;
			fread(aName, aLen, 1, aFP);
			aName[aLen] = 0;
			int aScore;
			fread(&aScore, 1, 4, aFP);
			int aChecksum;
			fread(&aChecksum, 1, 4, aFP);

			int aCheck = 0x42BEEF ^ aScore;
			for (int i = 0; i < aLen; i++)
				aCheck ^= 17 * aName[i];

			if ((aDay == GetDay()) && (aChecksum == aCheck))
			{
				mTodayNames[aMode] = aName;
				mTodayScores[aMode] = aScore;
			}

			for (int j = 0; j < 5; j++)
			{
				unsigned short aLen;
				fread(&aLen, 1, 2, aFP);
				if (aLen > 255)
					aLen = 255;
				fread(aName, aLen, 1, aFP);
				aName[aLen] = 0;
				int aScore;
				fread(&aScore, 1, 4, aFP);
				int aChecksum;
				fread(&aChecksum, 1, 4, aFP);

				int aCheck = 0x42BEEF ^ aScore;
				for (int i = 0; i < aLen; i++)
					aCheck ^= 17 * aName[i];

				mNames[aMode][j] = aName;
				if (aChecksum == aCheck)
					mScores[aMode][j] = aScore;
				else
					mScores[aMode][j] = aScore % 256;
			}
		}

		fclose(aFP);
	}
}

// FUNCTION: GAME 0x4068c4
// FUNCTION: GAME187 0x4cf1b4
void HighScoreWidget::Save()
{
#if BEJEWELED_VERSION >= 1870
	DeleteFileA((GetAppDataFolder() + "userdata\\hiscores.dat").c_str());
	MkDir(GetAppDataFolder() + "userdata");
	FILE* aFP = fopen((GetAppDataFolder() + "userdata\\hiscores.dat").c_str(), "wb");
#else
	DeleteFileA("hiscores.dat");
	FILE* aFP = fopen("hiscores.dat", "wb");
#endif
	if (aFP == NULL)
	{
		mApp->Popup("Failed to create high score file!\nPlease report to support@popcap.com.");
		return;
	}

	for (int aMode = 0; aMode < 2; aMode++)
	{
		int aDay = GetDay();
		fwrite(&aDay, 1, 4, aFP);

		short aLen = (short)mTodayNames[aMode].length();
		const char* aName = mTodayNames[aMode].c_str();
		fwrite(&aLen, 1, 2, aFP);
		fwrite(aName, aLen, 1, aFP);
		int aScore = mTodayScores[aMode];
		fwrite(&aScore, 1, 4, aFP);
		int aChecksum = 0x42BEEF ^ aScore;
		for (int i = 0; i < aLen; i++)
			aChecksum ^= 17 * aName[i];
		fwrite(&aChecksum, 1, 4, aFP);

		for (int j = 0; j < 5; j++)
		{
			short aLen = (short)mNames[aMode][j].length();
			const char* aName = mNames[aMode][j].c_str();
			fwrite(&aLen, 1, 2, aFP);
			fwrite(aName, aLen, 1, aFP);
			int aScore = mScores[aMode][j];
			fwrite(&aScore, 1, 4, aFP);
			int aChecksum = 0x42BEEF ^ aScore;
			for (int i = 0; i < aLen; i++)
				aChecksum ^= 17 * aName[i];
			fwrite(&aChecksum, 1, 4, aFP);
		}
	}

	fclose(aFP);
}

// FUNCTION: GAME 0x406ae4
// FUNCTION: GAME187 0x4cf4a7
std::string HighScoreWidget::GetOrdinal(int theNum)
{
	if ((theNum % 100) / 10 == 1)
		return "th";

	switch (theNum % 10)
	{
	case 1:
		return "st";
	case 2:
		return "nd";
	case 3:
		return "rd";
	default:
		return "th";
	}
}

// FUNCTION: GAME 0x406b47
// FUNCTION: GAME187 0x4cf50a
void HighScoreWidget::Draw(Graphics* g)
{
	char aBuffer[256];
	std::string aString;

	g->SetColor(Color::Black);
	g->FillRect(0, 0, mWidth, mHeight);

	int aClipY = mHeight + mSlideY;
	if (aClipY > 436)
		aClipY = 436;

	Graphics aG(*g);
	aG.ClipRect(0, 0, mWidth, aClipY);
	aG.Translate(0, mSlideY);

	aG.SetColor(Color(21, 0, 56));
	aG.FillRect(0, 0, mWidth, mHeight);

	aG.SetFont(mApp->m_unk0x68c);
	aG.SetColor(Color(255, 0, 177));
	aString = mApp->GetString("HSWIDGET_GAME_OVER");
	aG.DrawString(aString, 408 - aG.GetFont()->StringWidth(aString) / 2, 45);

	aG.SetFont(mApp->m_unk0x690);
	aG.SetColor(Color(252, 0, 195));
	aString = mApp->GetString("HSWIDGET_GAME_DURATION");
	aG.DrawString(aString, 341 - aG.GetFont()->StringWidth(aString), 88);
	aString = mApp->GetString("HSWIDGET_GEMS_CLEARED");
	aG.DrawString(aString, 341 - aG.GetFont()->StringWidth(aString), 105);
	aString = mApp->GetString("HSWIDGET_LONGEST_CASCADE");
	aG.DrawString(aString, 546 - aG.GetFont()->StringWidth(aString), 88);
	aString = mApp->GetString("HSWIDGET_BEST_SINGLE_MOVE");
	aG.DrawString(aString, 546 - aG.GetFont()->StringWidth(aString), 105);

	aG.SetColor(Color(255, 100, 220));
	sprintf(aBuffer, "%2d.%02d", mApp->mBoard->mGameTime / 60, mApp->mBoard->mGameTime % 60);
	aG.DrawString(aBuffer, 382 - aG.GetFont()->StringWidth(aBuffer), 88);
	sprintf(aBuffer, "%d", mApp->mBoard->mGemsCleared);
	aG.DrawString(aBuffer, 382 - aG.GetFont()->StringWidth(aBuffer), 105);
	sprintf(aBuffer, "%d", mApp->mBoard->mLongestCascade);
	aG.DrawString(aBuffer, 587 - aG.GetFont()->StringWidth(aBuffer), 88);
	sprintf(aBuffer, "%d", mApp->mBoard->mBestMove);
	aG.DrawString(aBuffer, 587 - aG.GetFont()->StringWidth(aBuffer), 105);

	aG.SetFont(mApp->m_unk0x690);
	aG.SetColor(Color(250, 0, 0));
	aG.DrawString(mTip, 408 - aG.GetFont()->StringWidth(mTip) / 2, 136);

	aG.SetColor(Color(0, 218, 0));
	aG.FillRect(191, 151, 434, 23);

	aG.SetColor(Color(0, 0, 0));
	aG.SetFont(mApp->m_unk0x68c);
	aString = mApp->GetString("HSWIDGET_HIGH_SCORES");
	aG.DrawString(aString, (434 - aG.GetFont()->StringWidth(aString)) / 2 + 191, (aG.GetFont()->GetAscent() + 23) / 2 + 150);

	aG.SetColor(Color(0, 218, 0));
	aG.FillRect(191, 389, 434, 23);

	for (int i = 0; i < 2; i++)
	{
		int aCenterX = 238 * i + 287;

		aG.SetFont(mApp->m_unk0x690);

		aString = (i == 0) ? mApp->GetString("HSWIDGET_NORMAL") : mApp->GetString("HSWIDGET_TIMED");
		int aWidth = aG.GetFont()->StringWidth(aString);
		aG.SetColor(Color(255, 193, 0));
		int aX = aCenterX - aWidth / 2;
		aG.DrawString(aString, aX, 197);
		aG.FillRect(aX, 200, aWidth, 1);

		aString = mApp->GetString("HSWIDGET_TODAYS_HIGH_SCORE");
		aWidth = aG.GetFont()->StringWidth(aString);
		aG.SetColor(Color(46, 216, 0));
		aX = aCenterX - aWidth / 2;
		aG.DrawString(aString, aX, 223);
		aG.FillRect(aX, 226, aWidth, 1);

		aG.SetColor(mApp->HSLToRGB(20 * mColorCycle % 256, 255, 180));
		if (mTodayScores[i] > 0)
		{
			aString = mTodayNames[i].c_str();
			aG.DrawString(aString, aCenterX - 74, 249);

			sprintf(aBuffer, "%d", mTodayScores[i]);
			aWidth = aG.GetFont()->StringWidth(aBuffer);
			aG.DrawString(aBuffer, aCenterX - aWidth + 74, 249);
		}
		else
		{
			aString = mApp->GetString("HSWIDGET_EMPTY");
			aG.DrawString(aString, aCenterX - aG.GetFont()->StringWidth(aString) / 2, 249);
		}

		aString = mApp->GetString("HSWIDGET_ALL_TIME_HIGH_SCORES");
		aWidth = aG.GetFont()->StringWidth(aString);
		aG.SetColor(Color(46, 216, 0));
		aX = aCenterX - aWidth / 2;
		aG.DrawString(aString, aX, 279);
		aG.FillRect(aX, 282, aWidth, 1);

		for (int j = 0; j < 5; j++)
		{
			int aY = 303 + j * 17;

			aG.SetColor(mApp->HSLToRGB(20 * (j + mColorCycle + 2) % 256, 255, 180));

			aString = mNames[i][j].c_str();
			aG.DrawString(aString, aCenterX - 74, aY);

			sprintf(aBuffer, "%d", mScores[i][j]);
			aWidth = aG.GetFont()->StringWidth(aBuffer);
			aG.DrawString(aBuffer, aCenterX - aWidth + 74, aY);
		}
	}

	aG.Translate(0, -mSlideY);
	aG.DrawImage(mApp->mImages[7], 0, 0);

	aG.SetFont(mApp->m_unk0x688);
	char aScoreStr[12];
	sprintf(aScoreStr, "%d", mApp->mBoard->mScore);
	aG.SetColor(Color(173, 231, 255));
	aG.DrawString(aScoreStr, 93 - aG.GetFont()->StringWidth(aScoreStr) / 2, 106);

	Graphics aBoardG(*g);
	aBoardG.ClipRect(0, aClipY, mWidth, mHeight - aClipY);
	aBoardG.DrawImage(mBoardImage, 0, 0);
}

// FUNCTION: GAME 0x407af8
// FUNCTION: GAME187 0x4d06d8
void HighScoreWidget::SubmitScore(const std::string& theName)
{
	if (theName.length() == 0)
		return;

	int aScore = mApp->mBoard->mScore;
	int aMode = mApp->mBoard->mTimeTrial ? 1 : 0;
	int aRank = GetRank(aMode, aScore);

	if (aScore > mTodayScores[aMode])
	{
		mTodayScores[aMode] = aScore;
		mTodayNames[aMode] = theName;
	}

	if (aRank != 0)
	{
		for (int i = 4; i >= aRank; i--)
		{
			mNames[aMode][i] = mNames[aMode][i - 1];
			mScores[aMode][i] = mScores[aMode][i - 1];
		}
		mNames[aMode][aRank - 1] = theName;
		mScores[aMode][aRank - 1] = aScore;
	}

	Save();
}

// FUNCTION: GAME 0x407be8
// FUNCTION: GAME187 0x4d07cd
HighScoreWidget::HighScoreWidget(WinDMApp* theApp)
{
	mSlideY = 0;
	mApp = theApp;
	mColorCycle = 0;

	mPlayAgainLink = new HyperlinkWidget(0, this);
	mPlayAgainLink->mLabel = mApp->GetString("HSWIDGET_CLICK_HERE_TO_PLAY_AGAIN");
	mPlayAgainLink->SetFont(mApp->m_unk0x68c);
	mPlayAgainLink->mColor = Color(0, 0, 0);
	mPlayAgainLink->mOverColor = Color(255, 255, 255);
	mPlayAgainLink->mUnderlineSize = 0;

	mBoardImage = NULL;

	switch (Rand() % 9)
	{
	case 0:
		mTip = mApp->GetString("HSWIDGET_TIP0");
		break;
	case 1:
		mTip = mApp->GetString("HSWIDGET_TIP1");
		break;
	case 2:
		mTip = mApp->GetString("HSWIDGET_TIP2");
		break;
	case 3:
		mTip = mApp->GetString("HSWIDGET_TIP3");
		break;
	case 4:
		mTip = mApp->GetString("HSWIDGET_TIP4");
		break;
	case 5:
		mTip = mApp->GetString("HSWIDGET_TIP5");
		break;
	case 6:
		mTip = mApp->GetString("HSWIDGET_TIP6");
		break;
	case 7:
		mTip = mApp->GetString("HSWIDGET_TIP7");
		break;
	default:
		mTip = mApp->GetString("HSWIDGET_TIP_DEFAULT");
		break;
	}

	mTodayScores[0] = 0;
	mNames[0][0] = "Ace";
	mScores[0][0] = 1000;
	mNames[0][1] = "John";
	mScores[0][1] = 900;
	mNames[0][2] = "Chris";
	mScores[0][2] = 800;
	mNames[0][3] = "Brian";
	mScores[0][3] = 700;
	mNames[0][4] = "Ben";
	mScores[0][4] = 600;

	mTodayScores[1] = 0;
	mNames[1][0] = "Bradley";
	mScores[1][0] = 1000;
	mNames[1][1] = "Mike";
	mScores[1][1] = 900;
	mNames[1][2] = "Billy";
	mScores[1][2] = 800;
	mNames[1][3] = "Jason";
	mScores[1][3] = 700;
	mNames[1][4] = "Wim";
	mScores[1][4] = 600;

	Load();
}

// FUNCTION: GAME 0x40816a
// FUNCTION: GAME187 0x4d0dad
HighScoreWidget::~HighScoreWidget()
{
	delete mBoardImage;
	delete mPlayAgainLink;
	Save();
}

// FUNCTION: GAME 0x408203
// FUNCTION: GAME187 0x4d0e4b
void HighScoreWidget::DoShowScoreMessage()
{
	int aScore = mApp->mBoard->mScore;
	int aMode = mApp->mTimeTrial ? 1 : 0;
	std::string aMessage;
	bool isTodayBest = aScore > mTodayScores[aMode];
	int aRank = GetRank(aMode, aScore);
	char aRankStr[4];

	if (!mApp->mIsRegistered)
	{
		if (isTodayBest)
		{
			if (aRank > 0)
			{
				sprintf(aRankStr, "%d", aRank);
				aMessage = "You got the highest score of\nthe day and " + (aRankStr + GetOrdinal(aRank)) + " place\nof all time!\n\nWould you like to register so\nyou can record your score?";
			}
			else
				aMessage = "You got the highest score\nof the day!\n\nWould you like to register so\nyou can record your score?";

			mApp->ShowHighScoreMessage(aMessage);
		}
		else if (aRank > 0)
		{
			sprintf(aRankStr, "%d", aRank);
			aMessage = "You got " + (aRankStr + GetOrdinal(aRank)) + " place of all time!\n\nWould you like to register so\nyou can record your score?";
			mApp->ShowHighScoreMessage(aMessage);
		}
	}
	else
	{
		if (isTodayBest)
		{
			if (aRank > 0)
			{
				sprintf(aRankStr, "%d", aRank);
				aMessage = "You got the highest score of\nthe day and " + (aRankStr + GetOrdinal(aRank)) + " place\nof all time!";
			}
			else
				aMessage = "You got the highest score\nof the day!";

			mApp->ShowRecordScoreDialog(aMessage);
		}
		else if (aRank > 0)
		{
			sprintf(aRankStr, "%d", aRank);
			aMessage = "You got " + (aRankStr + GetOrdinal(aRank)) + " place of all time!";
			mApp->ShowRecordScoreDialog(aMessage);
		}
	}
}
// FUNCTION: GAME 0x40853d
// FUNCTION: GAME187 0x4d1187
void HighScoreWidget::Update()
{
	Widget::Update();

	if (mSlideY < 0)
	{
		mSlideY += 8;
		if (mSlideY >= 0)
		{
			mSlideY = 0;
			DoShowScoreMessage();
		}
		MarkDirty();
		mPlayAgainLink->Resize(265, mSlideY + 389, 286, 23);
	}

	if (mUpdateCnt % 10 == 0)
	{
		mColorCycle++;
		MarkDirty();
	}
}

// FUNCTION: GAME 0x4085a9
// FUNCTION: GAME187 0x4d11fd
void HighScoreWidget::ShowScoreMessage()
{
	DoShowScoreMessage();
}