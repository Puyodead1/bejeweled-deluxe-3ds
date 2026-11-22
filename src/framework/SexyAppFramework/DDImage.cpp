#include "DDImage.h"
#include "DDInterface.h"
#include "C3DInterface.h"
#include "SexyAppBase.h"
#include "Graphics.h"

using namespace Sexy;

// FUNCTION: GAME187 0x40b4b0
DDImage::DDImage(DDInterface* theDDInterface) :
	MemoryImage(theDDInterface->mApp)
{
	mDDInterface = theDDInterface;
	Init();
}

DDImage::DDImage() :
	MemoryImage(gSexyAppBase)
{
	mDDInterface = gSexyAppBase->mDDInterface;
	Init();
}

// FUNCTION: GAME187 0x40b540
DDImage::~DDImage()
{
	mDDInterface->RemoveDDImage(this);
}

// FUNCTION: GAME187 0x40b5b0
void DDImage::Init()
{
	mSurface = NULL;
	mDDInterface->AddDDImage(this);

	mNoLock = false;
	mVideoMemory = false;
	mFirstPixelTrans = false;
	mWantDDSurface = false;
	mDrawToBits = false;
	mSurfaceSet = false;

	mLockCount = 0;
}

bool DDImage::Check3D(DDImage* theImage)
{
	if (theImage == NULL)
		return false;
	DDInterface* anInterface = theImage->mDDInterface;
	return anInterface->mIs3D && anInterface->mC3DInterface != NULL && theImage == anInterface->mScreenImage;
}

bool DDImage::Check3D(Image* theImage)
{
	return Check3D(dynamic_cast<DDImage*>(theImage));
}

void DDImage::PurgeBits()
{
	if (mDDInterface->mIs3D)
	{
		// The texture holds the pixels (C3DInterface calls this again once it's made).
		MemoryImage::PurgeBits();
		return;
	}
	// On Windows the software path freed the 32-bit bits once a surface or the native alpha data held the pixels.
	// Memory isn't tight in software mode and keeping the bits avoids reconverting them on every GetBits(), so only
	// the flag is kept.
	mPurgeBits = true;
}

uint32_t* DDImage::GetBits()
{
	if (Check3D(this))
	{
		// The screen is the GPU's render target; read it back (slow: only screenshots want this).
		if (mBits == NULL)
		{
			mBits = new uint32_t[mWidth * mHeight + 1];
			mBits[mWidth * mHeight] = MEMORYCHECK_ID;
		}
		if (!mDDInterface->mC3DInterface->ReadTarget(mBits))
			memset(mBits, 0, mWidth * mHeight * 4);
		return mBits;
	}
	return MemoryImage::GetBits();
}

///////////////////////////////////////////////////////////////////////////////
// Drawing: the 1.87 DDImage's D3D routing, to C3DInterface.

void DDImage::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->FillRect(theRect, theColor, theDrawMode);
		return;
	}
	MemoryImage::FillRect(theRect, theColor, theDrawMode);
}

void DDImage::ClearRect(const Rect& theRect)
{
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->ClearRect(theRect);
		return;
	}
	MemoryImage::ClearRect(theRect);
}

void DDImage::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
		return;
	}
	MemoryImage::DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void DDImage::DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
		return;
	}
	MemoryImage::DrawLineAA(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void DDImage::FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight)
{
	if (theSpanCount == 0)
		return;

	if (Check3D(this))
	{
		// As 1.87 did: fill a temporary image the size of the spans in software and draw that.
		int l = theSpans[0].mX, t = theSpans[0].mY;
		int r = l + theSpans[0].mWidth, b = t;
		for (int i = 1; i < theSpanCount; ++i)
		{
			l = std::min(theSpans[i].mX, l);
			r = std::max(theSpans[i].mX + theSpans[i].mWidth - 1, r);
			t = std::min(theSpans[i].mY, t);
			b = std::max(theSpans[i].mY, b);
		}
		for (int i = 0; i < theSpanCount; ++i)
		{
			theSpans[i].mX -= l;
			theSpans[i].mY -= t;
		}

		MemoryImage aTempImage(mApp);
		aTempImage.Create(r - l + 1, b - t + 1);
		aTempImage.FillScanLinesWithCoverage(theSpans, theSpanCount, theColor, theDrawMode, theCoverage, theCoverX - l, theCoverY - t, theCoverWidth, theCoverHeight);
		mDDInterface->mC3DInterface->Blt(&aTempImage, (float)l, (float)t, Rect(0, 0, r - l + 1, b - t + 1), Color::White, theDrawMode);
		return;
	}
	MemoryImage::FillScanLinesWithCoverage(theSpans, theSpanCount, theColor, theDrawMode, theCoverage, theCoverX, theCoverY, theCoverWidth, theCoverHeight);
}

bool DDImage::PolyFill3D(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty, bool convex)
{
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->FillPoly(theVertices, theNumVertices, theClipRect, theColor, theDrawMode, tx, ty);
		return true;
	}
	return false;
}

void DDImage::Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode)
{
	theImage->mDrawn = true;
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->Blt(theImage, (float)theX, (float)theY, theSrcRect, theColor, theDrawMode);
		return;
	}
	MemoryImage::Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
}

void DDImage::BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode)
{
	theImage->mDrawn = true;
	if (Check3D(this))
	{
		FRect aClipRect((float)theClipRect.mX, (float)theClipRect.mY, (float)theClipRect.mWidth, (float)theClipRect.mHeight);
		FRect aDestRect(theX, theY, (float)theSrcRect.mWidth, (float)theSrcRect.mHeight);
		FRect anIntersect = aDestRect.Intersection(aClipRect);
		if (anIntersect.mWidth != aDestRect.mWidth || anIntersect.mHeight != aDestRect.mHeight)
		{
			if (anIntersect.mWidth != 0 && anIntersect.mHeight != 0)
				mDDInterface->mC3DInterface->BltClipF(theImage, theX, theY, theSrcRect, &theClipRect, theColor, theDrawMode);
		}
		else
			mDDInterface->mC3DInterface->Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode, true);
		return;
	}
	MemoryImage::BltF(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode);
}

void DDImage::BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY)
{
	theImage->mDrawn = true;
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->BltRotated(theImage, theX, theY, &theClipRect, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY, theSrcRect);
		return;
	}
	MemoryImage::BltRotated(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY);
}

void DDImage::StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch)
{
	theImage->mDrawn = true;
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->StretchBlt(theImage, theDestRect, theSrcRect, &theClipRect, theColor, theDrawMode, fastStretch);
		return;
	}
	MemoryImage::StretchBlt(theImage, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, fastStretch);
}

void DDImage::BltMatrix(Image* theImage, float x, float y, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend)
{
	theImage->mDrawn = true;
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->BltTransformed(theImage, &theClipRect, theColor, theDrawMode, theSrcRect, theMatrix, blend, x, y, true);
		return;
	}
	MemoryImage::BltMatrix(theImage, x, y, theMatrix, theClipRect, theColor, theDrawMode, theSrcRect, blend);
}

void DDImage::BltTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect& theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend)
{
	theTexture->mDrawn = true;
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->DrawTrianglesTex(theVertices, theNumTriangles, &theClipRect, theColor, theDrawMode, theTexture, tx, ty, blend);
		return;
	}
	MemoryImage::BltTrianglesTex(theTexture, theVertices, theNumTriangles, theClipRect, theColor, theDrawMode, tx, ty, blend);
}

void DDImage::BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode)
{
	theImage->mDrawn = true;
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->BltMirror(theImage, (float)theX, (float)theY, theSrcRect, theColor, theDrawMode);
		return;
	}
	MemoryImage::BltMirror(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
}

void DDImage::StretchBltMirror(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch)
{
	theImage->mDrawn = true;
	if (Check3D(this))
	{
		mDDInterface->mC3DInterface->StretchBlt(theImage, theDestRect, theSrcRect, &theClipRect, theColor, theDrawMode, fastStretch, true);
		return;
	}
	MemoryImage::StretchBltMirror(theImage, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, fastStretch);
}
