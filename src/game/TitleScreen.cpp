#include "TitleScreen.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include "WinDMApp.h"
#include "SexyAppFramework/HyperlinkWidget.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Font.h"
#include "SexyAppFramework/Image.h"
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/DDInterface.h"
#include "SexyAppFramework/DDImage.h"

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

	// 3DS: the bottom screen is the one you tap, so the loading bar and "tap here" stay there; the top screen shows
	// the title art by itself, now and (in its default view) for the rest of the game.
	if (mApp->mDDInterface != NULL)
	{
		SetTopScreenArt();
		mApp->mDDInterface->ForceTopImage(true);
	}
}

// The title art without the loading bar's empty groove, which is part of the picture: a patch of the plain background
// just below covers it.
void TitleScreen::SetTopScreenArt()
{
	MemoryImage* aTitle = mApp->mImages[0];
	if (aTitle == NULL)
		return;
	int aWidth = aTitle->mWidth, aHeight = aTitle->mHeight;
	bool aRepurge = aTitle->mBits == NULL && aTitle->mD3DData != NULL && aTitle->mPurgeBits;

	MemoryImage anArt;
	anArt.Create(aWidth, aHeight);
	uint32_t* aBits = anArt.GetBits();
	memcpy(aBits, aTitle->GetBits(), aWidth * aHeight * 4);
	if (aRepurge)
		aTitle->PurgeBits();

	// The groove (with its shadow and highlight) around the bar Draw puts at (143, LoadingBarY), 340 wide, measured on
	// the 640x480 art. The background gets darker downwards, so the patch's brightness is corrected to match the rows
	// just above and below it (per column, blended from top to bottom), and its ends fade into the original.
	int aBarY = mApp->GetInteger("LoadingBarY", 275);
	const int aLeft = 124, aRight = 491, aTop = aBarY - 20, aBottom = aBarY + 38;
	const int aSrcOffset = aBottom - aTop + 2;		// rows below, plain background down to the PopCap logo
	const int aFade = 4, aBand = 3, aSmooth = 6;
	if (aLeft - aSmooth < 0 || aRight + aSmooth > aWidth || aTop - aBand < 0 || aBottom + aBand + aSrcOffset > aHeight)
		return;

	// Per column and channel: original minus patch, averaged over aBand rows just above and just below the patch.
	const int aCols = aRight - aLeft + 2 * aSmooth;
	std::vector<int> aRawDiff[2], aDiff[2];
	for (int e = 0; e < 2; e++)
	{
		aRawDiff[e].assign(aCols * 3, 0);
		aDiff[e].assign((aRight - aLeft) * 3, 0);
		int aRow0 = e == 0 ? aTop - aBand : aBottom;
		for (int i = 0; i < aCols; i++)
		{
			int x = std::min(aLeft - aSmooth + i, aRight - 1);	// not right of the patch: the PopCap logo
			for (int r = aRow0; r < aRow0 + aBand; r++)
			{
				uint32_t o = aBits[r * aWidth + x], s = aBits[(r + aSrcOffset) * aWidth + x];
				for (int c = 0; c < 3; c++)
					aRawDiff[e][i * 3 + c] += (int)((o >> (c * 8)) & 0xFF) - (int)((s >> (c * 8)) & 0xFF);
			}
		}
		// Smoothed across columns, so the background's grain doesn't streak down the patch.
		for (int i = 0; i < aRight - aLeft; i++)
			for (int c = 0; c < 3; c++)
			{
				int aSum = 0;
				for (int k = 0; k <= 2 * aSmooth; k++)
					aSum += aRawDiff[e][(i + k) * 3 + c];
				aDiff[e][i * 3 + c] = aSum / ((2 * aSmooth + 1) * aBand);
			}
	}

	for (int y = aTop; y < aBottom; y++)
	{
		int t = (y - aTop) * 256 / (aBottom - aTop - 1);
		for (int x = aLeft; x < aRight; x++)
		{
			int i = x - aLeft;
			int anEdge = std::min(x - aLeft, aRight - 1 - x);
			int aWeight = std::min(256, (anEdge + 1) * 256 / (aFade + 1));
			uint32_t aDest = aBits[y * aWidth + x];
			uint32_t aSrc = aBits[(y + aSrcOffset) * aWidth + x];
			uint32_t anOut = 0xFF000000;
			for (int c = 0; c < 3; c++)
			{
				int aCorr = (aDiff[0][i * 3 + c] * (256 - t) + aDiff[1][i * 3 + c] * t) / 256;
				int s = std::max(0, std::min(255, (int)((aSrc >> (c * 8)) & 0xFF) + aCorr));
				int d = (aDest >> (c * 8)) & 0xFF;
				anOut |= (uint32_t)((d * (256 - aWeight) + s * aWeight) >> 8) << (c * 8);
			}
			aBits[y * aWidth + x] = anOut;
		}
	}

	mApp->mDDInterface->SetTopImage(&anArt);
}

// FUNCTION: GAME 0x40ae45
// FUNCTION: GAME187 0x4d3691
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mStartLink);

	// The art stays for the top screen's art view (SetTopView, L/R); the others come back now.
	if (mApp->mDDInterface != NULL)
		mApp->mDDInterface->ForceTopImage(false);
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
