#include "DSoundInstance.h"
#include "DSoundManager.h"

using namespace Sexy;

// FUNCTION: GAME187 0x48f7b0
DSoundInstance::DSoundInstance(DSoundManager* theSoundManager, LPDIRECTSOUNDBUFFER theSourceSound)
{
	mSoundManagerP = theSoundManager;
	mReleased = false;
	mAutoRelease = false;
	mHasPlayed = false;
	mSourceSoundBuffer = theSourceSound;
	mSoundBuffer = NULL;

	mBaseVolume = 1.0;
	mBasePan = 0;

	mVolume = 1.0;
	mPan = 0;

	mDefaultFrequency = 44100;

	HRESULT hr;

	if (mSourceSoundBuffer != NULL)
	{
		hr=mSoundManagerP->mDirectSound->DuplicateSoundBuffer(mSourceSoundBuffer, &mSoundBuffer);
		if (hr!=DS_OK)
		{
			switch (hr)
			{
			case DSERR_ALLOCATED: MessageBoxA(0,"DSERR_ALLOCATED","Hey",MB_OK);break;
			case DSERR_INVALIDCALL: MessageBoxA(0,"DSERR_INVALIDCALL","Hey",MB_OK);break;
			case DSERR_INVALIDPARAM: MessageBoxA(0,"DSERR_INVALIDPARAM","Hey",MB_OK);break;
			case DSERR_OUTOFMEMORY: MessageBoxA(0,"DSERR_OUTOFMEMORY","Hey",MB_OK);break;
			case DSERR_UNINITIALIZED: MessageBoxA(0,"DSERR_UNINITIALIZED","Hey",MB_OK);break;
			}
			exit(0);
		}

		mSoundBuffer->GetFrequency(&mDefaultFrequency);
	}

	RehupVolume();
}

// FUNCTION: GAME187 0x48f8f0
DSoundInstance::~DSoundInstance()
{
	if (mSoundBuffer != NULL)
		mSoundBuffer->Release();
}

// FUNCTION: GAME187 0x48f950
void DSoundInstance::RehupVolume()
{
	if (mSoundBuffer != NULL)
		mSoundBuffer->SetVolume(mSoundManagerP->VolumeToDB(mBaseVolume * mVolume * mSoundManagerP->mMasterVolume));
}

// FUNCTION: GAME187 0x48f990
void DSoundInstance::RehupPan()
{
	if (mSoundBuffer != NULL)
		mSoundBuffer->SetPan(mBasePan + mPan);
}

// FUNCTION: GAME187 0x48f9b0
void DSoundInstance::Release()
{
	Stop();
	mReleased = true;			
}

// FUNCTION: GAME187 0x48f9c0
void DSoundInstance::SetVolume(double theVolume) // 0 = max
{
	mVolume = theVolume;
	RehupVolume();	
}

// FUNCTION: GAME187 0x48f9e0
void DSoundInstance::SetPan(int thePosition) //-db to =db = left to right
{
	mPan = thePosition;
	RehupPan();	
}

// FUNCTION: GAME187 0x48fa00
void DSoundInstance::SetBaseVolume(double theBaseVolume)
{
	mBaseVolume = theBaseVolume;
	RehupVolume();
}

// FUNCTION: GAME187 0x48fa20
void DSoundInstance::SetBasePan(int theBasePan)
{
	mBasePan = theBasePan;
	RehupPan();
}

// FUNCTION: GAME187 0x48fa40
bool DSoundInstance::Play(bool looping, bool autoRelease)
{
	Stop();

	mHasPlayed = true;	
	mAutoRelease = autoRelease;	

	if (mSoundBuffer == NULL)
	{
		return false;
	}

	if (looping)
	{
		if (mSoundBuffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
			return false;
	}
	else
	{
		if (mSoundBuffer->Play(0, 0, 0) != DS_OK)
		{
			return false;
		}
	}

	return true;
}

// FUNCTION: GAME187 0x48faa0
void DSoundInstance::Stop()
{
	if (mSoundBuffer != NULL)
	{
		mSoundBuffer->Stop();
		mSoundBuffer->SetCurrentPosition(0);
		mAutoRelease = false;
	}
}

#include "DirectXErrorString.h"
// FUNCTION: GAME187 0x48fad0
void DSoundInstance::AdjustPitch(double theNumSteps)
{
	if (mSoundBuffer != NULL)
	{
		double aFrequencyMult = pow(1.0594630943592952645618252949463, theNumSteps);
		double aNewFrequency = mDefaultFrequency*aFrequencyMult;
		if (aNewFrequency < DSBFREQUENCY_MIN)
			aNewFrequency = DSBFREQUENCY_MIN;
		if (aNewFrequency > DSBFREQUENCY_MAX)
			aNewFrequency = DSBFREQUENCY_MAX;

		mSoundBuffer->SetFrequency((DWORD)aNewFrequency);
	}
}

// FUNCTION: GAME187 0x48fb60
bool DSoundInstance::IsPlaying()
{
	if (!mHasPlayed)
		return false;

	if (mSoundBuffer == NULL)
		return false;

	DWORD aStatus;
	if (mSoundBuffer->GetStatus(&aStatus) == DS_OK)
		// Has the sound stopped?
		return ((aStatus & DSBSTATUS_PLAYING) != 0);
	else
		return false;
}

// FUNCTION: GAME187 0x48fb90
bool DSoundInstance::IsReleased()
{
	if ((!mReleased) && (mAutoRelease) && (mHasPlayed) && (!IsPlaying()))	
		Release();	

	return mReleased;
}

// FUNCTION: GAME187 0x48fbc0
double DSoundInstance::GetVolume()
{
	return mVolume; 
}

