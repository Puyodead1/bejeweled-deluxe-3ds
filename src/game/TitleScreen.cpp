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
	delete mStartLink;
}

// FUNCTION: GAME 0x40ae16
// FUNCTION: GAME187 0x4d3661
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mStartLink);
}

// FUNCTION: GAME 0x40ae45
// FUNCTION: GAME187 0x4d3691
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mStartLink);
}

// FUNCTION: GAME 0x40ae74
// FUNCTION: GAME187 0x4d36c1
void TitleScreen::ButtonDepress(int theId)
{
	mApp->FinishLoading();
}

// FUNCTION: GAME 0x40af0e
// FUNCTION: GAME187 0x4d370b
void TitleScreen::Draw(Graphics* g)
{
	// GLOBAL: GAME 0x4f6528
	static int aLoadingBarY = mApp->GetInteger("LoadingBarY", 275);

	g->DrawImage(mApp->mImages[0], 0, 0);
	g->DrawImage(mApp->mImages[1], 143, aLoadingBarY, Rect(0, 0, (int)(mCurBarWidth * 340.0), mApp->mImages[1]->GetHeight()));

	g->SetFont(mApp->m_unk0x690);
	g->SetColor(Color(128, 128, 128));
	std::string aVersion = mApp->GetString("VERSION") + " " + std::string(mApp->mProductVersion);
	g->DrawString(aVersion, 537 - g->GetFont()->StringWidth(aVersion) / 2, 221);
}

// FUNCTION: GAME 0x40b3b8
// FUNCTION: GAME187 0x4d3c6a
void TitleScreen::Update()
{
	// GLOBAL: GAME 0x4f6538
	static int aPlayLinkY = mApp->GetInteger("PlayLinkY", 386);

	Widget::Update();

	mStartLink->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY");
	int aWidth = mStartLink->mFont->StringWidth(mStartLink->mLabel);
	mStartLink->Resize((mWidth - aWidth) / 2, aPlayLinkY, aWidth, 24);

	if ((!mLoadingComplete) && (mUpdateCnt % 4 == 0))
	{
		// Switch port: loading from RomFS takes a fraction of a second, so the original bar (which jumps straight to
		// the progress) went from empty to full in one step. Let it fill over at least ~0.6 s instead.
		mCurBarWidth = min(mApp->GetLoadingThreadProgress(), mCurBarWidth + 0.07);

		if ((mCurBarWidth == 1.0) && (mApp->mLoaded))
		{
			mStartLink->SetVisible(true);
			mLoadingComplete = true;
		}

		MarkDirty();
	}
}

// FUNCTION: GAME 0x40b8a6
// FUNCTION: GAME187 0x4d41c8
TitleScreen::TitleScreen(WinDMApp* theApp)
	: mCurBarWidth(0.0), mLoadingComplete(false)
{
	mApp = theApp;

	mStartLink = new HyperlinkWidget(1, this);
	mStartLink->SetFont(mApp->m_unk0x68c);
	mStartLink->SetVisible(false);
	mStartLink->mColor = Color(32, 255, 32);

	Update();
}
