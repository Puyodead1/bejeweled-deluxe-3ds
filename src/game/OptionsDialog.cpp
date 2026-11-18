#include "OptionsDialog.h"
#include "WinDMApp.h"
#include "SexyAppFramework/Slider.h"
#include "SexyAppFramework/DialogButton.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/Image.h"

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
	// The PC dialog was 256 + 34 per web link; the checkbox row and the link are gone (with a little air left above
	// Done).
	return 236;
}

// FUNCTION: GAME 0x408774
// FUNCTION: GAME187 0x4d143a
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mHighScoresButton);
	theWidgetManager->AddWidget(mMusicVolumeSlider);
	theWidgetManager->AddWidget(mSfxVolumeSlider);
}

// FUNCTION: GAME 0x408825
// FUNCTION: GAME187 0x4d14f3
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mHighScoresButton);
	theWidgetManager->RemoveWidget(mMusicVolumeSlider);
	theWidgetManager->RemoveWidget(mSfxVolumeSlider);
}

// FUNCTION: GAME 0x4088d6
// FUNCTION: GAME187 0x4d15ac
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aButtonWidth = (mWidth - 54) / 2;
	mHelpButton->Resize(mX + 24, mY + 132, aButtonWidth, 32);
	mHighScoresButton->Resize(mX + aButtonWidth + 30, mY + 132, aButtonWidth, 32);

	mMusicVolumeSlider->Resize(mX + 136, mY + 60, mWidth - 160, 32);
	mSfxVolumeSlider->Resize(mX + 136, mY + 92, mWidth - 160, 32);
}

// FUNCTION: GAME 0x408b66
// FUNCTION: GAME187 0x4d18f6
OptionsDialog::~OptionsDialog()
{
	delete mHelpButton;
	delete mHighScoresButton;
	delete mMusicVolumeSlider;
	delete mSfxVolumeSlider;
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

	g->SetColorizeImages(false);
}

// FUNCTION: GAME 0x408e5a
// FUNCTION: GAME187 0x4d1c58
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);

	switch (theId)
	{
	case 0:
		mApp->ShowFullInstructions();
		break;
	case 2:
		mApp->ShowHighScoreDialog();
		break;
	}
}

// FUNCTION: GAME 0x409287
// FUNCTION: GAME187 0x4d1fda
OptionsDialog::OptionsDialog(WinDMApp* theApp)
	: Dialog(theApp->mImages[10], theApp->mImages[11], 2, true, theApp->GetString("OPTIONSDLG_OPTIONS"), "", theApp->GetString("OPTIONSDLG_DONE"), 3)
{
	mApp = theApp;

	SetButtonFont(mApp->m_unk0x690);
	SetHeaderFont(mApp->m_unk0x68c);
	SetLinesFont(mApp->m_unk0x690);

	mHelpButton = new DialogButton(theApp->mImages[11], 0, this);
	mHelpButton->SetFont(mApp->m_unk0x690);
	mHelpButton->mLabel = mApp->GetString("OPTIONSDLG_HELP");

	mHighScoresButton = new DialogButton(theApp->mImages[11], 2, this);
	mHighScoresButton->SetFont(mApp->m_unk0x690);
	mHighScoresButton->mLabel = mApp->GetString("OPTIONSDLG_HIGH_SCORES");

	mMusicVolumeSlider = new Slider(mApp->mImages[25], mApp->mImages[26], 0, this);
	mMusicVolumeSlider->SetValue(theApp->GetMusicVolume());

	mSfxVolumeSlider = new Slider(mApp->mImages[25], mApp->mImages[26], 1, this);
	mSfxVolumeSlider->SetValue(theApp->GetSfxVolume());
}
