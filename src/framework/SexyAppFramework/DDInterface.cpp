#include "DDInterface.h"
#include "DDImage.h"
#include "SexyAppBase.h"
#include "AutoCrit.h"

#include <SDL.h>

using namespace Sexy;

DDInterface::DDInterface(SexyAppBase* theApp)
{
	mApp = theApp;
	mIs3D = false;
	mInRedraw = false;
	mWidth = theApp->mWidth;
	mHeight = theApp->mHeight;
	mAspect.Set(mWidth, mHeight);
	mDisplayWidth = 0;
	mDisplayHeight = 0;
	mRefreshRate = 60;
	mMillisecondsPerFrame = 1000 / mRefreshRate;

	mInitialized = false;
	mHWnd = NULL;
	mIsWindowed = false;
	mScreenImage = NULL;
	mVideoOnlyDraw = false;
	mInitCount = 0;

	mNextCursorX = mCursorX = 0;
	mNextCursorY = mCursorY = 0;
	mCursorImage = NULL;
	mShowCursor = true;

	mWindow = NULL;
	mRenderer = NULL;
	mTexture = NULL;

	// The screen and every image are 32-bit A8R8G8B8, which is what the software blitters' "native" format means.
	mRGBBits = 32;
	mRedMask = 0xFF0000;
	mGreenMask = 0x00FF00;
	mBlueMask = 0x0000FF;
	mRedBits = mGreenBits = mBlueBits = 8;
	mRedShift = 16;
	mGreenShift = 8;
	mBlueShift = 0;
}

DDInterface::~DDInterface()
{
	Cleanup();
}

std::string DDInterface::ResultToString(int theResult)
{
	switch (theResult)
	{
	case RESULT_OK:					return "RESULT_OK";
	case RESULT_FAIL:				return "RESULT_FAIL";
	case RESULT_DD_CREATE_FAIL:		return "RESULT_DD_CREATE_FAIL";
	case RESULT_SURFACE_FAIL:		return "RESULT_SURFACE_FAIL";
	case RESULT_EXCLUSIVE_FAIL:		return "RESULT_EXCLUSIVE_FAIL";
	case RESULT_DISPCHANGE_FAIL:	return "RESULT_DISPCHANGE_FAIL";
	case RESULT_INVALID_COLORDEPTH:	return "RESULT_INVALID_COLORDEPTH";
	case RESULT_3D_FAIL:			return "RESULT_3D_FAIL";
	default:						return "RESULT_UNKNOWN";
	}
}

void DDInterface::Cleanup()
{
	AutoCrit anAutoCrit(mCritSect);

	mInitialized = false;

	delete mScreenImage;
	mScreenImage = NULL;

	if (mTexture != NULL)
		SDL_DestroyTexture(mTexture);
	mTexture = NULL;
	if (mRenderer != NULL)
		SDL_DestroyRenderer(mRenderer);
	mRenderer = NULL;
}

int DDInterface::Init(HWND theWindow, bool IsWindowed)
{
	AutoCrit anAutoCrit(mCritSect);

	Cleanup();

	mHWnd = theWindow;
	mWindow = (SDL_Window*)theWindow;
	mIsWindowed = IsWindowed;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mAspect.Set(mWidth, mHeight);
	mErrorString.clear();

	if (mWindow == NULL)
	{
		mErrorString = "no window";
		return RESULT_FAIL;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (mRenderer == NULL)
	{
		mErrorString = std::string("SDL_CreateRenderer: ") + SDL_GetError();
		return RESULT_DD_CREATE_FAIL;
	}

	mTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, mWidth, mHeight);
	if (mTexture == NULL)
	{
		mErrorString = std::string("SDL_CreateTexture: ") + SDL_GetError();
		return RESULT_SURFACE_FAIL;
	}

	SDL_DisplayMode aMode;
	if (SDL_GetCurrentDisplayMode(0, &aMode) == 0 && aMode.refresh_rate > 0)
		mRefreshRate = aMode.refresh_rate;
	mMillisecondsPerFrame = 1000 / mRefreshRate;
	UpdatePresentationRect();

	mScreenImage = new DDImage(this);
	mScreenImage->Create(mWidth, mHeight);
	mScreenImage->Clear();

	mInitCount++;
	mInitialized = true;
	return RESULT_OK;
}

void DDInterface::UpdatePresentationRect()
{
	int aWindowWidth = 0, aWindowHeight = 0;
	if (mRenderer != NULL)
		SDL_GetRendererOutputSize(mRenderer, &aWindowWidth, &aWindowHeight);
	if (aWindowWidth <= 0 || aWindowHeight <= 0)
	{
		aWindowWidth = 1280;
		aWindowHeight = 720;
	}
	if (aWindowWidth != mDisplayWidth || aWindowHeight != mDisplayHeight)
		fprintf(stderr, "video: output %dx%d\n", aWindowWidth, aWindowHeight);
	mDisplayWidth = aWindowWidth;
	mDisplayHeight = aWindowHeight;

	// Largest rectangle with the game's aspect ratio that fits, centred.
	int aWidth = aWindowWidth;
	int aHeight = aWindowWidth * mHeight / mWidth;
	if (aHeight > aWindowHeight)
	{
		aHeight = aWindowHeight;
		aWidth = aWindowHeight * mWidth / mHeight;
	}
	mPresentationRect = Rect((aWindowWidth - aWidth) / 2, (aWindowHeight - aHeight) / 2, aWidth, aHeight);
}

DDImage* DDInterface::GetScreenImage()
{
	return mScreenImage;
}

void DDInterface::AddDDImage(DDImage* theDDImage)
{
	AutoCrit anAutoCrit(mCritSect);
	mDDImageSet.insert(theDDImage);
}

void DDInterface::RemoveDDImage(DDImage* theDDImage)
{
	AutoCrit anAutoCrit(mCritSect);
	mDDImageSet.erase(theDDImage);
}

void DDInterface::Remove3DData(MemoryImage* theImage)
{
}

void DDInterface::SetVideoOnlyDraw(bool videoOnly)
{
	mVideoOnlyDraw = videoOnly;
}

void DDInterface::RemapMouse(int& theX, int& theY)
{
	// Window pixels -> game pixels.
	if (mPresentationRect.mWidth > 0 && mPresentationRect.mHeight > 0)
	{
		theX = (theX - mPresentationRect.mX) * mWidth / mPresentationRect.mWidth;
		theY = (theY - mPresentationRect.mY) * mHeight / mPresentationRect.mHeight;
	}
}

bool DDInterface::SetCursorImage(Image* theImage)
{
	AutoCrit anAutoCrit(mCritSect);

	if (mCursorImage != theImage)
	{
		mCursorImage = theImage;
		return true;
	}
	return false;
}

void DDInterface::SetCursorPos(int theCursorX, int theCursorY)
{
	AutoCrit anAutoCrit(mCritSect);

	mNextCursorX = theCursorX;
	mNextCursorY = theCursorY;
}

void DDInterface::DrawCursorTo(uint32_t* theBits, int thePitch)
{
	MemoryImage* aCursor = dynamic_cast<MemoryImage*>(mCursorImage);
	if (aCursor == NULL || !mShowCursor)
		return;

	uint32_t* aSrcBits = aCursor->GetBits();
	if (aSrcBits == NULL)
		return;

	// SexyAppBase positions cursors by their top-left corner (hot spot at 0,0).
	int aWidth = aCursor->GetWidth();
	int aHeight = aCursor->GetHeight();
	for (int y = 0; y < aHeight; y++)
	{
		int aDestY = mCursorY + y;
		if (aDestY < 0 || aDestY >= mHeight)
			continue;

		uint32_t* aDestRow = (uint32_t*)((uint8_t*)theBits + aDestY * thePitch);
		const uint32_t* aSrcRow = aSrcBits + y * aWidth;
		for (int x = 0; x < aWidth; x++)
		{
			int aDestX = mCursorX + x;
			if (aDestX < 0 || aDestX >= mWidth)
				continue;

			uint32_t aSrc = aSrcRow[x];
			uint32_t anAlpha = aSrc >> 24;
			if (anAlpha == 0)
				continue;

			uint32_t& aDest = aDestRow[aDestX];
			if (anAlpha == 255)
			{
				aDest = aSrc;
				continue;
			}

			uint32_t aSrcRB = aSrc & 0xFF00FF, aSrcG = aSrc & 0x00FF00;
			uint32_t aDestRB = aDest & 0xFF00FF, aDestG = aDest & 0x00FF00;
			uint32_t aRB = (aDestRB + (((aSrcRB - aDestRB) * anAlpha) >> 8)) & 0xFF00FF;
			uint32_t aG = (aDestG + (((aSrcG - aDestG) * anAlpha) >> 8)) & 0x00FF00;
			aDest = 0xFF000000 | aRB | aG;
		}
	}
}

bool DDInterface::Redraw(Rect* theClipRect)
{
	AutoCrit anAutoCrit(mCritSect);

	if (!mInitialized || mScreenImage == NULL)
		return false;

	mInRedraw = true;
	mCursorX = mNextCursorX;
	mCursorY = mNextCursorY;

	void* aPixels = NULL;
	int aPitch = 0;
	if (SDL_LockTexture(mTexture, NULL, &aPixels, &aPitch) == 0)
	{
		uint32_t* aSrc = mScreenImage->GetBits();
		for (int y = 0; y < mHeight; y++)
			memcpy((uint8_t*)aPixels + y * aPitch, aSrc + y * mWidth, mWidth * sizeof(uint32_t));
		DrawCursorTo((uint32_t*)aPixels, aPitch);
		SDL_UnlockTexture(mTexture);
	}

	UpdatePresentationRect();
	SDL_Rect aDest = { mPresentationRect.mX, mPresentationRect.mY, mPresentationRect.mWidth, mPresentationRect.mHeight };
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mRenderer);
	SDL_RenderCopy(mRenderer, mTexture, NULL, &aDest);
	SDL_RenderPresent(mRenderer);

	mInRedraw = false;
	return true;
}
