#include <SDL.h>
#include <tremor/ivorbisfile.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "SDLSoundManager.h"
#include "../PakLib/PakInterface.h"

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------------
// SDLSoundInstance

SDLSoundInstance::SDLSoundInstance(SDLSoundManager* theManager, SDLSoundSample* theSample)
{
	mManager = theManager;
	mSample = theSample;
	mBaseVolume = 1.0;
	mVolume = 1.0;
	mBasePan = 0;
	mPan = 0;
	mHasPlayed = false;
	mAutoRelease = false;
	mReleased = false;

	mPlaying = false;
	mLooping = false;
	mPos = 0;
	mStep = 0x10000;
	mGainL = mGainR = 0;
	RehupGains();
}

SDLSoundInstance::~SDLSoundInstance()
{
}

void SDLSoundInstance::RehupGains()
{
	// As DSoundInstance::RehupVolume/RehupPan: one volume in hundredths of a dB, and the pan attenuating the other
	// side by up to 100 dB.
	double aGain = SDLSoundManager::DBToGain(SDLSoundManager::VolumeToDB(mBaseVolume * mVolume * mManager->mMasterVolume));
	int aPan = mBasePan + mPan;
	if (aPan < -10000)
		aPan = -10000;
	if (aPan > 10000)
		aPan = 10000;
	double aLeft = aGain * ((aPan > 0) ? SDLSoundManager::DBToGain(-aPan) : 1.0);
	double aRight = aGain * ((aPan < 0) ? SDLSoundManager::DBToGain(aPan) : 1.0);

	mManager->Lock();
	mGainL = (int)(aLeft * 16384.0 + 0.5);
	mGainR = (int)(aRight * 16384.0 + 0.5);
	mManager->Unlock();
}

void SDLSoundInstance::Release()
{
	Stop();
	mReleased = true;
}

void SDLSoundInstance::SetBaseVolume(double theBaseVolume)
{
	mBaseVolume = theBaseVolume;
	RehupGains();
}

void SDLSoundInstance::SetBasePan(int theBasePan)
{
	mBasePan = theBasePan;
	RehupGains();
}

void SDLSoundInstance::AdjustPitch(double theNumSteps)
{
	double aMult = pow(1.0594630943592952645618252949463, theNumSteps);
	// DirectSound's frequency range was 100 Hz..200 kHz around a 44.1 kHz original.
	if (aMult < 100.0 / 44100.0)
		aMult = 100.0 / 44100.0;
	if (aMult > 200000.0 / 44100.0)
		aMult = 200000.0 / 44100.0;

	mManager->Lock();
	mStep = (uint32_t)(aMult * 65536.0 + 0.5);
	mManager->Unlock();
}

void SDLSoundInstance::SetVolume(double theVolume)
{
	mVolume = theVolume;
	RehupGains();
}

void SDLSoundInstance::SetPan(int thePosition)
{
	mPan = thePosition;
	RehupGains();
}

bool SDLSoundInstance::Play(bool looping, bool autoRelease)
{
	Stop();

	mHasPlayed = true;
	mAutoRelease = autoRelease;

	if (mSample == NULL || mSample->mFrames <= 0)
		return false;

	mManager->Lock();
	mPos = 0;
	mLooping = looping;
	mPlaying = true;
	mManager->Unlock();
	return true;
}

void SDLSoundInstance::Stop()
{
	mManager->Lock();
	mPlaying = false;
	mPos = 0;
	mManager->Unlock();
	mAutoRelease = false;
}

bool SDLSoundInstance::IsPlaying()
{
	return mHasPlayed && mPlaying;
}

bool SDLSoundInstance::IsReleased()
{
	if ((!mReleased) && (mAutoRelease) && (mHasPlayed) && (!IsPlaying()))
		Release();

	return mReleased;
}

double SDLSoundInstance::GetVolume()
{
	return mVolume;
}

// ---------------------------------------------------------------------------------------------------------------
// SDLSoundManager

SDLSoundManager::SDLSoundManager()
{
	mDevice = 0;
	mFreq = 48000;
	mMasterVolume = 1.0;
	mMusicMix = NULL;
	mMusicUserData = NULL;
	mCaptureFile = NULL;
	mCaptureFramesLeft = 0;

	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
	{
		mSamples[i] = NULL;
		mBaseVolumes[i] = 1.0;
		mBasePans[i] = 0;
	}
	for (int i = 0; i < MAX_CHANNELS; i++)
		mChannels[i] = NULL;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		fprintf(stderr, "SDL audio init failed: %s\n", SDL_GetError());
		return;
	}

	// 48 kHz is the Switch's own rate, so nothing gets resampled twice.
	SDL_AudioSpec aWant, aHave;
	SDL_zero(aWant);
	aWant.freq = 48000;
	aWant.format = AUDIO_S16SYS;
	aWant.channels = 2;
	aWant.samples = 1024;
	aWant.callback = AudioCallback;
	aWant.userdata = this;
	mDevice = SDL_OpenAudioDevice(NULL, 0, &aWant, &aHave, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if (mDevice == 0)
	{
		fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		return;
	}

	mFreq = aHave.freq;
	mMixBuffer.resize(aHave.samples * 2);
	fprintf(stderr, "audio: %d Hz, %d frames per callback, driver %s\n", aHave.freq, aHave.samples, SDL_GetCurrentAudioDriver());
	SDL_PauseAudioDevice(mDevice, 0);
}

SDLSoundManager::~SDLSoundManager()
{
	if (mDevice != 0)
		SDL_CloseAudioDevice(mDevice);
	mDevice = 0;
	if (mCaptureFile != NULL)
		fclose(mCaptureFile);
	mCaptureFile = NULL;

	ReleaseChannels();
	ReleaseSounds();
}

void SDLSoundManager::Lock()
{
	if (mDevice != 0)
		SDL_LockAudioDevice(mDevice);
}

void SDLSoundManager::Unlock()
{
	if (mDevice != 0)
		SDL_UnlockAudioDevice(mDevice);
}

void SDLSoundManager::SetMusicMixer(MusicMixFunc theFunc, void* theUserData)
{
	Lock();
	mMusicMix = theFunc;
	mMusicUserData = theUserData;
	Unlock();
}

void SDLSoundManager::StartCapture(const std::string& theFileName, int theMilliseconds)
{
	FILE* aFile = fopen(theFileName.c_str(), "wb");
	if (aFile == NULL)
		return;

	Lock();
	if (mCaptureFile != NULL)
		fclose(mCaptureFile);
	mCaptureFile = aFile;
	mCaptureFramesLeft = (int)((int64_t)mFreq * theMilliseconds / 1000);
	Unlock();
}

int SDLSoundManager::VolumeToDB(double theVolume)
{
	int aVol = (int)((log10(1 + theVolume * 9) - 1.0) * 2333);
	if (aVol < -2000)
		aVol = -10000;

	return aVol;
}

double SDLSoundManager::DBToGain(int theDB)
{
	if (theDB <= -10000)
		return 0.0;
	return pow(10.0, theDB / 2000.0);
}

void SDLSoundManager::AudioCallback(void* theUserData, uint8_t* theStream, int theLen)
{
	((SDLSoundManager*)theUserData)->Mix((int16_t*)theStream, theLen / 4);
}

void SDLSoundManager::Mix(int16_t* theOut, int theFrames)
{
	if ((int)mMixBuffer.size() < theFrames * 2)
		mMixBuffer.resize(theFrames * 2);
	int32_t* aBuf = &mMixBuffer[0];
	memset(aBuf, 0, theFrames * 2 * sizeof(int32_t));

	if (mMusicMix != NULL)
		mMusicMix(mMusicUserData, aBuf, theFrames);

	for (int aChannel = 0; aChannel < MAX_CHANNELS; aChannel++)
	{
		SDLSoundInstance* anInst = mChannels[aChannel];
		if (anInst == NULL || !anInst->mPlaying || anInst->mSample == NULL)
			continue;

		const int16_t* aData = &anInst->mSample->mData[0];
		int aLength = anInst->mSample->mFrames;
		int aGainL = anInst->mGainL, aGainR = anInst->mGainR;
		uint64_t aPos = anInst->mPos;
		uint32_t aStep = anInst->mStep;

		for (int i = 0; i < theFrames; i++)
		{
			int anIdx = (int)(aPos >> 16);
			if (anIdx >= aLength)
			{
				if (!anInst->mLooping)
				{
					anInst->mPlaying = false;
					break;
				}
				aPos -= (uint64_t)aLength << 16;
				anIdx = (int)(aPos >> 16);
			}

			int aL = aData[anIdx * 2], aR = aData[anIdx * 2 + 1];
			if (aStep != 0x10000)
			{
				// Linear interpolation when pitched.
				int aNext = (anIdx + 1 < aLength) ? anIdx + 1 : (anInst->mLooping ? 0 : anIdx);
				int aFrac = (int)(aPos & 0xFFFF);
				aL += ((aData[aNext * 2] - aL) * aFrac) >> 16;
				aR += ((aData[aNext * 2 + 1] - aR) * aFrac) >> 16;
			}

			aBuf[i * 2] += (aL * aGainL) >> 14;
			aBuf[i * 2 + 1] += (aR * aGainR) >> 14;
			aPos += aStep;
		}
		anInst->mPos = aPos;
	}

	for (int i = 0; i < theFrames * 2; i++)
	{
		int32_t aSample = aBuf[i];
		theOut[i] = (int16_t)((aSample > 32767) ? 32767 : ((aSample < -32768) ? -32768 : aSample));
	}

	if (mCaptureFile != NULL)
	{
		int aFrames = (theFrames < mCaptureFramesLeft) ? theFrames : mCaptureFramesLeft;
		fwrite(theOut, 4, aFrames, mCaptureFile);
		mCaptureFramesLeft -= aFrames;
		if (mCaptureFramesLeft <= 0)
		{
			fclose(mCaptureFile);
			mCaptureFile = NULL;
		}
	}
}

static bool ConvertToDevice(const uint8_t* theData, int theLen, SDL_AudioFormat theFormat, int theChannels, int theFreq,
	int theDeviceFreq, SDLSoundSample* theSample)
{
	SDL_AudioCVT aCVT;
	if (SDL_BuildAudioCVT(&aCVT, theFormat, (Uint8)theChannels, theFreq, AUDIO_S16SYS, 2, theDeviceFreq) < 0)
		return false;

	std::vector<uint8_t> aBuf((size_t)theLen * aCVT.len_mult + 16);
	memcpy(&aBuf[0], theData, theLen);
	aCVT.buf = &aBuf[0];
	aCVT.len = theLen;
	if (aCVT.needed && SDL_ConvertAudio(&aCVT) != 0)
		return false;

	int aBytes = aCVT.needed ? aCVT.len_cvt : theLen;
	theSample->mFrames = aBytes / 4;
	theSample->mData.assign((int16_t*)&aBuf[0], (int16_t*)&aBuf[0] + theSample->mFrames * 2);
	return true;
}

bool SDLSoundManager::DecodeFile(const std::string& theFilename, SDLSoundSample* theSample)
{
	std::string aPath = ResolveDataPath(theFilename);
	std::string anExt;
	size_t aDot = aPath.find_last_of('.');
	size_t aSlash = aPath.find_last_of('/');
	if (aDot != std::string::npos && (aSlash == std::string::npos || aDot > aSlash))
		anExt = aPath.substr(aDot + 1);

	if (anExt == "ogg")
	{
		FILE* aFP = fopen(aPath.c_str(), "rb");
		if (aFP == NULL)
			return false;

		OggVorbis_File aVF;
		if (ov_open(aFP, &aVF, NULL, 0) != 0)
		{
			fclose(aFP);
			return false;
		}

		vorbis_info* anInfo = ov_info(&aVF, -1);
		int aChannels = anInfo->channels;
		int aFreq = (int)anInfo->rate;

		std::vector<uint8_t> aPCM;
		char aChunk[4096];
		int aBitstream = 0;
		for (;;)
		{
			long aRead = ov_read(&aVF, aChunk, sizeof(aChunk), &aBitstream);
			if (aRead <= 0)
				break;
			aPCM.insert(aPCM.end(), aChunk, aChunk + aRead);
		}
		ov_clear(&aVF); // closes aFP

		if (aPCM.empty() || aChannels < 1 || aChannels > 2)
			return false;
		return ConvertToDevice(&aPCM[0], (int)aPCM.size(), AUDIO_S16SYS, aChannels, aFreq, mFreq, theSample);
	}

	if (anExt == "wav")
	{
		SDL_AudioSpec aSpec;
		Uint8* aBuf = NULL;
		Uint32 aLen = 0;
		if (SDL_LoadWAV(aPath.c_str(), &aSpec, &aBuf, &aLen) == NULL)
			return false;
		bool aResult = ConvertToDevice(aBuf, (int)aLen, aSpec.format, aSpec.channels, aSpec.freq, mFreq, theSample);
		SDL_FreeWAV(aBuf);
		return aResult;
	}

	return false;
}

bool SDLSoundManager::Initialized()
{
	return mDevice != 0;
}

bool SDLSoundManager::LoadSound(unsigned int theSfxID, const std::string& theFilename)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;

	ReleaseSound(theSfxID);

	// Like DSoundManager: the game names sounds without an extension, so try the formats in turn.
	SDLSoundSample* aSample = new SDLSoundSample();
	aSample->mFrames = 0;
	bool aLoaded = DecodeFile(theFilename, aSample) || DecodeFile(theFilename + ".ogg", aSample) ||
		DecodeFile(theFilename + ".wav", aSample);
	if (!aLoaded)
	{
		delete aSample;
		return false;
	}

	Lock();
	mSamples[theSfxID] = aSample;
	Unlock();
	return true;
}

int SDLSoundManager::LoadSound(const std::string& theFilename)
{
	int anId = GetFreeSoundId();
	if (anId == -1)
		return -1;
	if (!LoadSound(anId, theFilename))
		return -1;
	return anId;
}

void SDLSoundManager::ReleaseSound(unsigned int theSfxID)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS || mSamples[theSfxID] == NULL)
		return;

	// DirectSound kept duplicated buffers alive; here the channels share the data, so they stop with it.
	Lock();
	SDLSoundSample* aSample = mSamples[theSfxID];
	mSamples[theSfxID] = NULL;
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (mChannels[i] != NULL && mChannels[i]->mSample == aSample)
		{
			mChannels[i]->mPlaying = false;
			mChannels[i]->mSample = NULL;
		}
	}
	Unlock();
	delete aSample;
}

void SDLSoundManager::SetVolume(double theVolume)
{
	mMasterVolume = theVolume;

	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mChannels[i] != NULL)
			mChannels[i]->RehupGains();
}

bool SDLSoundManager::SetBaseVolume(unsigned int theSfxID, double theBaseVolume)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;
	mBaseVolumes[theSfxID] = theBaseVolume;
	return true;
}

bool SDLSoundManager::SetBasePan(unsigned int theSfxID, int theBasePan)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;
	mBasePans[theSfxID] = theBasePan;
	return true;
}

void SDLSoundManager::ReleaseFreeChannels()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (mChannels[i] != NULL && mChannels[i]->IsReleased())
		{
			Lock();
			SDLSoundInstance* anInst = mChannels[i];
			mChannels[i] = NULL;
			Unlock();
			delete anInst;
		}
	}
}

SoundInstance* SDLSoundManager::GetSoundInstance(unsigned int theSfxID)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS || mDevice == 0)
		return NULL;

	ReleaseFreeChannels();

	if (mSamples[theSfxID] == NULL)
		return NULL;

	int aFreeChannel = -1;
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (mChannels[i] == NULL)
		{
			aFreeChannel = i;
			break;
		}
	}
	if (aFreeChannel == -1)
		return NULL;

	SDLSoundInstance* anInst = new SDLSoundInstance(this, mSamples[theSfxID]);
	anInst->SetBasePan(mBasePans[theSfxID]);
	anInst->SetBaseVolume(mBaseVolumes[theSfxID]);

	Lock();
	mChannels[aFreeChannel] = anInst;
	Unlock();
	return anInst;
}

void SDLSoundManager::ReleaseSounds()
{
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		ReleaseSound(i);
}

void SDLSoundManager::ReleaseChannels()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (mChannels[i] != NULL)
		{
			Lock();
			SDLSoundInstance* anInst = mChannels[i];
			mChannels[i] = NULL;
			Unlock();
			delete anInst;
		}
	}
}

double SDLSoundManager::GetMasterVolume()
{
	return mMasterVolume;
}

void SDLSoundManager::SetMasterVolume(double theVolume)
{
	// On Windows this was the system mixer's wave volume; the console has its own, so it's the same as SetVolume.
	SetVolume(theVolume);
}

void SDLSoundManager::Flush()
{
}

void SDLSoundManager::SetCooperativeWindow(HWND theHWnd, bool isWindowed)
{
}

void SDLSoundManager::StopAllSounds()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mChannels[i] != NULL)
		{
			bool isAutoRelease = mChannels[i]->mAutoRelease;
			mChannels[i]->Stop();
			mChannels[i]->mAutoRelease = isAutoRelease;
		}
}

int SDLSoundManager::GetFreeSoundId()
{
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSamples[i] == NULL)
			return i;
	return -1;
}

int SDLSoundManager::GetNumSounds()
{
	int aCount = 0;
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSamples[i] != NULL)
			aCount++;
	return aCount;
}
