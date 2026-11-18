#ifndef __DDINTERFACE_H__
#define __DDINTERFACE_H__

#include "Common.h"
#include "CritSect.h"
#include "NativeDisplay.h"
#include "Rect.h"
#include "Ratio.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace Sexy
{

class SexyAppBase;
class DDImage;
class Image;
class MemoryImage;

typedef std::set<DDImage*> DDImageSet;

// Switch port of the DirectDraw display. The game draws into mScreenImage, a plain 32-bit ARGB MemoryImage at the
// game's resolution (640x480). Redraw() uploads it to an SDL streaming texture, draws the cursor on top and presents
// it scaled to fit the Switch screen (letterboxed, linear filtering); mPresentationRect is that destination rectangle
// in window pixels, which RemapMouse() uses to turn touch positions into game coordinates.
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
	HWND					mHWnd;		// the SDL_Window, as an opaque handle
	bool					mIsWindowed;
	DDImage*				mScreenImage;
	DDImageSet				mDDImageSet;
	bool					mVideoOnlyDraw;
	uint32_t				mInitCount;

	int						mNextCursorX;
	int						mNextCursorY;
	int						mCursorX;
	int						mCursorY;
	Image*					mCursorImage;
	bool					mShowCursor;

	std::string				mErrorString;

	SDL_Window*				mWindow;
	SDL_Renderer*			mRenderer;
	SDL_Texture*			mTexture;

public:
	void					AddDDImage(DDImage* theDDImage);
	void					RemoveDDImage(DDImage* theDDImage);
	void					Remove3DData(MemoryImage* theImage);

	void					Cleanup();
	void					UpdatePresentationRect();
	void					DrawCursorTo(uint32_t* theBits, int thePitch);

public:
	DDInterface(SexyAppBase* theApp);
	virtual ~DDInterface();

	static std::string		ResultToString(int theResult);

	DDImage*				GetScreenImage();
	int						Init(HWND theWindow, bool IsWindowed);
	bool					Redraw(Rect* theClipRect = NULL);
	void					SetVideoOnlyDraw(bool videoOnly);
	void					RemapMouse(int& theX, int& theY);

	bool					SetCursorImage(Image* theImage);
	void					SetCursorPos(int theCursorX, int theCursorY);
};

}

#endif //__DDINTERFACE_H__
