#include "OptionsDialog.h"
#include "WinDMApp.h"
#include "SexyAppFramework/Slider.h"
#include "SexyAppFramework/Checkbox.h"
#include "SexyAppFramework/DialogButton.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/Rect.h"
#include "SexyAppFramework/Image.h"
#include "SexyAppFramework/Common.h"

using namespace Sexy;

// FUNCTION: GAME 0x4085d0
// FUNCTION: GAME187 0x4d1227
void OptionsDialog::SliderVal(int theId, double theVal)
{
	switch (theId)
	{
	case 0:
		mApp->SetMusicVolume(theVal);
		break;
	case 1:
		mApp->SetSfxVolume(theVal);
		break;
	}
}

// FUNCTION: GAME 0x40860a
// FUNCTION: GAME187 0x4d1261
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	if ((theId == 0) && (!checked) && (mApp->mForceFullscreen))
	{
		mApp->ShowColorDepthExplanation();
		mWindowedCheckbox->SetChecked(true);
	}
}

// FUNCTION: GAME 0x40863e
// FUNCTION: GAME187 0x4d1297
void OptionsDialog::ButtonPress(int theId)
{
	Dialog::ButtonPress(theId);
	mApp->PlaySample(12);
}

// FUNCTION: GAME 0x408687
// FUNCTION: GAME187 0x4d1424
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	return 34 * mLinkButtons.size() + 256;
}

// FUNCTION: GAME 0x408774
// FUNCTION: GAME187 0x4d143a
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mRegisterButton);
	theWidgetManager->AddWidget(mHighScoresButton);
	theWidgetManager->AddWidget(mMusicVolumeSlider);
	theWidgetManager->AddWidget(mSfxVolumeSlider);
	theWidgetManager->AddWidget(mCustomCursorsCheckbox);
	theWidgetManager->AddWidget(mWindowedCheckbox);
	theWidgetManager->AddWidget(mUpdateButton);

	for (int i = 0; i < (int)mLinkButtons.size(); i++)
		theWidgetManager->AddWidget(mLinkButtons[i]);
}

// FUNCTION: GAME 0x408825
// FUNCTION: GAME187 0x4d14f3
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mRegisterButton);
	theWidgetManager->RemoveWidget(mHighScoresButton);
	theWidgetManager->RemoveWidget(mMusicVolumeSlider);
	theWidgetManager->RemoveWidget(mSfxVolumeSlider);
	theWidgetManager->RemoveWidget(mWindowedCheckbox);
	theWidgetManager->RemoveWidget(mCustomCursorsCheckbox);
	theWidgetManager->RemoveWidget(mUpdateButton);

	for (int i = 0; i < (int)mLinkButtons.size(); i++)
		theWidgetManager->RemoveWidget(mLinkButtons[i]);
}

// FUNCTION: GAME 0x4088d6
// FUNCTION: GAME187 0x4d15ac
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	if (mApp->mBuildUnlocked)
	{
		int aButtonWidth = (mWidth - 54) / 2;
		mHelpButton->Resize(mX + 24, mY + 166, aButtonWidth, 32);
		mHighScoresButton->Resize(mX + aButtonWidth + 30, mY + 166, aButtonWidth, 32);
	}
	else
	{
		int aButtonWidth = (mWidth - 60) / 3;
		mHelpButton->Resize(mX + 24, mY + 166, aButtonWidth, 32);
		mRegisterButton->Resize(mX + aButtonWidth + 30, mY + 166, aButtonWidth, 32);
		mHighScoresButton->Resize(mX + aButtonWidth * 2 + 36, mY + 166, aButtonWidth, 32);
	}

	for (int i = 0; i < (int)mLinkButtons.size(); i++)
		mLinkButtons[i]->Resize(mX + 24, mYesButton->mY + 34 * (i - mLinkButtons.size()), mWidth - 48, 32);

	mMusicVolumeSlider->Resize(mX + 136, mY + 60, mWidth - 160, 32);
	mSfxVolumeSlider->Resize(mX + 136, mY + 92, mWidth - 160, 32);
	mWindowedCheckbox->Resize(mX + 32, mY + 124, 32, 32);
	mCustomCursorsCheckbox->Resize(mX + 146, mY + 124, 32, 32);
}

// FUNCTION: GAME 0x408b66
// FUNCTION: GAME187 0x4d18f6
OptionsDialog::~OptionsDialog()
{
	delete mHelpButton;
	delete mRegisterButton;
	delete mHighScoresButton;
	delete mMusicVolumeSlider;
	delete mSfxVolumeSlider;

	for (int i = 0; i < (int)mLinkButtons.size(); i++)
		delete mLinkButtons[i];

	delete mUpdateButton;
	delete mWindowedCheckbox;
	delete mCustomCursorsCheckbox;
	delete mCheckedImage;
	delete mUncheckedImage;
}

// FUNCTION: GAME 0x408c92
// FUNCTION: GAME187 0x4d1a8f
void OptionsDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	g->SetFont(mApp->m_unk0x690);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 0));

	g->DrawString(mApp->GetString("OPTIONSDLG_MUSIC"), 32, 80);
	g->DrawString(mApp->GetString("OPTIONSDLG_SOUND_FX"), 32, 112);
	g->DrawString(mApp->GetString("OPTIONSDLG_FULLSCREEN"), mWindowedCheckbox->mX - mX + 34, mWindowedCheckbox->mY - mY + 21);
	g->DrawString(mApp->GetString("OPTIONSDLG_CUSTOM_CURSORS"), mCustomCursorsCheckbox->mX - mX + 34, mCustomCursorsCheckbox->mY - mY + 21);

	g->SetColorizeImages(false);
}

// FUNCTION: GAME 0x408e5a
// FUNCTION: GAME187 0x4d1c58
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);

	if ((theId >= 100) && (theId < 200))
	{
		mApp->OpenURL(mApp->GetString(StrFormat("OptionsLinkURL%d", theId - 100), "http://www.popcap.com"), false);
		return;
	}

	switch (theId)
	{
	case 0:
		mApp->ShowFullInstructions();
		break;
	case 1:
		mApp->DoRegisterDialog();
		break;
	case 2:
		mApp->ShowHighScoreDialog();
		break;
	case 3:
		mApp->OpenURL("http://www.popcap.com", false);
		break;
	case 4:
		mApp->CheckForUpdates();
		break;
	}
}

// FUNCTION: GAME 0x409287
// FUNCTION: GAME187 0x4d1fda
OptionsDialog::OptionsDialog(WinDMApp* theApp)
	: Dialog(theApp->mImages[10], theApp->mImages[11], 2, true, theApp->GetString("OPTIONSDLG_OPTIONS"), "", theApp->GetString("OPTIONSDLG_DONE"), 3)
{
	mApp = theApp;

	mCheckedImage = theApp->CopyImage(theApp->mImages[27], Rect(0, 0, 31, 32));
	mUncheckedImage = mApp->CopyImage(mApp->mImages[27], Rect(31, 0, 31, 32));

	SetButtonFont(mApp->m_unk0x690);
	SetHeaderFont(mApp->m_unk0x68c);
	SetLinesFont(mApp->m_unk0x690);

	mHelpButton = new DialogButton(theApp->mImages[11], 0, this);
	mHelpButton->SetFont(mApp->m_unk0x690);
	mHelpButton->mLabel = mApp->GetString("OPTIONSDLG_HELP");

	mRegisterButton = new DialogButton(theApp->mImages[11], 1, this);
	mRegisterButton->SetFont(mApp->m_unk0x690);
	mRegisterButton->mLabel = mApp->GetString("OPTIONSDLG_REGISTER");

	mHighScoresButton = new DialogButton(theApp->mImages[11], 2, this);
	mHighScoresButton->SetFont(mApp->m_unk0x690);
	mHighScoresButton->mLabel = mApp->GetString("OPTIONSDLG_HIGH_SCORES");

	for (int i = 0; i < mApp->GetInteger("NumOptionsLinks", 1); i++)
	{
		DialogButton* aButton = new DialogButton(theApp->mImages[11], i + 100, this);
		aButton->SetFont(mApp->m_unk0x690);
		aButton->mLabel = mApp->GetString(StrFormat("OptionsLinkText%d", i), "Visit PopCap.com For More Games");
		mLinkButtons.push_back(aButton);
	}

	mUpdateButton = new DialogButton(theApp->mImages[11], 4, this);
	mUpdateButton->SetFont(mApp->m_unk0x690);
	mUpdateButton->mLabel = mApp->GetString("OPTIONSDLG_CHECK_FOR_UPDATES");

	mMusicVolumeSlider = new Slider(mApp->mImages[25], mApp->mImages[26], 0, this);
	mMusicVolumeSlider->SetValue(theApp->GetMusicVolume());

	mSfxVolumeSlider = new Slider(mApp->mImages[25], mApp->mImages[26], 1, this);
	mSfxVolumeSlider->SetValue(theApp->GetSfxVolume());

	mWindowedCheckbox = new Checkbox(mUncheckedImage, mCheckedImage, 0, this);
	mWindowedCheckbox->SetChecked(!theApp->mIsWindowed);

	mCustomCursorsCheckbox = new Checkbox(mUncheckedImage, mCheckedImage, 1, this);
	mCustomCursorsCheckbox->SetChecked(theApp->mCustomCursorsEnabled);
}