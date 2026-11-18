#ifndef __SDLSOUNDMANAGER_H__
#define __SDLSOUNDMANAGER_H__

// Switch port: the SoundManager on one SDL audio device, standing in for DSoundManager/DSoundInstance. Sounds are
// decoded at load time (Ogg Vorbis through Tremor, or WAV) into 16-bit stereo at the device rate and mixed in the SDL
// audio callback. Volume and pan follow DirectSound's rules (volume through DSoundManager::VolumeToDB, pan in
// hundredths of a dB) so the game's levels sound as they did. The music interface mixes into the same device through
// SetMusicMixer.

#include "SoundManager.h"
#include "SoundInstance.h"

#include <stdint.h>
#include <stdio.h>
#include <vector>

namespace Sexy
{

class SDLSoundManager;

struct SDLSoundSample
{
	std::vector<int16_t>	mData;			// interleaved stereo at the device rate
	int						mFrames;
};

class SDLSoundInstance : public SoundInstance
{
	friend class SDLSoundManager;

protected:
	SDLSoundManager*		mManager;
	SDLSoundSample*			mSample;
	double					mBaseVolume;
	double					mVolume;
	int						mBasePan;
	int						mPan;
	bool					mHasPlayed;
	bool					mAutoRelease;
	bool					mReleased;

	// Read by the audio callback; written with the device locked.
	bool					mPlaying;
	bool					mLooping;
	uint64_t				mPos;			// in frames, 16.16 fixed point
	uint32_t				mStep;			// 16.16; 1.0 unless AdjustPitch was used
	int						mGainL;			// 2.14 fixed point
	int						mGainR;

	void					RehupGains();

public:
	SDLSoundInstance(SDLSoundManager* theManager, SDLSoundSample* theSample);
	virtual ~SDLSoundInstance();

	virtual void			Release();

	virtual void			SetBaseVolume(double theBaseVolume);
	virtual void			SetBasePan(int theBasePan);

	virtual void			AdjustPitch(double theNumSteps);

	virtual void			SetVolume(double theVolume);
	virtual void			SetPan(int thePosition);

	virtual bool			Play(bool looping, bool autoRelease);
	virtual void			Stop();
	virtual bool			IsPlaying();
	virtual bool			IsReleased();
	virtual double			GetVolume();
};

class SDLSoundManager : public SoundManager
{
	friend class SDLSoundInstance;

public:
	// Mixes theFrames stereo frames into theBuffer (interleaved, 16-bit range, summed). Called on the audio thread.
	typedef void			(*MusicMixFunc)(void* theUserData, int32_t* theBuffer, int theFrames);

protected:
	uint32_t				mDevice;
	int						mFreq;
	double					mMasterVolume;

	SDLSoundSample*			mSamples[MAX_SOURCE_SOUNDS];
	double					mBaseVolumes[MAX_SOURCE_SOUNDS];
	int						mBasePans[MAX_SOURCE_SOUNDS];
	SDLSoundInstance*		mChannels[MAX_CHANNELS];

	MusicMixFunc			mMusicMix;
	void*					mMusicUserData;
	std::vector<int32_t>	mMixBuffer;

	FILE*					mCaptureFile;
	int						mCaptureFramesLeft;

	static void				AudioCallback(void* theUserData, uint8_t* theStream, int theLen);
	void					Mix(int16_t* theOut, int theFrames);

	bool					DecodeFile(const std::string& theFilename, SDLSoundSample* theSample);
	void					ReleaseFreeChannels();

public:
	SDLSoundManager();
	virtual ~SDLSoundManager();

	// The device is locked while the callback runs; hold it to change anything the callback reads.
	void					Lock();
	void					Unlock();
	int						GetFrequency() { return mFreq; }
	void					SetMusicMixer(MusicMixFunc theFunc, void* theUserData);
	// For testing: writes the next theMilliseconds of output to theFileName as raw 16-bit stereo at GetFrequency().
	void					StartCapture(const std::string& theFileName, int theMilliseconds);

	// Linear 0..1 volume to DirectSound hundredths of a dB, as DSoundManager::VolumeToDB did, then to a gain.
	static int				VolumeToDB(double theVolume);
	static double			DBToGain(int theDB);

	virtual bool			Initialized();

	virtual bool			LoadSound(unsigned int theSfxID, const std::string& theFilename);
	virtual int				LoadSound(const std::string& theFilename);
	virtual void			ReleaseSound(unsigned int theSfxID);

	virtual void			SetVolume(double theVolume);
	virtual bool			SetBaseVolume(unsigned int theSfxID, double theBaseVolume);
	virtual bool			SetBasePan(unsigned int theSfxID, int theBasePan);

	virtual SoundInstance*	GetSoundInstance(unsigned int theSfxID);

	virtual void			ReleaseSounds();
	virtual void			ReleaseChannels();

	virtual double			GetMasterVolume();
	virtual void			SetMasterVolume(double theVolume);

	virtual void			Flush();
	virtual void			SetCooperativeWindow(HWND theHWnd, bool isWindowed);
	virtual void			StopAllSounds();
	virtual int				GetFreeSoundId();
	virtual int				GetNumSounds();
};

}

#endif
