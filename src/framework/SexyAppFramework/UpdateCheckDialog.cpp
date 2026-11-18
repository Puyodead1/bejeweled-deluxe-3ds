#include "UpdateCheckDialog.h"
#include "SexyApp.h"
#include "WidgetManager.h"
#include "InternetManager.h"
#include "Graphics.h"

using namespace Sexy;

// FUNCTION: GAME187 0x482770
UpdateCheckDialog::UpdateCheckDialog(Image* theComponentImage, Image* theButtonComponentImage, Image* theImage3, int theId) :
	Dialog(theComponentImage, theButtonComponentImage, theId, true,
		gSexyAppBase->GetString("UPDATE_CHECK_TITLE"), gSexyAppBase->GetString("UPDATE_CHECK_BODY"),
		gSexyAppBase->GetString("DIALOG_BUTTON_CANCEL"), BUTTONS_FOOTER)
{
	m_unk0x114 = theImage3;
	m_unk0x110 = false;
	m_unk0x118 = 0;
	m_unk0x11c = 66;
}

// FUNCTION: GAME187 0x4829f0
UpdateCheckDialog::~UpdateCheckDialog()
{
}

// FUNCTION: GAME187 0x482b60
void UpdateCheckDialog::Update()
{
	Widget::Update();

	if (!m_unk0x110 && mUpdateCnt % 3 == 0)
	{
		m_unk0x118 = (m_unk0x118 + 1) % 32;
		MarkDirty();
	}

	bool aFailed = gSexyApp->mInternetManager->GetCheckStatus() == 3;
	bool aDone = gSexyApp->mInternetManager->GetCheckStatus() == 2;

	if (!m_unk0x110 && (aFailed || aDone))
	{
		m_unk0x110 = true;
		if (aDone)
			gSexyApp->mInternetManager->RemoveExistingFiles();

		Dialog* aDialog;
		if (!aFailed && !gSexyApp->mInternetManager->IsUpToDate())
		{
			aDialog = gSexyApp->DoDialog(mId + 10000, true,
				gSexyApp->GetString("NEW_VERSION_TITLE"),
				gSexyApp->GetString("NEW_VERSION_BODY"),
				"", BUTTONS_YES_NO);
		}
		else
		{
			aDialog = gSexyApp->DoDialog(mId + 20000, true,
				gSexyApp->GetString("UP_TO_DATE_TITLE"),
				gSexyApp->GetString("UP_TO_DATE_BODY"),
				gSexyApp->GetString("DIALOG_BUTTON_OK"), BUTTONS_FOOTER);
		}
		aDialog->Move(mX + 32, mY - 32);
	}
}

// FUNCTION: GAME187 0x482a20
void UpdateCheckDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	Rect aRect(mContentInsets.mLeft + 24, mHeight - m_unk0x11c - mContentInsets.mBottom,
		mWidth - mContentInsets.mRight - mContentInsets.mLeft - 48, 16);

	Graphics aClipG(*g);
	aClipG.ClipRect(aRect);
	for (int i = 0; i < aRect.mWidth / 32 + 2; i++)
		aClipG.DrawImage(m_unk0x114, aRect.mX + i * 32 - m_unk0x118, aRect.mY);

	g->SetColor(Color(0, 0, 0));
	g->DrawRect(aRect.mX - 1, aRect.mY - 1, aRect.mWidth + 1, aRect.mHeight + 1);
}

// FUNCTION: GAME187 0x482a10
int UpdateCheckDialog::GetPreferredHeight(int theWidth)
{
	return Dialog::GetPreferredHeight(theWidth) + 32;
}
