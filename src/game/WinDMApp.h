#ifndef __WINDMAPP_H__
#define __WINDMAPP_H__

#include <list>
#include "SexyAppFramework/SexyApp.h"
#include "SexyAppFramework/Rect.h"
#include "SexyAppFramework/DDImage.h"
namespace Sexy
{

class Board;
class Font;
class Image;
class DDImage;
class ImageFont;
class ButtonWidget;
class TitleScreen;
class HighScoreWidget;

// The Bejeweled Deluxe application class (RTTI .?AVWinDMApp@Sexy@@), size 0x738.
// One global instance at 0x4f6548, driven from WinMain.

// VTABLE: GAME 0x4c7854
// VTABLE: GAME187 0x507cc0 Sexy::DialogListener
// VTABLE: GAME187 0x507ccc Sexy::ButtonListener
class WinDMApp : public SexyApp
{
public:
	WinDMApp();
	virtual ~WinDMApp();

	virtual void ButtonPress(int theId);   // vtable+0x04
	virtual void ButtonDepress(int theId); // vtable+0x08
	virtual void UpdateFrames();        // vtable+0x14
	virtual MusicInterface* CreateMusicInterface(HWND theHWnd); // vtable+0x38
	virtual void Init();              // vtable+0x3c
	virtual void LoadingThreadProc();        // vtable+0x40
	virtual void WriteToRegistry();   // vtable+0x44
	virtual void ReadFromRegistry();  // vtable+0x48
	virtual Dialog* NewDialog(int theDialogId, bool isModal, const std::string& theDialogHeader, const std::string& theDialogLines, const std::string& theDialogFooter, int theButtonMode); // vtable+0x4c
	virtual void URLOpenFailed(const std::string& theURL); // vtable+0x5c
	virtual void URLOpenSucceeded(const std::string& theURL); // vtable+0x60
	virtual bool OpenURL(const std::string& theURL, bool shutdownOnOpen); // vtable+0x64
	virtual std::string GetGameSEHInfo(); // vtable+0x7c
	virtual void Shutdown();          // vtable+0x80
	// 1.87 moved this into PreDisplayHook and leaves SexyApp::InitPropertiesHook alone, which loads the
	// signed properties\partner.xml (NoReg, ProdName, ...). Overriding InitPropertiesHook here makes it a trial.
	virtual void PreDisplayHook();     // vtable+0x68
	virtual void SetMusicVolume(double theVolume); // vtable+0xb4
	virtual bool KillDialog(int theDialogId); // vtable+0xe0
	virtual void ModalOpen();        // vtable+0xe8 (same code as 0xf4, folded by the linker)
	virtual void ModalClose();        // vtable+0xec (same code as 0xf0, folded by the linker)
	virtual void GotFocus();        // vtable+0xf0
	virtual void LostFocus();        // vtable+0xf4
	virtual bool GetAutoInputMove(int& theX1, int& theY1, int& theX2, int& theY2); // Switch port: scripted testing

	// Names below are guesses from what the code does.
	void NewBoard();
	void ShowRecordScoreDialog(const std::string& theLines);
	void SetGameMode(bool timeTrial, bool newBoard);
	void ShowHighScores();
	void ShowOptionsDialog();
	void ShowHighScoreDialog();
	bool LoadGameImage(int theIndex);
	void DoRegisterDialog();
	void ShowRegistrationInvalid();
	void CheckForUpdates();
	void ConfirmNewGame();
	void ConfirmNormalMode();
	void ConfirmTimedMode();
	void ConfirmQuit();
	void ShowHighScoreMessage(const std::string& theLines);
	void ConfirmLoadGame();
	void ShowInstructions();
	void ShowFullInstructions();
	void ShowColorDepthExplanation();
	void ShowRegisterNag();
	ButtonWidget* MakeButton(int theId, const Rect& theRect, int theOverImage, int theDownImage);
	void FinishLoading();

	// 0x5ec-0x648: difficulty settings read from data\diff.txt
	int m_unk0x5ec;                   // 0x5ec
	double m_unk0x5f0;                // 0x5f0
	double m_unk0x5f8;                // 0x5f8
	double m_unk0x600;                // 0x600
	int m_unk0x608;                   // 0x608
	double m_unk0x610;                // 0x610
	double m_unk0x618;                // 0x618
	double m_unk0x620;                // 0x620
	double m_unk0x628;                // 0x628
	double m_unk0x630;                // 0x630
	double m_unk0x638;                // 0x638
	double m_unk0x640;                // 0x640
	Board* mBoard;                    // 0x648
	TitleScreen* mTitleScreen;        // 0x64c
	DDImage** mImages;                // 0x650  36 entries, see gImageNames
	Image*** m_unk0x654;              // 0x654  7 x 6 generated gem images
	HighScoreWidget* mHighScoreWidget; // 0x658
	std::string mLastScoreName;       // 0x65c  registry "LastScoreName"
	std::list<ButtonWidget*> m_unk0x678;    // 0x678
	ImageFont* m_unk0x684;            // 0x684  data\alphadigits4.txt
	ImageFont* m_unk0x688;            // 0x688  data\scorefont1.txt (not deleted)
	Font* m_unk0x68c;                 // 0x68c  data\afont1.txt, dialog header font
	Font* m_unk0x690;                 // 0x690  data\mainfont1.txt, dialog button/lines font
	ImageFont* m_unk0x694;            // 0x694  data\overfont2.txt
	Image* m_unk0x698[6];             // 0x698
	Image* m_unk0x6b0[6];             // 0x6b0
	ButtonWidget* m_unk0x6c8;         // 0x6c8  normal-mode button
	ButtonWidget* m_unk0x6cc;         // 0x6cc  time-trial button
	Image* m_unk0x6d0[25];            // 0x6d0  colorized images
	bool mTimeTrial;                  // 0x734
	bool m_unk0x735;                  // 0x735
	bool mNoPopLink;                  // 0x736  registry "NoPopLink"
	bool mIsDiamondMine;              // 0x737  property "IsDiamondMine"
};

}

#endif // __WINDMAPP_H__