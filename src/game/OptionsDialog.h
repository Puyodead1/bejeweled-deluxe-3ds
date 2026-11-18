#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

#include <vector>
#include "SexyAppFramework/Dialog.h"
#include "SexyAppFramework/SliderListener.h"
#include "SexyAppFramework/CheckboxListener.h"
#include "decomp.h"

namespace Sexy
{

class WinDMApp;
class Checkbox;
class Slider;
class DialogButton;
class Image;

// RTTI: .?AVOptionsDialog@Sexy@@ - the options dialog, size 0x154.

// VTABLE: GAME 0x4c6d64 Sexy::Widget
// VTABLE: GAME 0x4c6d4c Sexy::ButtonListener
// VTABLE: GAME 0x4c6d44 Sexy::SliderListener
// VTABLE: GAME 0x4c6d3c Sexy::CheckboxListener
// VTABLE: GAME187 0x506fa0 Sexy::CheckboxListener
// VTABLE: GAME187 0x506fa8 Sexy::SliderListener
// VTABLE: GAME187 0x506fb0 Sexy::ButtonListener
// VTABLE: GAME187 0x506fd4 Sexy::Widget
class OptionsDialog : public Dialog, public SliderListener, public CheckboxListener
{
public:
	OptionsDialog(WinDMApp* theApp);
	virtual ~OptionsDialog();

	virtual void AddedToManager(WidgetManager* theWidgetManager);
	virtual void RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void Draw(Graphics* g);
	virtual int GetPreferredHeight(int theWidth);
	virtual void ButtonPress(int theId);
	virtual void ButtonDepress(int theId);
	virtual void SliderVal(int theId, double theVal);
	virtual void CheckboxChecked(int theId, bool checked);

	// 0x110: SliderListener vtable
	// 0x114: CheckboxListener vtable
	WinDMApp* mApp;                         // 0x118
	Slider* mMusicVolumeSlider;             // 0x11c
	Slider* mSfxVolumeSlider;               // 0x120
	Checkbox* mWindowedCheckbox;            // 0x124 (checked = full screen)
	Checkbox* mCustomCursorsCheckbox;       // 0x128
	DialogButton* mHelpButton;              // 0x12c
	DialogButton* mRegisterButton;          // 0x130
	DialogButton* mHighScoresButton;        // 0x134
	std::vector<DialogButton*> mLinkButtons; // 0x138
	DialogButton* mUpdateButton;            // 0x148
	Image* mCheckedImage;                   // 0x14c
	Image* mUncheckedImage;                 // 0x150
};

}

#endif // __OPTIONSDIALOG_H__