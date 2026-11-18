#ifndef __REGISTERDIALOG_H__
#define __REGISTERDIALOG_H__

#include "SexyAppFramework/Dialog.h"
#include "SexyAppFramework/Rect.h"
#include "SexyAppFramework/EditListener.h"
#include "decomp.h"

namespace Sexy
{

class EditWidget;
class HyperlinkWidget;
class WinDMApp;
class Graphics;

// RTTI: .?AVRegisterDialog@Sexy@@ - asks for the registration name and code. Size 0x128.

// VTABLE: GAME 0x4c70e4 Sexy::Widget
// VTABLE: GAME 0x4c70cc Sexy::ButtonListener
// VTABLE: GAME 0x4c70b8
// VTABLE: GAME187 0x50740c
// VTABLE: GAME187 0x507420 Sexy::ButtonListener
// VTABLE: GAME187 0x507444 Sexy::Widget
class RegisterDialog : public Dialog, public EditListener
{
public:
	RegisterDialog(WinDMApp* theApp);
	virtual ~RegisterDialog();

	virtual void AddedToManager(WidgetManager* theWidgetManager);
	virtual void RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void Draw(Graphics* g);
	virtual int GetPreferredHeight(int theWidth);
	virtual void ButtonDepress(int theId);
	virtual void EditWidgetText(int theId, const std::string& theString);

	void DrawEditBox(Graphics* g, const Rect& theRect);

	// 0x110: EditListener vtable
	WinDMApp* mApp;                   // 0x114
	EditWidget* mUserNameEdit;        // 0x118
	EditWidget* mRegCodeEdit;         // 0x11c
	HyperlinkWidget* mRegisterLink;   // 0x120
	bool mSwitchedToWindowed;         // 0x124
};

}

#endif // __REGISTERDIALOG_H__