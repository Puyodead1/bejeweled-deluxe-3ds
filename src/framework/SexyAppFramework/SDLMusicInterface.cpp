#include <libmodplug/modplug.h>
#include <stdio.h>
#include <string.h>

#include "SDLMusicInterface.h"
#include "SDLSoundManager.h"
#include "../PakLib/PakInterface.h"

using namespace Sexy;

SDLMusicInfo::SDLMusicInfo()
{
	mFile = NULL;
	mVolume = 0.0;
	mVolumeAdd = 0.0;
	mVolumeCap = 1.0;
	mStopOnFade = false;
	mPlaying = false;
	mPaused = false;
	mLoop = true;
}

SDLMusicInterface::SDLMusicInterface(SDLSoundManager* theSoundManager)
{
	mSoundManager = theSoundManager;
	mGlobalVolume = 1.0;

	// Settings apply to songs loaded after this.
	ModPlug_Settings aSettings;
	ModPlug_GetSettings(&aSettings);
	aSettings.mChannels = 2;
	aSettings.mBits = 16;
	aSettings.mFrequency = (mSoundManager != NULL) ? mSoundManager->GetFrequency() : 48000;
	aSettings.mResamplingMode = MODPLUG_RESAMPLE_LINEAR;
	// Follow the songs' own jumps back forever (BASS_MUSIC_LOOP); a song that simply ends is restarted in Mix.
	aSettings.mLoopCount = -1;
	ModPlug_SetSettings(&aSettings);

	if (mSoundManager != NULL)
		mSoundManager->SetMusicMixer(MixCallback, this);
}

SDLMusicInterface::~SDLMusicInterface()
{
	if (mSoundManager != NULL)
		mSoundManager->SetMusicMixer(NULL, NULL);
	UnloadAllMusic();
}

void SDLMusicInterface::MixCallback(void* theUserData, int32_t* theBuffer, int theFrames)
{
	((SDLMusicInterface*)theUserData)->Mix(theBuffer, theFrames);
}

void SDLMusicInterface::Mix(int32_t* theBuffer, int theFrames)
{
	if ((int)mRenderBuffer.size() < theFrames * 2)
		mRenderBuffer.resize(theFrames * 2);

	for (MusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		SDLMusicInfo* anInfo = &anItr->second;
		if (!anInfo->mPlaying || anInfo->mPaused || anInfo->mFile == NULL)
			continue;

		int aGain = (int)(anInfo->mVolume * mGlobalVolume * 16384.0 + 0.5);
		int aDone = 0;
		bool aRestarted = false;
		while (aDone < theFrames)
		{
			int aBytes = ModPlug_Read(anInfo->mFile, &mRenderBuffer[aDone * 2], (theFrames - aDone) * 4);
			if (aBytes > 0)
			{
				aDone += aBytes / 4;
				continue;
			}

			// The song ended.
			if (!anInfo->mLoop || aRestarted)
			{
				anInfo->mPlaying = false;
				break;
			}
			ModPlug_Seek(anInfo->mFile, 0);
			aRestarted = true;
		}

		if (aGain <= 0)
			continue;
		const int16_t* aSrc = &mRenderBuffer[0];
		for (int i = 0; i < aDone * 2; i++)
			theBuffer[i] += (aSrc[i] * aGain) >> 14;
	}
}

bool SDLMusicInterface::LoadMusic(int theSongId, const std::string& theFileName)
{
	FILE* aFP = fopen(ResolveDataPath(theFileName).c_str(), "rb");
	if (aFP == NULL)
	{
		fprintf(stderr, "music: can't open %s\n", theFileName.c_str());
		return false;
	}

	fseek(aFP, 0, SEEK_END);
	long aSize = ftell(aFP);
	fseek(aFP, 0, SEEK_SET);
	std::vector<char> aData(aSize > 0 ? aSize : 0);
	if (aSize > 0 && fread(&aData[0], 1, aSize, aFP) != (size_t)aSize)
		aData.clear();
	fclose(aFP);
	if (aData.empty())
		return false;

	ModPlugFile* aFile = ModPlug_Load(&aData[0], (int)aData.size());
	if (aFile == NULL)
	{
		fprintf(stderr, "music: modplug can't load %s\n", theFileName.c_str());
		return false;
	}

	UnloadMusic(theSongId);

	SDLMusicInfo anInfo;
	anInfo.mFile = aFile;
	if (mSoundManager != NULL)
		mSoundManager->Lock();
	SDLMusicInfo& aStored = mMusicMap[theSongId];
	aStored = anInfo;
	aStored.mData.swap(aData);
	if (mSoundManager != NULL)
		mSoundManager->Unlock();
	return true;
}

// Called with the device locked. theOffset is an order number, or -1 to carry on from where the song stopped.
void SDLMusicInterface::StartSong(SDLMusicInfo* theInfo, int theOffset, bool noLoop)
{
	if (theOffset == 0)
		ModPlug_Seek(theInfo->mFile, 0);	// a full reset of tempo and channels, like BASS_MUSIC_POSRESET
	else if (theOffset > 0)
		ModPlug_SeekOrder(theInfo->mFile, theOffset);
	theInfo->mLoop = !noLoop;
	theInfo->mPaused = false;
	theInfo->mPlaying = true;
}

void SDLMusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end())
		return;

	SDLMusicInfo* anInfo = &anItr->second;
	mSoundManager->Lock();
	anInfo->mVolume = anInfo->mVolumeCap;
	anInfo->mVolumeAdd = 0.0;
	anInfo->mStopOnFade = noLoop;
	StartSong(anInfo, (theOffset < 0) ? 0 : theOffset, noLoop);
	mSoundManager->Unlock();
}

void SDLMusicInterface::StopMusic(int theSongId)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end())
		return;

	mSoundManager->Lock();
	anItr->second.mVolume = 0.0;
	anItr->second.mPlaying = false;
	mSoundManager->Unlock();
}

void SDLMusicInterface::StopAllMusic()
{
	mSoundManager->Lock();
	for (MusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		anItr->second.mVolume = 0.0;
		anItr->second.mPlaying = false;
	}
	mSoundManager->Unlock();
}

void SDLMusicInterface::UnloadMusic(int theSongId)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end())
		return;

	if (mSoundManager != NULL)
		mSoundManager->Lock();
	ModPlugFile* aFile = anItr->second.mFile;
	mMusicMap.erase(anItr);
	if (mSoundManager != NULL)
		mSoundManager->Unlock();
	if (aFile != NULL)
		ModPlug_Unload(aFile);
}

void SDLMusicInterface::UnloadAllMusic()
{
	while (!mMusicMap.empty())
		UnloadMusic(mMusicMap.begin()->first);
}

void SDLMusicInterface::PauseMusic(int theSongId)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
		anItr->second.mPaused = true;
}

void SDLMusicInterface::ResumeMusic(int theSongId)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
		anItr->second.mPaused = false;
}

void SDLMusicInterface::PauseAllMusic()
{
	for (MusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
		if (anItr->second.mPlaying)
			anItr->second.mPaused = true;
}

void SDLMusicInterface::ResumeAllMusic()
{
	for (MusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
		anItr->second.mPaused = false;
}

void SDLMusicInterface::FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end())
		return;

	// From the song's current volume (0 after a fade out or a stop), as BassMusicInterface did.
	SDLMusicInfo* anInfo = &anItr->second;
	mSoundManager->Lock();
	anInfo->mVolumeAdd = theSpeed;
	anInfo->mStopOnFade = noLoop;
	StartSong(anInfo, theOffset, noLoop);
	mSoundManager->Unlock();
}

void SDLMusicInterface::FadeOut(int theSongId, bool stopSong, double theSpeed)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr == mMusicMap.end())
		return;

	SDLMusicInfo* anInfo = &anItr->second;
	if (anInfo->mVolume != 0.0)
		anInfo->mVolumeAdd = -theSpeed;
	anInfo->mStopOnFade = stopSong;
}

void SDLMusicInterface::FadeOutAll(bool stopSong, double theSpeed)
{
	for (MusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		anItr->second.mVolumeAdd = -theSpeed;
		anItr->second.mStopOnFade = stopSong;
	}
}

void SDLMusicInterface::SetSongVolume(int theSongId, double theVolume)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
		anItr->second.mVolume = theVolume;
}

void SDLMusicInterface::SetSongMaxVolume(int theSongId, double theMaxVolume)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		anItr->second.mVolumeCap = theMaxVolume;
		if (anItr->second.mVolume > theMaxVolume)
			anItr->second.mVolume = theMaxVolume;
	}
}

bool SDLMusicInterface::IsPlaying(int theSongId)
{
	MusicMap::iterator anItr = mMusicMap.find(theSongId);
	return (anItr != mMusicMap.end()) && anItr->second.mPlaying && !anItr->second.mPaused;
}

void SDLMusicInterface::SetVolume(double theVolume)
{
	mGlobalVolume = theVolume;
}

void SDLMusicInterface::SetMusicAmplify(int theSongId, double theAmp)
{
	// BASS_MusicSetAmplify; the game never calls it, and modplug's levels are its own anyway.
}

void SDLMusicInterface::Update()
{
	for (MusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		SDLMusicInfo* anInfo = &anItr->second;
		if (anInfo->mVolumeAdd == 0.0)
			continue;

		anInfo->mVolume += anInfo->mVolumeAdd;
		if (anInfo->mVolume > anInfo->mVolumeCap)
		{
			anInfo->mVolume = anInfo->mVolumeCap;
			anInfo->mVolumeAdd = 0.0;
		}
		else if (anInfo->mVolume < 0.0)
		{
			anInfo->mVolume = 0.0;
			anInfo->mVolumeAdd = 0.0;

			if (anInfo->mStopOnFade)
				anInfo->mPlaying = false;
		}
	}
}
