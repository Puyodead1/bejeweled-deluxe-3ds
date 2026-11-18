#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

#include "SexyAppFramework/Dialog.h"
#include "SexyAppFramework/SliderListener.h"
#include "decomp.h"

namespace Sexy
{

class WinDMApp;
class Slider;
class DialogButton;

// RTTI: .?AVOptionsDialog@Sexy@@ - the options dialog, size 0x154. Switch port: volume sliders, Help and High Scores
// only. The PC dialog also had Register, the partner web links, Check for Updates, and Fullscreen and Custom Cursors
// checkboxes, none of which mean anything on the console.

// VTABLE: GAME 0x4c6d64 Sexy::Widget
// VTABLE: GAME 0x4c6d4c Sexy::ButtonListener
// VTABLE: GAME 0x4c6d44 Sexy::SliderListener
// VTABLE: GAME 0x4c6d3c Sexy::CheckboxListener
// VTABLE: GAME187 0x506fa0 Sexy::CheckboxListener
// VTABLE: GAME187 0x506fa8 Sexy::SliderListener
// VTABLE: GAME187 0x506fb0 Sexy::ButtonListener
// VTABLE: GAME187 0x506fd4 Sexy::Widget
class OptionsDialog : public Dialog, public SliderListener
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

	WinDMApp* mApp;                         // 0x118
	Slider* mMusicVolumeSlider;             // 0x11c
	Slider* mSfxVolumeSlider;               // 0x120
	DialogButton* mHelpButton;              // 0x12c
	DialogButton* mHighScoresButton;        // 0x134
};

}

#endif // __OPTIONSDIALOG_H__
