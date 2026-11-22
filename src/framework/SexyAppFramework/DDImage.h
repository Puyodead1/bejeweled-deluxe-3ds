#ifndef __DDIMAGE_H__
#define __DDIMAGE_H__

#include "MemoryImage.h"

namespace Sexy
{

class DDInterface;
class SysFont;

// Texture hints from D3DInterface.h, set by ResourceManager; C3DInterface reads them.
enum D3DImageFlags
{
	D3DImageFlag_MinimizeNumSubdivisions = 0x0001,
	D3DImageFlag_Use64By64Subdivisions = 0x0002,
	D3DImageFlag_UseA4R4G4B4 = 0x0004,
	D3DImageFlag_UseA8R8G8B8 = 0x0008
};

// 3DS port: DDImage has no DirectDraw surface; it is a MemoryImage that registers with the DDInterface. As on Windows,
// what's drawn onto the screen image goes to the GPU when the display is 3D (Check3D: C3DInterface, citro3d), and
// everything else goes through MemoryImage's software blitters. The surface members stay so the framework and game
// code that sets them compiles unchanged; mSurface is always NULL.
// VTABLE: GAME187 0x50cabc
class DDImage : public MemoryImage
{
protected:
	friend class			SysFont;

public:
	DDInterface*			mDDInterface;
	void*					mSurface;
	bool					mSurfaceSet;
	bool					mNoLock;
	bool					mVideoMemory;
	bool					mFirstPixelTrans;
	bool					mWantDDSurface;
	bool					mDrawToBits;

	int						mLockCount;

private:
	void					Init();

public:
	bool					GenerateDDSurface() { return false; }
	void					DeleteDDSurface() { }
	virtual void			SetVideoMemory(bool wantVideoMemory) { mVideoMemory = wantVideoMemory; }
	virtual void			RehupFirstPixelTrans() { }

	void*					GetSurface() { return NULL; }

	// True for the screen image of a 3D display: its drawing goes to the GPU.
	static bool				Check3D(DDImage *theImage);
	static bool				Check3D(Image *theImage);

public:
	DDImage();
	DDImage(DDInterface* theDDInterface);
	virtual ~DDImage();

	virtual bool			LockSurface() { return false; }
	virtual bool			UnlockSurface() { return false; }

	virtual void			PurgeBits();
	virtual uint32_t*		GetBits();

	virtual void			FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	virtual void			ClearRect(const Rect& theRect);
	virtual void			DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode);
	virtual void			DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode);
	virtual void			FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight);
	virtual bool			PolyFill3D(const Point theVertices[], int theNumVertices, const Rect *theClipRect, const Color &theColor, int theDrawMode, int tx, int ty, bool convex);

	virtual void			Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	virtual void			BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect &theClipRect, const Color& theColor, int theDrawMode);
	virtual void			BltRotated(Image* theImage, float theX, float theY, const Rect &theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY);
	virtual void			StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch);
	virtual void			BltMatrix(Image* theImage, float x, float y, const SexyMatrix3 &theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, bool blend);
	virtual void			BltTrianglesTex(Image *theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect& theClipRect, const Color &theColor, int theDrawMode, float tx, float ty, bool blend);
	virtual void			BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	virtual void			StretchBltMirror(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch);
};

}

#endif //__DDIMAGE_H__
