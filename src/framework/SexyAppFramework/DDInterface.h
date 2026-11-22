#ifndef __DDINTERFACE_H__
#define __DDINTERFACE_H__

#include "Common.h"
#include "CritSect.h"
#include "NativeDisplay.h"
#include "Rect.h"
#include "Ratio.h"

namespace Sexy
{

class SexyAppBase;
class DDImage;
class Image;
class MemoryImage;
struct CtrVideo;

typedef std::set<DDImage*> DDImageSet;

// 3DS port of the DirectDraw display. The game draws into mScreenImage, a plain 32-bit ARGB MemoryImage at the
// game's resolution (640x480) whose pixels live in linear memory. Redraw() has the GPU's DMA engine copy it into a
// texture (tiling it on the way) and draws that on both screens with citro3d:
//   bottom screen (320x240, the touch screen): the whole picture at half size, so a touch at (x, y) is game pixel
//     (2x, 2y);
//   top screen (400x240): the same picture, pillarboxed, or with SetMagnifier(true) a 400x240 window at full size
//     centred on the last touch (SetFocusPoint).
// Window pixels are game pixels here: mPresentationRect is (0, 0, 640, 480) and RemapMouse leaves points alone.
// There is no cursor: touch is the only pointer.
// VTABLE: GAME187 0x50cc74
class DDInterface : public NativeDisplay
{
public:
	enum
	{
		RESULT_OK					= 0,
		RESULT_FAIL					= 1,
		RESULT_DD_CREATE_FAIL		= 2,
		RESULT_SURFACE_FAIL			= 3,
		RESULT_EXCLUSIVE_FAIL		= 4,
		RESULT_DISPCHANGE_FAIL		= 5,
		RESULT_INVALID_COLORDEPTH	= 6,
		RESULT_3D_FAIL				= 7
	};

	SexyAppBase*			mApp;
	bool					mIs3D;		// always false: everything is drawn by the software renderer

	CritSect				mCritSect;
	bool					mInRedraw;
	int						mWidth;
	int						mHeight;
	Ratio					mAspect;
	int						mDisplayWidth;
	int						mDisplayHeight;

	Rect					mPresentationRect;
	DWORD					mRefreshRate;
	DWORD					mMillisecondsPerFrame;

	bool					mInitialized;
	HWND					mHWnd;		// unused; there are no windows
	bool					mIsWindowed;
	DDImage*				mScreenImage;
	DDImageSet				mDDImageSet;
	bool					mVideoOnlyDraw;
	uint32_t				mInitCount;

	std::string				mErrorString;

	// What the top screen shows (see the class comment). mViewDirty asks for a present even if the game drew
	// nothing new.
	bool					mMagnifier;
	int						mFocusX;
	int						mFocusY;
	bool					mViewDirty;

	CtrVideo*				mVideo;		// citro3d state, kept out of this header (see DDInterface.cpp)
	uint32_t*				mLinearBits;	// the screen image's pixels, in linear memory for the DMA engine

public:
	void					AddDDImage(DDImage* theDDImage);
	void					RemoveDDImage(DDImage* theDDImage);
	void					Remove3DData(MemoryImage* theImage);

	void					Cleanup();
	void					UpdatePresentationRect();

public:
	DDInterface(SexyAppBase* theApp);
	virtual ~DDInterface();

	static std::string		ResultToString(int theResult);

	DDImage*				GetScreenImage();
	int						Init(HWND theWindow, bool IsWindowed);
	bool					Redraw(Rect* theClipRect = NULL);
	void					SetVideoOnlyDraw(bool videoOnly);
	void					RemapMouse(int& theX, int& theY);

	void					SetMagnifier(bool on);
	void					SetFocusPoint(int theX, int theY);
};

}

#endif //__DDINTERFACE_H__
