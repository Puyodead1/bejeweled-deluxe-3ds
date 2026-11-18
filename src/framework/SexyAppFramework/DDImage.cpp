#include "DDImage.h"
#include "DDInterface.h"
#include "SexyAppBase.h"

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

void DDImage::PurgeBits()
{
	// On Windows this freed the 32-bit bits once a surface or the native alpha data held the pixels. Memory isn't
	// tight here and keeping the bits avoids reconverting them on every GetBits(), so only the flag is kept.
	mPurgeBits = true;
}
