#ifndef __C3DINTERFACE_H__
#define __C3DINTERFACE_H__

#include "Common.h"
#include "MemoryImage.h"
#include "SexyMatrix.h"

namespace Sexy
{

class DDInterface;
class TriVertex;
struct C3DState;
struct C3DVertex;
struct C3DTexture;
struct ClipVert;

// 3DS port of the 1.87 D3DInterface: what DDImage sends here when the screen is drawn with the GPU
// (DDImage::Check3D). Everything is drawn with citro3d into a 1024x512 RGBA8 render target texture, of which the
// game's 640x480 is used; DDInterface::Redraw shows that texture on both screens.
//
// The target keeps its pixels from frame to frame, like the Windows draw surface, so the widget manager's dirty
// redraws work unchanged.
//
// Every MemoryImage that's drawn gets a texture set in its mD3DData (see C3DInterface.cpp): pieces of power-of-two
// textures in linear memory, swizzled by the CPU, RGBA8, RGB565 for images without alpha or RGBA4 for images with
// the D3DImageFlag_UseA4R4G4B4 hint, re-uploaded when mBitsChangedCount moves. Images with mPurgeBits drop their CPU
// bits after the upload; RecoverBits reads them back from the texture if they're asked for again.
//
// Draws are batched: quads and triangles go into one vertex buffer per frame and a draw call is issued when the
// texture, filter or blend mode changes. Clip rects and transforms are applied on the CPU, as D3D7's pre-transformed
// vertices were.
class C3DInterface
{
public:
	DDInterface*			mDDInterface;
	int						mWidth;
	int						mHeight;

	typedef std::set<MemoryImage*> ImageSet;
	ImageSet				mImageSet;

	typedef std::list<SexyMatrix3> TransformStack;
	TransformStack			mTransformStack;

	C3DState*				mState;		// citro3d objects, kept out of this header (NULL when not initialised)

	// Counters for the perf log, reset by TakeStats.
	int						mStatDraws;
	int						mStatVertices;
	int						mStatUploads;
	int						mStatUploadBytes;
	int						mTexMemSize;		// bytes of texture memory held by images
	int						mPurgedImages;
	int						mStatFrames;
	uint64_t				mStatWaitUs;		// time C3D_FrameBegin spent waiting for the GPU

public:
	C3DInterface(DDInterface* theDDInterface);
	virtual ~C3DInterface();

	// theProgram is the sprite shader's shaderProgram_s, shared with DDInterface's presenter.
	bool					Init(int theWidth, int theHeight, void* theProgram, int theProjectionLoc, std::string& theError);
	// Frees every texture. recoverBits gives images whose bits were purged their pixels back first (switching to the
	// software renderer); at shutdown they're just dropped.
	void					Cleanup(bool recoverBits);

	// Frames. PreDraw starts one (on the first draw after a present); FinishScene flushes it and returns the target
	// (a C3D_Tex*) for DDInterface to draw on the screens; SceneEnded is called after C3D_FrameEnd.
	bool					PreDraw();
	void*					FinishScene();
	void					SceneEnded();
	bool					InFrame();
	void					Flush();

	void					PushTransform(const SexyMatrix3& theTransform, bool concatenate = true);
	void					PopTransform();

	bool					CreateImageTexture(MemoryImage* theImage);
	void					RemoveMemoryImage(MemoryImage* theImage);
	bool					RecoverBits(MemoryImage* theImage);
	bool					ReadTarget(uint32_t* theBits);		// the frame as ARGB, mWidth x mHeight
	void					InvalidateTarget();		// clear the frame to black when the next one starts
	std::string				TakeStats();

	void					Blt(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter = false);
	void					BltClipF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode);
	void					BltMirror(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter = false);
	void					StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror = false);
	void					BltRotated(Image* theImage, float theX, float theY, const Rect* theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY, const Rect& theSrcRect);
	void					BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, const SexyMatrix3& theTransform, bool linearFilter, float theX = 0, float theY = 0, bool center = false);
	void					DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode);
	void					FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	void					ClearRect(const Rect& theRect);
	void					FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty);
	void					DrawTrianglesTex(const TriVertex theVertices[][3], int theNumTriangles, const Rect* theClipRect, const Color& theColor, int theDrawMode, Image* theTexture, float tx = 0, float ty = 0, bool blend = true);

private:
	void					SetupFrameState();
	void					BeginFrame(bool sync);
	void					FlushBatch();
	void					FlushVertexCache();
	void					Restart();
	void					SetState(void* theTex, bool linear, int theBlend);
	C3DVertex*				AllocVertices(int theCount);
	void					EmitPoly(const ClipVert* theVerts, int theCount, const float* theClip);
	void					FreeGarbage();
	void					ReleasePieces(C3DTexture* theTexture);
	C3DTexture*				GetTexture(MemoryImage* theImage);
	void					DrawImage(Image* theImage, const Rect& theSrcRect, const SexyMatrix3* theMatrix, float theX, float theY, bool center,
								const Rect* theClipRect, const Color& theColor, int theDrawMode, bool linearFilter);
	void					DrawRectQuad(const Rect& theRect, const Color& theColor);
};

}

#endif //__C3DINTERFACE_H__
