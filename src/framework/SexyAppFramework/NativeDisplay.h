#ifndef __NATIVEDISPLAY_H__
#define __NATIVEDISPLAY_H__
#include "Common.h"

namespace Sexy
{

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// VTABLE: GAME187 0x50cab4
class NativeDisplay
{
public:
	int						mRGBBits;
	ulong					mRedMask;
	ulong					mGreenMask;
	ulong					mBlueMask;
	int						mRedBits;
	int						mGreenBits;
	int						mBlueBits;
	int						mRedShift;
	int						mGreenShift;
	int						mBlueShift;

public:
	NativeDisplay();
	virtual ~NativeDisplay();
};

};


#endif