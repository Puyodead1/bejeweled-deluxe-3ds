#include "DDInterface.h"
#include "DDImage.h"
#include "C3DInterface.h"
#include "SexyAppBase.h"
#include "AutoCrit.h"

#include <unistd.h>

#include <3ds.h>
#include <citro3d.h>
#include "sprite_shader_shbin.h"

using namespace Sexy;

// The frame texture is 1024x512 (textures are powers of two); the game's 640x480 sits in its first 640 columns and
// 480 rows. There are two, used in turn: while the GPU may still be showing one, the next frame goes into the other.
static const int TEX_WIDTH = 1024;
static const int TEX_HEIGHT = 512;

namespace Sexy
{

struct CtrVertex
{
	float					mX, mY, mZ;
	float					mU, mV;
	uint32_t				mColor;
};

struct CtrVideo
{
	C3D_RenderTarget*		mTop;
	C3D_RenderTarget*		mBottom;
	C3D_Tex					mTex[2];
	bool					mTexValid[2];
	int						mNextTex;
	uint32_t*				mTiled;			// the frame after tiling, before it goes into the texture
	DVLB_s*					mShader;
	shaderProgram_s			mProgram;
	int						mProjectionLoc;
	CtrVertex*				mVertices;
	C3D_Mtx					mTopProj;
	C3D_Mtx					mBottomProj;
};

}

static bool gC3DUp = false;

// citro3d's command buffer. The GPU renderer issues a draw call per texture/blend change, and a frame with a lot of
// text or gems changes often; C3DInterface splits the frame if it fills up anyway.
static const int CMDBUF_SIZE = 0x80000;

// Its presence selects the software renderer at startup (the GPU one is the default).
static const char* SOFTWARE_RENDERER_FLAG = "sdmc:/3ds/Bejeweled/software-renderer";

DDInterface::DDInterface(SexyAppBase* theApp)
{
	mApp = theApp;
	mIs3D = access(SOFTWARE_RENDERER_FLAG, F_OK) != 0;
	mC3DInterface = NULL;
	mShuttingDown = false;
	mInRedraw = false;
	mWidth = theApp->mWidth;
	mHeight = theApp->mHeight;
	mAspect.Set(mWidth, mHeight);
	mDisplayWidth = 400;
	mDisplayHeight = 240;
	mRefreshRate = 60;
	mMillisecondsPerFrame = 1000 / mRefreshRate;

	mInitialized = false;
	mHWnd = NULL;
	mIsWindowed = false;
	mScreenImage = NULL;
	mVideoOnlyDraw = false;
	mInitCount = 0;

	mMagnifier = false;
	mFocusX = mWidth / 2;
	mFocusY = mHeight / 2;
	mViewDirty = false;

	mVideo = NULL;
	mLinearBits = NULL;

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
	mShuttingDown = true;
	Cleanup();
	if (gC3DUp)
	{
		C3D_Fini();
		gC3DUp = false;
	}
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

	// The GPU renderer first: switching renderers, images whose bits were purged get them back from their textures.
	if (mC3DInterface != NULL)
	{
		mC3DInterface->Cleanup(!mShuttingDown);
		delete mC3DInterface;
		mC3DInterface = NULL;
	}

	if (mScreenImage != NULL && mLinearBits != NULL && mScreenImage->mBits == mLinearBits)
		mScreenImage->mBits = NULL;		// not MemoryImage's to delete[]
	delete mScreenImage;
	mScreenImage = NULL;
	if (mLinearBits != NULL)
		linearFree(mLinearBits);
	mLinearBits = NULL;

	if (mVideo != NULL)
	{
		CtrVideo* v = mVideo;
		if (gC3DUp)
			C3D_FrameSync();
		for (int i = 0; i < 2; i++)
			if (v->mTexValid[i])
				C3D_TexDelete(&v->mTex[i]);
		if (v->mTop != NULL)
			C3D_RenderTargetDelete(v->mTop);
		if (v->mBottom != NULL)
			C3D_RenderTargetDelete(v->mBottom);
		if (v->mTiled != NULL)
			linearFree(v->mTiled);
		if (v->mVertices != NULL)
			linearFree(v->mVertices);
		if (v->mShader != NULL)
		{
			shaderProgramFree(&v->mProgram);
			DVLB_Free(v->mShader);
		}
		delete v;
		mVideo = NULL;
	}
}

int DDInterface::Init(HWND theWindow, bool IsWindowed)
{
	AutoCrit anAutoCrit(mCritSect);

	Cleanup();

	mHWnd = theWindow;
	mIsWindowed = IsWindowed;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mAspect.Set(mWidth, mHeight);
	mErrorString.clear();

	if ((mWidth % 8) != 0 || (mHeight % 8) != 0 || mWidth > TEX_WIDTH || mHeight > TEX_HEIGHT)
	{
		mErrorString = "the screen must be a multiple of 8 pixels and fit 1024x512";
		return RESULT_FAIL;
	}

	if (!gC3DUp)
	{
		if (!C3D_Init(CMDBUF_SIZE))
		{
			mErrorString = "C3D_Init failed";
			return RESULT_DD_CREATE_FAIL;
		}
		gC3DUp = true;
	}

	CtrVideo* v = new CtrVideo;
	memset(v, 0, sizeof(*v));
	mVideo = v;

	// The screens. Their framebuffers are portrait (240 wide); Mtx_OrthoTilt turns that round.
	const u32 aDisplayFlags = GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
		GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);
	v->mTop = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, -1);
	v->mBottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, -1);
	if (v->mTop == NULL || v->mBottom == NULL)
	{
		mErrorString = "can't create the screen render targets";
		return RESULT_SURFACE_FAIL;
	}
	C3D_RenderTargetSetOutput(v->mTop, GFX_TOP, GFX_LEFT, aDisplayFlags);
	C3D_RenderTargetSetOutput(v->mBottom, GFX_BOTTOM, GFX_LEFT, aDisplayFlags);
	Mtx_OrthoTilt(&v->mTopProj, 0.0f, 400.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
	Mtx_OrthoTilt(&v->mBottomProj, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);

	// The sprite shader (platform/3ds/shaders/sprite.v.pica): position through the projection, texcoord and colour
	// passed on. The GPU renderer uses it too.
	v->mShader = DVLB_ParseFile((u32*)sprite_shader_shbin, sprite_shader_shbin_size);
	if (v->mShader == NULL)
	{
		mErrorString = "can't load the shader";
		return RESULT_3D_FAIL;
	}
	shaderProgramInit(&v->mProgram);
	shaderProgramSetVsh(&v->mProgram, &v->mShader->DVLE[0]);
	v->mProjectionLoc = shaderInstanceGetUniformLocation(v->mProgram.vertexShader, "projection");

	v->mVertices = (CtrVertex*)linearAlloc(16 * sizeof(CtrVertex));
	if (v->mVertices == NULL)
	{
		mErrorString = "out of linear memory";
		return RESULT_SURFACE_FAIL;
	}

	mPresentationRect = Rect(0, 0, mWidth, mHeight);

	if (mIs3D)
	{
		mC3DInterface = new C3DInterface(this);
		std::string anError;
		if (!mC3DInterface->Init(mWidth, mHeight, &v->mProgram, v->mProjectionLoc, anError))
		{
			fprintf(stderr, "video: GPU renderer failed (%s); using the software renderer\n", anError.c_str());
			mC3DInterface->Cleanup(false);
			delete mC3DInterface;
			mC3DInterface = NULL;
			mIs3D = false;
		}
	}

	if (mIs3D)
	{
		// The pixels are in the render target; the screen image has no bits of its own (DDImage::GetBits reads the
		// target back if anything asks).
		mScreenImage = new DDImage(this);
		mScreenImage->mWidth = mWidth;
		mScreenImage->mHeight = mHeight;
		fprintf(stderr, "video: %dx%d frame, GPU renderer\n", mWidth, mHeight);
	}
	else
	{
		// The frame textures: VRAM if there's room (faster for the GPU to read), else linear memory.
		for (int i = 0; i < 2; i++)
		{
			v->mTexValid[i] = C3D_TexInitVRAM(&v->mTex[i], TEX_WIDTH, TEX_HEIGHT, GPU_RGBA8) ||
				C3D_TexInit(&v->mTex[i], TEX_WIDTH, TEX_HEIGHT, GPU_RGBA8);
			if (!v->mTexValid[i])
			{
				mErrorString = "can't allocate the frame textures";
				return RESULT_SURFACE_FAIL;
			}
			C3D_TexSetFilter(&v->mTex[i], GPU_LINEAR, GPU_LINEAR);
			C3D_TexSetWrap(&v->mTex[i], GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
		}

		v->mTiled = (uint32_t*)linearAlloc(mWidth * mHeight * 4);
		mLinearBits = (uint32_t*)linearAlloc((mWidth * mHeight + 1) * 4);
		if (v->mTiled == NULL || mLinearBits == NULL)
		{
			mErrorString = "out of linear memory";
			return RESULT_SURFACE_FAIL;
		}

		mScreenImage = new DDImage(this);
		mScreenImage->Create(mWidth, mHeight);
		// Draw straight into linear memory, so presenting needs no copy by the CPU.
		delete [] mScreenImage->mBits;
		mScreenImage->mBits = mLinearBits;
		mScreenImage->Clear();

		fprintf(stderr, "video: %dx%d frame, software renderer, textures in %s\n", mWidth, mHeight,
			(osConvertVirtToPhys(v->mTex[0].data) >= OS_VRAM_PADDR && osConvertVirtToPhys(v->mTex[0].data) < OS_VRAM_PADDR + OS_VRAM_SIZE) ? "VRAM" : "linear memory");
	}

	mInitCount++;
	mInitialized = true;
	mViewDirty = true;
	return RESULT_OK;
}

void DDInterface::UpdatePresentationRect()
{
	mPresentationRect = Rect(0, 0, mWidth, mHeight);
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
	if (mC3DInterface != NULL)
		mC3DInterface->RemoveMemoryImage(theImage);
}

bool DDInterface::RecoverBits(MemoryImage* theImage)
{
	return mC3DInterface != NULL && mC3DInterface->RecoverBits(theImage);
}

void DDInterface::SetVideoOnlyDraw(bool videoOnly)
{
	mVideoOnlyDraw = videoOnly;
}

void DDInterface::RemapMouse(int& theX, int& theY)
{
	// Touch positions arrive in game pixels already.
}

void DDInterface::SetMagnifier(bool on)
{
	if (on != mMagnifier)
	{
		mMagnifier = on;
		mViewDirty = true;
	}
}

void DDInterface::SetFocusPoint(int theX, int theY)
{
	if (theX != mFocusX || theY != mFocusY)
	{
		mFocusX = theX;
		mFocusY = theY;
		if (mMagnifier)
			mViewDirty = true;
	}
}

void DDInterface::Restored()
{
	mViewDirty = true;
	if (mC3DInterface != NULL)
		mC3DInterface->InvalidateTarget();
}

// The texture combiner. The frame's pixels are 0xAARRGGBB words; the GPU reads an RGBA8 texel as 0xRRGGBBAA, so it
// sees R = alpha, G = red, B = green, A = blue. Three stages put red, green and blue back where they belong:
//   (G, 0, 0), + (0, B, 0), + (0, 0, A); alpha is 1.
static void SetFrameTexEnv()
{
	C3D_TexEnv* anEnv = C3D_GetTexEnv(0);
	C3D_TexEnvInit(anEnv);
	C3D_TexEnvSrc(anEnv, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, GPU_PRIMARY_COLOR);
	C3D_TexEnvOpRgb(anEnv, GPU_TEVOP_RGB_SRC_G, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
	C3D_TexEnvFunc(anEnv, C3D_RGB, GPU_MODULATE);
	C3D_TexEnvSrc(anEnv, C3D_Alpha, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
	C3D_TexEnvFunc(anEnv, C3D_Alpha, GPU_REPLACE);
	C3D_TexEnvColor(anEnv, 0xFF0000FF);		// 0xAABBGGRR: red

	anEnv = C3D_GetTexEnv(1);
	C3D_TexEnvInit(anEnv);
	C3D_TexEnvSrc(anEnv, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, GPU_PREVIOUS);
	C3D_TexEnvOpRgb(anEnv, GPU_TEVOP_RGB_SRC_B, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
	C3D_TexEnvFunc(anEnv, C3D_RGB, GPU_MULTIPLY_ADD);
	C3D_TexEnvColor(anEnv, 0xFF00FF00);		// green

	anEnv = C3D_GetTexEnv(2);
	C3D_TexEnvInit(anEnv);
	C3D_TexEnvSrc(anEnv, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, GPU_PREVIOUS);
	C3D_TexEnvOpRgb(anEnv, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
	C3D_TexEnvFunc(anEnv, C3D_RGB, GPU_MULTIPLY_ADD);
	C3D_TexEnvColor(anEnv, 0xFFFF0000);		// blue

	for (int i = 3; i < 6; i++)
		C3D_TexEnvInit(C3D_GetTexEnv(i));
}

// A screen-space rectangle showing the game rectangle (theSrcX, theSrcY, theSrcW, theSrcH).
static void AddQuad(CtrVertex* theVerts, float x, float y, float w, float h, float theSrcX, float theSrcY, float theSrcW, float theSrcH)
{
	// Tiling put game row 0 in the texture's first memory row, which the GPU samples as t = 1 (the top).
	float u0 = theSrcX / TEX_WIDTH, u1 = (theSrcX + theSrcW) / TEX_WIDTH;
	float v0 = 1.0f - theSrcY / TEX_HEIGHT, v1 = 1.0f - (theSrcY + theSrcH) / TEX_HEIGHT;
	const float aCorner[4][4] = { { x, y, u0, v0 }, { x + w, y, u1, v0 }, { x, y + h, u0, v1 }, { x + w, y + h, u1, v1 } };
	static const int kOrder[6] = { 0, 2, 1, 1, 2, 3 };
	for (int i = 0; i < 6; i++)
	{
		const float* c = aCorner[kOrder[i]];
		theVerts[i] = { c[0], c[1], 0.5f, c[2], c[3], 0xFFFFFFFF };
	}
}

bool DDInterface::Redraw(Rect* theClipRect)
{
	AutoCrit anAutoCrit(mCritSect);

	if (!mInitialized || mScreenImage == NULL || mVideo == NULL)
		return false;

	mInRedraw = true;
	CtrVideo* v = mVideo;

	C3D_Tex* aTex;
	if (mC3DInterface != NULL)
	{
		// The GPU drew the frame into its render target; this frame (begun by the first draw, or here if nothing was
		// drawn) goes on to show it.
		aTex = (C3D_Tex*)mC3DInterface->FinishScene();
		if (aTex == NULL)
		{
			mInRedraw = false;
			return false;
		}
	}
	else
	{
		uint32_t* aBits = mScreenImage->GetBits();
		if (aBits != mLinearBits)
		{
			// Something gave the screen image new pixels; copy them where the DMA engine can see them.
			static bool aLogged = false;
			if (!aLogged)
				fprintf(stderr, "video: screen image bits moved out of linear memory; copying each frame\n");
			aLogged = true;
			memcpy(mLinearBits, aBits, mWidth * mHeight * 4);
		}

		// 1. The frame into the next texture: the display transfer engine tiles it into mTiled, then a texture copy
		// puts each 8-row strip of tiles into the wider texture (640 of its 1024 columns).
		int aTexIdx = v->mNextTex;
		v->mNextTex ^= 1;
		aTex = &v->mTex[aTexIdx];
		GSPGPU_FlushDataCache(mLinearBits, mWidth * mHeight * 4);
		C3D_SyncDisplayTransfer((u32*)mLinearBits, GX_BUFFER_DIM(mWidth, mHeight), (u32*)v->mTiled, GX_BUFFER_DIM(mWidth, mHeight),
			GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
			GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
			GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
		u32 aStripBytes = mWidth * 8 * 4;
		u32 aGapBytes = (TEX_WIDTH - mWidth) * 8 * 4;
		C3D_SyncTextureCopy((u32*)v->mTiled, GX_BUFFER_DIM(aStripBytes >> 4, 0), (u32*)aTex->data,
			GX_BUFFER_DIM(aStripBytes >> 4, aGapBytes >> 4), mWidth * mHeight * 4, GX_TRANSFER_RAW_COPY(1));

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	}

	// 2. Both screens.
	C3D_BindProgram(&v->mProgram);
	C3D_AttrInfo* anAttrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(anAttrInfo);
	AttrInfo_AddLoader(anAttrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(anAttrInfo, 1, GPU_FLOAT, 2);
	AttrInfo_AddLoader(anAttrInfo, 2, GPU_UNSIGNED_BYTE, 4);
	C3D_BufInfo* aBufInfo = C3D_GetBufInfo();
	BufInfo_Init(aBufInfo);
	BufInfo_Add(aBufInfo, v->mVertices, sizeof(CtrVertex), 3, 0x210);
	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
	C3D_CullFace(GPU_CULL_NONE);
	C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
	C3D_TexBind(0, aTex);
	if (mC3DInterface != NULL)
	{
		// The render target is a native RGBA8 texture: use it as it is.
		C3D_TexEnv* anEnv = C3D_GetTexEnv(0);
		C3D_TexEnvInit(anEnv);
		C3D_TexEnvSrc(anEnv, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
		C3D_TexEnvFunc(anEnv, C3D_RGB, GPU_REPLACE);
		C3D_TexEnvSrc(anEnv, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
		C3D_TexEnvFunc(anEnv, C3D_Alpha, GPU_REPLACE);
		for (int i = 1; i < 6; i++)
			C3D_TexEnvInit(C3D_GetTexEnv(i));
	}
	else
		SetFrameTexEnv();
	// Top: the whole picture at half size in the middle, or a full-size window around the last touch.
	if (mMagnifier)
	{
		int x0 = std::max(0, std::min(mFocusX - 200, mWidth - 400));
		int y0 = std::max(0, std::min(mFocusY - 120, mHeight - 240));
		AddQuad(&v->mVertices[0], 0, 0, 400, 240, (float)x0, (float)y0, 400, 240);
	}
	else
		AddQuad(&v->mVertices[0], 40, 0, 320, 240, 0, 0, (float)mWidth, (float)mHeight);
	// Bottom: the whole picture at half size.
	AddQuad(&v->mVertices[6], 0, 0, 320, 240, 0, 0, (float)mWidth, (float)mHeight);
	GSPGPU_FlushDataCache(v->mVertices, 12 * sizeof(CtrVertex));

	C3D_RenderTargetClear(v->mTop, C3D_CLEAR_COLOR, 0x000000FF, 0);
	C3D_FrameDrawOn(v->mTop);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, v->mProjectionLoc, &v->mTopProj);
	C3D_DrawArrays(GPU_TRIANGLES, 0, 6);

	C3D_RenderTargetClear(v->mBottom, C3D_CLEAR_COLOR, 0x000000FF, 0);
	C3D_FrameDrawOn(v->mBottom);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, v->mProjectionLoc, &v->mBottomProj);
	C3D_DrawArrays(GPU_TRIANGLES, 6, 6);

	C3D_FrameEnd(0);
	if (mC3DInterface != NULL)
		mC3DInterface->SceneEnded();

	mViewDirty = false;
	mInRedraw = false;
	return true;
}
