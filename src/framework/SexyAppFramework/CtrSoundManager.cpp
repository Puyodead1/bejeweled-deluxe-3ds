#include <tremor/ivorbisfile.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "CtrSoundManager.h"
#include "CtrAudio.h"
#include "../PakLib/PakInterface.h"

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------------
// CtrSoundInstance

CtrSoundInstance::CtrSoundInstance(CtrSoundManager* theManager, CtrSoundSample* theSample)
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

CtrSoundInstance::~CtrSoundInstance()
{
}

void CtrSoundInstance::RehupGains()
{
	// As DSoundInstance::RehupVolume/RehupPan: one volume in hundredths of a dB, and the pan attenuating the other
	// side by up to 100 dB.
	double aGain = CtrSoundManager::DBToGain(CtrSoundManager::VolumeToDB(mBaseVolume * mVolume * mManager->mMasterVolume));
	int aPan = mBasePan + mPan;
	if (aPan < -10000)
		aPan = -10000;
	if (aPan > 10000)
		aPan = 10000;
	double aLeft = aGain * ((aPan > 0) ? CtrSoundManager::DBToGain(-aPan) : 1.0);
	double aRight = aGain * ((aPan < 0) ? CtrSoundManager::DBToGain(aPan) : 1.0);

	mManager->Lock();
	mGainL = (int)(aLeft * 16384.0 + 0.5);
	mGainR = (int)(aRight * 16384.0 + 0.5);
	mManager->Unlock();
}

void CtrSoundInstance::Release()
{
	Stop();
	mReleased = true;
}

void CtrSoundInstance::SetBaseVolume(double theBaseVolume)
{
	mBaseVolume = theBaseVolume;
	RehupGains();
}

void CtrSoundInstance::SetBasePan(int theBasePan)
{
	mBasePan = theBasePan;
	RehupGains();
}

void CtrSoundInstance::AdjustPitch(double theNumSteps)
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

void CtrSoundInstance::SetVolume(double theVolume)
{
	mVolume = theVolume;
	RehupGains();
}

void CtrSoundInstance::SetPan(int thePosition)
{
	mPan = thePosition;
	RehupGains();
}

bool CtrSoundInstance::Play(bool looping, bool autoRelease)
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

void CtrSoundInstance::Stop()
{
	mManager->Lock();
	mPlaying = false;
	mPos = 0;
	mManager->Unlock();
	mAutoRelease = false;
}

bool CtrSoundInstance::IsPlaying()
{
	return mHasPlayed && mPlaying;
}

bool CtrSoundInstance::IsReleased()
{
	if ((!mReleased) && (mAutoRelease) && (mHasPlayed) && (!IsPlaying()))
		Release();

	return mReleased;
}

double CtrSoundInstance::GetVolume()
{
	return mVolume;
}

// ---------------------------------------------------------------------------------------------------------------
// CtrSoundManager

CtrSoundManager::CtrSoundManager()
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

	// The DSP's own rate, so nothing gets resampled twice.
	mFreq = CtrAudio::GetRate();
	mMixBuffer.resize(1024 * 2);
	mDevice = CtrAudio::Open(AudioCallback, this);
}

CtrSoundManager::~CtrSoundManager()
{
	if (mDevice)
		CtrAudio::Close();
	mDevice = false;
	if (mCaptureFile != NULL)
		fclose(mCaptureFile);
	mCaptureFile = NULL;

	ReleaseChannels();
	ReleaseSounds();
}

void CtrSoundManager::Lock()
{
	if (mDevice)
		CtrAudio::Lock();
}

void CtrSoundManager::Unlock()
{
	if (mDevice)
		CtrAudio::Unlock();
}

void CtrSoundManager::SetMusicMixer(MusicMixFunc theFunc, void* theUserData)
{
	Lock();
	mMusicMix = theFunc;
	mMusicUserData = theUserData;
	Unlock();
}

void CtrSoundManager::StartCapture(const std::string& theFileName, int theMilliseconds)
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

int CtrSoundManager::VolumeToDB(double theVolume)
{
	int aVol = (int)((log10(1 + theVolume * 9) - 1.0) * 2333);
	if (aVol < -2000)
		aVol = -10000;

	return aVol;
}

double CtrSoundManager::DBToGain(int theDB)
{
	if (theDB <= -10000)
		return 0.0;
	return pow(10.0, theDB / 2000.0);
}

void CtrSoundManager::AudioCallback(void* theUserData, int16_t* theOut, int theFrames)
{
	((CtrSoundManager*)theUserData)->Mix(theOut, theFrames);
}

void CtrSoundManager::Mix(int16_t* theOut, int theFrames)
{
	if ((int)mMixBuffer.size() < theFrames * 2)
		mMixBuffer.resize(theFrames * 2);
	int32_t* aBuf = &mMixBuffer[0];
	memset(aBuf, 0, theFrames * 2 * sizeof(int32_t));

	if (mMusicMix != NULL)
		mMusicMix(mMusicUserData, aBuf, theFrames);

	for (int aChannel = 0; aChannel < MAX_CHANNELS; aChannel++)
	{
		CtrSoundInstance* anInst = mChannels[aChannel];
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

// 16-bit PCM (mono or stereo, any rate) to 16-bit stereo at the device rate, resampled linearly. Done once per sound
// at load time; the sounds are short.
static bool ConvertToDevice(const int16_t* theData, int theFrames, int theChannels, int theFreq, int theDeviceFreq,
	CtrSoundSample* theSample)
{
	if (theFrames <= 0 || theChannels < 1 || theChannels > 2 || theFreq <= 0)
		return false;

	int64_t aOutFrames = (int64_t)theFrames * theDeviceFreq / theFreq;
	if (aOutFrames < 1)
		aOutFrames = 1;
	theSample->mFrames = (int)aOutFrames;
	theSample->mData.resize((size_t)aOutFrames * 2);
	int16_t* anOut = &theSample->mData[0];

	// Source position in 32.32 fixed point.
	uint64_t aStep = ((uint64_t)theFreq << 32) / theDeviceFreq;
	uint64_t aPos = 0;
	for (int64_t i = 0; i < aOutFrames; i++, aPos += aStep)
	{
		int anIdx = (int)(aPos >> 32);
		int aNext = (anIdx + 1 < theFrames) ? anIdx + 1 : anIdx;
		int aFrac = (int)((aPos >> 16) & 0xFFFF);
		for (int c = 0; c < 2; c++)
		{
			int aChan = (theChannels == 2) ? c : 0;
			int a = theData[anIdx * theChannels + aChan];
			int b = theData[aNext * theChannels + aChan];
			anOut[i * 2 + c] = (int16_t)(a + (((b - a) * aFrac) >> 16));
		}
	}
	return true;
}

static uint32_t ReadLE32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t ReadLE16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }

// A RIFF WAVE file with 8- or 16-bit PCM, to 16-bit samples.
static bool LoadWav(const std::string& thePath, std::vector<int16_t>& theSamples, int& theChannels, int& theFreq)
{
	FILE* aFP = fopen(thePath.c_str(), "rb");
	if (aFP == NULL)
		return false;
	std::vector<uint8_t> aFile;
	fseek(aFP, 0, SEEK_END);
	long aSize = ftell(aFP);
	fseek(aFP, 0, SEEK_SET);
	if (aSize > 12)
	{
		aFile.resize(aSize);
		if (fread(&aFile[0], 1, aSize, aFP) != (size_t)aSize)
			aFile.clear();
	}
	fclose(aFP);
	if (aFile.size() < 12 || memcmp(&aFile[0], "RIFF", 4) != 0 || memcmp(&aFile[8], "WAVE", 4) != 0)
		return false;

	int aBits = 0;
	theChannels = 0;
	theFreq = 0;
	size_t aPos = 12;
	while (aPos + 8 <= aFile.size())
	{
		uint32_t aChunkSize = ReadLE32(&aFile[aPos + 4]);
		const uint8_t* aChunk = &aFile[aPos + 8];
		size_t anAvail = aFile.size() - (aPos + 8);
		if (aChunkSize > anAvail)
			aChunkSize = (uint32_t)anAvail;

		if (memcmp(&aFile[aPos], "fmt ", 4) == 0 && aChunkSize >= 16)
		{
			if (ReadLE16(aChunk) != 1)		// PCM only
				return false;
			theChannels = ReadLE16(aChunk + 2);
			theFreq = (int)ReadLE32(aChunk + 4);
			aBits = ReadLE16(aChunk + 14);
		}
		else if (memcmp(&aFile[aPos], "data", 4) == 0 && theChannels > 0)
		{
			if (aBits == 16)
			{
				theSamples.resize(aChunkSize / 2);
				for (size_t i = 0; i < theSamples.size(); i++)
					theSamples[i] = (int16_t)ReadLE16(aChunk + i * 2);
			}
			else if (aBits == 8)
			{
				theSamples.resize(aChunkSize);
				for (size_t i = 0; i < theSamples.size(); i++)
					theSamples[i] = (int16_t)((aChunk[i] - 128) << 8);
			}
			else
				return false;
			return true;
		}
		aPos += 8 + aChunkSize + (aChunkSize & 1);
	}
	return false;
}

bool CtrSoundManager::DecodeFile(const std::string& theFilename, CtrSoundSample* theSample)
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
		return ConvertToDevice((const int16_t*)&aPCM[0], (int)(aPCM.size() / (2 * aChannels)), aChannels, aFreq, mFreq, theSample);
	}

	if (anExt == "wav")
	{
		std::vector<int16_t> aSamples;
		int aChannels = 0, aFreq = 0;
		if (!LoadWav(aPath, aSamples, aChannels, aFreq) || aSamples.empty() || aChannels < 1 || aChannels > 2)
			return false;
		return ConvertToDevice(&aSamples[0], (int)(aSamples.size() / aChannels), aChannels, aFreq, mFreq, theSample);
	}

	return false;
}

bool CtrSoundManager::Initialized()
{
	return mDevice != 0;
}

bool CtrSoundManager::LoadSound(unsigned int theSfxID, const std::string& theFilename)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;

	ReleaseSound(theSfxID);

	// Like DSoundManager: the game names sounds without an extension, so try the formats in turn.
	CtrSoundSample* aSample = new CtrSoundSample();
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

int CtrSoundManager::LoadSound(const std::string& theFilename)
{
	int anId = GetFreeSoundId();
	if (anId == -1)
		return -1;
	if (!LoadSound(anId, theFilename))
		return -1;
	return anId;
}

void CtrSoundManager::ReleaseSound(unsigned int theSfxID)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS || mSamples[theSfxID] == NULL)
		return;

	// DirectSound kept duplicated buffers alive; here the channels share the data, so they stop with it.
	Lock();
	CtrSoundSample* aSample = mSamples[theSfxID];
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

void CtrSoundManager::SetVolume(double theVolume)
{
	mMasterVolume = theVolume;

	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mChannels[i] != NULL)
			mChannels[i]->RehupGains();
}

bool CtrSoundManager::SetBaseVolume(unsigned int theSfxID, double theBaseVolume)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;
	mBaseVolumes[theSfxID] = theBaseVolume;
	return true;
}

bool CtrSoundManager::SetBasePan(unsigned int theSfxID, int theBasePan)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;
	mBasePans[theSfxID] = theBasePan;
	return true;
}

void CtrSoundManager::ReleaseFreeChannels()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (mChannels[i] != NULL && mChannels[i]->IsReleased())
		{
			Lock();
			CtrSoundInstance* anInst = mChannels[i];
			mChannels[i] = NULL;
			Unlock();
			delete anInst;
		}
	}
}

SoundInstance* CtrSoundManager::GetSoundInstance(unsigned int theSfxID)
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

	CtrSoundInstance* anInst = new CtrSoundInstance(this, mSamples[theSfxID]);
	anInst->SetBasePan(mBasePans[theSfxID]);
	anInst->SetBaseVolume(mBaseVolumes[theSfxID]);

	Lock();
	mChannels[aFreeChannel] = anInst;
	Unlock();
	return anInst;
}

void CtrSoundManager::ReleaseSounds()
{
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		ReleaseSound(i);
}

void CtrSoundManager::ReleaseChannels()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (mChannels[i] != NULL)
		{
			Lock();
			CtrSoundInstance* anInst = mChannels[i];
			mChannels[i] = NULL;
			Unlock();
			delete anInst;
		}
	}
}

double CtrSoundManager::GetMasterVolume()
{
	return mMasterVolume;
}

void CtrSoundManager::SetMasterVolume(double theVolume)
{
	// On Windows this was the system mixer's wave volume; the console has its own, so it's the same as SetVolume.
	SetVolume(theVolume);
}

void CtrSoundManager::Flush()
{
}

void CtrSoundManager::SetCooperativeWindow(HWND theHWnd, bool isWindowed)
{
}

void CtrSoundManager::StopAllSounds()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mChannels[i] != NULL)
		{
			bool isAutoRelease = mChannels[i]->mAutoRelease;
			mChannels[i]->Stop();
			mChannels[i]->mAutoRelease = isAutoRelease;
		}
}

int CtrSoundManager::GetFreeSoundId()
{
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSamples[i] == NULL)
			return i;
	return -1;
}

int CtrSoundManager::GetNumSounds()
{
	int aCount = 0;
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSamples[i] != NULL)
			aCount++;
	return aCount;
}
