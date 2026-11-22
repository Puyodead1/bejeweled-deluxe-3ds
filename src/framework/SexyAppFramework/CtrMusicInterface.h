#ifndef __CTRMUSICINTERFACE_H__
#define __CTRMUSICINTERFACE_H__

// Console port: the MusicInterface on libmodplug, standing in for BassMusicInterface. Songs (the game's S3Ms) are
// rendered on the audio thread and mixed into CtrSoundManager's device. Volumes, fades and the offset (an order
// number) behave as in BassMusicInterface: fades step once per Update(), i.e. per game update.

#include "MusicInterface.h"

#include <stdint.h>
#include <map>
#include <vector>

struct _ModPlugFile;

namespace Sexy
{

class CtrSoundManager;

struct CtrMusicInfo
{
	_ModPlugFile*			mFile;
	std::vector<char>		mData;
	double					mVolume;
	double					mVolumeAdd;
	double					mVolumeCap;
	bool					mStopOnFade;
	bool					mPlaying;			// read by the audio thread
	bool					mPaused;
	bool					mLoop;

	CtrMusicInfo();
};

class CtrMusicInterface : public MusicInterface
{
protected:
	typedef std::map<int, CtrMusicInfo> MusicMap;

	CtrSoundManager*		mSoundManager;
	MusicMap				mMusicMap;
	double					mGlobalVolume;
	std::vector<int16_t>	mRenderBuffer;

	static void				MixCallback(void* theUserData, int32_t* theBuffer, int theFrames);
	void					Mix(int32_t* theBuffer, int theFrames);
	void					StartSong(CtrMusicInfo* theInfo, int theOffset, bool noLoop);

public:
	CtrMusicInterface(CtrSoundManager* theSoundManager);
	virtual ~CtrMusicInterface();

	virtual bool			LoadMusic(int theSongId, const std::string& theFileName);
	virtual void			PlayMusic(int theSongId, int theOffset = 0, bool noLoop = false);
	virtual void			StopMusic(int theSongId);
	virtual void			PauseMusic(int theSongId);
	virtual void			ResumeMusic(int theSongId);
	virtual void			StopAllMusic();

	virtual void			UnloadMusic(int theSongId);
	virtual void			UnloadAllMusic();
	virtual void			PauseAllMusic();
	virtual void			ResumeAllMusic();

	virtual void			FadeIn(int theSongId, int theOffset = -1, double theSpeed = 0.002, bool noLoop = false);
	virtual void			FadeOut(int theSongId, bool stopSong = true, double theSpeed = 0.004);
	virtual void			FadeOutAll(bool stopSong = true, double theSpeed = 0.004);
	virtual void			SetSongVolume(int theSongId, double theVolume);
	virtual void			SetSongMaxVolume(int theSongId, double theMaxVolume);
	virtual bool			IsPlaying(int theSongId);

	virtual void			SetVolume(double theVolume);
	virtual void			SetMusicAmplify(int theSongId, double theAmp);
	virtual void			Update();
};

}

#endif
