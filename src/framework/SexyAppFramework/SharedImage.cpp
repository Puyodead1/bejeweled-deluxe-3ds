#include "SharedImage.h"
#include "DDImage.h"
#include "SexyAppBase.h"

using namespace Sexy;

// FUNCTION: GAME187 0x437c20
SharedImage::SharedImage()
{
	mImage = NULL;
	mRefCount = 0;
}

// FUNCTION: GAME187 0x437c30
SharedImageRef::SharedImageRef(const SharedImageRef& theSharedImageRef)
{
	mSharedImage = theSharedImageRef.mSharedImage;
	if (mSharedImage != NULL)
		mSharedImage->mRefCount++;
	mUnsharedImage = theSharedImageRef.mUnsharedImage;	
	mOwnsUnshared = false;
}

SharedImageRef::SharedImageRef()
{
	mSharedImage = NULL;
	mUnsharedImage = NULL;
	mOwnsUnshared = false;
}

// FUNCTION: GAME187 0x437c50
SharedImageRef::SharedImageRef(SharedImage* theSharedImage)
{
	mSharedImage = theSharedImage;
	if (theSharedImage != NULL)
		mSharedImage->mRefCount++;

	mUnsharedImage = NULL;
	mOwnsUnshared = false;
}

// FUNCTION: GAME187 0x437c60
SharedImageRef::~SharedImageRef()
{
	Release();
}

// FUNCTION: GAME187 0x437c70
void SharedImageRef::Release()
{	
	if (mOwnsUnshared)
		delete mUnsharedImage;
	mUnsharedImage = NULL;
	if (mSharedImage != NULL)
	{
		if (--mSharedImage->mRefCount == 0)
			gSexyAppBase->mCleanupSharedImages = true;
	}
	mSharedImage = NULL;
}

// FUNCTION: GAME187 0x437cc0
SharedImageRef& SharedImageRef::operator=(const SharedImageRef& theSharedImageRef)
{
	Release();
	mSharedImage = theSharedImageRef.mSharedImage;
	if (mSharedImage != NULL)
		mSharedImage->mRefCount++;
	return *this;
}

// FUNCTION: GAME187 0x437ce0
SharedImageRef&	SharedImageRef::operator=(SharedImage* theSharedImage)
{
	Release();
	mSharedImage = theSharedImage;
	mSharedImage->mRefCount++;
	return *this;
}

// FUNCTION: GAME187 0x437d00
SharedImageRef& SharedImageRef::operator=(MemoryImage* theUnsharedImage)
{
	Release();
	mUnsharedImage = theUnsharedImage;	
	return *this;
}

MemoryImage* SharedImageRef::operator->()
{
	return (MemoryImage*) *this;
}


// FUNCTION: GAME187 0x437d20
SharedImageRef::operator Image*()
{	
	return (MemoryImage*) *this;
}

// FUNCTION: GAME187 0x437d30
SharedImageRef::operator MemoryImage*()
{
	if (mUnsharedImage != NULL)
		return mUnsharedImage;
	else
		return (DDImage*) *this;
}

// FUNCTION: GAME187 0x437d40
SharedImageRef::operator DDImage*()
{
	if (mSharedImage != NULL)
		return mSharedImage->mImage;
	else
		return NULL;
}
