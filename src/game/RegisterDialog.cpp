#include "RegisterDialog.h"
#include "WinDMApp.h"
#include "SexyAppFramework/EditWidget.h"
#include "SexyAppFramework/HyperlinkWidget.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/Rect.h"

using namespace Sexy;

// GLOBAL: GAME 0x4f12d0
int gRegisterEditColors[][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}};

// FUNCTION: GAME 0x409e97
// FUNCTION: GAME187 0x4d2c19
RegisterDialog::~RegisterDialog()
{
	delete mUserNameEdit;
	delete mRegCodeEdit;
	delete mRegisterLink;

	if (mSwitchedToWindowed)
		mApp->SwitchScreenMode(false);
}

// FUNCTION: GAME 0x409f24
// FUNCTION: GAME187 0x4d2ca2
int RegisterDialog::GetPreferredHeight(int theWidth)
{
	return 320;
}

// FUNCTION: GAME 0x409f2c
// FUNCTION: GAME187 0x4d2caa
void RegisterDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterLink);
	theWidgetManager->AddWidget(mUserNameEdit);
	theWidgetManager->AddWidget(mRegCodeEdit);
}

// FUNCTION: GAME 0x409f69
// FUNCTION: GAME187 0x4d2ce7
void RegisterDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterLink);
	theWidgetManager->RemoveWidget(mUserNameEdit);
	theWidgetManager->RemoveWidget(mRegCodeEdit);
}

// FUNCTION: GAME 0x409fa6
// FUNCTION: GAME187 0x4d2d24
void RegisterDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mRegisterLink->Resize(mX + 48, mY + 55, mWidth - 96, 24);
	mUserNameEdit->Resize(mX + 48, mY + 164, mWidth - 96, 24);
	mRegCodeEdit->Resize(mX + 48, mY + 220, mWidth - 96, 24);
}

// FUNCTION: GAME 0x40a02e
// FUNCTION: GAME187 0x4d2db5
void RegisterDialog::EditWidgetText(int theId, const std::string& theString)
{
	if (theId == 0)
		mWidgetManager->SetFocus(mRegCodeEdit);
	else
		mApp->ButtonDepress(mId + 2000);
}

// FUNCTION: GAME 0x40a164
// FUNCTION: GAME187 0x4d2e04
void RegisterDialog::DrawEditBox(Graphics* g, const Rect& theRect)
{
	g->DrawImage(mApp->mImages[33], theRect.mX, theRect.mY, Rect(0, 0, 32, 32));
	g->DrawImage(mApp->mImages[33], theRect.mX + theRect.mWidth - 32, theRect.mY, Rect(64, 0, 32, 32));

	Graphics aClipG(*g);
	aClipG.ClipRect(theRect.mX + 32, theRect.mY, theRect.mWidth - 32, 32);
	for (int i = 0; i < (theRect.mWidth - 33) / 32; i++)
		aClipG.DrawImage(mApp->mImages[33], theRect.mX + i * 32 + 32, theRect.mY, Rect(32, 0, 32, 32));
}

// FUNCTION: GAME 0x40a370
// FUNCTION: GAME187 0x4d2f3b
RegisterDialog::RegisterDialog(WinDMApp* theApp)
	: Dialog(theApp->mImages[10], theApp->mImages[11], 5, true,
		theApp->mIsDiamondMine ? theApp->GetString("REGISTER_DIAMOND_MINE") : theApp->GetString("REGISTER_BEJEWELED"),
		"", "", 2)
{
	mApp = theApp;

	SetButtonFont(theApp->m_unk0x690);
	SetHeaderFont(mApp->m_unk0x68c);
	SetLinesFont(mApp->m_unk0x690);

	mUserNameEdit = new EditWidget(0, this);
	mUserNameEdit->SetFont(mApp->m_unk0x690);
	mUserNameEdit->SetColors(gRegisterEditColors, 5);

	mRegCodeEdit = new EditWidget(1, this);
	mRegCodeEdit->SetFont(mApp->m_unk0x690);
	mRegCodeEdit->SetColors(gRegisterEditColors, 5);

	mUserNameEdit->mTabNext = mRegCodeEdit;
	mUserNameEdit->mTabPrev = mRegCodeEdit;
	mRegCodeEdit->mTabNext = mUserNameEdit;
	mRegCodeEdit->mTabPrev = mUserNameEdit;

	mRegisterLink = new HyperlinkWidget(0, this);
	mRegisterLink->mLabel = mApp->GetString("CLICK_HERE_TO_REGISTER_ONLINE");
	mRegisterLink->SetFont(mApp->m_unk0x690);
	mRegisterLink->mColor = Color(255, 255, 0);

	mSwitchedToWindowed = false;
}

// FUNCTION: GAME 0x40a6db
// FUNCTION: GAME187 0x4d32c9
void RegisterDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	g->SetFont(mApp->m_unk0x690);
	g->SetColor(Color(200, 200, 255));
	g->DrawString(mApp->GetString("WEB_BROWSER_WILL_OPEN_NOTICE1"), 32, 92);
	g->DrawString(mApp->GetString("WEB_BROWSER_WILL_OPEN_NOTICE2"), 32, 116);
	g->DrawString(mApp->GetString("WEB_BROWSER_WILL_OPEN_NOTICE3"), 32, 128);

	Rect aRect = mUserNameEdit->GetRect();
	aRect.Offset(-mX, -mY);
	aRect.Inflate(4, 4);
	DrawEditBox(g, aRect);

	g->SetFont(mApp->m_unk0x690);
	g->SetColor(Color(255, 255, 0));
	g->DrawString(mApp->GetString("REGISTERED_NAME"), mUserNameEdit->mX - mX, mUserNameEdit->mY - mY - 8);

	aRect = mRegCodeEdit->GetRect();
	aRect.Offset(-mX, -mY);
	aRect.Inflate(4, 4);
	DrawEditBox(g, aRect);

	g->SetFont(mApp->m_unk0x690);
	g->SetColor(Color(255, 255, 0));
	g->DrawString(mApp->GetString("LICENSE_CODE"), mRegCodeEdit->mX - mX, mRegCodeEdit->mY - mY - 8);
}

// FUNCTION: GAME 0x40ad69
// FUNCTION: GAME187 0x4d35b5
void RegisterDialog::ButtonDepress(int theId)
{
	if (theId != 0)
	{
		Dialog::ButtonDepress(theId);
	}
	else
	{
		mApp->OpenRegisterPage(DefinesMap());
		if (!mApp->mIsWindowed)
		{
			mSwitchedToWindowed = true;
			mApp->SwitchScreenMode(true);
		}
	}
}