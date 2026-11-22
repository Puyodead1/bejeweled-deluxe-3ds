#include "C3DInterface.h"
#include "DDInterface.h"
#include "DDImage.h"
#include "Graphics.h"
#include "TriVertex.h"
#include "AutoCrit.h"
#include "SexyAppBase.h"
#include "CtrPlatform.h"

#include <3ds.h>
#include <citro3d.h>
#include <math.h>

using namespace Sexy;

// The render target. The game's 640x480 frame is its top-left corner.
static const int TARGET_WIDTH = 1024;
static const int TARGET_HEIGHT = 512;

// One frame's vertices. A frame that needs more is split (C3DInterface::Restart).
static const int MAX_VERTICES = 0x8000;

static const int MIN_TEX_SIZE = 8;
static const int MAX_TEX_SIZE = 1024;

// Texture rows. Every texture coordinate here is sampled as t = 1 - y / height, image row 0 at the top (the same
// rule DDInterface's presenter uses for the frame textures, and the projection below gives the render target), with
// image row 0 written to the texture's first memory row. If images ever come out upside down, this flips the upload.
static const bool kFlipUpload = false;

// Row order of what the display transfer engine reads back from the render target (ReadTarget).
static const bool kFlipReadback = false;

// Wait for the vertical blank before starting a frame (C3D_FRAME_SYNCDRAW). The framework's loop runs 100 updates a
// second and draws whenever it's caught up; blocking for vblank inside a draw makes it miss the next one, so frames
// only wait for the GPU to finish the previous one. The screens still flip at vblank.
static const bool kVSync = false;

// Opaque images become RGB565 textures, as D3D7 made them R5G6B5 surfaces (half the memory; colour is 5/6/5 bits).
static const bool kUse565 = true;

enum
{
	BLEND_NORMAL,
	BLEND_ADDITIVE,
	BLEND_REPLACE
};

namespace Sexy
{

struct C3DVertex
{
	float					mX, mY, mZ;
	float					mU, mV;
	uint32_t				mColor;			// R, G, B, A bytes
};

// One run of an image's columns or rows that's stored in one texture: [mStart, mStart + mSize) of the image, in a
// texture mTexSize long (padded by repeating the last pixel).
struct C3DSeg
{
	int						mStart;
	int						mSize;
	int						mTexSize;
};

struct C3DPiece
{
	C3D_Tex*				mTex;
	uint32_t				mUseFrame;		// the last frame that drew with it (0 = none)
};

// What MemoryImage::mD3DData holds.
struct C3DTexture
{
	int						mWidth;
	int						mHeight;
	int						mBitsChangedCount;
	GPU_TEXCOLOR			mFormat;
	DWORD					mFlags;
	std::vector<C3DSeg>		mCols;
	std::vector<C3DSeg>		mRows;
	std::vector<C3DPiece>	mPieces;		// row by row
	int						mMemSize;
};

struct C3DState
{
	shaderProgram_s*		mProgram;
	int						mProjectionLoc;
	C3D_Tex					mTargetTex;
	bool					mTargetTexValid;
	C3D_RenderTarget*		mTarget;
	C3D_Mtx					mProjection;

	C3DVertex*				mVertices;
	int						mVertCount;
	int						mBatchStart;
	int						mFlushedCount;	// vertices whose cache lines have been flushed

	bool					mInFrame;
	uint32_t				mFrameNum;

	// The state the pending batch is drawn with.
	C3D_Tex*				mCurTex;		// NULL = untextured
	bool					mCurLinear;
	int						mCurBlend;
	bool					mCurTextured;
	bool					mStateValid;

	std::vector<C3D_Tex*>	mGarbage;		// textures the GPU may still read, freed after the next C3D_FrameBegin
	bool					mOutOfMemoryLogged;
	uint32_t*				mReadBuf;
	bool					mNeedClear;
};

// A vertex being clipped: x, y, u, v, then R, G, B, A (0..255).
struct ClipVert
{
	float					a[8];
};

}

static const int MAX_CLIP_VERTS = 32;

// Morton order of the pixels in an 8x8 tile.
static uint8_t gMorton[64];

static void InitMorton()
{
	for (int y = 0; y < 8; y++)
		for (int x = 0; x < 8; x++)
			gMorton[y * 8 + x] = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
}

static inline int TexelOffset(int x, int y, int theTexWidth)
{
	return (((y >> 3) * (theTexWidth >> 3) + (x >> 3)) << 6) + gMorton[((y & 7) << 3) | (x & 7)];
}

static int BytesPerTexel(GPU_TEXCOLOR theFormat)
{
	return theFormat == GPU_RGBA8 ? 4 : 2;
}

// Splits theSize pixels into texture sizes, the way D3DTextureData::CreateTextureDimensions did: a power of two, or
// two smaller ones when rounding up would waste more than a quarter.
static void MakeSegs(int theSize, DWORD theFlags, std::vector<C3DSeg>& theSegs)
{
	theSegs.clear();
	int aStart = 0;
	while (aStart < theSize)
	{
		int aLeft = theSize - aStart;
		int aTexSize;
		if (theFlags & D3DImageFlag_Use64By64Subdivisions)
			aTexSize = 64;
		else
		{
			aTexSize = MIN_TEX_SIZE;
			while (aTexSize < aLeft && aTexSize < MAX_TEX_SIZE)
				aTexSize <<= 1;
			if (aTexSize > aLeft && aTexSize > 64 && (aTexSize - aLeft) * 3 > aLeft && !(theFlags & D3DImageFlag_MinimizeNumSubdivisions))
				aTexSize >>= 1;
		}
		C3DSeg aSeg = { aStart, std::min(aTexSize, aLeft), aTexSize };
		theSegs.push_back(aSeg);
		aStart += aSeg.mSize;
	}
}

static inline uint32_t PackColor(float r, float g, float b, float a)
{
	return (uint32_t)(int)(r + 0.5f) | ((uint32_t)(int)(g + 0.5f) << 8) | ((uint32_t)(int)(b + 0.5f) << 16) | ((uint32_t)(int)(a + 0.5f) << 24);
}

static inline void SetVertColor(ClipVert& v, const Color& c)
{
	v.a[4] = (float)c.mRed;
	v.a[5] = (float)c.mGreen;
	v.a[6] = (float)c.mBlue;
	v.a[7] = (float)c.mAlpha;
}

static inline void SetVertColorARGB(ClipVert& v, uint32_t c)
{
	v.a[4] = (float)((c >> 16) & 0xFF);
	v.a[5] = (float)((c >> 8) & 0xFF);
	v.a[6] = (float)(c & 0xFF);
	v.a[7] = (float)(c >> 24);
}

// Keeps the part of a polygon where a[axis] >= theValue (keepGreater) or <= theValue.
static int ClipAxis(const ClipVert* theIn, int theCount, ClipVert* theOut, int theAxis, float theValue, bool keepGreater)
{
	int aCount = 0;
	for (int i = 0; i < theCount; i++)
	{
		const ClipVert& a = theIn[i];
		const ClipVert& b = theIn[(i + 1) % theCount];
		float da = keepGreater ? a.a[theAxis] - theValue : theValue - a.a[theAxis];
		float db = keepGreater ? b.a[theAxis] - theValue : theValue - b.a[theAxis];
		bool aIn = da >= 0;
		bool bIn = db >= 0;
		if (aIn && aCount < MAX_CLIP_VERTS)
			theOut[aCount++] = a;
		if (aIn != bIn && aCount < MAX_CLIP_VERTS)
		{
			float t = da / (da - db);
			ClipVert& c = theOut[aCount++];
			for (int k = 0; k < 8; k++)
				c.a[k] = a.a[k] + (b.a[k] - a.a[k]) * t;
		}
	}
	return aCount;
}

// Clips a polygon to [x0, x1] x [y0, y1] on axes theAxis, theAxis + 1. Returns the vertex count (in theBuf).
static int ClipToBox(ClipVert* theBuf, ClipVert* theTemp, int theCount, int theAxis, float x0, float y0, float x1, float y1)
{
	theCount = ClipAxis(theBuf, theCount, theTemp, theAxis, x0, true);
	theCount = ClipAxis(theTemp, theCount, theBuf, theAxis, x1, false);
	theCount = ClipAxis(theBuf, theCount, theTemp, theAxis + 1, y0, true);
	theCount = ClipAxis(theTemp, theCount, theBuf, theAxis + 1, y1, false);
	return theCount;
}

static inline void TransformPoint(const SexyMatrix3& m, float& x, float& y)
{
	float nx = m.m00 * x + m.m01 * y + m.m02;
	float ny = m.m10 * x + m.m11 * y + m.m12;
	x = nx;
	y = ny;
}

///////////////////////////////////////////////////////////////////////////////

C3DInterface::C3DInterface(DDInterface* theDDInterface)
{
	mDDInterface = theDDInterface;
	mWidth = 0;
	mHeight = 0;
	mState = NULL;
	mStatDraws = mStatVertices = mStatUploads = mStatUploadBytes = 0;
	mTexMemSize = 0;
	mPurgedImages = 0;
	mStatFrames = 0;
	mStatWaitUs = 0;
}

C3DInterface::~C3DInterface()
{
	Cleanup(false);
}

bool C3DInterface::Init(int theWidth, int theHeight, void* theProgram, int theProjectionLoc, std::string& theError)
{
	Cleanup(false);
	InitMorton();

	mWidth = theWidth;
	mHeight = theHeight;
	if (mWidth > TARGET_WIDTH || mHeight > TARGET_HEIGHT)
	{
		theError = "the screen doesn't fit the 1024x512 render target";
		return false;
	}

	C3DState* s = new C3DState;
	s->mProgram = (shaderProgram_s*)theProgram;
	s->mProjectionLoc = theProjectionLoc;
	s->mTargetTexValid = false;
	s->mTarget = NULL;
	s->mVertices = NULL;
	s->mVertCount = s->mBatchStart = s->mFlushedCount = 0;
	s->mInFrame = false;
	s->mFrameNum = 0;
	s->mCurTex = NULL;
	s->mCurLinear = false;
	s->mCurBlend = BLEND_NORMAL;
	s->mCurTextured = false;
	s->mStateValid = false;
	s->mOutOfMemoryLogged = false;
	s->mReadBuf = NULL;
	s->mNeedClear = true;
	mState = s;

	// The target: VRAM if there's room.
	s->mTargetTexValid = C3D_TexInitVRAM(&s->mTargetTex, TARGET_WIDTH, TARGET_HEIGHT, GPU_RGBA8) ||
		C3D_TexInit(&s->mTargetTex, TARGET_WIDTH, TARGET_HEIGHT, GPU_RGBA8);
	if (!s->mTargetTexValid)
	{
		theError = "can't allocate the render target";
		return false;
	}
	C3D_TexSetFilter(&s->mTargetTex, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetWrap(&s->mTargetTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
	s->mTarget = C3D_RenderTargetCreateFromTex(&s->mTargetTex, GPU_TEXFACE_2D, 0, -1);
	if (s->mTarget == NULL)
	{
		theError = "can't create the render target";
		return false;
	}

	// Game y = 0 at clip y = +1, the top of the texture as it's sampled.
	Mtx_Ortho(&s->mProjection, 0.0f, (float)TARGET_WIDTH, (float)TARGET_HEIGHT, 0.0f, 1.0f, -1.0f, true);

	s->mVertices = (C3DVertex*)linearAlloc(MAX_VERTICES * sizeof(C3DVertex));
	if (s->mVertices == NULL)
	{
		theError = "out of linear memory for vertices";
		return false;
	}

	fprintf(stderr, "c3d: %dx%d into a %dx%d target in %s, %d KB of vertices\n", mWidth, mHeight, TARGET_WIDTH, TARGET_HEIGHT,
		(osConvertVirtToPhys(s->mTargetTex.data) >= OS_VRAM_PADDR && osConvertVirtToPhys(s->mTargetTex.data) < OS_VRAM_PADDR + OS_VRAM_SIZE) ? "VRAM" : "linear memory",
		(int)(MAX_VERTICES * sizeof(C3DVertex) / 1024));
	return true;
}

void C3DInterface::Cleanup(bool recoverBits)
{
	C3DState* s = mState;
	if (s == NULL)
		return;

	AutoCrit anAutoCrit(mDDInterface->mCritSect);

	if (s->mInFrame)
	{
		FlushBatch();
		C3D_FrameEnd(0);
		s->mInFrame = false;
	}
	// Wait for the GPU to be done with everything; after this no texture is in use. (Render targets can only be
	// deleted outside a frame, so this one ends at once.)
	C3D_FrameBegin(0);
	C3D_FrameEnd(0);
	s->mFrameNum++;
	s->mCurTex = NULL;
	s->mStateValid = false;

	// Images whose bits were purged get them back before their textures go (the software renderer needs them).
	ImageSet aSet = mImageSet;
	for (ImageSet::iterator anItr = aSet.begin(); anItr != aSet.end(); ++anItr)
	{
		MemoryImage* anImage = *anItr;
		if (recoverBits && anImage->mBits == NULL && anImage->mColorTable == NULL)
			anImage->GetBits();
		RemoveMemoryImage(anImage);
	}
	mImageSet.clear();
	FreeGarbage();

	if (s->mTarget != NULL)
		C3D_RenderTargetDelete(s->mTarget);
	if (s->mTargetTexValid)
		C3D_TexDelete(&s->mTargetTex);
	if (s->mVertices != NULL)
		linearFree(s->mVertices);
	if (s->mReadBuf != NULL)
		linearFree(s->mReadBuf);

	delete s;
	mState = NULL;
	mTransformStack.clear();
}

///////////////////////////////////////////////////////////////////////////////
// Frames

void C3DInterface::SetupFrameState()
{
	C3DState* s = mState;
	C3D_BindProgram(s->mProgram);
	C3D_AttrInfo* anAttrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(anAttrInfo);
	AttrInfo_AddLoader(anAttrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(anAttrInfo, 1, GPU_FLOAT, 2);
	AttrInfo_AddLoader(anAttrInfo, 2, GPU_UNSIGNED_BYTE, 4);
	C3D_BufInfo* aBufInfo = C3D_GetBufInfo();
	BufInfo_Init(aBufInfo);
	BufInfo_Add(aBufInfo, s->mVertices, sizeof(C3DVertex), 3, 0x210);
	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
	C3D_CullFace(GPU_CULL_NONE);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, s->mProjectionLoc, &s->mProjection);
	s->mStateValid = false;
	s->mCurTex = NULL;
}

void C3DInterface::BeginFrame(bool sync)
{
	C3DState* s = mState;
	uint64_t aStart = CtrPlatform::MicroTime();
	C3D_FrameBegin(sync && kVSync ? C3D_FRAME_SYNCDRAW : 0);
	mStatWaitUs += CtrPlatform::MicroTime() - aStart;
	if (sync)
		mStatFrames++;
	s->mInFrame = true;
	s->mFrameNum++;
	FreeGarbage();
	s->mVertCount = s->mBatchStart = s->mFlushedCount = 0;
	if (s->mNeedClear)
	{
		C3D_RenderTargetClear(s->mTarget, C3D_CLEAR_COLOR, 0x000000FF, 0);
		s->mNeedClear = false;
	}
	C3D_FrameDrawOn(s->mTarget);
	SetupFrameState();
}

bool C3DInterface::PreDraw()
{
	C3DState* s = mState;
	if (s == NULL)
		return false;
	if (!s->mInFrame)
		BeginFrame(true);
	return true;
}

bool C3DInterface::InFrame()
{
	return mState != NULL && mState->mInFrame;
}

void C3DInterface::FlushVertexCache()
{
	C3DState* s = mState;
	if (s->mVertCount > s->mFlushedCount)
	{
		GSPGPU_FlushDataCache(&s->mVertices[s->mFlushedCount], (s->mVertCount - s->mFlushedCount) * sizeof(C3DVertex));
		s->mFlushedCount = s->mVertCount;
	}
}

void C3DInterface::FlushBatch()
{
	C3DState* s = mState;
	int aCount = s->mVertCount - s->mBatchStart;
	if (aCount > 0)
	{
		C3D_DrawArrays(GPU_TRIANGLES, s->mBatchStart, aCount);
		s->mBatchStart = s->mVertCount;
		mStatDraws++;
	}
}

void C3DInterface::Flush()
{
	if (mState == NULL || !mState->mInFrame)
		return;
	FlushBatch();
	if (C3D_GetCmdBufUsage() > 0.8f)
		Restart();
}

// Ends the frame so far (the GPU draws it into the target) and starts another on the same target, for frames that
// run out of vertices or command buffer.
void C3DInterface::Restart()
{
	C3DState* s = mState;
	FlushBatch();
	FlushVertexCache();
	C3D_FrameEnd(0);
	BeginFrame(false);
}

void* C3DInterface::FinishScene()
{
	if (!PreDraw())
		return NULL;
	FlushBatch();
	FlushVertexCache();
	return &mState->mTargetTex;
}

void C3DInterface::SceneEnded()
{
	if (mState == NULL)
		return;
	mState->mInFrame = false;
	mState->mStateValid = false;
	mState->mCurTex = NULL;
}

void C3DInterface::SetState(void* theTex, bool linear, int theBlend)
{
	C3DState* s = mState;
	C3D_Tex* aTex = (C3D_Tex*)theTex;
	if (s->mStateValid && s->mCurTex == aTex && (aTex == NULL || s->mCurLinear == linear) && s->mCurBlend == theBlend)
		return;

	FlushBatch();
	if (C3D_GetCmdBufUsage() > 0.8f)
		Restart();

	bool textured = aTex != NULL;
	if (!s->mStateValid || textured != s->mCurTextured)
	{
		C3D_TexEnv* anEnv = C3D_GetTexEnv(0);
		C3D_TexEnvInit(anEnv);
		if (textured)
		{
			C3D_TexEnvSrc(anEnv, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
			C3D_TexEnvFunc(anEnv, C3D_Both, GPU_MODULATE);
		}
		else
		{
			C3D_TexEnvSrc(anEnv, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
			C3D_TexEnvFunc(anEnv, C3D_Both, GPU_REPLACE);
		}
		for (int i = 1; i < 6; i++)
			C3D_TexEnvInit(C3D_GetTexEnv(i));
	}

	if (textured && (!s->mStateValid || aTex != s->mCurTex || linear != s->mCurLinear))
	{
		GPU_TEXTURE_FILTER_PARAM aFilter = linear ? GPU_LINEAR : GPU_NEAREST;
		C3D_TexSetFilter(aTex, aFilter, aFilter);
		C3D_TexBind(0, aTex);
	}

	if (!s->mStateValid || theBlend != s->mCurBlend)
	{
		switch (theBlend)
		{
		case BLEND_ADDITIVE:
			C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE, GPU_ZERO, GPU_ONE);
			break;
		case BLEND_REPLACE:
			C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
			break;
		default:
			C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA);
			break;
		}
	}

	s->mCurTex = aTex;
	s->mCurLinear = linear;
	s->mCurBlend = theBlend;
	s->mCurTextured = textured;
	s->mStateValid = true;
}

C3DVertex* C3DInterface::AllocVertices(int theCount)
{
	C3DState* s = mState;
	if (s->mVertCount + theCount > MAX_VERTICES)
	{
		C3D_Tex* aTex = s->mCurTex;
		bool aLinear = s->mCurLinear;
		int aBlend = s->mCurBlend;
		Restart();
		SetState(aTex, aLinear, aBlend);
	}
	C3DVertex* aVerts = &s->mVertices[s->mVertCount];
	s->mVertCount += theCount;
	mStatVertices += theCount;
	return aVerts;
}

// Draws a convex polygon (fan) with the current state, clipped to theClip (x0, y0, x1, y1) if given.
void C3DInterface::EmitPoly(const ClipVert* theVerts, int theCount, const float* theClip)
{
	if (theCount < 3)
		return;

	ClipVert aBuf[MAX_CLIP_VERTS];
	ClipVert aTemp[MAX_CLIP_VERTS];
	const ClipVert* aVerts = theVerts;

	if (theClip != NULL)
	{
		float aMinX = theVerts[0].a[0], aMaxX = aMinX, aMinY = theVerts[0].a[1], aMaxY = aMinY;
		for (int i = 1; i < theCount; i++)
		{
			aMinX = std::min(aMinX, theVerts[i].a[0]);
			aMaxX = std::max(aMaxX, theVerts[i].a[0]);
			aMinY = std::min(aMinY, theVerts[i].a[1]);
			aMaxY = std::max(aMaxY, theVerts[i].a[1]);
		}
		if (aMaxX <= theClip[0] || aMinX >= theClip[2] || aMaxY <= theClip[1] || aMinY >= theClip[3])
			return;
		if (aMinX < theClip[0] || aMaxX > theClip[2] || aMinY < theClip[1] || aMaxY > theClip[3])
		{
			theCount = std::min(theCount, MAX_CLIP_VERTS);
			memcpy(aBuf, theVerts, theCount * sizeof(ClipVert));
			theCount = ClipToBox(aBuf, aTemp, theCount, 0, theClip[0], theClip[1], theClip[2], theClip[3]);
			if (theCount < 3)
				return;
			aVerts = aBuf;
		}
	}

	int aTris = theCount - 2;
	C3DVertex* aOut = AllocVertices(aTris * 3);
	for (int i = 0; i < aTris; i++)
	{
		const ClipVert* aTri[3] = { &aVerts[0], &aVerts[i + 1], &aVerts[i + 2] };
		for (int j = 0; j < 3; j++)
		{
			const float* a = aTri[j]->a;
			C3DVertex& v = *aOut++;
			v.mX = a[0];
			v.mY = a[1];
			v.mZ = 0.5f;
			v.mU = a[2];
			v.mV = a[3];
			v.mColor = PackColor(a[4], a[5], a[6], a[7]);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Textures

void C3DInterface::FreeGarbage()
{
	C3DState* s = mState;
	AutoCrit anAutoCrit(mDDInterface->mCritSect);
	for (size_t i = 0; i < s->mGarbage.size(); i++)
	{
		C3D_TexDelete(s->mGarbage[i]);
		delete s->mGarbage[i];
	}
	s->mGarbage.clear();
}

void C3DInterface::ReleasePieces(C3DTexture* theTexture)
{
	C3DState* s = mState;
	for (size_t i = 0; i < theTexture->mPieces.size(); i++)
	{
		C3DPiece& aPiece = theTexture->mPieces[i];
		if (aPiece.mTex == s->mCurTex)
		{
			// Pending vertices use it: draw them now, while citro3d's pointer to it is still good.
			if (s->mInFrame)
				FlushBatch();
			s->mStateValid = false;
			s->mCurTex = NULL;
		}
		if (aPiece.mUseFrame != 0 && aPiece.mUseFrame == s->mFrameNum)
			s->mGarbage.push_back(aPiece.mTex);
		else
		{
			C3D_TexDelete(aPiece.mTex);
			delete aPiece.mTex;
		}
	}
	theTexture->mPieces.clear();
	mTexMemSize -= theTexture->mMemSize;
	theTexture->mMemSize = 0;
}

// Writes one piece of theImage into its texture, swizzled.
static void UploadPiece(MemoryImage* theImage, C3D_Tex* theTex, GPU_TEXCOLOR theFormat, const C3DSeg& theCol, const C3DSeg& theRow)
{
	const uint32_t* aBits = theImage->mBits;
	const uint32_t* aTable = theImage->mColorTable;
	const uchar* anIndices = theImage->mColorIndices;
	int anImageWidth = theImage->mWidth;
	int aTexW = theCol.mTexSize;
	int aTexH = theRow.mTexSize;

	uint32_t aRow[MAX_TEX_SIZE];
	for (int py = 0; py < aTexH; py++)
	{
		int y = theRow.mStart + std::min(py, theRow.mSize - 1);
		int aRowOfs = y * anImageWidth + theCol.mStart;
		for (int px = 0; px < aTexW; px++)
		{
			int i = aRowOfs + std::min(px, theCol.mSize - 1);
			aRow[px] = aBits != NULL ? aBits[i] : aTable[anIndices[i]];
		}

		int aMemRow = kFlipUpload ? aTexH - 1 - py : py;
		switch (theFormat)
		{
		case GPU_RGBA8:
		{
			uint32_t* aDest = (uint32_t*)theTex->data;
			for (int px = 0; px < aTexW; px++)
			{
				uint32_t p = aRow[px];
				aDest[TexelOffset(px, aMemRow, aTexW)] = (p << 8) | (p >> 24);
			}
			break;
		}
		case GPU_RGB565:
		{
			uint16_t* aDest = (uint16_t*)theTex->data;
			for (int px = 0; px < aTexW; px++)
			{
				uint32_t p = aRow[px];
				aDest[TexelOffset(px, aMemRow, aTexW)] = (uint16_t)(((p >> 8) & 0xF800) | ((p >> 5) & 0x07E0) | ((p >> 3) & 0x001F));
			}
			break;
		}
		default:	// GPU_RGBA4
		{
			uint16_t* aDest = (uint16_t*)theTex->data;
			for (int px = 0; px < aTexW; px++)
			{
				uint32_t p = aRow[px];
				aDest[TexelOffset(px, aMemRow, aTexW)] = (uint16_t)(((p >> 8) & 0xF000) | ((p >> 4) & 0x0F00) | (p & 0x00F0) | (p >> 28));
			}
			break;
		}
		}
	}
	GSPGPU_FlushDataCache(theTex->data, theTex->size);
}

// The reverse of UploadPiece, into theImage->mBits.
static void DownloadPiece(MemoryImage* theImage, C3D_Tex* theTex, GPU_TEXCOLOR theFormat, const C3DSeg& theCol, const C3DSeg& theRow)
{
	uint32_t* aBits = theImage->mBits;
	int aTexW = theCol.mTexSize;
	int aTexH = theRow.mTexSize;
	for (int py = 0; py < theRow.mSize; py++)
	{
		uint32_t* aDest = &aBits[(theRow.mStart + py) * theImage->mWidth + theCol.mStart];
		int aMemRow = kFlipUpload ? aTexH - 1 - py : py;
		for (int px = 0; px < theCol.mSize; px++)
		{
			int anOfs = TexelOffset(px, aMemRow, aTexW);
			uint32_t p;
			switch (theFormat)
			{
			case GPU_RGBA8:
				p = ((uint32_t*)theTex->data)[anOfs];
				p = (p >> 8) | (p << 24);
				break;
			case GPU_RGB565:
			{
				uint32_t t = ((uint16_t*)theTex->data)[anOfs];
				uint32_t r = (t >> 11) & 0x1F, g = (t >> 5) & 0x3F, b = t & 0x1F;
				p = 0xFF000000 | (((r << 3) | (r >> 2)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((b << 3) | (b >> 2));
				break;
			}
			default:
			{
				uint32_t t = ((uint16_t*)theTex->data)[anOfs];
				p = (((t >> 12) & 0xF) * 17 << 16) | (((t >> 8) & 0xF) * 17 << 8) | (((t >> 4) & 0xF) * 17) | ((t & 0xF) * 17 << 24);
				break;
			}
			}
			aDest[px] = p;
		}
	}
}

C3DTexture* C3DInterface::GetTexture(MemoryImage* theImage)
{
	if (!CreateImageTexture(theImage))
		return NULL;
	return (C3DTexture*)theImage->mD3DData;
}

bool C3DInterface::CreateImageTexture(MemoryImage* theImage)
{
	C3DState* s = mState;
	if (s == NULL || theImage->mWidth <= 0 || theImage->mHeight <= 0)
		return false;

	AutoCrit anAutoCrit(mDDInterface->mCritSect);

	C3DTexture* t = (C3DTexture*)theImage->mD3DData;
	if (t != NULL && t->mBitsChangedCount == theImage->mBitsChangedCount && !t->mPieces.empty())
		return true;

	if (t == NULL)
	{
		t = new C3DTexture;
		t->mWidth = t->mHeight = 0;
		t->mBitsChangedCount = theImage->mBitsChangedCount - 1;
		t->mFormat = GPU_RGBA8;
		t->mFlags = 0;
		t->mMemSize = 0;
		theImage->mD3DData = t;
		mImageSet.insert(theImage);
	}

	// The pixels: mBits, or a palette, or whatever GetBits makes.
	if (theImage->mBits == NULL && (theImage->mColorTable == NULL || theImage->mColorIndices == NULL))
		theImage->GetBits();
	theImage->CommitBits();

	DWORD aFlags = theImage->mD3DFlags;
	GPU_TEXCOLOR aFormat = GPU_RGBA8;
	if (kUse565 && !theImage->mHasAlpha && !theImage->mHasTrans && !(aFlags & D3DImageFlag_UseA8R8G8B8))
		aFormat = GPU_RGB565;
	else if (aFlags & D3DImageFlag_UseA4R4G4B4)
		aFormat = GPU_RGBA4;

	bool aRealloc = t->mPieces.empty() || t->mWidth != theImage->mWidth || t->mHeight != theImage->mHeight ||
		t->mFormat != aFormat || t->mFlags != aFlags;
	for (size_t i = 0; i < t->mPieces.size() && !aRealloc; i++)
		if (t->mPieces[i].mUseFrame != 0 && t->mPieces[i].mUseFrame == s->mFrameNum)
			aRealloc = true;		// this frame's draws still need the old pixels

	if (aRealloc)
	{
		ReleasePieces(t);
		t->mWidth = theImage->mWidth;
		t->mHeight = theImage->mHeight;
		t->mFormat = aFormat;
		t->mFlags = aFlags;
		MakeSegs(t->mWidth, aFlags, t->mCols);
		MakeSegs(t->mHeight, aFlags, t->mRows);
		for (size_t r = 0; r < t->mRows.size(); r++)
		{
			for (size_t c = 0; c < t->mCols.size(); c++)
			{
				C3DPiece aPiece;
				aPiece.mTex = new C3D_Tex;
				aPiece.mUseFrame = 0;
				if (!C3D_TexInit(aPiece.mTex, t->mCols[c].mTexSize, t->mRows[r].mTexSize, aFormat))
				{
					delete aPiece.mTex;
					if (!s->mOutOfMemoryLogged)
						fprintf(stderr, "c3d: out of linear memory for a %dx%d texture (%d KB of textures, %d KB linear free)\n",
							t->mCols[c].mTexSize, t->mRows[r].mTexSize, mTexMemSize / 1024, (int)(linearSpaceFree() / 1024));
					s->mOutOfMemoryLogged = true;
					ReleasePieces(t);
					return false;
				}
				C3D_TexSetWrap(aPiece.mTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
				t->mPieces.push_back(aPiece);
				int aSize = t->mCols[c].mTexSize * t->mRows[r].mTexSize * BytesPerTexel(aFormat);
				t->mMemSize += aSize;
				mTexMemSize += aSize;
			}
		}
	}

	for (size_t r = 0; r < t->mRows.size(); r++)
		for (size_t c = 0; c < t->mCols.size(); c++)
			UploadPiece(theImage, t->mPieces[r * t->mCols.size() + c].mTex, aFormat, t->mCols[c], t->mRows[r]);
	t->mBitsChangedCount = theImage->mBitsChangedCount;
	mStatUploads++;
	mStatUploadBytes += t->mMemSize;

	// The texture is the copy now; the software renderer's caches aren't needed, and images loaded with mPurgeBits
	// drop their bits (RecoverBits brings them back if anything asks).
	theImage->DeleteSWBuffers();
	if (theImage->mPurgeBits)
	{
		theImage->PurgeBits();
		if (theImage->mBits == NULL)
			mPurgedImages++;
	}
	return true;
}

void C3DInterface::RemoveMemoryImage(MemoryImage* theImage)
{
	if (mState == NULL)
		return;
	AutoCrit anAutoCrit(mDDInterface->mCritSect);
	C3DTexture* t = (C3DTexture*)theImage->mD3DData;
	if (t != NULL)
	{
		ReleasePieces(t);
		delete t;
		theImage->mD3DData = NULL;
	}
	mImageSet.erase(theImage);
}

bool C3DInterface::RecoverBits(MemoryImage* theImage)
{
	if (mState == NULL || theImage->mBits == NULL)
		return false;
	AutoCrit anAutoCrit(mDDInterface->mCritSect);
	C3DTexture* t = (C3DTexture*)theImage->mD3DData;
	if (t == NULL || t->mPieces.empty() || t->mBitsChangedCount != theImage->mBitsChangedCount ||
		t->mWidth != theImage->mWidth || t->mHeight != theImage->mHeight)
		return false;
	for (size_t r = 0; r < t->mRows.size(); r++)
		for (size_t c = 0; c < t->mCols.size(); c++)
			DownloadPiece(theImage, t->mPieces[r * t->mCols.size() + c].mTex, t->mFormat, t->mCols[c], t->mRows[r]);
	if (mPurgedImages > 0)
		mPurgedImages--;
	return true;
}

bool C3DInterface::ReadTarget(uint32_t* theBits)
{
	C3DState* s = mState;
	if (s == NULL)
		return false;

	if (s->mReadBuf == NULL)
	{
		s->mReadBuf = (uint32_t*)linearAlloc(TARGET_WIDTH * TARGET_HEIGHT * 4);
		if (s->mReadBuf == NULL)
			return false;
	}

	// Finish what's been drawn: the transfer has to run outside a frame, after it.
	if (s->mInFrame)
	{
		FlushBatch();
		FlushVertexCache();
		C3D_FrameEnd(0);
		SceneEnded();
	}

	C3D_SyncDisplayTransfer((u32*)s->mTargetTex.data, GX_BUFFER_DIM(TARGET_WIDTH, TARGET_HEIGHT), (u32*)s->mReadBuf, GX_BUFFER_DIM(TARGET_WIDTH, TARGET_HEIGHT),
		GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
		GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
	GSPGPU_InvalidateDataCache(s->mReadBuf, TARGET_WIDTH * TARGET_HEIGHT * 4);

	for (int y = 0; y < mHeight; y++)
	{
		int aSrcRow = kFlipReadback ? TARGET_HEIGHT - 1 - y : y;
		const uint32_t* aSrc = &s->mReadBuf[aSrcRow * TARGET_WIDTH];
		uint32_t* aDest = &theBits[y * mWidth];
		for (int x = 0; x < mWidth; x++)
		{
			uint32_t p = aSrc[x];
			aDest[x] = (p >> 8) | (p << 24);
		}
	}
	return true;
}

std::string C3DInterface::TakeStats()
{
	char aBuf[320];
	int aFrames = std::max(mStatFrames, 1);
	sprintf(aBuf, "gpu %d draws %d verts per frame, GPU wait %.2f ms per frame, %d uploads %d KB, textures %d KB in %d images (%d purged), linear free %d KB",
		mStatDraws / aFrames, mStatVertices / aFrames, mStatWaitUs / 1000.0 / aFrames, mStatUploads, mStatUploadBytes / 1024,
		mTexMemSize / 1024, (int)mImageSet.size(), mPurgedImages, (int)(linearSpaceFree() / 1024));
	mStatDraws = mStatVertices = mStatUploads = mStatUploadBytes = 0;
	mStatFrames = 0;
	mStatWaitUs = 0;
	return aBuf;
}

///////////////////////////////////////////////////////////////////////////////
// Transforms

void C3DInterface::PushTransform(const SexyMatrix3& theTransform, bool concatenate)
{
	if (mTransformStack.empty() || !concatenate)
		mTransformStack.push_back(theTransform);
	else
		mTransformStack.push_back(mTransformStack.back() * theTransform);
}

void C3DInterface::PopTransform()
{
	if (!mTransformStack.empty())
		mTransformStack.pop_back();
}

///////////////////////////////////////////////////////////////////////////////
// Drawing

static inline void ClipRectToFloats(const Rect* theClipRect, float* theOut)
{
	theOut[0] = (float)theClipRect->mX;
	theOut[1] = (float)theClipRect->mY;
	theOut[2] = (float)(theClipRect->mX + theClipRect->mWidth);
	theOut[3] = (float)(theClipRect->mY + theClipRect->mHeight);
}

static inline int BlendFor(int theDrawMode)
{
	return theDrawMode == Graphics::DRAWMODE_ADDITIVE ? BLEND_ADDITIVE : BLEND_NORMAL;
}

// Draws theSrcRect of theImage: each source pixel (sx, sy) lands at M * (sx - srcX - center, sy - srcY - center) +
// (theX, theY), or without M just offset.
void C3DInterface::DrawImage(Image* theImage, const Rect& theSrcRect, const SexyMatrix3* theMatrix, float theX, float theY, bool center,
	const Rect* theClipRect, const Color& theColor, int theDrawMode, bool linearFilter)
{
	MemoryImage* anImage = dynamic_cast<MemoryImage*>(theImage);
	if (anImage == NULL || !PreDraw())
		return;
	theImage->mDrawn = true;
	C3DTexture* t = GetTexture(anImage);
	if (t == NULL)
		return;

	Rect aSrc = theSrcRect.Intersection(Rect(0, 0, anImage->mWidth, anImage->mHeight));
	if (aSrc.mWidth <= 0 || aSrc.mHeight <= 0)
		return;

	float aClip[4];
	if (theClipRect != NULL)
		ClipRectToFloats(theClipRect, aClip);
	float aCenterX = center ? theSrcRect.mWidth / 2.0f : 0.0f;
	float aCenterY = center ? theSrcRect.mHeight / 2.0f : 0.0f;
	int aBlend = BlendFor(theDrawMode);
	int aCols = (int)t->mCols.size();

	for (int r = 0; r < (int)t->mRows.size(); r++)
	{
		const C3DSeg& aRow = t->mRows[r];
		int y0 = std::max(aSrc.mY, aRow.mStart);
		int y1 = std::min(aSrc.mY + aSrc.mHeight, aRow.mStart + aRow.mSize);
		if (y0 >= y1)
			continue;
		for (int c = 0; c < aCols; c++)
		{
			const C3DSeg& aCol = t->mCols[c];
			int x0 = std::max(aSrc.mX, aCol.mStart);
			int x1 = std::min(aSrc.mX + aSrc.mWidth, aCol.mStart + aCol.mSize);
			if (x0 >= x1)
				continue;

			C3DPiece& aPiece = t->mPieces[r * aCols + c];
			SetState(aPiece.mTex, linearFilter, aBlend);
			aPiece.mUseFrame = mState->mFrameNum;

			float u0 = (float)(x0 - aCol.mStart) / aCol.mTexSize;
			float u1 = (float)(x1 - aCol.mStart) / aCol.mTexSize;
			float v0 = 1.0f - (float)(y0 - aRow.mStart) / aRow.mTexSize;
			float v1 = 1.0f - (float)(y1 - aRow.mStart) / aRow.mTexSize;
			float lx0 = x0 - theSrcRect.mX - aCenterX, lx1 = x1 - theSrcRect.mX - aCenterX;
			float ly0 = y0 - theSrcRect.mY - aCenterY, ly1 = y1 - theSrcRect.mY - aCenterY;

			ClipVert v[4];
			const float aCorners[4][4] = { { lx0, ly0, u0, v0 }, { lx1, ly0, u1, v0 }, { lx1, ly1, u1, v1 }, { lx0, ly1, u0, v1 } };
			for (int i = 0; i < 4; i++)
			{
				float x = aCorners[i][0], y = aCorners[i][1];
				if (theMatrix != NULL)
					TransformPoint(*theMatrix, x, y);
				v[i].a[0] = x + theX;
				v[i].a[1] = y + theY;
				v[i].a[2] = aCorners[i][2];
				v[i].a[3] = aCorners[i][3];
				SetVertColor(v[i], theColor);
			}
			EmitPoly(v, 4, theClipRect != NULL ? aClip : NULL);
		}
	}
}

void C3DInterface::Blt(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter)
{
	if (!mTransformStack.empty())
	{
		BltClipF(theImage, theX, theY, theSrcRect, NULL, theColor, theDrawMode);
		return;
	}
	DrawImage(theImage, theSrcRect, NULL, theX, theY, false, NULL, theColor, theDrawMode, linearFilter);
}

void C3DInterface::BltClipF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode)
{
	SexyTransform2D aTransform;
	aTransform.Translate(theX, theY);
	BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, true);
}

void C3DInterface::BltMirror(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter)
{
	SexyTransform2D aTransform;
	aTransform.Translate(-(float)theSrcRect.mWidth, 0);
	aTransform.Scale(-1, 1);
	aTransform.Translate(theX, theY);
	BltTransformed(theImage, NULL, theColor, theDrawMode, theSrcRect, aTransform, linearFilter);
}

void C3DInterface::StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror)
{
	if (theSrcRect.mWidth <= 0 || theSrcRect.mHeight <= 0)
		return;
	float aXScale = (float)theDestRect.mWidth / theSrcRect.mWidth;
	float aYScale = (float)theDestRect.mHeight / theSrcRect.mHeight;

	SexyTransform2D aTransform;
	if (mirror)
	{
		aTransform.Translate(-(float)theSrcRect.mWidth, 0);
		aTransform.Scale(-aXScale, aYScale);
	}
	else
		aTransform.Scale(aXScale, aYScale);
	aTransform.Translate((float)theDestRect.mX, (float)theDestRect.mY);
	BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, !fastStretch);
}

void C3DInterface::BltRotated(Image* theImage, float theX, float theY, const Rect* theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY, const Rect& theSrcRect)
{
	SexyTransform2D aTransform;
	aTransform.Translate(-theRotCenterX, -theRotCenterY);
	aTransform.RotateRad((float)theRot);
	aTransform.Translate(theX + theRotCenterX, theY + theRotCenterY);
	BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, true);
}

void C3DInterface::BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, const SexyMatrix3& theTransform, bool linearFilter, float theX, float theY, bool center)
{
	if (mTransformStack.empty())
	{
		DrawImage(theImage, theSrcRect, &theTransform, theX, theY, center, theClipRect, theColor, theDrawMode, linearFilter);
		return;
	}

	// A global transform: always filtered.
	if (theX != 0 || theY != 0)
	{
		SexyTransform2D aTransform;
		if (center)
			aTransform.Translate(-theSrcRect.mWidth / 2.0f, -theSrcRect.mHeight / 2.0f);
		aTransform = theTransform * aTransform;
		aTransform.Translate(theX, theY);
		aTransform = mTransformStack.back() * aTransform;
		DrawImage(theImage, theSrcRect, &aTransform, 0, 0, false, theClipRect, theColor, theDrawMode, true);
	}
	else
	{
		SexyMatrix3 aTransform = mTransformStack.back() * theTransform;
		DrawImage(theImage, theSrcRect, &aTransform, theX, theY, center, theClipRect, theColor, theDrawMode, true);
	}
}

void C3DInterface::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (!PreDraw())
		return;
	SetState(NULL, false, BlendFor(theDrawMode));

	// A one pixel wide quad through the pixel centres, half a pixel past each end.
	float x0 = (float)theStartX + 0.5f, y0 = (float)theStartY + 0.5f;
	float x1 = (float)theEndX + 0.5f, y1 = (float)theEndY + 0.5f;
	float dx = x1 - x0, dy = y1 - y0;
	float aLen = sqrtf(dx * dx + dy * dy);
	if (aLen < 0.0001f)
	{
		dx = 1;
		dy = 0;
	}
	else
	{
		dx /= aLen;
		dy /= aLen;
	}
	dx *= 0.5f;
	dy *= 0.5f;
	const float aPts[4][2] = {
		{ x0 - dx - dy, y0 - dy + dx }, { x1 + dx - dy, y1 + dy + dx },
		{ x1 + dx + dy, y1 + dy - dx }, { x0 - dx + dy, y0 - dy - dx } };

	ClipVert v[4];
	for (int i = 0; i < 4; i++)
	{
		float x = aPts[i][0], y = aPts[i][1];
		if (!mTransformStack.empty())
			TransformPoint(mTransformStack.back(), x, y);
		v[i].a[0] = x;
		v[i].a[1] = y;
		v[i].a[2] = v[i].a[3] = 0;
		SetVertColor(v[i], theColor);
	}
	EmitPoly(v, 4, NULL);
}

void C3DInterface::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode)
{
	if (!PreDraw())
		return;
	SetState(NULL, false, BlendFor(theDrawMode));
	DrawRectQuad(theRect, theColor);
}

void C3DInterface::ClearRect(const Rect& theRect)
{
	if (!PreDraw())
		return;
	SetState(NULL, false, BLEND_REPLACE);
	DrawRectQuad(theRect, Color(0, 0, 0, 0));
}

void C3DInterface::DrawRectQuad(const Rect& theRect, const Color& theColor)
{
	float x0 = (float)theRect.mX, y0 = (float)theRect.mY;
	float x1 = x0 + theRect.mWidth, y1 = y0 + theRect.mHeight;
	const float aPts[4][2] = { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
	ClipVert v[4];
	for (int i = 0; i < 4; i++)
	{
		float x = aPts[i][0], y = aPts[i][1];
		if (!mTransformStack.empty())
			TransformPoint(mTransformStack.back(), x, y);
		v[i].a[0] = x;
		v[i].a[1] = y;
		v[i].a[2] = v[i].a[3] = 0;
		SetVertColor(v[i], theColor);
	}
	EmitPoly(v, 4, NULL);
}

void C3DInterface::FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty)
{
	if (theNumVertices < 3 || !PreDraw())
		return;
	SetState(NULL, false, BlendFor(theDrawMode));

	float aClip[4];
	if (theClipRect != NULL)
		ClipRectToFloats(theClipRect, aClip);

	ClipVert aFirst, aPrev;
	for (int i = 0; i < theNumVertices; i++)
	{
		ClipVert v;
		float x = (float)(theVertices[i].mX + tx), y = (float)(theVertices[i].mY + ty);
		if (!mTransformStack.empty())
			TransformPoint(mTransformStack.back(), x, y);
		v.a[0] = x;
		v.a[1] = y;
		v.a[2] = v.a[3] = 0;
		SetVertColor(v, theColor);
		if (i == 0)
			aFirst = v;
		else if (i >= 2)
		{
			ClipVert aTri[3] = { aFirst, aPrev, v };
			EmitPoly(aTri, 3, theClipRect != NULL ? aClip : NULL);
		}
		aPrev = v;
	}
}

void C3DInterface::DrawTrianglesTex(const TriVertex theVertices[][3], int theNumTriangles, const Rect* theClipRect, const Color& theColor, int theDrawMode, Image* theTexture, float tx, float ty, bool blend)
{
	MemoryImage* anImage = dynamic_cast<MemoryImage*>(theTexture);
	if (anImage == NULL || !PreDraw())
		return;
	theTexture->mDrawn = true;
	C3DTexture* t = GetTexture(anImage);
	if (t == NULL)
		return;

	float aClip[4];
	if (theClipRect != NULL)
		ClipRectToFloats(theClipRect, aClip);
	int aBlend = BlendFor(theDrawMode);
	int aCols = (int)t->mCols.size();
	bool aOnePiece = t->mPieces.size() == 1;

	for (int i = 0; i < theNumTriangles; i++)
	{
		// The triangle with texture coordinates in image pixels.
		ClipVert aTri[3];
		for (int j = 0; j < 3; j++)
		{
			const TriVertex& tv = theVertices[i][j];
			float x = tv.x + tx, y = tv.y + ty;
			if (!mTransformStack.empty())
				TransformPoint(mTransformStack.back(), x, y);
			aTri[j].a[0] = x;
			aTri[j].a[1] = y;
			aTri[j].a[2] = tv.u * anImage->mWidth;
			aTri[j].a[3] = tv.v * anImage->mHeight;
			if (tv.color == 0)
				SetVertColor(aTri[j], theColor);
			else
				SetVertColorARGB(aTri[j], tv.color);
		}

		for (int r = 0; r < (int)t->mRows.size(); r++)
		{
			const C3DSeg& aRow = t->mRows[r];
			for (int c = 0; c < aCols; c++)
			{
				const C3DSeg& aCol = t->mCols[c];
				ClipVert aBuf[MAX_CLIP_VERTS];
				ClipVert aTemp[MAX_CLIP_VERTS];
				memcpy(aBuf, aTri, sizeof(aTri));
				int aCount = 3;
				if (!aOnePiece)
				{
					aCount = ClipToBox(aBuf, aTemp, aCount, 2, (float)aCol.mStart, (float)aRow.mStart,
						(float)(aCol.mStart + aCol.mSize), (float)(aRow.mStart + aRow.mSize));
					if (aCount < 3)
						continue;
				}
				for (int k = 0; k < aCount; k++)
				{
					aBuf[k].a[2] = (aBuf[k].a[2] - aCol.mStart) / aCol.mTexSize;
					aBuf[k].a[3] = 1.0f - (aBuf[k].a[3] - aRow.mStart) / aRow.mTexSize;
				}
				C3DPiece& aPiece = t->mPieces[r * aCols + c];
				SetState(aPiece.mTex, blend, aBlend);
				aPiece.mUseFrame = mState->mFrameNum;
				EmitPoly(aBuf, aCount, theClipRect != NULL ? aClip : NULL);
			}
		}
	}
}
