#ifndef __DDIMAGE_H__
#define __DDIMAGE_H__

#include "MemoryImage.h"

namespace Sexy
{

class DDInterface;
class SysFont;

// Texture hints from D3DInterface.h; ResourceManager still sets them, nothing reads them.
enum D3DImageFlags
{
	D3DImageFlag_MinimizeNumSubdivisions = 0x0001,
	D3DImageFlag_Use64By64Subdivisions = 0x0002,
	D3DImageFlag_UseA4R4G4B4 = 0x0004,
	D3DImageFlag_UseA8R8G8B8 = 0x0008
};

// Switch port: DDImage no longer has a DirectDraw surface. It is a MemoryImage that registers with the DDInterface;
// all drawing goes through MemoryImage's software blitters. The surface-related members stay so the framework and
// game code that sets them compiles unchanged; mSurface is always NULL.
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

	static bool				Check3D(DDImage *theImage) { return false; }
	static bool				Check3D(Image *theImage) { return false; }

public:
	DDImage();
	DDImage(DDInterface* theDDInterface);
	virtual ~DDImage();

	virtual bool			LockSurface() { return false; }
	virtual bool			UnlockSurface() { return false; }

	virtual void			PurgeBits();
};

}

#endif //__DDIMAGE_H__
