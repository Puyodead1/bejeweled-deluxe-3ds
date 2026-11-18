#include "TitleScreen.h"
#include <stdio.h>
#include "WinDMApp.h"
#include "SexyAppFramework/HyperlinkWidget.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Font.h"
#include "SexyAppFramework/Image.h"
#include "SexyAppFramework/Color.h"

using namespace Sexy;

// FUNCTION: GAME 0x40adbc
// FUNCTION: GAME187 0x4d360b
TitleScreen::~TitleScreen()
{
	delete mRegisterLink;
	delete mStartLink;
}

// FUNCTION: GAME 0x40ae16
// FUNCTION: GAME187 0x4d3661
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterLink);
	theWidgetManager->AddWidget(mStartLink);
}

// FUNCTION: GAME 0x40ae45
// FUNCTION: GAME187 0x4d3691
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterLink);
	theWidgetManager->RemoveWidget(mStartLink);
}

// FUNCTION: GAME 0x40ae74
// FUNCTION: GAME187 0x4d36c1
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == 0)
	{
		mRegisterClicked = true;
	}
	else if (mDemoExpired)
	{
		mApp->Shutdown();
	}
	else
	{
		mApp->FinishLoading();
	}
}

// FUNCTION: GAME 0x40af0e
// FUNCTION: GAME187 0x4d370b
void TitleScreen::Draw(Graphics* g)
{
	// GLOBAL: GAME 0x4f652c
	// $S1

	// GLOBAL: GAME 0x4f6528
	static int aLoadingBarY = mApp->GetInteger("LoadingBarY", 275);
	// GLOBAL: GAME 0x4f6524
	static int aRegisterStringY = mApp->GetInteger("RegisterStringY", 319);
	// GLOBAL: GAME 0x4f6520
	static int aMaxPlays = mApp->GetInteger("MaxPlays", 15);
	// GLOBAL: GAME 0x4f651c
	static int aMaxExecutions = mApp->GetInteger("MaxExecutions", 0);

	g->DrawImage(mApp->mImages[0], 0, 0);
#if BEJEWELED_VERSION >= 1870
	if (!mApp->mIsRegistered && !mApp->GetBoolean("Cobranded", false))
		g->DrawImage(mApp->mImages[2], 31, 346);
#else
	if (!mApp->mIsRegistered)
	{
		if (!mApp->GetBoolean("Cobranded", false))
			g->DrawImage(mApp->mImages[2], 31, 346);
	}
#endif

	g->DrawImage(mApp->mImages[1], 143, aLoadingBarY, Rect(0, 0, (int)(mCurBarWidth * 340.0), mApp->mImages[1]->GetHeight()));

	g->SetFont(mApp->m_unk0x690);
	g->SetColor(Color(255, 64, 255));

	char aBuffer[256];
	if (!mApp->mIsRegistered)
	{
		if (mDemoExpired)
		{
			if (mApp->mIsDiamondMine)
				sprintf(aBuffer, mApp->GetString("YOU_MUST_REG1").c_str());
			else
				sprintf(aBuffer, mApp->GetString("YOU_MUST_REG2").c_str());
		}
#if BEJEWELED_VERSION < 1870
		else if (mApp->mMaxTime > 0)
		{
			int aMinutesLeft = mApp->mMaxTime - mApp->mTimesPlayed;
			if (aMinutesLeft == 1)
				sprintf(aBuffer, "YOU HAVE %d MINUTE LEFT IN THIS TRIAL", aMinutesLeft);
			else
				sprintf(aBuffer, "YOU HAVE %d MINUTES LEFT IN THIS TRIAL", aMinutesLeft);
		}
#endif
		else if (aMaxExecutions == 0)
		{
#if BEJEWELED_VERSION >= 1870
			// 1.87 uses the plural string for both cases.
#endif
			int aPlaysLeft = aMaxPlays - mApp->mTimesPlayed;
			if (aPlaysLeft == 1)
#if BEJEWELED_VERSION >= 1870
				sprintf(aBuffer, "YOU HAVE %d PLAYS REMAINING", aPlaysLeft);
#else
				sprintf(aBuffer, "YOU HAVE %d PLAY REMAINING", aPlaysLeft);
#endif
			else
				sprintf(aBuffer, "YOU HAVE %d PLAYS REMAINING", aPlaysLeft);
		}
		else
		{
			int aRunsLeft = aMaxExecutions - mApp->mTimesExecuted;
			if (aRunsLeft == 1)
				sprintf(aBuffer, "YOU CAN RUN BEJEWELED %d MORE TIME FOR FREE", aRunsLeft);
			else
				sprintf(aBuffer, "YOU CAN RUN BEJEWELED %d MORE TIMES FOR FREE", aRunsLeft);
		}
	}
	else
	{
		sprintf(aBuffer, mApp->GetString("THANKS_FOR_REGISTERING").c_str());
	}

	if (!mApp->mBuildUnlocked)
		WriteCenteredLine(g, aRegisterStringY, aBuffer);

	g->SetColor(Color(128, 128, 128));
	std::string aVersion = mApp->GetString("VERSION") + " " + std::string(mApp->mProductVersion);
	g->DrawString(aVersion, 537 - g->GetFont()->StringWidth(aVersion) / 2, 221);
}

// FUNCTION: GAME 0x40b3b8
// FUNCTION: GAME187 0x4d3c6a
void TitleScreen::Update()
{
	// GLOBAL: GAME 0x4f653c
	// $S2

	// GLOBAL: GAME 0x4f6538
	static int aPlayLinkY = mApp->GetInteger("PlayLinkY", 386);
	// GLOBAL: GAME 0x4f6534
	static int aMaxPlays = mApp->GetInteger("MaxPlays", 15);
	// GLOBAL: GAME 0x4f6530
	static int aMaxExecutions = mApp->GetInteger("MaxExecutions", 0);

	Widget::Update();

#if BEJEWELED_VERSION >= 1870
	mDemoExpired = (mApp->mTimesPlayed >= aMaxPlays) && !mApp->mIsRegistered;
#else
	if (mApp->mMaxTime > 0)
		mDemoExpired = (mApp->mTimesPlayed >= mApp->mMaxTime) && !mApp->mIsRegistered;
	else
		mDemoExpired = (mApp->mTimesPlayed >= aMaxPlays) && !mApp->mIsRegistered;
#endif

	if (aMaxExecutions > 0)
		mDemoExpired = (mApp->mTimesExecuted > aMaxExecutions) && !mApp->mIsRegistered;

	if (mDemoExpired)
		mApp->mTimedOut = true;

	if (!mDemoExpired)
	{
		mStartLink->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY");
		int aWidth = mStartLink->mFont->StringWidth(mStartLink->mLabel);
		mStartLink->Resize((mWidth - aWidth) / 2, aPlayLinkY, aWidth, 24);

		mRegisterLink->mLabel = mApp->GetString("REGISTER_NOW_TO_REMOVE_THIS_DELAY");

		mTimerProgress += 0.01 / mTotalTimerSec;
		if (mTimerProgress > 1.0)
			mTimerProgress = 1.0;

		int aCount = (int)((mApp->mTimesPlayed + 1) * mTimerProgress);
		if ((aCount <= mApp->mTimesPlayed) && (aCount > m_unk0x74))
		{
			m_unk0x74 = aCount;
			MarkDirty();
		}

		if ((!mLoadingComplete) && (mUpdateCnt % 4 == 0))
		{
			if (mApp->mIsRegistered)
				mCurBarWidth = mApp->GetLoadingThreadProgress();
			else
				mCurBarWidth = min(mTimerProgress, mApp->GetLoadingThreadProgress());

			if ((mCurBarWidth == 1.0) && (mApp->mLoaded))
			{
				mStartLink->SetVisible(true);
				mCurBarWidth = 1.0;
				mLoadingComplete = true;
			}

			MarkDirty();
		}
	}
	else
	{
		if (mUpdateCnt == 1000)
		{
			mStartLink->mLabel = mApp->GetString("CLICK_HERE_TO_QUIT");
			int aWidth = mStartLink->mFont->StringWidth(mStartLink->mLabel);
			mStartLink->Resize((mWidth - aWidth) / 2, 386, aWidth, 24);
			mStartLink->SetVisible(true);
		}

		mRegisterLink->mLabel = mApp->GetString("CLICK_HERE_TO_REGISTER_NOW");
	}

	mRegisterLink->SetVisible(!mApp->mIsRegistered);

	if ((mApp->mLoaded) && (mRegisterClicked))
	{
		mApp->DoRegisterDialog();
		mRegisterClicked = false;
	}
}

// FUNCTION: GAME 0x40b7f8
// FUNCTION: GAME187 0x4d4118
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// GLOBAL: GAME 0x4f6544
	// $S3

	// GLOBAL: GAME 0x4f6540
	static int aRegisterLinkY = mApp->GetInteger("RegisterLinkY", 324);

	Widget::Resize(theX, theY, theWidth, theHeight);

	int aWidth = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel);
	mRegisterLink->Resize((mWidth - aWidth) / 2, aRegisterLinkY, aWidth, 16);
}

// FUNCTION: GAME 0x40b8a6
// FUNCTION: GAME187 0x4d41c8
TitleScreen::TitleScreen(WinDMApp* theApp)
	: mCurBarWidth(0.0), mTimerProgress(0.0), mLoadingComplete(false), mRegisterClicked(false)
{
	mApp = theApp;

#if BEJEWELED_VERSION >= 1870
	int aPlays = mApp->mTimesPlayed;
	if (aPlays <= 10)
		mTotalTimerSec = 10;
	else if (aPlays <= 20)
		mTotalTimerSec = aPlays;
	else if (aPlays <= 50)
		mTotalTimerSec = 2 * aPlays - 20;
#else
	if (mApp->mMaxTime == 0)
	{
		int aPlays = mApp->mTimesPlayed;
		if (aPlays <= 10)
			mTotalTimerSec = 10;
		else if (aPlays <= 20)
			mTotalTimerSec = aPlays;
		else if (aPlays <= 50)
			mTotalTimerSec = 2 * aPlays - 20;
		else
			mTotalTimerSec = 80;
	}
#endif
	else
#if BEJEWELED_VERSION >= 1870
		mTotalTimerSec = 80;
	mTotalTimerSec = mApp->GetInteger("TotalTimerSec", mTotalTimerSec);
#else
	{
		mTotalTimerSec = 0;
	}
#endif

#if BEJEWELED_VERSION >= 1870
	if (mApp->mTimesPlayed < 2)
		m_unk0x74 = 0;
	else if (mApp->mTimesPlayed < 10)
		m_unk0x74 = mApp->mTimesPlayed;
#else
	if (mApp->mMaxTime == 0)
	{
		if (mApp->mTimesPlayed >= 2)
		{
			if (mApp->mTimesPlayed < 10)
				m_unk0x74 = mApp->mTimesPlayed;
			else
				m_unk0x74 = 2;
		}
		else
			m_unk0x74 = 0;
	}
#endif
	else
#if BEJEWELED_VERSION >= 1870
		m_unk0x74 = 2;
#else
		m_unk0x74 = mApp->mTimesPlayed;

	mTotalTimerSec = mApp->GetInteger("TotalTimerSec", mTotalTimerSec);
#endif

	mRegisterLink = new HyperlinkWidget(0, this);
	mRegisterLink->SetFont(mApp->m_unk0x690);
	mRegisterLink->mColor = Color(32, 255, 32);
	mRegisterLink->SetVisible(!mApp->mIsRegistered);

	mStartLink = new HyperlinkWidget(1, this);
	mStartLink->SetFont(mApp->m_unk0x68c);
	mStartLink->SetVisible(false);
	mStartLink->mColor = Color(32, 255, 32);

	Update();
}