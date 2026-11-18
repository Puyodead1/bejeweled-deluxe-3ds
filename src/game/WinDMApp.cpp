#if BEJEWELED_VERSION >= 1870 || _MSC_VER > 1300
#include <fstream>
#else
#include <fstream.h> // pre-standard iostreams, gone after VC 7.0
#endif
#include "WinDMApp.h"
#include <stdio.h>
#include "Board.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Dialog.h"
#include "TitleScreen.h"
#include "HighScoreWidget.h"
#include "RecordScoreDialog.h"
#include "OptionsDialog.h"
#include "HighScoreDialog.h"
#include "RegisterDialog.h"
#include "SexyAppFramework/UpdateCheckDialog.h"
#include "SexyAppFramework/InternetManager.h"
#if BEJEWELED_VERSION >= 1870
#include "SexyAppFramework/MigrationManager.h"
#include "SexyAppFramework/Common.h"
#endif
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/BassMusicInterface.h"
#include "SexyAppFramework/ButtonWidget.h"
#include "SexyAppFramework/Image.h"
#include "SexyAppFramework/ImageFont.h"
#include "SexyAppFramework/MemoryImage.h"
#include "SexyAppFramework/DDImage.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Color.h"
#include "SexyAppFramework/SoundManager.h"
#include "SexyAppFramework/EditWidget.h"
#include "SexyAppFramework/Checkbox.h"
#include "SexyAppFramework/Rect.h"

using namespace Sexy;

// GLOBAL: GAME 0x4f1388
const char* gImageNames[36] = {
	"titlescreen6", "titlegreenbar2", "trialcircle2", "cursor1", "pointer2", "overhand", "typer",
	"alphaback1", "gemsheet6", "target1", "dialogbox2", "boxbutton2", "waitbar",
	"over2-newgame-pushed1", "over3-normal-pushed1", "over7-normal-selected1", "over4-timetrial-pushed1",
	"over8-timetrial-selected1", "over5-options-pushed1", "over6-quit-pushed1", "timerbar", "alphadigits4",
	"sparkle", "scorefont1", "over1-hint1", "slider-track", "slider-thumb", "checkbox1", "checkertextures2",
	"overfont2", "blackhole", "holemask", "elect", "editbox", "swapper1", "swapper2"
};

// GLOBAL: GAME 0x4f1418
const char* gSoundNames[21] = {
	"gotset2", "gotsetbig2", "combo22", "combo32", "combo42", "combo52", "combo52", "bad2", "gemongem2",
	"explode2", "click2", "select", "menuclick", "multishot", "go1", "levelup1", "gameover1", "firecrackle",
	"warning1", "whirlpool1", "excellent1"
};

// GLOBAL: GAME 0x4f6548
WinDMApp gWinDMApp;

// FUNCTION: GAME 0x40ba8c
// FUNCTION: GAME187 0x4d43cc
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	gHInstance = hInstance;

	gWinDMApp.Init();
	gWinDMApp.Start();
	gWinDMApp.Shutdown();

	return 0;
}

// FUNCTION: GAME 0x40bab9
// FUNCTION: GAME187 0x4d43f9
void WinDMApp::Shutdown()
{
	if (!mShutdown)
	{
#if BEJEWELED_VERSION < 1870
		if (mMaxTime > 0)
			mTimesPlayed += m_unk0x5e8 / 6000;
		else
			mTimesPlayed += m_unk0x5e4;

#endif
		if (mBoard != NULL && mBoard->CanPlay())
		{
			mBoard->SaveGame();
			mWidgetManager->RemoveWidget(mBoard);
			SafeDeleteWidget(mBoard);
			mBoard = NULL;
		}

		SexyApp::Shutdown();
	}
}

// FUNCTION: GAME 0x40bc7f
// FUNCTION: GAME187 0x4d4592
void WinDMApp::URLOpenSucceeded(const std::string& theURL)
{
	SexyApp::URLOpenSucceeded(theURL);
	KillDialog(10);
}

// FUNCTION: GAME 0x40bdff
// FUNCTION: GAME187 0x4d4716
void WinDMApp::ModalClose()
{
	if (mBoard != NULL)
		mBoard->Unpause(mTimeTrial);
}

void WinDMApp::GotFocus()
{
	if (mBoard != NULL)
		mBoard->Unpause(mTimeTrial);
}

// FUNCTION: GAME 0x40be19
// FUNCTION: GAME187 0x4d46fc
void WinDMApp::ModalOpen()
{
	if (mBoard != NULL)
		mBoard->Pause(mTimeTrial);
}

void WinDMApp::LostFocus()
{
	if (mBoard != NULL)
		mBoard->Pause(mTimeTrial);
}

// FUNCTION: GAME 0x40bfad
// FUNCTION: GAME187 0x4d48a9
void WinDMApp::ButtonPress(int theId)
{
	PlaySample(12);
}

// FUNCTION: GAME 0x40c02e
// FUNCTION: GAME187 0x4d4921
void WinDMApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
}

// FUNCTION: GAME 0x40c033
// FUNCTION: GAME187 0x4d4926
void WinDMApp::SetMusicVolume(double theVolume)
{
	SexyApp::SetMusicVolume(theVolume);
}

// FUNCTION: GAME 0x40c047
// FUNCTION: GAME187 0x4d4a0e
bool WinDMApp::KillDialog(int theDialogId)
{
	if (SexyApp::KillDialog(theDialogId))
	{
		if (mDialogMap.size() == 0)
			mWidgetManager->SetFocus(mBoard);

		return true;
	}

	return false;
}

// FUNCTION: GAME 0x40bc9e
// FUNCTION: GAME187 0x4d45b1
MusicInterface* WinDMApp::CreateMusicInterface(HWND theHWnd)
{
	return new BassMusicInterface(theHWnd);
}

// FUNCTION: GAME 0x40bd4e
// FUNCTION: GAME187 0x4d4653
Dialog* WinDMApp::NewDialog(int theDialogId, bool isModal, const std::string& theDialogHeader, const std::string& theDialogLines, const std::string& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog(mImages[10], mImages[11], theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(m_unk0x690);
	aDialog->SetHeaderFont(m_unk0x68c);
	aDialog->SetLinesFont(m_unk0x690);
	aDialog->Resize(250, 50, 316, aDialog->GetPreferredHeight(316));
	return aDialog;
}

// FUNCTION: GAME 0x40c0bc
// FUNCTION: GAME187 0x4d4ad9
std::string WinDMApp::GetGameSEHInfo()
{
	return SexyApp::GetGameSEHInfo();
}

// FUNCTION: GAME 0x40cd65
// FUNCTION: GAME187 0x4d5174
void WinDMApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	RegistryWriteString("LastScoreName", mLastScoreName);
}

// FUNCTION: GAME 0x40cdb4
// FUNCTION: GAME187 0x4d51bc
void WinDMApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
#if BEJEWELED_VERSION >= 1870
	mRegisterLink = "http://www.popcap.com/register.php?theGame=bejeweled&referid=" + mReferId;
#endif
	RegistryReadString("LastScoreName", &mLastScoreName);
	RegistryReadBoolean("NoPopLink", &mNoPopLink);
}

// FUNCTION: GAME 0x40ca85
// FUNCTION: GAME187 0x4d4e97
void WinDMApp::URLOpenFailed(const std::string& theURL)
{
	SexyApp::URLOpenFailed(theURL);
	KillDialog(10);

	std::string aString = GetString("URL_OPEN_ERROR1") + theURL + GetString("URL_OPEN_ERROR2");
	CopyToClipboard(theURL);

	Dialog* aDialog = NewDialog(11, true, GetString("OPEN_BROWSER"), aString, GetString("DIALOG_BUTTON_OK"), 3);
	aDialog->Resize(100, 50, 440, aDialog->GetPreferredHeight(440));
	AddDialog(11, aDialog);
}

// FUNCTION: GAME 0x40cc63
// FUNCTION: GAME187 0x4d5077
bool WinDMApp::OpenURL(const std::string& theURL, bool shutdownOnOpen)
{
	Dialog* aDialog = NewDialog(10, true, "", GetString("OPENING_BROWSER"), "", 0);
	aDialog->mContentInsets = Insets(16, 26, 16, 16);
	aDialog->Resize(180, 140, 280, 70);
	AddDialog(10, aDialog);

	return SexyApp::OpenURL(theURL, shutdownOnOpen);
}

#if BEJEWELED_VERSION >= 1870
// FUNCTION: GAME187 0x4d6399
void WinDMApp::PreDisplayHook()
{
	mIsDiamondMine = GetBoolean("IsDiamondMine", false);
	if (mIsDiamondMine)
		mTitle = GetString("Title", "Diamond Mine") + " " + mProductVersion;
}
#else
// FUNCTION: GAME 0x40ce37
void WinDMApp::InitPropertiesHook()
{
	mIsDiamondMine = GetBoolean("IsDiamondMine", false);
	if (mIsDiamondMine)
		mTitle = GetString("Title", "Diamond Mine") + " " + mProductVersion;
}
#endif

// FUNCTION: GAME 0x40bb37
// FUNCTION: GAME187 0x4d4455
void WinDMApp::NewBoard()
{
	if (mHighScoreWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		SafeDeleteWidget(mHighScoreWidget);
		mHighScoreWidget = NULL;
	}

	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}

	mBoard = new Board(this, mTimeTrial);
	mBoard->Resize(0, 0, 640, 480);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);

	if (mTimeTrial)
	{
		mMusicInterface->FadeOut(0, true, 0.004);
		mMusicInterface->FadeIn(1, -1, 0.002, false);
	}
	else
	{
		mMusicInterface->FadeOut(1, true, 0.004);
		mMusicInterface->FadeIn(0, -1, 0.002, false);
	}
}

// FUNCTION: GAME 0x40bcd5
// FUNCTION: GAME187 0x4d45e3
void WinDMApp::ShowRecordScoreDialog(const std::string& theLines)
{
	RecordScoreDialog* aDialog = new RecordScoreDialog(this, theLines);
	aDialog->Resize(250, 32, 316, aDialog->GetPreferredHeight(316));
	AddDialog(8, aDialog);
}

// FUNCTION: GAME 0x40be33
// FUNCTION: GAME187 0x4d4730
void WinDMApp::SetGameMode(bool timeTrial, bool newBoard)
{
	mTimeTrial = timeTrial;

	if (timeTrial)
	{
		m_unk0x6c8->mButtonImage = NULL;
		m_unk0x6cc->mButtonImage = mImages[17];
	}
	else
	{
		m_unk0x6c8->mButtonImage = mImages[15];
		m_unk0x6cc->mButtonImage = NULL;
	}

	m_unk0x6c8->MarkDirty();
	m_unk0x6cc->MarkDirty();

	if (newBoard)
		NewBoard();
}

// FUNCTION: GAME 0x40beae
// FUNCTION: GAME187 0x4d47b1
void WinDMApp::ShowHighScores()
{
	mWidgetManager->RemoveWidget(mBoard);
	mHighScoreWidget = new HighScoreWidget(this);
	mHighScoreWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHighScoreWidget);
	mWidgetManager->BringToBack(mHighScoreWidget);
}

// FUNCTION: GAME 0x40bf39
// FUNCTION: GAME187 0x4d483e
void WinDMApp::ShowOptionsDialog()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
	AddDialog(2, aDialog);
}

// FUNCTION: GAME 0x40bfba
// FUNCTION: GAME187 0x4d48b6
void WinDMApp::ShowHighScoreDialog()
{
	HighScoreDialog* aDialog = new HighScoreDialog(this);
	aDialog->Resize(194, 50, 428, aDialog->GetPreferredHeight(428));
	AddDialog(13, aDialog);
}

// FUNCTION: GAME 0x40c120
// FUNCTION: GAME187 0x4d4b08
WinDMApp::~WinDMApp()
{
	if (mImages != NULL)
	{
		for (int i = 0; i < 36; i++)
			delete mImages[i];
#ifdef BEJEWELED_MATCH
		delete mImages; // scalar delete of a new[] array, as in the original
#else
		delete[] mImages;
#endif
	}

	if (m_unk0x654 != NULL)
	{
		for (int i = 0; i < 7; i++)
			for (int j = 0; j < 6; j++)
				delete m_unk0x654[i][j];
	}

#if BEJEWELED_VERSION >= 1870
	for (std::list<ButtonWidget*>::iterator anItr = m_unk0x678.begin(); anItr != m_unk0x678.end(); ++anItr)
#else
	std::list<ButtonWidget*>::iterator anItr;
	for (anItr = m_unk0x678.begin(); anItr != m_unk0x678.end(); ++anItr)
#endif
	{
		mWidgetManager->RemoveWidget(*anItr);
		delete *anItr;
	}

	int i;
	for (i = 0; i < 6; i++)
	{
		delete m_unk0x698[i];
		delete m_unk0x6b0[i];
	}

	for (i = 0; i < 25; i++)
		delete m_unk0x6d0[i];

	if (mHighScoreWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		delete mHighScoreWidget;
	}

	delete m_unk0x684;
	delete m_unk0x68c;
	delete m_unk0x690;
	delete m_unk0x694;

	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
	}

	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		delete mTitleScreen;
	}
}

// FUNCTION: GAME 0x40c35c
// FUNCTION: GAME187 0x4d4d82
WinDMApp::WinDMApp()
{
#if BEJEWELED_VERSION < 1870
	m_unk0x5e8 = 0;
	mMaxTime = 0;
	m_unk0x5e4 = 0;
#endif
	m_unk0x735 = true;
	mImages = NULL;
	m_unk0x654 = NULL;
	mTitleScreen = NULL;
	mBoard = NULL;
	mNoPopLink = false;

	mProdName = "Bejeweled";
#if BEJEWELED_VERSION >= 1870
	mTitle = "Bejeweled Deluxe " + mProductVersion;
#else
	mTitle = "Bejeweled " + mProductVersion;
#endif
	mRegKey = "PopCap\\Bejeweled";

	m_unk0x684 = NULL;
	m_unk0x688 = NULL;
	m_unk0x68c = NULL;
	m_unk0x690 = NULL;
	mHighScoreWidget = NULL;
	m_unk0x694 = NULL;

	int i;
	for (i = 0; i < 6; i++)
	{
		m_unk0x698[i] = NULL;
		m_unk0x6b0[i] = NULL;
	}

	for (i = 0; i < 25; i++)
		m_unk0x6d0[i] = NULL;

	mCustomCursorsEnabled = false;
	mIsDiamondMine = false;
}

// FUNCTION: GAME 0x40c4a8
// FUNCTION: GAME187 0x4d62b8
bool WinDMApp::LoadGameImage(int theIndex)
{
	mImages[theIndex] = GetImage(std::string("images\\") + gImageNames[theIndex]);
	if (mImages[theIndex] == NULL)
	{
		Popup(std::string("Failed to load images\\") + gImageNames[theIndex]);
		mLoadingFailed = true;
		return false;
	}

	mImages[theIndex]->GenerateDDSurface();
	return true;
}

// FUNCTION: GAME 0x40cf43
// FUNCTION: GAME187 0x4d649e
void WinDMApp::Init()
{
	SexyApp::Init();

#if BEJEWELED_VERSION >= 1870
	// 1.87: on Vista, saves and high scores live under the per-user data folder.
	if (CheckForVista())
	{
		MkDir(GetAppDataFolder());
		AllowAllAccess(GetAppDataFolder());

		MigrationManager aMigrationManager;
		aMigrationManager.AddFile("userdata");
		aMigrationManager.DoMigration();
	}

#else
	mMaxTime = GetInteger("MaxTime", 0);
#endif
	LoadProperties();

#if BEJEWELED_VERSION >= 1870
	DIALOG_YES_STRING = GetString("DIALOG_BUTTON_YES");
	DIALOG_NO_STRING = GetString("DIALOG_BUTTON_NO");
	DIALOG_OK_STRING = GetString("DIALOG_BUTTON_OK");
	DIALOG_CANCEL_STRING = GetString("DIALOG_BUTTON_CANCEL");
#else
	Dialog::YES_BUTTON_TEXT = GetString("DIALOG_BUTTON_YES");
	Dialog::NO_BUTTON_TEXT = GetString("DIALOG_BUTTON_NO");
	Dialog::OK_BUTTON_TEXT = GetString("DIALOG_BUTTON_OK");
	Dialog::CANCEL_BUTTON_TEXT = GetString("DIALOG_BUTTON_CANCEL");
#endif

	mImages = new DDImage*[36];
	for (int i = 0; i < 36; i++)
		mImages[i] = NULL;

	for (int j = 0; j < 7; j++)
	{
		if (!LoadGameImage(j))
		{
			Shutdown();
			return;
		}
	}

	m_unk0x68c = new ImageFont(this, "data\\afont1.txt");
	m_unk0x690 = new ImageFont(this, "data\\mainfont1.txt");

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);

	mMusicInterface->LoadMusic(0, "music\\1984-network.s3m");
	mMusicInterface->LoadMusic(1, "music\\1981-datajack.s3m");
	mMusicInterface->LoadMusic(2, "music\\1980-autonomus.s3m");
	mMusicInterface->PlayMusic(2, 0, false);

	SetCursorImage(0, mImages[3]);
	SetCursorImage(1, mImages[4]);
	SetCursorImage(2, mImages[5]);
	SetCursorImage(3, mImages[6]);
}
// FUNCTION: GAME 0x40c586
// FUNCTION: GAME187 0x4d82d2
void WinDMApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = 60;

#if BEJEWELED_VERSION >= 1870
	std::fstream aStream("data\\diff.txt", std::ios::in);
#elif _MSC_VER > 1300
	std::ifstream aStream("data\\diff.txt");
#else
	ifstream aStream("data\\diff.txt");
#endif
	aStream >> m_unk0x5ec >> m_unk0x5f0 >> m_unk0x5f8 >> m_unk0x600 >> m_unk0x608 >> m_unk0x610 >> m_unk0x618
		>> m_unk0x620 >> m_unk0x628 >> m_unk0x630 >> m_unk0x638 >> m_unk0x640;

	int i;
	for (i = 7; i < 36; i++)
	{
		if (!LoadGameImage(i))
			return;
		mCompletedLoadingThreadTasks++;
	}

	if (mSoundManager->Initialized())
	{
		for (i = 0; i < 21; i++)
		{
			if (!mSoundManager->LoadSound(i, std::string("sounds\\") + gSoundNames[i]))
			{
				Popup(std::string("Failed to load sounds\\") + gSoundNames[i]);
				mLoadingFailed = true;
				return;
			}
			mCompletedLoadingThreadTasks++;
		}
	}
	else
	{
		mCompletedLoadingThreadTasks += 21;
	}

	m_unk0x684 = new ImageFont(mImages[21], "data\\alphadigits4.txt");
	m_unk0x688 = new ImageFont(mImages[23], "data\\scorefont1.txt");
	m_unk0x694 = new ImageFont(mImages[29], "data\\overfont2.txt");
	mCompletedLoadingThreadTasks += 2;

	for (i = 0; i < 24; i++)
		m_unk0x6d0[i] = CreateColorizedImage(mImages[20], Color(HSLToRGB(i * 85 / 23, 255, 128)));
	m_unk0x6d0[24] = CreateColorizedImage(mImages[20], Color(255, 255, 255));

	m_unk0x654 = new Image**[7];
#if BEJEWELED_VERSION < 1870
	MemoryImage* anImage;
#endif
	for (i = 0; i < 7; i++)
	{
		m_unk0x654[i] = new Image*[6];
		for (int j = 0; j < 6; j++)
		{
#if BEJEWELED_VERSION >= 1870
			MemoryImage* anImage = new MemoryImage(this);
#else
			anImage = new MemoryImage(this);
#endif
			anImage->Create(49, 49);

			int aSize = (int)((1.0 - (j + 1.0) / 7.0) * 49.0) | 1;
			Graphics g(anImage);
			g.DrawImage(mImages[8], Rect((49 - aSize) / 2, (49 - aSize) / 2, aSize, aSize), Rect(2, i * 104, 49, 49));
			m_unk0x654[i][j] = anImage;
		}
		mCompletedLoadingThreadTasks++;
	}

	PrecacheAdditive(mImages[30]);
	PrecacheAlpha(mImages[31]);
	mCompletedLoadingThreadTasks++;
}
// FUNCTION: GAME 0x40e1d7
// FUNCTION: GAME187 0x4d6a40
void WinDMApp::ButtonDepress(int theId)
{
	if ((theId % 10000 >= 2000) && (theId % 10000 < 3000))
	{
		switch (theId - 2000)
		{
		case 0:
			mBoard->FUN_004010cf();
			KillDialog(0);
			break;
		case 1:
			Shutdown();
			KillDialog(1);
			break;
		case 2:
		{
			OptionsDialog* aDialog = (OptionsDialog*)GetDialog(2);
			bool wantWindowed = !aDialog->mWindowedCheckbox->IsChecked();
			if (mIsWindowed != wantWindowed)
				SwitchScreenMode(wantWindowed);
			EnableCustomCursors(aDialog->mCustomCursorsCheckbox->IsChecked());
			KillDialog(2);
			break;
		}
		case 3:
			KillDialog(3);
			SetGameMode(false, true);
			break;
		case 4:
			KillDialog(4);
			SetGameMode(true, true);
			break;
		case 5:
		{
			RegisterDialog* aDialog = (RegisterDialog*)GetDialog(5);
			std::string aUserName = aDialog->mUserNameEdit->mString;
			std::string aRegCode = aDialog->mRegCodeEdit->mString;
			if (!Validate(aUserName, aRegCode))
			{
				ShowRegistrationInvalid();
			}
			else
			{
				mUserName = aUserName;
				mRegCode = aRegCode;
				mIsRegistered = true;
				KillDialog(5);
				mWidgetManager->MarkAllDirty();
				if (mHighScoreWidget != NULL)
					mHighScoreWidget->ShowScoreMessage();
			}
			break;
		}
		case 7:
			DoRegisterDialog();
			KillDialog(7);
			break;
		case 8:
		{
			RecordScoreDialog* aDialog = (RecordScoreDialog*)GetDialog(8);
			mHighScoreWidget->SubmitScore(aDialog->mNameEdit->mString);
			mLastScoreName = aDialog->mNameEdit->mString;
			KillDialog(8);
			break;
		}
		case 15:
			KillDialog(15);
			DoRegisterDialog();
			break;
		case 16:
			KillDialog(16);
			mBoard->LoadGame();
			mBoard->DeleteSavedGame();
			break;
		case 17:
			KillDialog(17);
			CheckForUpdates();
			break;
		case 10018:
			KillDialog(10018);
			KillDialog(18);
			OpenUpdateURL();
			break;
		case 20018:
			KillDialog(20018);
			KillDialog(18);
			break;
		default:
			KillDialog(theId - 2000);
			break;
		}
	}
	else if ((theId % 10000 >= 3000) && (theId % 10000 < 4000))
	{
		switch (theId - 3000)
		{
		case 16:
			KillDialog(16);
			mBoard->DeleteSavedGame();
			break;
		case 10018:
			KillDialog(10018);
			KillDialog(18);
			break;
		default:
			KillDialog(theId - 3000);
			break;
		}
	}

	switch (theId)
	{
	case 10:
		ConfirmNewGame();
		break;
	case 11:
		if (!mTimeTrial)
			ConfirmNewGame();
		else if (mBoard->m_unk0x170)
			SetGameMode(false, true);
		else
			ConfirmNormalMode();
		break;
	case 12:
		if (mTimeTrial)
			ConfirmNewGame();
		else if (mBoard->m_unk0x170)
			SetGameMode(true, true);
		else
			ConfirmTimedMode();
		break;
	case 13:
		ShowOptionsDialog();
		break;
	case 14:
		ConfirmQuit();
		break;
	case 15:
		mBoard->Hint();
		break;
	}
}
// FUNCTION: GAME 0x40d301
// FUNCTION: GAME187 0x4d5272
void WinDMApp::DoRegisterDialog()
{
	if (mIsRegistered)
	{
		Dialog* aDialog = DoDialog(19, true, "Already Registered", "You have already registered Bejeweled!", "OK", 3);
		aDialog->Move(aDialog->mX, aDialog->mY + 24);
	}
	else
	{
		RegisterDialog* aDialog = new RegisterDialog(this);
		aDialog->Resize((mWidth - 348) / 2, 32, 348, aDialog->GetPreferredHeight(348));
		AddDialog(5, aDialog);
	}
}

// FUNCTION: GAME 0x40d413
// FUNCTION: GAME187 0x4d5386
void WinDMApp::ShowRegistrationInvalid()
{
	DoDialog(6, true, GetString("REGISTRATION_INVALID_DIALOG1"), GetString("REGISTRATION_INVALID_DIALOG2"), GetString("REGISTRATION_INVALID_DIALOG3"), 3);
	PlaySample(7);
}

// FUNCTION: GAME 0x40d510
// FUNCTION: GAME187 0x4d5486
void WinDMApp::ShowHighScoreMessage(const std::string& theLines)
{
	DoDialog(7, true, GetString("HIGH_SCORE"), theLines, "", 1);
}

// FUNCTION: GAME 0x40d59a
// FUNCTION: GAME187 0x4d550d
void WinDMApp::ConfirmLoadGame()
{
	if (mBoard->HasSavedGame())
	{
		Dialog* aDialog = DoDialog(16, true, GetString("CONFIRM_LOAD_DIALOG1"), GetString("CONFIRM_LOAD_DIALOG2"), "", 1);
		aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
	}
}

// FUNCTION: GAME 0x40d69e
// FUNCTION: GAME187 0x4d68da
void WinDMApp::CheckForUpdates()
{
	KillDialog(18);

	UpdateCheckDialog* aDialog = new UpdateCheckDialog(mImages[10], mImages[11], mImages[12], 18);
	aDialog->SetButtonFont(m_unk0x690);
	aDialog->SetHeaderFont(m_unk0x68c);
	aDialog->SetLinesFont(m_unk0x690);
	aDialog->Resize(146, 50, 348, aDialog->GetPreferredHeight(348));
	AddDialog(18, aDialog);

	mInternetManager->CheckForUpdates("http://www.popcap.com/win32updatecheck.php?prod=" + mProdName + "&ver=" + mProductVersion + "&referid=" + mReferId);
}

// FUNCTION: GAME 0x40d807
// FUNCTION: GAME187 0x4d5610
void WinDMApp::ConfirmNewGame()
{
	if (mBoard->mState == 6)
		NewBoard();
	else
		DoDialog(0, true, GetString("NEWGAMEDIALOG1"), GetString("NEWGAMEDIALOG2"), "", 1);
}

// FUNCTION: GAME 0x40d8e8
// FUNCTION: GAME187 0x4d56ec
void WinDMApp::ConfirmNormalMode()
{
	if (mBoard->mState == 6)
		SetGameMode(false, true);
	else
		DoDialog(3, true, GetString("CONFIRMNORMALMODE1"), GetString("CONFIRMNORMALMODE2"), "", 1);
}

// FUNCTION: GAME 0x40d9cd
// FUNCTION: GAME187 0x4d57cd
void WinDMApp::ConfirmTimedMode()
{
	if (mBoard->mState == 6)
		SetGameMode(true, true);
	else
		DoDialog(4, true, GetString("CONFIRMTIMEDMODE1"), GetString("CONFIRMTIMEDMODE2"), "", 1);
}

// FUNCTION: GAME 0x40dab3
// FUNCTION: GAME187 0x4d58af
void WinDMApp::ConfirmQuit()
{
	std::string aLines = GetString("CONFIRMQUIT1");
	if ((mBoard != NULL) && mBoard->CanPlay())
		aLines += GetString("CONFIRMQUIT2");

	DoDialog(1, true, GetString("QUIT"), aLines, "", 1);
}
// FUNCTION: GAME 0x40dbea
// FUNCTION: GAME187 0x4d59cc
void WinDMApp::ShowInstructions()
{
	Dialog* aDialog = NewDialog(9, false, "", GetString("INSTRUCTIONDIALOG"), "", 0);
	aDialog->mContentInsets = Insets(16, 24, 16, 24);
	aDialog->Resize(201, 38, 418, aDialog->GetPreferredHeight(418));
	AddDialog(9, aDialog);
}

// FUNCTION: GAME 0x40dce4
// FUNCTION: GAME187 0x4d5ac1
void WinDMApp::ShowFullInstructions()
{
	Dialog* aDialog = NewDialog(12, true, GetString("FULL_INSTRUCTIONS1"), GetString("FULL_INSTRUCTIONS2"), GetString("DONE"), 3);
	aDialog->mTextAlign = -1;
	aDialog->Resize(32, 8, mWidth - 64, aDialog->GetPreferredHeight(mWidth - 64));
	AddDialog(12, aDialog);
}

// FUNCTION: GAME 0x40de0a
// FUNCTION: GAME187 0x4d5bef
void WinDMApp::ShowColorDepthExplanation()
{
	DoDialog(12, true, GetString("COLOR_DEPTH_EXPLANATION1"), GetString("COLOR_DEPTH_EXPLANATION2"), GetString("DIALOG_BUTTON_OK"), 3);
}

// FUNCTION: GAME 0x40def7
// FUNCTION: GAME187 0x4d5cdf
void WinDMApp::ShowRegisterNag()
{
#if BEJEWELED_VERSION < 1870
	if (mMaxTime > 0)
		return;

#endif
	char aBuffer[256];
	if (mIsDiamondMine)
		sprintf(aBuffer, GetString("YOU_HAVE_PLAYED_DIAMOND_MINE_D_TIMES").c_str(), mTimesPlayed);
	else
		sprintf(aBuffer, GetString("YOU_HAVE_PLAYED_BEJEWELLED_D_TIMES").c_str(), mTimesPlayed);

	std::string aLines = aBuffer;
	if (mTimesPlayed >= 40)
	{
		if (mIsDiamondMine)
			aLines += GetString("YOU_ARE_A_DIAMOND_MINE_AHOLIC");
		else
			aLines += GetString("YOU_ARE_A_BEJEWELAHOLIC");
	}
	else if (mTimesPlayed >= 30)
	{
		aLines += GetString("LOOKS_LIKE_YOU_ARE_ADDICTED");
	}
	else if (mTimesPlayed >= 25)
	{
		aLines += GetString("ARE_YOU_GETTING_HOOKED");
	}
	aLines += GetString("LOADING_NAG");

	Dialog* aDialog = NewDialog(15, true, GetString("REGISTER_NOW"), aLines, "", 1);
	aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
	AddDialog(15, aDialog);
}

// FUNCTION: GAME 0x40e5e7
// FUNCTION: GAME187 0x4d6e71
ButtonWidget* WinDMApp::MakeButton(int theId, const Rect& theRect, int theOverImage, int theDownImage)
{
	ButtonWidget* aButton = new ButtonWidget(theId, this);
	aButton->Resize(theRect);
	aButton->mDoFinger = true;
	if (theOverImage != -1)
		aButton->mOverImage = mImages[theOverImage];
	aButton->mDownImage = mImages[theDownImage];
	aButton->mMouseInsets = Insets(1, 1, 3, 3);
	mWidgetManager->AddWidget(aButton);
	m_unk0x678.push_back(aButton);
	return aButton;
}

// FUNCTION: GAME 0x40e6ac
// FUNCTION: GAME187 0x4d6f28
void WinDMApp::FinishLoading()
{
	mMusicInterface->FadeOut(2, true, 0.004);
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	MakeButton(10, Rect(18, 142, 156, 41), -1, 13);
	m_unk0x6c8 = MakeButton(11, Rect(18, 185, 78, 31), -1, 14);
	m_unk0x6cc = MakeButton(12, Rect(96, 185, 78, 31), -1, 16);
	MakeButton(13, Rect(18, 241, 156, 41), -1, 18);
	MakeButton(14, Rect(18, 405, 156, 40), -1, 19);
	ButtonWidget* aButton = MakeButton(15, Rect(64, 303, 53, 73), 24, 24);
	aButton->mMouseInsets = Insets(-16, -8, -16, -8);

	SetGameMode(false, true);
	NewBoard();
	ConfirmLoadGame();
}
#if BEJEWELED_VERSION >= 1870

#endif