#ifndef __DIALOGLISTENER_H__
#define __DIALOGLISTENER_H__

namespace Sexy
{

// VTABLE: GAME187 0x50de8c
class DialogListener
{
public:
	virtual void			DialogButtonPress(int theDialogId, int theButtonId) {}
	virtual void			DialogButtonDepress(int theDialogId, int theButtonId) {}
};

}

#endif // __DIALOGLISTENER_H__