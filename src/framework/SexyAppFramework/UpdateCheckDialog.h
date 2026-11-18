#ifndef __UPDATECHECKDIALOG_H__
#define __UPDATECHECKDIALOG_H__

#include "Dialog.h"
#include "decomp.h"

namespace Sexy
{

// RTTI: .?AVUpdateCheckDialog@Sexy@@ - "checking for updates" dialog, size 0x120.
// VTABLE: GAME187 0x51da14 Sexy::Widget
// VTABLE: GAME187 0x51db48 Sexy::ButtonListener
class UpdateCheckDialog : public Dialog
{
public:
	UpdateCheckDialog(Image* theComponentImage, Image* theButtonComponentImage, Image* theImage3, int theId); // 0x462610
	virtual ~UpdateCheckDialog(); // 0x45ee00

	virtual void Draw(Graphics* g); // 0x461680
	virtual void Update(); // 0x45ee20
	virtual int GetPreferredHeight(int theWidth); // 0x45d8c0

	bool m_unk0x110;                  // 0x110
	Image* m_unk0x114;                // 0x114
	int m_unk0x118;                   // 0x118
	int m_unk0x11c;                   // 0x11c
};

}

#endif // __UPDATECHECKDIALOG_H__