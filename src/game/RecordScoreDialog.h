#ifndef __RECORDSCOREDIALOG_H__
#define __RECORDSCOREDIALOG_H__

#include "SexyAppFramework/Dialog.h"
#include "SexyAppFramework/Rect.h"
#include "SexyAppFramework/EditListener.h"
#include "decomp.h"

namespace Sexy
{

class WinDMApp;
class EditWidget;
class Graphics;

// RTTI: .?AVRecordScoreDialog@Sexy@@ - asks for a name after a high score, size 0x11c.

// VTABLE: GAME 0x4c6fb4 Sexy::Widget
// VTABLE: GAME 0x4c6fa0 Sexy::ButtonListener
// VTABLE: GAME 0x4c6f8c
// VTABLE: GAME187 0x507268
// VTABLE: GAME187 0x50727c Sexy::ButtonListener
// VTABLE: GAME187 0x50729c Sexy::Widget
class RecordScoreDialog : public Dialog, public EditListener
{
public:
	RecordScoreDialog(WinDMApp* theApp, const std::string& theLines);
	virtual ~RecordScoreDialog();

	virtual void AddedToManager(WidgetManager* theWidgetManager);
	virtual void RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void Draw(Graphics* g);
	virtual int GetPreferredHeight(int theWidth);
	virtual void EditWidgetText(int theId, const std::string& theString);
	virtual void Update();

	void DrawEditBox(Graphics* g, const Rect& theRect);
	// Switch port: asks for the name with the system keyboard and records it when the player presses OK.
	void OpenKeyboard();

	// 0x110: EditListener vtable
	WinDMApp* mApp;            // 0x114
	EditWidget* mNameEdit;     // 0x118
	bool mKeyboardShown;       // Switch port: the keyboard has come up on its own once
};

}

#endif // __RECORDSCOREDIALOG_H__