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
class C3DInterface;
struct CtrVideo;

typedef std::set<DDImage*> DDImageSet;

// 3DS port of the DirectDraw display. The game draws into mScreenImage, at the game's resolution (640x480), in one of
// two ways:
//   3D (mIs3D, the default): DDImage sends the screen image's drawing to mC3DInterface, which draws it with the GPU
//     into a render target texture;
//   software (mIs3D false; the file sdmc:/3ds/Bejeweled/software-renderer, or Set3DAcclerated(false)): the screen
//     image is a plain 32-bit ARGB MemoryImage whose pixels live in linear memory, and Redraw() has the GPU's DMA
//     engine copy it into a texture (tiling it on the way).
// Either way Redraw() draws that texture on both screens with citro3d:
//   bottom screen (320x240, the touch screen): the whole picture at half size, so a touch at (x, y) is game pixel
//     (2x, 2y);
//   top screen (400x240), by SetTopView (L/R cycles):
//     TOP_VIEW_ART (the default): its own picture, SetTopImage's (the title art), at half size; also shown in the
//       other views while ForceTopImage(true) (the title screen);
//     TOP_VIEW_CLOSEUP: a 400x240 window at full size around the focus point (SetFocusPoint: the open dialog, or the
//       last touch), gliding to it rather than jumping;
//     TOP_VIEW_WHOLE: the same picture as the bottom screen, pillarboxed.
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

	enum
	{
		TOP_VIEW_ART				= 0,
		TOP_VIEW_CLOSEUP			= 1,
		TOP_VIEW_WHOLE				= 2,
		NUM_TOP_VIEWS				= 3
	};

	SexyAppBase*			mApp;
	bool					mIs3D;		// the screen is drawn by the GPU (mC3DInterface)

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
	int						mTopView;	// TOP_VIEW_*
	int						mFocusX;
	int						mFocusY;
	bool					mViewDirty;
	float					mViewX;		// the magnifier window's top left now, easing towards the focus point
	float					mViewY;		// (negative: jump straight there)
	uint64_t				mViewTimeUs;

	// SetTopImage's picture for TOP_VIEW_ART (half size, centred).
	int						mTopImageWidth;	// 0: none
	int						mTopImageHeight;
	bool					mTopImageForced;

	CtrVideo*				mVideo;		// citro3d state, kept out of this header (see DDInterface.cpp)
	uint32_t*				mLinearBits;	// software: the screen image's pixels, in linear memory for the DMA engine
	C3DInterface*			mC3DInterface;	// 3D: the GPU renderer
	bool					mShuttingDown;

public:
	void					AddDDImage(DDImage* theDDImage);
	void					RemoveDDImage(DDImage* theDDImage);
	void					Remove3DData(MemoryImage* theImage);
	bool					RecoverBits(MemoryImage* theImage);

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

	void					SetTopView(int theView);
	void					SetFocusPoint(int theX, int theY);
	// The top screen's own picture (TOP_VIEW_ART), shown at half size, so at most 800x480; NULL drops it. The image
	// is copied, so it can change or go away afterwards.
	void					SetTopImage(MemoryImage* theImage);
	// Show that picture whatever the view (the title screen, whose bottom screen has the loading bar and "tap to
	// play").
	void					ForceTopImage(bool force);

	// Back from the HOME menu or sleep: the GPU renderer's frame may have been overwritten, so it starts again from
	// black (the caller marks every widget dirty).
	void					Restored();
};

}

#endif //__DDINTERFACE_H__
