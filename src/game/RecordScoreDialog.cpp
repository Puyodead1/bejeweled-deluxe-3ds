#include "RecordScoreDialog.h"
#include "WinDMApp.h"
#include "SexyAppFramework/EditWidget.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/Rect.h"
#include "CtrPlatform.h"

using namespace Sexy;

namespace
{

// Console port: a tap on the name box brings up the system keyboard; there's no
// physical one to type with.
class NameEditWidget : public EditWidget
{
public:
	NameEditWidget(RecordScoreDialog* theDialog) : EditWidget(0, theDialog), mDialog(theDialog) {}

	virtual void MouseUp(int x, int y, int theBtnNum, int theClickCount)
	{
		EditWidget::MouseUp(x, y, theBtnNum, theClickCount);
		if (mIsOver)
			mDialog->OpenKeyboard();
	}

	RecordScoreDialog* mDialog;
};

}

// GLOBAL: GAME 0x4f1238
int gRecordScoreEditColors[][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {240, 240, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}};

// FUNCTION: GAME 0x40997e
// FUNCTION: GAME187 0x4d2728
RecordScoreDialog::~RecordScoreDialog()
{
	delete mNameEdit;
}

// FUNCTION: GAME 0x4099d2
// FUNCTION: GAME187 0x4d2778
int RecordScoreDialog::GetPreferredHeight(int theWidth)
{
	return Dialog::GetPreferredHeight(theWidth) + 56;
}

// FUNCTION: GAME 0x4099e4
// FUNCTION: GAME187 0x4d278a
void RecordScoreDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNameEdit);
	theWidgetManager->SetFocus(mNameEdit);
}

// FUNCTION: GAME 0x409a11
// FUNCTION: GAME187 0x4d27b8
void RecordScoreDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNameEdit);
}

// FUNCTION: GAME 0x409a32
// FUNCTION: GAME187 0x4d27db
void RecordScoreDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mNameEdit->Resize(mX + 48, mY + mHeight - 100, mWidth - 96, 24);
}

// FUNCTION: GAME 0x409a76
// FUNCTION: GAME187 0x4d2822
void RecordScoreDialog::EditWidgetText(int theId, const std::string& theString)
{
	mApp->ButtonDepress(mId + 2000);
}

void RecordScoreDialog::Update()
{
	Dialog::Update();

	// Let the dialog be seen for a moment, then ask for the name.
	if (!mKeyboardShown && mUpdateCnt >= 30 && mWidgetManager != NULL)
	{
		mKeyboardShown = true;
		OpenKeyboard();
	}
}

void RecordScoreDialog::OpenKeyboard()
{
	mKeyboardShown = true;

	std::string aName;
	if (!CtrPlatform::ShowKeyboard(mApp->GetString("RECORDSCOREDLG_ENTER_YOUR_NAME"), mNameEdit->mString, 12, aName))
		return;	// cancelled: the dialog stays, for another try or Cancel

	mNameEdit->SetText(aName);
	mNameEdit->EnforceMaxPixels();
	mNameEdit->SetText(mNameEdit->mString);	// puts the cursor back on the end after any trim
	mApp->ButtonDepress(mId + 2000);
}

// FUNCTION: GAME 0x409acd
void RecordScoreDialog::DrawEditBox(Graphics* g, const Rect& theRect)
{
	g->DrawImage(mApp->mImages[33], theRect.mX, theRect.mY, Rect(0, 0, 32, 32));
	g->DrawImage(mApp->mImages[33], theRect.mX + theRect.mWidth - 32, theRect.mY, Rect(64, 0, 32, 32));

	Graphics aClipG(*g);
	aClipG.ClipRect(theRect.mX + 32, theRect.mY, theRect.mWidth - 32, 32);
	for (int i = 0; i < (theRect.mWidth - 33) / 32; i++)
		aClipG.DrawImage(mApp->mImages[33], theRect.mX + i * 32 + 32, theRect.mY, Rect(32, 0, 32, 32));
}

// FUNCTION: GAME 0x409c2e
// FUNCTION: GAME187 0x4d29b6
RecordScoreDialog::RecordScoreDialog(WinDMApp* theApp, const std::string& theLines)
	: Dialog(theApp->mImages[10], theApp->mImages[11], 8, true, theApp->GetString("RECORDSCOREDLG_HIGH_SCORE"), theLines, "", 2)
{
	mApp = theApp;
	mKeyboardShown = false;

	SetButtonFont(theApp->m_unk0x690);
	SetHeaderFont(mApp->m_unk0x68c);
	SetLinesFont(mApp->m_unk0x690);

	mNameEdit = new NameEditWidget(this);
	mNameEdit->SetFont(mApp->m_unk0x690);
	mNameEdit->SetColors(gRecordScoreEditColors, 5);
	mNameEdit->mMaxPixels = 104;
	mNameEdit->SetText(mApp->mLastScoreName);
}

// FUNCTION: GAME 0x409da8
// FUNCTION: GAME187 0x4d2b34
void RecordScoreDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	Rect aRect = mNameEdit->GetRect();
	aRect.Offset(-mX, -mY);
	aRect.Inflate(4, 4);
	DrawEditBox(g, aRect);

	g->SetFont(mApp->m_unk0x690);
	g->SetColor(Color(255, 255, 0));
	g->DrawString(mApp->GetString("RECORDSCOREDLG_ENTER_YOUR_NAME"), mNameEdit->mX - mX, mNameEdit->mY - mY - 8);
}