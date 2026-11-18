#ifndef __NULLSOUNDMANAGER_H__
#define __NULLSOUNDMANAGER_H__

// Silent SoundManager used until the SDL mixer one lands: loads succeed, nothing plays. GetSoundInstance returns
// NULL, which every framework caller checks.

#include "SoundManager.h"

namespace Sexy
{

class NullSoundManager : public SoundManager
{
public:
	double					mMasterVolume;

	NullSoundManager() : mMasterVolume(1.0) {}

	virtual bool			Initialized() { return true; }

	virtual bool			LoadSound(unsigned int theSfxID, const std::string& theFilename) { return true; }
	virtual int				LoadSound(const std::string& theFilename) { return 0; }
	virtual void			ReleaseSound(unsigned int theSfxID) {}

	virtual void			SetVolume(double theVolume) {}
	virtual bool			SetBaseVolume(unsigned int theSfxID, double theBaseVolume) { return true; }
	virtual bool			SetBasePan(unsigned int theSfxID, int theBasePan) { return true; }

	virtual SoundInstance*	GetSoundInstance(unsigned int theSfxID) { return NULL; }

	virtual void			ReleaseSounds() {}
	virtual void			ReleaseChannels() {}

	virtual double			GetMasterVolume() { return mMasterVolume; }
	virtual void			SetMasterVolume(double theVolume) { mMasterVolume = theVolume; }

	virtual void			Flush() {}
	virtual void			SetCooperativeWindow(HWND theHWnd, bool isWindowed) {}
	virtual void			StopAllSounds() {}
	virtual int				GetFreeSoundId() { return 0; }
	virtual int				GetNumSounds() { return 0; }
};

}

#endif
